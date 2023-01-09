#include "pch.hpp"
#include "user_data_object.hpp"
#include "user_data_object_type.hpp"
#include "entitydef/user_data_object_description_map.hpp"
#include "entitydef/constants.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

#if defined( __GNUC__ )
#include <tr1/type_traits>
#else /* defined( __GNUC__ ) */
#include <type_traits>
#endif /* defined( __GNUC__ ) */

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "user_data_object_type.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: UserDataObjectType
// -----------------------------------------------------------------------------

/**
 *	Constructor
 *
 *	@param description	UserDataObject data description
 *	@param pType The Python class that is associated with this user data object type.
 *					This object steals the reference from the caller
 */
UserDataObjectType::UserDataObjectType( const UserDataObjectDescription& description,
		PyTypeObject * pType ) :
	description_( description ),
	pPyType_( pType )
{
	BW_GUARD;
	MF_ASSERT_DEV( !pType ||
			(PyType_Check( pType ) &&
			PyObject_IsSubclass( (PyObject *)pType,
				(PyObject*)&UserDataObject::s_type_ )) );

}


/**
 *	Destructor
 */
UserDataObjectType::~UserDataObjectType()
{
	BW_GUARD;
	Py_XDECREF( pPyType_ );
}


/**
 *	This method creates an user data object of this type.
 *
 *  @return A new UserDataObject of this type that has not been initialised.
 */
UserDataObject * UserDataObjectType::newUserDataObject(
		const UniqueID & guid ) const
{
	BW_GUARD;
	if (pPyType_ == NULL)
	{
		ERROR_MSG( "UserDataObjectType::newUserDataObject: "
					"%s is not a cell user data object type.\n",
				description_.name().c_str() );
		return NULL;
	}

	BW_STATIC_ASSERT( std::tr1::is_polymorphic<UserDataObject>::value == false,
		UserDataObject_is_virtual_but_uses_PyType_GenericAlloc );

	PyObject * pObject = PyType_GenericAlloc( pPyType_, 0 );

	if (!pObject)
	{
		PyErr_Print();
		ERROR_MSG( "UserDataObjectType::newUserDataObject: "
						"Allocation failed\n" );
		return NULL;
	}

	IF_NOT_MF_ASSERT_DEV( UserDataObject::Check( pObject ) )
	{
		return NULL;
	}

	UserDataObject * pNewUserDataObject =
		new (pObject) UserDataObject( const_cast<UserDataObjectType *>( this ),
				guid );

	return pNewUserDataObject;
}


namespace
{
/**
 *	This utility method loads the user data object class object. Returns NULL pointer
 * 	if there is an error in loading the class object.
 */
PyObjectPtr importUserDataObjectClass( const UserDataObjectDescription& desc )
{
	BW_GUARD;
	// Non-const name needed by Python API functions.
	char * name = const_cast<char *>( desc.name().c_str() );

	PyObjectPtr pModule( PyImport_ImportModule( name ),
			PyObjectPtr::STEAL_REFERENCE );

	if (!pModule)
	{
		WARNING_MSG( "UserDataObjectType::importUserDataObjectClass: Could not load module %s\n",
				name );
		PyErr_Clear();
		pModule = PyObjectPtr(PyImport_ImportModule( "BigWorld" ),
			PyObjectPtr::STEAL_REFERENCE );
		if (!pModule){
			ERROR_MSG( "UserDataObjectType::importUserDataObjectClass: Could not load module %s\n",
					"BigWorld");
			return PyObjectPtr();
		}
	}
	PyObjectPtr pClass = PyObjectPtr(
			PyObject_GetAttrString( pModule.getObject(), name ),
			PyObjectPtr::STEAL_REFERENCE );

	if (!pClass)
	{
		WARNING_MSG( "UserDataObjectType::importUserDataObjectClass: Could not find class %s\n",
				name );
		pClass = PyObjectPtr( PyObject_GetAttrString( pModule.getObject(), "UserDataObject" ),
			PyObjectPtr::STEAL_REFERENCE );
		if (!pClass)
		{
			ERROR_MSG( "UserDataObjectType::importUserDataObjectClass: Could not find class %s\n",
					"UserDataObject" );
			return PyObjectPtr();
		}
		PyErr_Clear();
	}
	int result = PyObject_IsSubclass( pClass.getObject(),
						(PyObject *)&UserDataObject::s_type_ );
	if (result != 1)
	{
		if (result == -1)
			PyErr_Print();
		ERROR_MSG( "UserDataObject::init: %s does not derive from UserDataObject\n",
				name );
		return PyObjectPtr();
	}

	IF_NOT_MF_ASSERT_DEV( PyType_Check( pClass.getObject() ) )
	{
		return PyObjectPtr();
	}

	return pClass;
}

} // end of anonymous namespace


