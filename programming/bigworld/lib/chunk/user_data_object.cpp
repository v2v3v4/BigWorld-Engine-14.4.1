#include "pch.hpp"
#include "user_data_object.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "entitydef/user_data_object_description_map.hpp"
#include "pyscript/pyobject_base.hpp"
#include "resmgr/bwresource.hpp"
#include <algorithm>

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "user_data_object.ipp"
#endif


namespace
{

UserDataObjectTypePtr s_baseType;

/*~ class BigWorld.UnresolvedUDORefException
 *  This custom exception is thrown when trying to access a reference to a
 *  UserDataObject that hasn't been loaded yet.
 */
PyObjectPtr s_udoRefException;

PyObjectPtr s_pLoadUDODict = NULL;

/**
 *	This is a helper class to release the static resources allocated by the
 *	UserDataObject class.
 */
class UserDataObjectIniter :
	public Script::InitTimeJob,
	public Script::FiniTimeJob
{
public:
	UserDataObjectIniter() : Script::InitTimeJob( 0 )
	{
	}

	virtual void init();

	virtual void fini()
	{
		s_udoRefException = NULL;
		s_pLoadUDODict = NULL;
	}
};

UserDataObjectIniter s_userDataObjectReferenceIniter;


void UserDataObjectIniter::init()
{
	BW_GUARD;
	PyObject * pBigWorldModule  = PyImport_AddModule( "BigWorld" );
	if (pBigWorldModule  == NULL)
	{
		PyErr_Print();
		CRITICAL_MSG( "Could not add module 'BigWorld'.\n" );
		return;
	}

	s_pLoadUDODict = PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );

	if (PyObject_SetAttrString( pBigWorldModule,
			USER_DATA_OBJECT_ATTR_STR, s_pLoadUDODict.get() ) == -1)
	{
		PyErr_Print();
		CRITICAL_MSG( "Could not create the 'BigWorld.%s' attribute.\n",
				USER_DATA_OBJECT_ATTR_STR );
	}
}

} // anonymous namespace


int PyUserDataObject_token = 1;
// Not sure where or how these are defined, so they are here for now...
PY_BASETYPEOBJECT( UserDataObject )
PY_BEGIN_METHODS( UserDataObject )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( UserDataObject )

/*~ attribute UserDataObject guid
	*  @components{ cell }
	*  This is the unique identifier for the UserDataObject. It is common across client,
	*  cell, and base applications.
	*  @type Read-only GUID (string)
	*/
	PY_ATTRIBUTE( guid )

	/*~ attribute UserDataObject position
	*  @components{ cell }
	*  The position attribute is the current location of the UserDataObject, in world
	*  coordinates.
	*  @type Read only Tuple of 3 floats as (x, y, z)
	*/
	PY_ATTRIBUTE( position )

	/*~ attribute UserDataObject yaw
	*  @components{ cell }
	*  yaw provides the yaw component of the direction attribute. It is the yaw
	*  of the current facing direction of the UserDataObject, in radians and in world
	*  space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( yaw )

	/*~ attribute UserDataObject pitch
	*  @components{ cell }
	*  pitch provides the pitch component of the direction attribute. It is
	*  the pitch of the current facing direction of the UserDataObject, in radians
	*  and in world space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( pitch )

	/*~ attribute UserDataObject roll
	*  @components{ cell }
	*  roll provides the roll component of the direction attribute. It is the
	*  roll of the current facing direction of the UserDataObject, in radians and in
	*  world space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( roll )
	/*~ attribute UserDataObject direction
	*  @components{ cell }
	*  The direction attribute is the current orientation of the UserDataObject, in
	*  world space.
	*  This property is roll, pitch and yaw combined (in that order).
	*  @type Read only Tuple of 3 floats as (roll, pitch, yaw)
	*/
	PY_ATTRIBUTE( direction )

PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: UserDataObject
// -----------------------------------------------------------------------------

/*static*/ UserDataObject::UdoMap UserDataObject::s_created_;

/**
 *	This static method returns a UDO by id if it has been created, or NULL
 *  otherwise.
 *
 *	@param guid		Unique id of the desired UDO.
 *	@return			UDO corresponding to the id, or NULL if the UDO with that
 *					id hasn't been loaded yet.
 */
/*static*/ UserDataObject* UserDataObject::get( const UniqueID& guid )
{
	BW_GUARD;
	UdoMap::iterator it = s_created_.find( guid );
	if ( it != s_created_.end() )
		return (*it).second;

	return NULL;
}


/**
 *	This method increments the count of ChunkUserDataObject instances that have
 *	a reference to this object.
 */
