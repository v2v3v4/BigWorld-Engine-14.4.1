#include "script/first_include.hpp"

#include "entity_creator.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "id_config.hpp"
#include "load_entity_handler.hpp"
#include "login_handler.hpp"
#include "proxy.hpp"
#include "py_cell_data.hpp"

#include "server/cell_properties_names.hpp"

#include "db/dbapp_interface.hpp"

#include "network/channel_owner.hpp"
#include "network/channel_sender.hpp"
#include "network/nub_exception.hpp"
#include "network/udp_bundle.hpp"

#include "pyscript/py_data_section.hpp"

#include <stdlib.h>


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
EntityCreator::EntityCreator( Mercury::NetworkInterface & intInterface ) :
	idClient_(),
	intInterface_( intInterface )
{
}


/**
 *	 This static method creates EntityCreator instances.
 */
shared_ptr< EntityCreator > EntityCreator::create( DBApp & idOwner,
		   Mercury::NetworkInterface & intInterface )
{
	shared_ptr< EntityCreator >
		pCreator( new EntityCreator( intInterface ) );

	if (!pCreator->init( idOwner ))
	{
		return shared_ptr< EntityCreator >();
	}

	return pCreator;
}


/**
 *	 This method initialises this class.
 */
bool EntityCreator::init( DBApp & idOwner )
{
	if (!idClient_.init( &idOwner,
			DBAppInterface::getIDs,
			DBAppInterface::putIDs,
			IDConfig::criticallyLowSize(),
			IDConfig::lowSize(),
			IDConfig::desiredSize(),
			IDConfig::highSize() ))
	{
		ERROR_MSG( "EntityCreator::init: Failed to get IDs\n" );
		return false;
	}

	return true;
}


/**
 *	 This method returns the DBApp.
 */
DBApp & EntityCreator::dbApp()
{
	return BaseApp::instance().dbApp();
}


/**
 *	 This method returns the DBApp.
 */
float EntityCreator::getLoad() const
{
	return BaseApp::instance().getLoad();
}


/**
 *	This method creates a base from a type and a dictionary. It is used to
 *	implement the BigWorld.createBase script method.
 */
PyObject * EntityCreator::createBaseLocally( PyObject * args, PyObject * kwargs )
{
#if 0
	// The implementation of this method should be the following. Unfortunately,
	// this is special-cased because left-over arguments are still added to the
	// base entity.
	return this->createBaseCommon( &intInterface_.address(), args, kwargs );
#endif

	PyObjectPtr pBaseArgs = kwargs;

	if (!pBaseArgs)
	{
		pBaseArgs = PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );
	}

	EntityTypePtr pType =
		this->consolidateCreateBaseArgs( pBaseArgs, args, 1, 0 );

	if (!pType)
	{
		return NULL;
	}

	const EntityDescription & desc = pType->description();

	uint count = desc.propertyCount();

	PyObjectPtr pCellData( desc.canBeOnCell() ? PyDict_New() : 0,
		PyObjectPtr::STEAL_REFERENCE );

	if (pCellData)
	{
		PyObject * pVec3Position = Script::getData( Vector3( 0, 0, 0 ) );
		PyDict_SetItemString( pCellData.get(), POSITION_PROPERTY_NAME, pVec3Position );
		Py_DECREF( pVec3Position );

		PyObject * pVec3Direction = Script::getData( Vector3( 0, 0, 0 ) );
		PyDict_SetItemString( pCellData.get(), DIRECTION_PROPERTY_NAME, pVec3Direction );
		Py_DECREF( pVec3Direction );

		PyObject * pSpaceID = Script::getData( 0 );
		PyDict_SetItemString( pCellData.get(), SPACE_ID_PROPERTY_NAME, pSpaceID );
		Py_DECREF( pSpaceID );
	}

	for (uint i = 0; i < count; i++)
	{
		DataDescription * pDataDesc = desc.property( i );
		const BW::string & name = pDataDesc->name();
		char * pName = const_cast<char *>( name.c_str() );
		bool addToDict = true;

		const bool isCellData = pDataDesc->isCellData();
		const bool isBaseData = pDataDesc->isBaseData();

		if (!isCellData && !isBaseData)
		{
			continue;
		}

		PyObject * pValue = PyDict_GetItemString( pBaseArgs.get(), pName );

		if (!pValue)
		{
			PyErr_Clear();
		}
		else
		{
			addToDict = false;
			Py_INCREF( pValue );
			if (isCellData)
			{
				PyDict_DelItemString( pBaseArgs.get(), pName );
			}
		}

		if (!pValue)
		{
			// Need the object to make sure that the reference count never goes
			// to 0.
			ScriptObject pSmartValue = pDataDesc->pInitialValue();
			pValue = pSmartValue.newRef();
		}

		if (pValue)
		{
			if (!pDataDesc->dataType()->isSameType( 
				ScriptObject( pValue, ScriptObject::FROM_BORROWED_REFERENCE ) ))
			{
				PyErr_Format( PyExc_ValueError,
						"Invalid property type for property %s\n",
						pDataDesc->name().c_str() );
				return NULL;
			}

			if (isCellData && pCellData)
			{
				// TODO: Should check return value.
				PyDict_SetItemString( pCellData.get(), pName, pValue );
			}
			else if (addToDict && isBaseData)
			{
				PyDict_SetItemString( pBaseArgs.get(), pName, pValue );
			}
			Py_DECREF( pValue );
		}
	}

	// Move position, direction etc from base dict to cell dict.

	const char * transfers[] = { POSITION_PROPERTY_NAME, 
								 DIRECTION_PROPERTY_NAME, 
								 SPACE_ID_PROPERTY_NAME };

	for (size_t i = 0; i < sizeof( transfers ) / sizeof( transfers[0] ); i++)
	{
		const char * attr = transfers[i];

		PyObject * pValue = PyDict_GetItemString( pBaseArgs.get(), attr );

		if (pValue)
		{
			if (pCellData)
			{
				PyDict_SetItemString( pCellData.get(), attr, pValue );
			}

			// Don't delete the properties if they are defined in the .def
			DataDescription * pProp = desc.findProperty( attr );
			if (!pProp || !pProp->isBaseData())
			{
				PyDict_DelItemString( pBaseArgs.get(), attr );
			}
		}
		else
		{
			PyErr_Clear();
		}
	}

	// Check for extra values and warn.
	{
		Py_ssize_t pos = 0;
		PyObject * pKey = NULL;

		while (PyDict_Next( pBaseArgs.get(), &pos, &pKey, NULL ))
		{
			const char * key = PyString_AsString( pKey );
			if (!pType->description().findCompoundProperty( key ))
			{
				WARNING_MSG( "EntityCreator::createBaseLocally: "
							"%s is not in the def file for type %s\n",
						key, pType->description().name().c_str() );
			}
		}
	}

	return this->createBase( pType.get(), pBaseArgs.get(), pCellData.get() );
}