// -----------------------------------------------------------------------------
// Section: UserDataObjectType static methods
// -----------------------------------------------------------------------------

/**
 *	This method needs to be called in order to initialise the different user data object
 *	types.
 *
 *	@return True on success, false otherwise.
 */
bool UserDataObjectType::init()
{
	return UserDataObjectType::load( s_curTypes_, 
		s_typesInDifferentDomain_) &&
			UserDataObject::createRefType();
}

/**
 *	This method loads all the user data type objects and puts them into types.
 *
 *	@return True on success, false otherwise.
 */
bool UserDataObjectType::load( UserDataObjectTypes& types, UserDataObjectTypesInDifferentDomain& typesInDifferentDomain )
{
	BW_GUARD;
	DataSectionPtr pSection = BWResource::openSection(
		EntityDef::Constants::userDataObjectsFile() );
	UserDataObjectDescriptionMap userDataObjectDescriptionMap;

	// Missing file is OK.
	if (!pSection)
	{
		WARNING_MSG( "UserDataObjectType::init: "
			"No user data object mapping present\n" );

		return true;
	}

	// If file is present but doesn't parse, that's not OK.
	if (!userDataObjectDescriptionMap.parse( pSection ))
	{
		ERROR_MSG( "UserDataObjectType::init: "
			"Failed to parse UserDataObjectDescriptionMap\n" );

		return false;
	}

	bool succeeded = true;

	for (UserDataObjectDescriptionMap::DescriptionMap::const_iterator i = userDataObjectDescriptionMap.begin();
			i != userDataObjectDescriptionMap.end(); i++)
	{
		// Don't try to import the object classes if the domain is not our current domain
#ifndef EDITOR_ENABLED
#ifdef MF_SERVER
		int domainMask = UserDataObjectDescription::CELL;
#else
		int domainMask = UserDataObjectDescription::CLIENT;
#endif
		if ((i->second.domain() & domainMask) == 0)
		{
			typesInDifferentDomain.insert( i->second.name() );
			continue;
		}
#endif
		const UserDataObjectDescription& desc = i->second;
		PyObjectPtr pClass = importUserDataObjectClass( desc );
		if (pClass)
		{
			// Reference consumed by UserDataObjectType constructor
			Py_INCREF( pClass.getObject() );
		}
		else
		{
			succeeded = false;
			continue;
		}
		types[desc.name()] = new UserDataObjectType( desc, (PyTypeObject *)pClass.getObject() );
	}

	return succeeded;
}

/**
 *	This static method migrates the collection of user data objects to
 *	a reloaded version of their modules.
 */