void UserDataObject::incChunkItemRefCount()
{
	++chunkItemRefCount_;
}


/**
 *	This method decrements the count of ChunkUserDataObject instances that have
 *	a reference to this object. When there are no more references, this object
 *	will be converted to an unloaded state. Note that there may still be Python
 *	references.
 */
void UserDataObject::decChunkItemRefCount()
{
	if (--chunkItemRefCount_ == 0)
	{
		this->unload();
	}
}


/**
 *  This static returns a loaded UDO. If necessary, it will create and an
 *	initialise a new UDO.
 *
 *	@param initData		Information used in the creation of the UDO
 *	@param pType		Python type of the UDO
 *	@return				New or recreated UDO
 */
/*static*/ UserDataObjectPtr UserDataObject::findOrLoad(
	const UserDataObjectInitData & initData, UserDataObjectTypePtr pType )
{
	BW_GUARD;
	UserDataObjectPtr pUDO( UserDataObject::get( initData.guid ) );

	if (!pUDO)
	{
		// Not created yet, so create it
		pUDO = UserDataObjectPtr(
				pType->newUserDataObject( initData.guid ),
				UserDataObjectPtr::STEAL_REFERENCE );
	}

	if (!pUDO->isLoaded())
	{
		pUDO->load( initData, pType );
	}
	else
	{
		const Vector3 & oldPos = pUDO->position();
		const Vector3 & newPos = initData.position;

		if (oldPos != newPos)
		{
			// Same geometry mapped into two different locations
			ERROR_MSG( "UserDataObject::findOrLoad: "
					"Reusing UDO %s but positions do not match. "
					"Old (%.2f, %.2f, %.2f). New (%.2f, %.2f, %.2f)\n",
				initData.guid.toString().c_str(),
				oldPos.x, oldPos.y, oldPos.z,
				newPos.x, newPos.y, newPos.z );
		}

		MF_ASSERT_DEV( pUDO->isInCollection() );
	}

	return pUDO;
}


/**
 *	This method returns whether this object is in BigWorld.userDataObjects. This
 *	should always be true when the object is loaded.
 */
bool UserDataObject::isInCollection() const
{
	BW::string guidStr = guid_.toString();
	char * guidCStr = const_cast<char*>( guidStr.c_str() );
	ScriptObject pDict = this->getUDODict();

	return PyMapping_HasKeyString( pDict.get(), guidCStr ) != 0;
}


/**
 *	This method converts this object to a loaded state.
 */
void UserDataObject::load( const UserDataObjectInitData & initData,
		UserDataObjectTypePtr pType )
{
	BW_GUARD;

	MF_ASSERT( !this->isLoaded() );
	MF_ASSERT( !initData.guid.toString().empty() );
	MF_ASSERT( isValidPosition( initData.position ) );
	MF_ASSERT( guid_== initData.guid );
	MF_ASSERT( pType );

	// Reuse the UDO object if it has already been created in an
	// unloaded state, using the new type.
	this->resetType( pType );

	globalPosition_ = initData.position;
	globalDirection_ = initData.direction;

	ScriptDict pDict = ScriptDict::create();
	pUserDataObjectType_->description().addToDictionary(
			initData.propertiesDS, pDict );

	//set the __dict__ property of myself to be this pDict
	if (PyObject_SetAttrString( this, "__dict__", pDict.get() ) == -1 )
	{
		WARNING_MSG("UserDataObject::init: "
					"Could not set __dict__ for user data object guid:%s\n",
				guid_.toString().c_str() );
		PyErr_Print();
	}

	isLoaded_ = true;
	this->addToCollection();

	// Now call the python init method */
	this->callScriptInit();
}


/**
 *	This method adds this object to the BigWorld.userDataObjects map.
 */
void UserDataObject::addToCollection()
{
	MF_ASSERT_DEV( !this->isInCollection() );

	ScriptObject pDict = this->getUDODict();

	BW::string guidStr = guid_.toString();
	char * guidCStr = const_cast<char*>(guidStr.c_str());

	if (PyMapping_SetItemString( pDict.get(), guidCStr, this ) == -1)
	{
		ERROR_MSG( "UserDataObject::addToCollection: Adding to dict failed\n" );
	}
}


/**
 *	This method adds this object to the BigWorld.userDataObjects map.
 */