/**
 *	This method returns a new reference to a newly created base with the input
 *	type, or NULL on failure (and Python error state is set).
 *
 *	@param pType		The type of the entity to create.
 *	@param pDict		The properties of the new base, or NULL.
 *	@param pCellData	A dictionary containing the cell arguments. If not NULL,
 *						this will be the cellData attribute of the new entity.
 */
PyObject * EntityCreator::createBase( EntityType * pType,
	PyObject * pDict, PyObject * pCellData ) const
{
	EntityID id = const_cast< IDClient & >( idClient_ ).getID();

	if (id == 0)
	{
		ERROR_MSG( "EntityCreator::createBase: Unable to allocate id.\n" );
		PyErr_SetString( PyExc_RuntimeError, "Unable to allocate id" );

		return NULL;
	}

	Base * pNewBase = pType->newEntityBase( id, DatabaseID(0) );

	if (pNewBase)
	{
		// We increment before calling init() as init() may release a reference
		// if the object self-destroys in script __init__()
		Py_INCREF( pNewBase );

		pNewBase->init( pDict, pCellData, NULL );
	}

	return pNewBase;
}


/**
 *	This class is used to handle reply messages from BaseApps when a createBase
 *	message has been sent.
 */
class CreateBaseReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	CreateBaseReplyHandler( const ScriptObject & callback ) :
		callback_( callback )
	{
	}
private:
	void handleMessage(const Mercury::Address& /*srcAddr*/,
		Mercury::UnpackedMessageHeader& /*header*/,
		BinaryIStream& data, void * /*arg*/)
	{
		EntityMailBoxRef baseRef;
		data >> baseRef;

		this->callback( 
			ScriptObject( PyEntityMailBox::constructFromRef( baseRef ), 
						  ScriptObject::FROM_NEW_REFERENCE ) );
		delete this;
	}

	void handleException(const Mercury::NubException& /*ne*/, void* /*arg*/)
	{
		this->callback();
		ERROR_MSG( "CreateBaseReplyHandler::handleException: "
			"Failed to create base\n" );
		delete this;
	}


	void handleShuttingDown(const Mercury::NubException& /*ne*/, void* /*arg*/)
	{
		INFO_MSG( "CreateBaseReplyHandler::handleShuttingDown: Ignoring\n" );
		delete this;
	}

	void callback( const ScriptObject & arg = ScriptObject::none() )
	{
		if (callback_.isCallable())
		{
			callback_.callFunction(ScriptArgs::create(arg), ScriptErrorPrint());
		}
	}

	const ScriptObject callback_;
};

/**
 *	This class is used to handle reply messages from BaseApps when a
 * 	createBaseFromDB message has been sent.
 */
class CreateBaseFromDBReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	CreateBaseFromDBReplyHandler( PyObject * pCallback ) :
		pCallback_( pCallback )
	{
		Py_XINCREF( pCallback_ );
	}
