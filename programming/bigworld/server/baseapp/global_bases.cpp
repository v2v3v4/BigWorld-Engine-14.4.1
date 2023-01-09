#include "script/first_include.hpp"

#include "global_bases.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "baseapp_config.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"
#include "pyscript/pickler.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * GlobalBases::s_subscript( PyObject * self, PyObject * index )
{
	return ((GlobalBases *) self)->subscript( index );
}


/**
 * 	This function returns the number of entities in the system.
 */
Py_ssize_t GlobalBases::s_length( PyObject * self )
{
	return ((GlobalBases *) self)->length();
}


/**
 *	This structure contains the function pointers necessary to provide
 * 	a Python Mapping interface.
 */
static PyMappingMethods g_entitiesMapping =
{
	GlobalBases::s_length,		// mp_length
	GlobalBases::s_subscript,	// mp_subscript
	NULL						// mp_ass_subscript
};

/*~ function GlobalBases has_key
 *  @components{ base }
 *  has_key reports whether an entity with a specific label is listed in this
 *	GlobalBases object.
 *  @param key key is the string key to be searched for.
 *  @return A boolean. True if the key was found, false if it was not.
 */
/*~ function GlobalBases keys
 *  @components{ base }
 *  keys returns a list of the labels of all of the bases listed in this object.
 *  @return A list containing all of the keys.
 */
/*~ function GlobalBases items
 *  @components{ base }
 *  items returns a list of the items, as (label, base) pairs, that are listed
 *  in this object.
 *  @return A list containing all of the (ID, entity) pairs, represented as
 *  tuples containing an string label and an base (or base mailbox).
 */
/*~ function GlobalBases values
 *  @components{ base }
 *  values returns a list of all the bases listed in this object.
 *  @return A list containing all of the bases (or base mailboxes).
 */
/*~ function GlobalBases get
 *  @components{ base }
 *  values return a base (or base mailbox) with the input key or the default value if not found.
 *  @return A base (or base mailbox) or the default value if not found.
 */
PY_TYPEOBJECT_WITH_MAPPING( GlobalBases, &g_entitiesMapping )

PY_BEGIN_METHODS( GlobalBases )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )
	PY_METHOD( get )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GlobalBases )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	The constructor for GlobalBases.
 */