void UserDataObject::removeFromCollection()
{
	MF_ASSERT_DEV( this->isInCollection() );

	ScriptObject pDict = this->getUDODict();

	// Remove from the collection
	MF_ASSERT( pDict );

	BW::string guidStr = guid_.toString();
	char *guid = const_cast<char*>( guidStr.c_str() );

	if (PyMapping_DelItemString( pDict.get(), guid ) == -1)
	{
		ERROR_MSG( "UserDataObject::removeFromCollection: "
					"Failed to removed '%s' from dictionary\n",
				guidStr.c_str() );
		PyErr_Clear();
	}
}


/**
 *	This method returns the dictionary containing the collection of all loaded
 *	UDOs.
 */
ScriptObject UserDataObject::getUDODict() const
{
	return ScriptObject( s_pLoadUDODict );
}


/**
 *	This method resets a UDO back to the unloaded state, clearing also its
 *	dicctionary, so links to other UDOs are broken, preventing leaks caused by
 *	circular references, etc.
 *	
 */
void UserDataObject::unload()
{
	BW_GUARD;

	MF_ASSERT( this->isLoaded() );

	// reset to the unloaded state
	if (s_baseType != NULL)
	{
		this->callScriptDel();

		if (PyObject_DelAttrString( this, "__dict__" ) == -1)
		{
			WARNING_MSG( "UserDataObject::unload: could not delete __dict__"
				" for user data object guid: %s\n",
				guid_.toString().c_str() );
			PyErr_Print();
		}

		this->resetType( s_baseType );
	}

	isLoaded_ = false;

	this->removeFromCollection();
}


/**
 *	This static method creates a UDO in an unloaded state, which is called a
 *  reference, that will be properly loaded at a later time when the chunk it
 *  lives in is loaded. This is used for links.
 *
 *  @param guid		String containing the unique id of the UDO to create.
 *  @return			New UDO reference.
 */
/*static*/ UserDataObject* UserDataObject::createRef( const BW::string& guid )
{
	BW_GUARD;
	if (guid.empty())
	{
		return NULL;
	}

	return UserDataObject::createRef( UniqueID( guid ) );
}


/**
 *	This static method creates a UDO in an unloaded state, which is called a
 *  reference, that will be properly loaded at a later time when the chunk it
 *  lives in is loaded. This is used for links.
 *
 *  @param guid		Unique identifier of the UDO to create.
 *  @return			New UDO reference.
 */
/*static*/ UserDataObject* UserDataObject::createRef( const UniqueID & guid )
{
	BW_GUARD;
	UserDataObject * pUDO = UserDataObject::get( guid );

	if (pUDO == NULL)
	{
		if (s_baseType != NULL)
		{
			pUDO = s_baseType->newUserDataObject( guid );
		}
	}
	else
	{
		Py_INCREF( pUDO );
	}

	return pUDO;
}


/**
 *	This static method creates the base type for a UserDataObjectRef type,
 *  which is used in UDOs when they are in an unloaded state. And example of
 *  a UDO in this state would be a UDO_REF property in a UDO that points to a
 *	UDO in an unloaded chunk.
 */
/*static*/ bool UserDataObject::createRefType()
{
	BW_GUARD;

	// Initialise our custom exception
	PyObject* module = PyImport_AddModule( "BigWorld" );
	s_udoRefException = PyErr_NewException(
		const_cast<char *>("BigWorld.UnresolvedUDORefException"), NULL, NULL );
	// Since 's_udoRefException' is a smart pointer that
	// incremented the reference, so its safe for
	// PyModule_AddObject to steal the reference.
	PyModule_AddObject( module, "UnresolvedUDORefException",
		s_udoRefException.getObject() );

	// Initialise the base user data object reference.
	PyObjectPtr pModule(
		PyImport_ImportModule( "UserDataObjectRef" ),
		PyObjectPtr::STEAL_REFERENCE );
	if (!pModule)
	{
		ERROR_MSG( "UserDataObject::createRefType: "
				"Could not load module UserDataObjectRef\n");
		PyErr_Print();
		return false;
	}

	PyObjectPtr pClass(
		PyObject_GetAttrString( pModule.getObject(), "UserDataObjectRef" ),
		PyObjectPtr::STEAL_REFERENCE );

	if (!pClass)
	{
		ERROR_MSG( "UserDataObject::createRefType: "
				"Could not get base class UserDataObjectRef\n" );
		PyErr_Print();
		return false;
	}

	s_baseType = new UserDataObjectType( UserDataObjectDescription(),
										 (PyTypeObject*)pClass.getObject() );

	return true;
}


/**
 *	The constructor for UserDataObject.
 *
 */
UserDataObject::UserDataObject( UserDataObjectTypePtr pUserDataObjectType,
	   		const UniqueID & guid ):
		PyObjectPlus( pUserDataObjectType->pPyType(), true ),
		pUserDataObjectType_( pUserDataObjectType ),
		guid_( guid ),
		isLoaded_( false ),
		chunkItemRefCount_( 0 )
{
	s_created_[ guid_ ] = this;
}