private:
	void handleMessage(const Mercury::Address& /*srcAddr*/,
		Mercury::UnpackedMessageHeader& /*header*/,
		BinaryIStream& data, void * /*arg*/)
	{
		uint8 status;
		data >> status;

		if (status != LOAD_FROM_DB_FAILED)
		{
			DatabaseID dbID;
			data >> dbID;

			EntityMailBoxRef baseRef;
			data >> baseRef;

			this->callback( PyEntityMailBox::constructFromRef( baseRef ),
					dbID, (status == LOAD_FROM_DB_FOUND_EXISTING) );
		}
		else
		{
			this->callback();
		}

		delete this;
	}

	void handleException(const Mercury::NubException& ne, void* /*arg*/)
	{
		this->callback();
		ERROR_MSG( "CreateBaseFromDBReplyHandler::handleException: %s\n",
				Mercury::reasonToString( ne.reason() ) );
		delete this;
	}

	void handleShuttingDown( const Mercury::NubException & ne, void * )
	{
		INFO_MSG( "CreateBaseFromDBReplyHandler::handleShuttingDown: "
				"Ignoring\n" );
		delete this;
	}

	void callback( PyObject * pArg = NULL, DatabaseID dbID = 0,
			bool wasActive = false )
	{
		if (pCallback_)
		{
			if (!pArg)
			{
				pArg = Script::newPyNoneRef();
			}

			PyObject* pDbID = Script::getData( dbID );
			PyObject* pWasActive = Script::getData( wasActive );
			// Note: This consumes the reference to pCallback_.
			Script::call( pCallback_,
					Py_BuildValue( "(OOO)", pArg, pDbID, pWasActive ) );
			pCallback_ = NULL;

			Py_DECREF( pDbID );
			Py_DECREF( pWasActive );
		}

		Py_XDECREF( pArg );
	}

	PyObject * 	pCallback_;
};


/**
 *	This method creates a base entity on a remote BaseApp.
 */
PyObject * EntityCreator::createBaseRemotely( PyObject * args, PyObject * kwargs )
{
	return this->createBaseCommon( NULL, args, kwargs );
}


/**
 *	This method creates a base entity possibly on a remote BaseApp.
 */
PyObject * EntityCreator::createBaseAnywhere( PyObject * args, PyObject * kwargs )
{
	Mercury::Address addr;

	if (this->shouldCreateLocally())
	{
		addr = intInterface_.address();
	}
	else
	{
		addr = this->getAddressFromDestinationBaseApps();
	}

	return this->createBaseCommon( &addr, args, kwargs );
}


/**
 *	This method is common to the script createBase methods.
 *
 *	@param pAddr If passed in, this is the address of the BaseApp where this
 *		base entity should be created. If NULL, the address is gotten from the
 *		second argument in args.
 *	@param args The Python arguments tuple.
 *	@param kwargs The Python dictionary containing the keyword args. Note that
 *		this dictionary may be modified.
 *	@param hasCallback Indicates whether the arguments may have a callback as
 *		the last argument.
 *
 */
PyObject * EntityCreator::createBaseCommon( const Mercury::Address * pAddr,
		PyObject * args, PyObject * kwargs, bool hasCallback )
{
	int numArgs = PyTuple_Size( args );

	// The number of non-variable arguments at head of args
	int headOffset = pAddr ? 1 : 2;

	if (numArgs < headOffset)
	{
		PyErr_Format( PyExc_TypeError,
				"Expected at least %d argument%s",
				headOffset, (headOffset == 1) ? "" : "s" );
		return NULL;
	}

	// The number of non-variable arguments at tail of args. This may be the
	// callback.
	int tailOffset = 0;

	Mercury::Address baseAppAddr;

	if (pAddr)
	{
		baseAppAddr = *pAddr;
	}
	else
	{
		// This is from createBaseRemotely. The second argument should be a
		// mailbox.
		PyObject * pDest = PyTuple_GET_ITEM( args, 1 );

		if (BaseEntityMailBox::Check( pDest ))
		{
			baseAppAddr = static_cast<BaseEntityMailBox *>( pDest )->address();
		}
		else if (Base::Check( pDest ))
		{
			baseAppAddr = intInterface_.address();
		}
		else
		{
			PyErr_Format( PyExc_TypeError,
				"Argument 2 must be a mailbox or an entity" );
			return NULL;
		}
	}

	PyObject * pCallback = NULL;

	if (hasCallback && (numArgs > headOffset))
	{
		PyObject * pPyBack = PyTuple_GET_ITEM( args, numArgs - 1 );

		if (PyCallable_Check( pPyBack ))
		{
			pCallback = pPyBack;
			tailOffset += 1;
		}
	}

	PyObjectPtr pDict = kwargs ? kwargs :
		PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );

	EntityTypePtr pType = this->consolidateCreateBaseArgs(
				pDict, args, headOffset, tailOffset );

	if (!pType)
	{
		return NULL;
	}

	if (baseAppAddr == intInterface_.address())
	{
		MemoryOStream stream;

		if (!this->addCreateBaseData( stream, pType, pDict, pCallback ))
		{
			return NULL;
		}

		BasePtr pBase = this->createBaseFromStream( stream );

		if (pBase)
		{
			if (pCallback)
			{
				Py_INCREF( pCallback );	// Script::call eats reference
				Script::call( pCallback, Py_BuildValue( "(O)", pBase.get() ) );
			}
		}
		else
		{
			PyErr_SetString( PyExc_ValueError,
					"Unable to create base entity from stream" );
		}

		// Return value must be new reference.
		Py_XINCREF( pBase.get() );
		return pBase.get();
	}
	else
	{
		Mercury::ReplyMessageHandler * pHandler =
			new CreateBaseReplyHandler( 
				ScriptObject( pCallback, ScriptObject::FROM_BORROWED_REFERENCE ) );

		Mercury::Channel & channel = BaseApp::getChannel( baseAppAddr );

		std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );

		pBundle->startRequest( BaseAppIntInterface::createBaseWithCellData,
			   pHandler );

		EntityID newID = 0;

		if (this->addCreateBaseData( *pBundle, pType, pDict, pCallback, 
				&newID ))
		{
			channel.send( pBundle.get() );
			return new BaseEntityMailBox( pType, baseAppAddr, newID );
		}
		else
		{
			return NULL;
		}
	}
}