GlobalBases::GlobalBases( PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pMap_( NULL ),
	registeredBases_()
{
	pMap_ = PyDict_New();
}


/**
 *	Destructor.
 */
GlobalBases::~GlobalBases()
{
	Py_DECREF( pMap_ );
}


// -----------------------------------------------------------------------------
// Section: Script Mapping Related
// -----------------------------------------------------------------------------

/**
 *	This method finds the base with the input label.
 *
 * 	@param key 	The label of the base to locate.
 *
 *	@return	A new reference to the object associated with the given label.
 */
PyObject * GlobalBases::subscript( PyObject* key )
{
	PyObject * pObject = PyDict_GetItem( pMap_, key );
	if (pObject == NULL)
	{
		PyErr_SetObject( PyExc_KeyError, key );
	}
	else
	{
		Py_INCREF( pObject );
	}

	return pObject;
}


/**
 * 	This method returns the number of global bases in the system.
 */
int GlobalBases::length()
{
	return PyDict_Size( pMap_ );
}


/**
 * 	This method returns true if the given entity exists.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * GlobalBases::py_has_key( PyObject* args )
{
	return PyObject_CallMethod( pMap_, "has_key", "O", args );
}


/**
 * 	This method returns the value with the input key or the default value if not
 *	found.
 *
 * 	@param args		A python tuple containing the arguments.
 */
PyObject * GlobalBases::py_get( PyObject* args )
{
	return PyObject_CallMethod( pMap_, "get", "O", args );
}


/**
 * 	This method returns a list of all the entity IDs in the system.
 */
PyObject* GlobalBases::py_keys(PyObject* /*args*/)
{
	return PyDict_Keys( pMap_ );
}


/**
 * 	This method returns a list of all the entity objects in the system.
 */
PyObject* GlobalBases::py_values( PyObject* /*args*/ )
{
	return PyDict_Values( pMap_ );
}


/**
 * 	This method returns a list of tuples of all the entity IDs
 *	and objects in the system.
 */
PyObject* GlobalBases::py_items( PyObject* /*args*/ )
{
	return PyDict_Items( pMap_ );
}


// -----------------------------------------------------------------------------
// Section: Misc.
// -----------------------------------------------------------------------------

namespace
{

class MakeGlobalHandler : public Mercury::ReplyMessageHandler
{
public:
	MakeGlobalHandler(
			GlobalBases * pGlobalBases,
			Base *        pBase,
			BW::string   label,
			PyObject *    pCallback ) :
		pGlobalBases_( pGlobalBases ),
		pBase_( pBase ),
		label_( label ),
		pCallback_( pCallback ) {}
	virtual ~MakeGlobalHandler() {}

private:
	void handleMessage( const Mercury::Address & /*srcAddr*/,
		Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data, void * /*arg*/ )
	{
		bool succeeded = false;

		if (data.remainingLength() == 1)
		{
			int8 successCode;
			data >> successCode;


			if (successCode > 0)
			{
				succeeded = true;

				// The add call handles the case where pBase_ may have been
				// destroyed in the meantime.
				pGlobalBases_->add( pBase_.get(), label_ );
			}
		}

		PyObject* pArg = succeeded ? Py_True : Py_False;
		Py_INCREF( pCallback_.get() );	// Decremented by Script::call()
		Script::call( pCallback_.get(), Py_BuildValue( "(O)", pArg ),
				"Base.registerGlobally callback" );
		delete this;
	}

	void handleException(const Mercury::NubException& /*ne*/, void* /*arg*/)
	{
		ERROR_MSG( "MakeGlobalHandler::handleException: "
			"Something went wrong\n" );
		Py_INCREF( pCallback_.get() );	// Decremented by Script::call()
		Script::call( pCallback_.get(), Py_BuildValue( "(O)", Py_False ),
				"Base.registerGlobally callback" );
		delete this;
	}

	void handleShuttingDown( const Mercury::NubException & ne, void * )
	{
		INFO_MSG( "MakeGlobalHandler::handleShuttingDown: Ignoring\n" );
		delete this;
	}

	SmartPointer<GlobalBases>	pGlobalBases_;
	BasePtr			pBase_;
	BW::string		label_;
	PyObjectPtr		pCallback_;
};

} // anonymous namespace


/**
 *	This method attempts to register the input base globally in the server with
 *	the input label. If successful, the callback is called with a value of True,
 *	otherwise the callback is called with a value of False.
 *
 *	@return On failure, false is returned and the Python exception state set.
 */
bool GlobalBases::registerRequest( Base * pBase, PyObject * pKey,
		PyObject * pCallback )
{
	long hash = PyObject_Hash( pKey );

	if ((hash == -1) && PyErr_Occurred())
	{
		return false;
	}

	BW::string pickledKey = Pickler::pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (pickledKey.empty())
	{
		PyErr_SetString( PyExc_TypeError, "Unable to pickle key argument" );
		return false;
	}

	ScriptObject pKey2 = Pickler::unpickle( pickledKey );

	if (!pKey2)
	{
		PyErr_SetString( PyExc_TypeError, "Unable to unpickle key argument" );
		return false;
	}

	if (PyObject_Compare( pKey, pKey2.get() ) != 0)
	{
		if (!PyErr_Occurred())
		{
			PyErr_SetString( PyExc_TypeError,
					"Unpickled key is not equal to original key" );
		}

		return false;
	}

	BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();

	baseAppMgr.registerBaseGlobally( pickledKey,
			pBase->baseEntityMailBoxRef(),
		new MakeGlobalHandler( this, pBase, pickledKey, pCallback ) );

	return true;
}


/**
 *	This method is used to remove the known key from the globalBases
 *	dictionary.
 */
bool GlobalBases::removeKeyFromDict( PyObject * pKey )
{
	// remove it from the dictionary
	if (PyDict_DelItem( pMap_, pKey ) == -1)
	{
		PyErr_Clear();

		PyObject * pReprString = PyObject_Repr( pKey );
		if (PyString_Check( pReprString ))
		{
			ERROR_MSG( "GlobalBases::removeKeyFromDict: "
				"Failed to remove %s\n", PyString_AsString( pReprString ) );
		}
		Py_XDECREF( pReprString );

		return false;
	}

	return true;
}


/**
 *	This method attempts to deregister the input base.
 *
 *	@return True on success, otherwise false.
 */
bool GlobalBases::deregister( Base * pBase, PyObject * pKey )
{
	BW::string pickledKey = Pickler::pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (pickledKey.empty())
	{
		ERROR_MSG( "GlobalBases::deregister: "
				"Unable to pickle key argument\n" );
		return false;
	}

	typedef RegisteredBases::iterator It;
	std::pair< It, It > range = registeredBases_.equal_range( pBase->id() );

	if (range.first != range.second)
	{
		It iter = range.first;
		BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();

		while (iter != range.second)
		{
			if (iter->second == pickledKey)
			{
				baseAppMgr.deregisterBaseGlobally( pickledKey );
				this->remove( iter->second );
				registeredBases_.erase( iter );

				return true;
			}

			++iter;
		}
	}
	else
	{
		// We may be in the process of restoring the entity, so check if
		// the global base exists and is being restored by us.

		PyObject * pValue = PyDict_GetItem( pMap_, pKey );
		if (!pValue)
		{
			return false;
		}

		if (!BaseEntityMailBox::Check( pValue ))
		{
			return false;
		}

		BaseEntityMailBox * pMailbox =
			static_cast< BaseEntityMailBox * >( pValue );

		const Bases & bases = BaseApp::instance().bases();
		if (bases.findEntity( pMailbox->id() ))
		{
			if (!this->removeKeyFromDict( pKey ))
			{
				return false;
			}

			// now deregister it
			BaseApp::instance().baseAppMgr().deregisterBaseGlobally(
				pickledKey );

			return true;
		}
	}

	return false;
}


/**
 *	This method handles a message from the BaseAppMgr informing us to add a
 *	global base.
 */
void GlobalBases::add( BinaryIStream & data )
{
	BW::string pickledKey;
	EntityMailBoxRef mailboxRef;
	data >> pickledKey >> mailboxRef;

	EntityTypePtr pBaseType = EntityType::getType( mailboxRef.type(),
			/*shouldExcludeServices*/ true );

	if (pBaseType)
	{
		BaseEntityMailBox * pMailBox = new BaseEntityMailBox(
			pBaseType, mailboxRef.addr, mailboxRef.id );

		this->add( pMailBox, pickledKey );

		Py_DECREF( pMailBox );
	}
	else
	{
		ERROR_MSG( "GlobalBases::add: Bad global base\n" );
	}
}


/**
 *	This method adds a base locally to the global collection of bases. This
 *	method is called in response to a successful registration from the
 *	BaseAppMgr.
 */
void GlobalBases::add( Base * pBase, const BW::string & pickledKey )
{
	ScriptObject pKey = Pickler::unpickle( pickledKey );

	IF_NOT_MF_ASSERT_DEV( pKey )
	{
		ERROR_MSG( "Global::add: Could not unpickle key\n" );
		PyErr_Print();

		return;
	}

	PyObject * pMailBox = pBase;

	if (!BaseAppConfig::shouldResolveMailBoxes())
	{
		pMailBox = PyEntityMailBox::constructFromRef(
						pBase->baseEntityMailBoxRef() );
	}
	else
	{
		Py_INCREF( pMailBox );
	}

	PyDict_SetItem( pMap_, pKey.get(), pMailBox );

	Py_DECREF( pMailBox );

	registeredBases_.insert( std::make_pair( pBase->id(), pickledKey ) );

	// Check that the base has not been destroyed.
	if (pBase->isDestroyed())
	{
		this->onBaseDestroyed( pBase );
	}
}


/**
 *	This method adds a base mailbox to the local list of global bases. It is
 *	called in response to a message from the BaseAppMgr.
 */
void GlobalBases::add( BaseEntityMailBox * pBase,
		const BW::string & pickledKey )
{
	ScriptObject pKey = Pickler::unpickle( pickledKey );

	IF_NOT_MF_ASSERT_DEV( pKey )
	{
		ERROR_MSG( "Global::add: Could not unpickle key\n" );
		PyErr_Print();

		return;
	}

	PyDict_SetItem( pMap_, pKey.get(), pBase );
}


/**
 *	This method handles a message from the BaseAppMgr to inform us that a global
 *	base has gone.
 */
void GlobalBases::remove( BinaryIStream & data )
{
	BW::string label;
	data >> label;

	this->remove( label );
}


/**
 *	This method removes an entry to the local copy of the global bases
 *	collection.
 */
void GlobalBases::remove( const BW::string & pickledKey )
{
	ScriptObject pKey = Pickler::unpickle( pickledKey );

	if (pKey)
	{
		this->removeKeyFromDict( pKey.get() );
	}
	else
	{
		ERROR_MSG( "GlobalBases::remove: Invalid pickled key %s\n",
				pickledKey.c_str() );
		PyErr_Clear();
	}
}


/**
 *	This method adds all of the local global bases to the stream. This is used
 *	when restoring the state of the BaseAppMgr.
 */
void GlobalBases::addLocalsToStream( BinaryOStream & stream ) const
{
	const Bases & bases = BaseApp::instance().bases();

	RegisteredBases::const_iterator iter = registeredBases_.begin();

	while (iter != registeredBases_.end())
	{
		EntityID id = iter->first;
		Base * pBase = bases.findEntity( id );
		if (pBase)
		{
			stream << iter->second << pBase->baseEntityMailBoxRef();
		}
		++iter;
	}
}


/**
 *	This method is called when a base is destroyed. It makes sure that any
 *	global labels to this base are cleaned up correctly.
 */
void GlobalBases::onBaseDestroyed( Base * pBase )
{
	typedef RegisteredBases::iterator It;
	std::pair< It, It > range = registeredBases_.equal_range( pBase->id() );

	if (range.first != range.second)
	{
		BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();

		It iter = range.first;

		while (iter != range.second)
		{
			this->remove( iter->second );
			baseAppMgr.deregisterBaseGlobally( iter->second );

			++iter;
		}

		registeredBases_.erase( range.first, range.second );
	}
}


/**
 *	This method removes any mailboxes that may have become invalid from
 *	entities that were destroyed during restoration.
 */
void GlobalBases::resolveInvalidMailboxes()
{
	ScriptDict pMap( pMap_, ScriptObject::FROM_BORROWED_REFERENCE );
	ScriptObject pKey;
	ScriptObject pValue;
	ScriptDict::size_type pos = 0;

	typedef BW::vector<ScriptObject> KeysForDeletionList;
	KeysForDeletionList keysForDeletion;

	const Bases & bases = BaseApp::instance().bases();
	BaseAppMgrGateway & baseAppMgr = BaseApp::instance().baseAppMgr();

	while (pMap.next( pos, pKey, pValue ))
	{
		typedef ScriptObjectPtr<BaseEntityMailBox> BaseEntityMailBoxPtr;
		if (BaseEntityMailBoxPtr pMailbox = BaseEntityMailBoxPtr::create( pValue ))
		{
			if ((pMailbox->address() ==
						BaseApp::instance().intInterface().address()))
			{
				if (!bases.findEntity( pMailbox->id() ))
				{
					const ScriptString pKeyStrObj(
						pKey.str( ScriptErrorPrint() ) );
					INFO_MSG( "GlobalBases::resolveInvalidMailboxes: "
							"De-registering global base for destroyed "
							"entity %d using key %s\n",
						pMailbox->id(), pKeyStrObj.c_str() );

					// We can't delete this key while we are iterating the dict
					keysForDeletion.push_back( pKey );

					// But we can ask the BaseAppMgr to deregister it
					const BW::string pickledKey = Pickler::pickle( pKey );
					baseAppMgr.deregisterBaseGlobally( pickledKey );
				}
			}
		}
	}

	// Now we can delete the items from our dict.
	KeysForDeletionList::const_iterator keyIter = keysForDeletion.begin();
	while (keyIter != keysForDeletion.end())
	{
		this->removeKeyFromDict( keyIter->get() );
		++keyIter;
	}
}


/**
 *	This method checks whether there are any global bases that have been
 *	restored here and adjusts appropriately.
 */
void GlobalBases::handleBaseAppDeath( const Mercury::Address & deadAddr )
{
	const Bases & bases = BaseApp::instance().bases();

	PyObject * pKey = NULL;
	PyObject * pValue = NULL;
	Py_ssize_t pos = 0;

	while (PyDict_Next( pMap_, &pos, &pKey, &pValue ))
	{
		if (BaseEntityMailBox::Check( pValue ))
		{
			BaseEntityMailBox * pMailbox =
				static_cast< BaseEntityMailBox * >( pValue );

			if (pMailbox->address() == deadAddr)
			{
				EntityID id = pMailbox->id();
				Base * pBase = bases.findEntity( id );

				if (pBase != NULL)
				{
					BW::string pickledKey = Pickler::pickle( ScriptObject( 
							pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

					registeredBases_.insert( std::make_pair( id, pickledKey ) );
				}
			}
		}
	}
}

BW_END_NAMESPACE

// global_bases.cpp