/**
 *	Destructor
 */
UserDataObject::~UserDataObject()
{
	BW_GUARD;
	s_created_.erase( guid_ );
}


/**
 *  This method returns true if the UDO has been fully loaded and is ready to
 *  be used.
 */
bool UserDataObject::isLoaded() const
{
	return isLoaded_;
}


/* Call the init method in the python script. */
void UserDataObject::callScriptInit()
{
	BW_GUARD;
	// Call the __init__ method of the object, if it has one.
	PyObject * pFunction = PyObject_GetAttrString( this,
		const_cast< char * >( "__init__" ) );

	if (pFunction != NULL)
	{
		PyObject * pResult = PyObject_CallFunction( pFunction, const_cast<char *>("()") );
		PY_ERROR_CHECK()
		Py_XDECREF( pResult );
		Py_DECREF( pFunction );
	}
	else
	{
		PyErr_Clear();
	}
}


/* Call the del method in the python script. */
void UserDataObject::callScriptDel()
{
	BW_GUARD;
	// Call the __init__ method of the object, if it has one.
	PyObject * pFunction = PyObject_GetAttrString( this,
		const_cast< char * >( "__del__" ) );

	if (pFunction != NULL)
	{
		PyObject * pResult = PyObject_CallFunction( pFunction, const_cast<char *>("()") );
		PY_ERROR_CHECK()
		Py_XDECREF( pResult );
		Py_DECREF( pFunction );
	}
	else
	{
		PyErr_Clear();
	}
}


/**
 *	This method resets our type object, e.g. after a reloadScript() operation.
 */
void UserDataObject::resetType( UserDataObjectTypePtr pNewType )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pNewType )
	{
		return;
	}

	pUserDataObjectType_ = pNewType;
	if (PyObject_SetAttrString( this, "__class__",
		reinterpret_cast< PyObject* >( pUserDataObjectType_->pPyType() ) ) == -1)
	{
		ERROR_MSG( "UserDataObject::resetType: "
			"Failed to update __class__ for %s to %s.\n",
			guid_.toString().c_str(),
			pNewType->description().name().c_str() );
		PyErr_Print();
	}
}

/**
 *	This method is responsible for getting script attributes associated with
 *	this object.
 */
ScriptObject UserDataObject::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	if (attr[0] != '_' &&
			strcmp( attr, "guid" ) != 0 && 
			!this->isLoaded())
	{
		IF_NOT_MF_ASSERT_DEV( s_udoRefException != NULL )
		{
			PyErr_Format( PyExc_AttributeError, "Cannot access attribute '%s' in UserDataObject '%s', not loaded and no exception set.",
						attr, guid_.toString().c_str() );
			return ScriptObject();
		}

		// Only allow getting the 'guid' when the UDO hasn't been loaded
		BW::string excstr =
			"Cannot access attribute '" + BW::string(attr) +
			"' in UserDataObject " + guid_.toString() +
			", it has not been loaded yet.";
		PyErr_SetString(
			s_udoRefException.getObject(),
			excstr.c_str() );
		return ScriptObject();
	}

	// finally let the base class have the scraps (ephemeral props, etc.)
	return PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	This method is responsible for setting script attributes associated with
 *	this object.
 *  Therefore in this method we search to see if we have a description available for the
 *  property, if not we allow it to be changed. This allows python scripts to have temporary
 *  scratchpad variables. However they will not be retained if the chunk is unloaded
 *  or the server is shut down.
*/
bool UserDataObject::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;
	
	const char * attr = attrObj.c_str();

	// If all my properties have been loaded do not allow them to change
	if (isLoaded_)
	{
		bool hasProperty = pUserDataObjectType_->hasProperty( attr );
		if (hasProperty == true)
		{
			PyErr_Format( PyExc_AttributeError,
				"UserDataObject.%s is a persistent UserDataObject"
				" property and cannot be changed", attr);
			return false;
		}
	}

	// Don't support changing properties other than the required built-in ones
	if (attr[0] != '_')
	{
		WARNING_MSG( "UserDataObject::pySetAttribute: Changing User Data "
				"Object attributes is not supported (type: %s, guid: %s, "
				"attribute: %s)\n",
				pUserDataObjectType_->description().name().c_str(),
				guid_.toString().c_str(),
				attr );
	}

	return this->PyObjectPlus::pySetAttribute( attrObj, value );
}

BW_END_NAMESPACE

// user_data_object.cpp