void UserDataObjectType::migrate( UserDataObjectTypes& types,
		UserDataObjectTypesInDifferentDomain& typesInDifferentDomain)
{
	BW_GUARD;
	s_curTypes_ = types;
	s_typesInDifferentDomain_ = typesInDifferentDomain;

	// Update the __class__ of all user data objects in BigWorld.userDataObjects
	PyObject* pyBigWorld  = PyImport_AddModule( "BigWorld" );
	IF_NOT_MF_ASSERT_DEV( pyBigWorld )
	{
		return;
	}

	// BigWorld.userDataObjects is a WeakValueDictionary, not a dict.
	PyObjectPtr pyDict(
			PyObject_GetAttrString( pyBigWorld, USER_DATA_OBJECT_ATTR_STR ),
			PyObjectPtr::STEAL_REFERENCE );
	if (pyDict)
	{
		IF_NOT_MF_ASSERT_DEV( PyMapping_Check( pyDict.getObject() ) )
		{
			return;
		}

		PyObjectPtr pyValues = PyMapping_Values( pyDict.getObject() );
		if (pyValues)
		{
			size_t numItems = PyList_GET_SIZE( pyValues.getObject() );
			for ( size_t i = 0; i < numItems; ++i )
			{
				PyObject* pyValue = PyList_GET_ITEM( pyValues.getObject(), i );
				IF_NOT_MF_ASSERT_DEV( UserDataObject::Check( pyValue ) )
				{
					return;
				}

				UserDataObject* pUDO = static_cast<UserDataObject*>( pyValue );

				const BW::string& typeName =
						pUDO->getType().description_.name();

				UserDataObjectTypes::iterator pItem =
						s_curTypes_.find( typeName );
				if (pItem != s_curTypes_.end())
				{
					 pUDO->resetType( pItem->second );
				}
				else
				{
					ERROR_MSG( "UserDataObjectType::migrate: %s is no longer "
							"a valid type! Still using old module.\n",
							typeName.c_str() );
				}
			}
		}
		else
		{
			ERROR_MSG( "UserDataObjectType::migrate: Failed to get user data "
					"objects from BigWorld.%s\n", USER_DATA_OBJECT_ATTR_STR );
			PyErr_Print();
		}
	}
	else
	{
		ERROR_MSG( "UserDataObjectType::migrate: Failed to find BigWorld.%s\n",
				USER_DATA_OBJECT_ATTR_STR );
		PyErr_Print();
	}
}

/**
 *	Sets our type object. Consumes reference to pPyType.
 */
void UserDataObjectType::setPyType( PyTypeObject * pPyType )
{
	BW_GUARD;
	Py_XDECREF( pPyType_ );
	pPyType_ = pPyType;
}


/**
 *	This static method is used to get the user data object type associated with the input
 *	name.
 */
UserDataObjectTypePtr UserDataObjectType::getType( const char * className )
{
	BW_GUARD;
	UserDataObjectTypes::iterator result = s_curTypes_.find( className );
	if (result == s_curTypes_.end() )
	{
		return NULL;
	}
	else
	{
		return result->second;
	}
}


/**
 *	This static method checks if a user data object type is defined, but in a different domain
 *	@return true if the type is defined false otherwise
 */
bool UserDataObjectType::knownTypeInDifferentDomain( const char* className )
{
	BW_GUARD;

	return s_typesInDifferentDomain_.find(className) != s_typesInDifferentDomain_.end();
}

/**
 *	This static method is used to get the vector of user data object types.
 */
UserDataObjectTypes& UserDataObjectType::getTypes()
{
	return s_curTypes_;
}

/**
 *	This method returns the DataDescription associated with the attribute with
 *	the input name. If no such DataDescription exists, NULL is returned.
 */
bool UserDataObjectType::hasProperty( const char * attr ) const
{
	return description_.findProperty( attr ) != NULL;
}

/**
 *	Clean up static data. Used during shutdown.
 */
void UserDataObjectType::clearStatics()
{
	s_curTypes_.clear();
}

/// static initialisers
UserDataObjectTypes UserDataObjectType::s_curTypes_;
UserDataObjectTypesInDifferentDomain UserDataObjectType::s_typesInDifferentDomain_;

BW_END_NAMESPACE

// user_data_object_type.cpp