/**
 *	This method is used to consolidate arguments from createBase* script calls
 *	into a single dictionary.
 *
 *	@param	pDestDict The dictionary to place the consolidated values.
 *	@param	args	The tuple of dictionary and data sections that should be
 *		consolidated.
 *	@param headOffset Indicates the number to skip from the start of args.
 *	@param tailOffset Indicates the number to skip from the end of args.
 *
 *	@return The EntityTypePtr associated with the first element of args. On
 *		error, NULL is returned and the Python exception state is set.
 */
EntityTypePtr EntityCreator::consolidateCreateBaseArgs(
					PyObjectPtr pDestDict, PyObject * args,
					int headOffset, int tailOffset ) const
{
	MF_ASSERT( pDestDict );
	MF_ASSERT( PyDict_Check( pDestDict.get() ) );

	int numArgs = PyTuple_Size( args );

	if (numArgs < headOffset)
	{
		PyErr_Format( PyExc_TypeError,
				"Expected at least %d argument%s",
				headOffset, (headOffset == 1) ? "" : "s" );
		return NULL;
	}

	PyObject * pPyTypeName = PyTuple_GET_ITEM( args, 0 );

	if (!PyString_Check( pPyTypeName ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Expected a string as first argument" );
		return NULL;
	}

	char * typeName = PyString_AS_STRING( pPyTypeName );
	EntityTypePtr pType = EntityType::getType( typeName );

	if (!pType || !pType->canBeOnBase() || pType->isService())
	{
		PyErr_Format( PyExc_TypeError, "Invalid entity type %s", typeName );

		return NULL;
	}


	for (int i = headOffset; i < numArgs - tailOffset; ++i)
	{
		// Need to convert the argument to a dictionary
		PyObjectPtr pCurr = PyTuple_GET_ITEM( args, i );

		if (PyDict_Check( pCurr.get() ))
		{
			// It's already a dictionary
		}
		else if (PyDataSection::Check( pCurr.get() ))
		{
			DataSectionPtr pDataSection =
				static_cast<PyDataSection *>( pCurr.get() )-> pSection();
			PyObject * pNewDict = PyDict_New();
			pCurr = pNewDict;
			Py_DECREF( pNewDict );

			if (!pType->description().addSectionToDictionary( pDataSection,
						ScriptDict( pCurr ),
						EntityDescription::BASE_DATA |
							EntityDescription::CELL_DATA ))
			{
				PyErr_Format( PyExc_TypeError,
						"Failed to set values from data section (argument %d)",
						i + 1 );
				return NULL;
			}
		}
		else if (PyCellData::Check( pCurr.get() ))
		{
			pCurr = ((PyCellData *)pCurr.get())->getDict();
		}
		else
		{
			PyErr_Format( PyExc_TypeError,
					"Expect dict or PyDataSection for argument %d", i + 1 );
			return NULL;
		}

		if (PyDict_Merge( pDestDict.get(), pCurr.get(), /*override:*/ 0 ) == -1)
		{
			return NULL;
		}
	}

	return pType;
}


/**
 *	@param stream	The stream to add the data to.
 *
 *
 *	@return True is successful, other false. Note that the Python exception
 *		state is set on failure.
 */
bool EntityCreator::addCreateBaseData( BinaryOStream & stream,
					EntityTypePtr pType, PyObjectPtr pDict,
					PyObject * pCallback,
	   				EntityID * pNewID )
{

	if (pCallback && !PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_TypeError,
				"createBase*(): callback argument must be callable" );
		return false;
	}

	if (!pDict)
	{
		pDict = PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );
	}

	// TODO: Should have a warning for properties that are specified that are
	// not in the def file.

	// Let the receiving side allocate the id if this fails.
	EntityID newID = idClient_.getID();
	stream << newID << pType->id() << DatabaseID( 0 );
	stream << Mercury::Address( 0, 0 ); // dummy client address.
	stream << BW::string(); // encryptionKey
	stream << false;	// No logOnData
	stream << false;	// Not persistent-only

	if (!pType->description().addDictionaryToStream( ScriptDict( pDict ),
		stream, EntityDescription::BASE_DATA | EntityDescription::CELL_DATA ))
	{
		PyErr_SetString( PyExc_ValueError,
				"BigWorld.createBaseRemotely: Bad dictionary" );
		return false;
	}

	if (pType->canBeOnCell())
	{
		Vector3 position( 0.f, 0.f, 0.f );
		Vector3 direction( 0.f, 0.f, 0.f );
		SpaceID spaceID = 0;

		if (!Script::getValueFromDict( pDict.get(), POSITION_PROPERTY_NAME, position ) ||
			!Script::getValueFromDict( pDict.get(), DIRECTION_PROPERTY_NAME, direction ) ||
			!Script::getValueFromDict( pDict.get(), SPACE_ID_PROPERTY_NAME, spaceID ))
		{
			return false;
		}

		stream << position << direction << spaceID;
	}

	if (pNewID)
	{
		*pNewID = newID;
	}

	return true;
}


/**
 *	This method creates a base (or proxy) from data that is loaded from the
 *	database.
 *
 *	@return True if successful, otherwise false. On failure, the Python error
 *	state is set.
 */
bool EntityCreator::createBaseFromDB( const BW::string& entityType,
					const BW::string& name,
					PyObjectPtr pResultHandler )
{
	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	if (pType->description().pIdentifier() == NULL)
	{
		PyErr_Format( PyExc_ValueError,
				"createBaseFromDB: Unable to look up entity type without an "
				"Identifier field. Use *FromDBID() instead." );

		return false;
	}

	EntityID entityID = idClient_.getID();
	if (entityID == 0)
	{
		PyErr_SetString( PyExc_RuntimeError, "Unable to allocate an id" );
		return false;
	}

	Mercury::Bundle & bundle = this->dbApp().bundle();

	Mercury::ReplyMessageHandler * pHandler =
		new LoadEntityHandlerWithCallback( pResultHandler, pType->id(),
				entityID );

	bundle.startRequest( DBAppInterface::loadEntity, pHandler );
	bundle << pType->id() << entityID << true << name;

	this->dbApp().send();
	return true;
}


/**
 *	This method creates a base (or proxy) from data that is loaded from the
 *	database.
 *
 *	@return True if successful, otherwise false. On failure, the Python error
 *	state is set.
 */
bool EntityCreator::createBaseFromDB( const BW::string& entityType,
					DatabaseID dbID,
					PyObjectPtr pResultHandler )
{
	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError, "DatabaseID must be non-zero" );
		return false;
	}

	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	EntityID entityID = idClient_.getID();
	if (entityID == 0)
	{
		PyErr_SetString( PyExc_RuntimeError, "Unable to allocate an id" );
		return false;
	}

	Mercury::Bundle & bundle = this->dbApp().bundle();

	Mercury::ReplyMessageHandler * pHandler =
		new LoadEntityHandlerWithCallback( pResultHandler, pType->id(),
				entityID );

	bundle.startRequest( DBAppInterface::loadEntity, pHandler );
	bundle << pType->id() << entityID << false << dbID;

	this->dbApp().send();

	return true;
}

/**
 *	This method handles a create base from DB request from another component.
 * 	It loads the entity data from the database and creates the base locally.
 */
void EntityCreator::createBaseFromDB( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	struct LocalHelperFunctions
	{
		void sendFailedReply( const Mercury::Address& addr,
			const Mercury::UnpackedMessageHeader& header )
		{
			Mercury::ChannelSender sender( BaseApp::getChannel( addr ) );
			sender.bundle().startReply( header.replyID );
			sender.bundle() << LOAD_FROM_DB_FAILED;
		}
	} helper;

	EntityID entityID = idClient_.getID();
	if (entityID == 0)
	{
		ERROR_MSG( "EntityCreator::createBaseFromDB: "
				"Unable to allocate entity ID.\n" );
		helper.sendFailedReply( srcAddr, header );
		data.finish();
		return;
	}

	EntityTypeID entityTypeID;
	data >> entityTypeID;

	Mercury::Bundle & bundle = this->dbApp().bundle();

	Mercury::ReplyMessageHandler * pHandler =
		new LoadEntityHandlerWithReply( header.replyID, srcAddr, entityTypeID,
				entityID );

	bundle.startRequest( DBAppInterface::loadEntity, pHandler );
	bundle << entityTypeID << entityID;
	bundle.transfer( data, data.remainingLength() );

	this->dbApp().send();
}


/**
 *	This method creates a base (or proxy) on a remote BaseApp from data that is
 * 	loaded from the	database.
 */
PyObject* EntityCreator::createRemoteBaseFromDB( const char* entityType,
					DatabaseID dbID,
					const char* name,
					const Mercury::Address* pDestAddr,
					PyObjectPtr pCallback,
					const char* origAPIFuncName )
{
	// Exactly one is set.
	MF_ASSERT( (dbID == 0) != (name == NULL) );

	// pIdentifier()
	EntityTypePtr pEntityType = EntityType::getType( entityType,
				/*shouldExcludeServices*/true );

	if (pEntityType == NULL)
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid entity type %s", entityType );
		return NULL;
	}

	// In order to use this method, the entity type must have an identifier
	// field.
	if (name && (pEntityType->description().pIdentifier() == NULL))
	{
		PyErr_Format( PyExc_ValueError,
				"%s: Unable to look up entity type without an Identifier "
				"field. Use *FromDBID() instead.",
			origAPIFuncName ); 
		return NULL;
	}

	if ((pDestAddr && (*pDestAddr == intInterface_.address())) ||
		(!pDestAddr && this->shouldCreateLocally()))
	{
		// Create base locally
		bool isOK = (name) ?
						this->createBaseFromDB( entityType, name, pCallback ) :
						this->createBaseFromDB( entityType, dbID, pCallback );
		if (isOK)
		{
			Py_RETURN_NONE;
		}
		else
		{
			return NULL;
		}
	}
	else if (!pDestAddr)
	{	// Create base anywhere
		pDestAddr = &this->getAddressFromDestinationBaseApps();
	}

	if (pCallback && !PyCallable_Check( pCallback.get() ))
	{
		BW::string errorMsg( origAPIFuncName );
		errorMsg += ": callback argument must be callable";

		PyErr_SetString( PyExc_TypeError, errorMsg.c_str() );
		return NULL;
	}

	Mercury::ReplyMessageHandler * pHandler =
		new CreateBaseFromDBReplyHandler( pCallback.get() );

	Mercury::ChannelSender sender( BaseApp::getChannel( *pDestAddr ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startRequest( BaseAppIntInterface::createBaseFromDB, pHandler );
	bundle << pEntityType->id();

	if (name)
	{
		bundle << true;
		bundle.appendString( name, strlen( name ) );
	}
	else
	{
		bundle << false << dbID;
	}

	Py_RETURN_NONE;
}


/**
 *	This method creates a base entity on this app. It is used to create a client
 *	proxy or base entities.
 */
BasePtr EntityCreator::createBaseFromStream( BinaryIStream & data,
		Mercury::Address * pClientAddr, BW::string * pEncryptionKey )
{
	EntityID		id = 0;
	EntityTypeID	type;
	DatabaseID		databaseID;

	if (uint( data.remainingLength() ) <
			sizeof( id ) + sizeof( type ) + sizeof( databaseID ))
	{
		ERROR_MSG( "EntityCreator::createBaseFromStream: "
				"Not enough data (%d)\n",
			data.remainingLength() );
		data.finish();

		return NULL;
	}

	data >> id >> type >> databaseID;

	if (id == 0)
	{
		id = idClient_.getID();

		if (id == 0)
		{
			ERROR_MSG( "EntityCreator::createBaseFromStream: "
				"Unable to allocate id.\n" );
			data.finish();

			return NULL;
		}
	}

	if (BaseApp::instance().bases().findEntity( id ) != NULL)
	{
		ERROR_MSG( "EntityCreator::createBaseFromStream: dup id %u\n", id );
		data.finish();
		return NULL;
	}

	EntityTypePtr pType = EntityType::getType( type );

	if (!pType || !pType->canBeOnBase())
	{
		ERROR_MSG( "EntityCreator::createBaseFromStream: "
			"Invalid type %d\n", type );
		data.finish();
		return NULL;
	}

	Mercury::Address clientAddr;
	data >> clientAddr;

	if (clientAddr != Mercury::Address::NONE && !pType->isProxy())
	{
		ERROR_MSG( "EntityCreator::createBaseFromStream: "
				"have a client address %s, but creating non-proxy entity: %s\n",
			clientAddr.c_str(), pType->name() );
		data.finish();

		return NULL;
	}

	BW::string encryptionKey;
	data >> encryptionKey;

	BW::string logOnData;
	const BW::string * pLogOnData = NULL;
	bool hasLogOnData;
	data >> hasLogOnData;
	if (hasLogOnData)
	{
		data >> logOnData;
		pLogOnData = &logOnData;
	}

	bool hasPersistentDataOnly;
	data >> hasPersistentDataOnly;

	BasePtr pBase = pType->create( id, databaseID, data, hasPersistentDataOnly,
		pLogOnData );

	if (!pBase)
	{
		ERROR_MSG( "EntityCreator::createBaseFromStream: "
			"pType->create failed\n" );
		data.finish();
		return NULL;
	}

	if (pClientAddr)
	{
		*pClientAddr = clientAddr;
	}

	if (pEncryptionKey)
	{
		*pEncryptionKey = encryptionKey;
	}

	return pBase;
}


/**
 *	This method creates a base entity on this app. It is used to create a client
 *	proxy or base entities.
 */
void EntityCreator::createBaseWithCellData( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data,
		LoginHandler * pLoginHandler )
{
	// TRACE_MSG( "BaseApp::createBaseWithCellData:\n" );

	// The format of the data is as follows:
	// EntityID		id
	// EntityTypeID typeId;
	// DatabaseID	databaseID;
	//
	// For proxy:
	// 	clientAddr
	//#include "address_load_pair.hpp"

	// BASE_DATA
	// CELL_DATA (if needed)
	// Vector3 position (if needed)
	// Vector3 direction (if needed)

	Mercury::Address clientAddr = Mercury::Address::NONE;
	BW::string encryptionKey;

	BasePtr pBase =
		this->createBaseFromStream( data, &clientAddr, &encryptionKey );

	// Replying with an empty response is considered to be a failure report, so
	// any of the early returns from this method will cause this ChannelSender
	// to do just that.
	Mercury::ChannelSender sender( BaseApp::getChannel( srcAddr ) );
	sender.bundle().startReply( header.replyID );

	EntityMailBoxRef ref;
	ref.init();

	if (!pBase)
	{
		sender.bundle() << ref;
		return;
	}

	// Note: If the reply format changes, check that BaseApp::logOnAttempt is
	// okay.
	ref = pBase->baseEntityMailBoxRef();
	sender.bundle() << ref;

	// This is ugly. We should avoid differences in Base and Proxy.
	if (pBase->isProxy() && clientAddr != Mercury::Address::NONE)
	{
		Proxy * pProxy = (Proxy*)pBase.get();
		SessionKey loginKey = pProxy->prepareForLogin( clientAddr );
		sender.bundle() << loginKey;

		if (!encryptionKey.empty())
		{
			pProxy->encryptionKey( encryptionKey );
		}
	}
}


/**
 *	This method creates a base entity possibly on this BaseApp
 *	using template to populate the properties.
 */
ScriptObject EntityCreator::createBaseLocallyFromTemplate(
		const BW::string & entityType,
		const BW::string & templateID )
{
	const EntityID newID = idClient_.getID();
	const EntityTypeID typeID = EntityType::nameToIndex( entityType.c_str() );

	if (typeID == EntityType::INVALID_INDEX)
	{
		PyErr_Format( PyExc_ValueError,
				"createBaseLocallyFromTemplate: "
				"Invalid entity type '%s'", entityType.c_str() );
		return ScriptObject();
	}
	
	const BasePtr pBase =
			this->createBaseFromTemplate( newID, typeID, templateID );

	if (!pBase)
	{
		PyErr_Format( PyExc_RuntimeError,
				"createBaseLocallyFromTemplate: "
				"Failed to create entity of type '%s' by template '%s'",
				entityType.c_str(), templateID.c_str() );
		return ScriptObject();
	}
	else
	{
		return ScriptObject( pBase.get(), ScriptObject::FROM_BORROWED_REFERENCE );
	}
}


/**
 *	This method creates a base entity on a remote BaseApp
 *	using template to populate the properties.
 */
ScriptObject EntityCreator::createBaseRemotelyFromTemplate(
		const Mercury::Address & destAddr,
		const BW::string & entityType,
		const BW::string & templateID,
		const ScriptObject & callback )
{
	if (destAddr.ip == 0)
	{
		PyErr_SetString( PyExc_ValueError,
				"createBaseRemotelyFromTemplate: Invalid address" );
		return ScriptObject();
	}
	
	if (callback && !callback.isNone() && !callback.isCallable())
	{
		PyErr_SetString( PyExc_TypeError,
				"createBaseRemotelyFromTemplate: "
				"Callback argument must be callable" );
		return ScriptObject();
	}
	
	const EntityID newID = idClient_.getID();
	const EntityTypeID typeID = EntityType::nameToIndex( entityType.c_str() );

	if (typeID == EntityType::INVALID_INDEX)
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid entity type '%s'", entityType.c_str() );
		return ScriptObject();
	}
	
	Mercury::ReplyMessageHandler * pHandler =
			new CreateBaseReplyHandler( callback );

	Mercury::Channel & channel = BaseApp::getChannel( destAddr );

	std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );

	pBundle->startRequest( BaseAppIntInterface::createBaseFromTemplate,
		   pHandler );

	*pBundle << newID << typeID << templateID;
	channel.send( pBundle.get() );

	BaseEntityMailBox * const pBaseMailBox = 
		new BaseEntityMailBox( EntityType::getType(typeID), destAddr, newID );

	return ScriptObject( pBaseMailBox, ScriptObject::FROM_NEW_REFERENCE );
}


/**
 *	This method creates a base entity possibly on a remote BaseApp
 *	using template to populate the properties.
 */
ScriptObject EntityCreator::createBaseAnywhereFromTemplate(
		const BW::string & entityType,
		const BW::string & templateID,
		const ScriptObject & callback )
{
	if (callback && !callback.isNone() && !callback.isCallable())
	{
		PyErr_SetString( PyExc_TypeError,
				"createBaseAnywhereFromTemplate: "
				"Callback argument must be callable" );
		return ScriptObject();
	}

	const Mercury::Address & baseAppAddr = this->shouldCreateLocally() ?
				intInterface_.address() :
				this->getAddressFromDestinationBaseApps();

	if (baseAppAddr != intInterface_.address()) // remote baseapp
	{
		return this->createBaseRemotelyFromTemplate( baseAppAddr,
			entityType,
			templateID,
			callback );
	}

	const ScriptObject base = 
		this->createBaseLocallyFromTemplate( entityType, templateID );

	ScriptObject resultMailBox;
	
	if (base)
	{
		MF_ASSERT( Base::Check( base.get() ) );

		const EntityMailBoxRef mailBoxRef = 
			static_cast<const Base *>( base.get() )->baseEntityMailBoxRef();

		resultMailBox = 
			ScriptObject( PyEntityMailBox::constructFromRef( mailBoxRef ),
						  ScriptObject::FROM_NEW_REFERENCE );
	}

	if (callback && !callback.isNone())
	{
		ScriptObject arg = resultMailBox ? resultMailBox : ScriptObject::none();

		callback.callFunction( ScriptArgs::create(arg), ScriptErrorRetain() );
		if (Script::hasError())
		{
			return ScriptObject();
		}
	}

	return resultMailBox;
}


/**
 *	This method creates a base entity on this app. It is used to create a client
 *	proxy or base entities.
 */
void EntityCreator::createBaseFromTemplate( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	Mercury::ChannelSender sender( BaseApp::getChannel( srcAddr ) );
	sender.bundle().startReply( header.replyID );

	// The format of the data is as follows:
	// EntityID		id
	// EntityTypeID typeID;
	// string		templateID;

	EntityID	 id = 0;
	EntityTypeID typeID;
	BW::string	 templateID;

	data >> id >> typeID >> templateID;

	EntityMailBoxRef newBaseMbx;
	newBaseMbx.init();

	if (data.error())
	{
		ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
				   "Failed to read data\n" );
	}
	else
	{
		const BasePtr pBase =
				this->createBaseFromTemplate( id, typeID, templateID );

		if (!pBase)
		{
			ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
					   "Failed to create entity #%d of type #%d by template '%s'\n",
					   id, typeID, templateID.c_str());
		}
		else
		{
			newBaseMbx = pBase->baseEntityMailBoxRef();
		}
	}
	sender.bundle() << newBaseMbx;
	data.finish();
}


BasePtr EntityCreator::createBaseFromTemplate( EntityID id,
											   EntityTypeID typeID,
											   const BW::string & templateID )
{
	if (id == NULL_ENTITY_ID)
	{
		id = idClient_.getID();

		if (id == NULL_ENTITY_ID)
		{
			ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
					   "Unable to allocate id.\n" );
			return NULL;
		}
	}

	if (templateID.empty())
	{
		ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
				   "Invalid template id '%s'\n", templateID.c_str() );
		return NULL;
	}

	if (BaseApp::instance().bases().findEntity( id ) != NULL)
	{
		ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
				   "Duplicated entity id %u\n", id );
		return NULL;
	}

	const EntityTypePtr pType = EntityType::getType( typeID );

	if (!pType)
	{
		ERROR_MSG( "EntityCreator::createBaseFromTemplate: "
				   "Invalid entity type id %d\n", typeID );
		return NULL;
	}

	return pType->create( id, templateID );
}

/**
 *	This method is called to set the information that is used to decide which
 *	BaseApp BigWorld.createBaseAnywhere should create base entities on.
 */
void EntityCreator::setCreateBaseInfo( BinaryIStream & data )
{
	destinationBaseApps_.clear();
	data >> destinationBaseApps_;

	if (data.remainingLength() != 0 || data.error())
	{
		destinationBaseApps_.clear();
		ERROR_MSG( "EntityCreator::setCreateBaseInfo: "
					"Invalid stream received from BaseAppMgr\n" );
	}
	return;
}


/**
 *	This method returns whether it is valid for a createBaseAnywhere call to be
 *	created locally.
 */
bool EntityCreator::shouldCreateLocally() const
{
	return (this->getLoad() < BaseAppConfig::createBaseElsewhereThreshold()) &&
		!BaseApp::instance().isServiceApp();
}


/**
 *	This method returns an address from destinationBaseApps_.
 *	It attempts to select different addresses on each call for
 *	better BaseApp load balancing.
 */
const Mercury::Address & EntityCreator::getAddressFromDestinationBaseApps()
{
	if (destinationBaseApps_.empty())
	{
		return intInterface_.address();
	}

	double totalWeight = 0;
	double maxLoad = 0;

	for (DestinationBaseApps::iterator iter = destinationBaseApps_.begin();
			iter != destinationBaseApps_.end();
			++iter)
	{
		if (iter->load_ > maxLoad)
		{
			maxLoad = iter->load_;
		}
		totalWeight += iter->load_;
	}
	// add 0.01 in order to avoid zero weights
	maxLoad += 0.01;
	totalWeight = maxLoad * destinationBaseApps_.size() -
					totalWeight;

	double randPick = ((double)rand() / (double)RAND_MAX) * totalWeight;
	double curr = 0;

	for (DestinationBaseApps::iterator iter = destinationBaseApps_.begin();
			iter != destinationBaseApps_.end();
			++iter)
	{
		curr +=  maxLoad - iter->load_;

		if (curr >= randPick)
		{
			iter->load_ += 0.01;
			return iter->addr_;
		}
	}

	// Now something really bad happened as it must have chosen
	// one of the targets in the previous loop.
	// Return just a random one.
	ERROR_MSG( "EntityCreator::getAddressFromDestinationBaseApps: "
			"Bad calculation for a random destination BaseApp.\n" );

	int randIndex = rand() % destinationBaseApps_.size();
	return destinationBaseApps_[ randIndex ].addr_;
}


/**
 *	This method handles a notification of a BaseApp death.
 *
 *	If the dead BaseApp is our create base anywhere target, we temporarily
 *	revert to our own process to avoid any failures until we receive the next
 *	update from BaseAppMgr.
 *
 *	@param addr The address of the dead BaseApp.
 */
void EntityCreator::handleBaseAppDeath( const Mercury::Address & addr )
{
	for (DestinationBaseApps::iterator iter = destinationBaseApps_.begin();
			iter != destinationBaseApps_.end(); ++iter)
	{
		if (iter->addr_ == addr)
		{
			destinationBaseApps_.erase( iter );
			break;
		}
	}

	if (destinationBaseApps_.empty())
	{
		INFO_MSG( "EntityCreator::handleBaseAppDeath: "
				"Temporarily reverting remote entity creation to local app.\n" );
	}
}

BW_END_NAMESPACE

// entity_creator.cpp
