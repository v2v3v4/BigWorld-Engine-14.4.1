#include "pch.hpp"

#include "entity_type.hpp"

#include "connection_control.hpp"
#include "entity.hpp"
#include "py_entity.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/entity_def_constants.hpp"
#include "connection/smart_server_connection.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/md5.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/script_data_sink.hpp"

#include "pyscript/script.hpp"

#include "resmgr/bwresource.hpp"

#if defined( __GNUC__ )
#include <tr1/type_traits>
#else /* defined( __GNUC__ ) */
#include <type_traits>
#endif /* defined( __GNUC__ ) */

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

// Static variable instantiations
BW::vector<EntityType*>			EntityType::s_types_;
BW::map<BW::string,EntityTypeID>	EntityType::s_typeNames_;
EntityDefConstants 					EntityType::s_entityDefConstants_;

/// Constructor (private)
EntityType::EntityType( const EntityDescription & description,
			PyObject * pModule,
			PyTypeObject * pClass ):
	description_( description ),
	pModule_( pModule ),
	pClass_( pClass ),
	pPlayerClass_( NULL )
{
	BW_GUARD;	
}


/// Destructor
EntityType::~EntityType()
{
	BW_GUARD;
	Py_XDECREF( pPlayerClass_ );
	Py_XDECREF( pClass_ );
}


/*static*/ void EntityType::fini()
{
	BW_GUARD;
	for (uint i = 0; i < s_types_.size(); i++)
	{
		bw_safe_delete(s_types_[i]);
	}
	s_types_.clear();
}


/**
 * Static function to return the EntityType instance for a given type id
 */
EntityType * EntityType::find( uint type )
{
	BW_GUARD;
	if (type >= s_types_.size())
	{
		CRITICAL_MSG( "EntityType::find: "
			"No such type index %d\n", type );
		return NULL;
	}
	return s_types_[ type ];
}


/**
 * Static function to return the EntityType instance for a given type name
 */
EntityType * EntityType::find( const BW::string & name )
{
	BW_GUARD;
	EntityTypeID	type = 0;

	BW::map<BW::string,EntityTypeID>::iterator found =
		s_typeNames_.find( name );

	return (found != s_typeNames_.end()) ? s_types_[found->second] : NULL;
}

//extern void memNow( const BW::string& token );

/**
 * Static function to initialise the entity type stuff
 * Direct copy from Entity::init in the server.
 */
bool EntityType::init()
{
	BW_GUARD;
	bool succeeded = true;

	// Initialise the entity descriptions.
	DataSectionPtr	pEntitiesList =
		BWResource::openSection( EntityDef::Constants::entitiesFile() );

	static EntityDescriptionMap entityDescriptionMap;	// static to keep refs

	if (entityDescriptionMap.parse( pEntitiesList,
            &ClientInterface::Range::entityPropertyRange,
            &ClientInterface::Range::entityMethodRange,
            &BaseAppExtInterface::Range::baseEntityMethodRange,
            &BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		s_entityDefConstants_ = EntityDefConstants(
			entityDescriptionMap.digest(),
			entityDescriptionMap.maxExposedClientMethodCount(),
			entityDescriptionMap.maxExposedBaseMethodCount(),
			entityDescriptionMap.maxExposedCellMethodCount() );
	}
	else
	{
		CRITICAL_MSG( "EntityType::init: "
				"EntityDescriptionMap::parse failed\n" );
	}

	// Create an EntityType for each EntityDescription read in.
	int numEntityTypes = entityDescriptionMap.size();
	s_types_.resize( numEntityTypes );

	if (numEntityTypes <= 0)
	{
		ERROR_MSG( "EntityType::init: There are no entity descriptions!\n" );
		return false;
	}

//	memNow( "Before ET loop" );
	int numClientTypes = 0;
	for (int i = 0; i < numEntityTypes; i++)
	{
		const EntityDescription & currDesc =
			entityDescriptionMap.entityDescription( i );

		if (!currDesc.isClientType())
		{
			continue;
		}

		// Load the module
		PyObject * pModule = PyImport_ImportModule(
			const_cast<char*>( currDesc.name().c_str() ) );
		if (pModule == NULL)
		{
			PyErr_PrintEx(0);
			CRITICAL_MSG( "EntityType::init: Could not load module %s.py\n",
				currDesc.name().c_str() );
			return false;
		}

		// Find the class
		PyObject * pClass = NULL;
		if (pModule != NULL)
		{
			pClass = PyObject_GetAttrString( pModule,
				const_cast<char *>( currDesc.name().c_str() ) );
		}

		if (PyErr_Occurred())
		{
			PyErr_Clear();
		}

		if (pClass != NULL)
		{
			if (!currDesc.checkMethods( currDesc.client().internalMethods(),
				ScriptObject( pClass, ScriptObject::FROM_BORROWED_REFERENCE ) ))
			{
				ERROR_MSG( "EntityType::init: class %s has a missing method.\n",
					currDesc.name().c_str() );
				succeeded = false;
			}
			else if (PyObject_IsSubclass( pClass, (PyObject *)&PyEntity::s_type_ ) != 1)
			{
				PyErr_Clear();
				ERROR_MSG( "EntityType::init: "
						"class %s does not derive from BigWorld.Entity.\n",
					currDesc.name().c_str() );
				succeeded = false;
			}
			else
			{
				EntityType * pType = new EntityType( currDesc,
						pModule, (PyTypeObject *)pClass );
				s_types_[ currDesc.clientIndex() ] = pType;
				s_typeNames_[currDesc.name()] = currDesc.clientIndex();
				numClientTypes =
					std::max( numClientTypes, currDesc.clientIndex() + 1 );
			}
		}
		else
		{
			succeeded = false;
			CRITICAL_MSG( "EntityType::init: Could not find class %s\n",
					currDesc.name().c_str() );
		}

	}
	s_types_.resize( numClientTypes );

	return succeeded;
}


/**
 * Static function to reload the entity scripts, and update all entity class
 * instances to be the new type
 */
bool EntityType::reload()
{
	BW_GUARD;
	// Go through all the classes we have and surreptitiously
	// swap any instances of them for instances of the new class
	for (uint i = 0; i < s_types_.size(); i++)
	{
		// types could only contain NULL if init failed
		MF_ASSERT_DEV( s_types_[i] != NULL && s_types_[i]->pModule_ != NULL );

		EntityType & et = *s_types_[i];

		// reload the module
		PyObject * pReloadedModule = PyImport_ReloadModule( et.pModule_ );
		if (pReloadedModule == NULL)
		{
			if (PyErr_Occurred())
			{
				PyErr_PrintEx(0);
				PyErr_Clear();
			}
			continue;
		}
		et.pModule_ = pReloadedModule;

		if (et.pClass_ != NULL)
		{
			et.swizzleClass( et.pClass_ );

			const EntityDescription & desc = et.description_;

			if (!desc.checkMethods( desc.client().internalMethods(),
				ScriptObject( (PyObject *)et.pClass_,
				ScriptObject::FROM_BORROWED_REFERENCE ) ))
			{
				ERROR_MSG( "EntityType::init: class %s has a missing method.\n",
					desc.name().c_str() );
			}
		}

		if (et.pPlayerClass_ != NULL)
		{
			et.swizzleClass( et.pPlayerClass_ );
		}
	}


	PyErr_Clear();

	return true;
}


/**
 *	Private method to swizzle references to pOldClass
 */
void EntityType::swizzleClass( PyTypeObject *& pOldClass )
{
	BW_GUARD;
	// ok, look for a newer class for this instance
	const char * className = pOldClass->tp_name;
	PyObject * pObjNewClass = PyObject_GetAttrString( pModule_, className );

	if (!pObjNewClass)
	{
		WARNING_MSG( "EntityType::reload: "
			"Class '%s' has disappeared!", className );
		PyErr_Clear();
		return;
	}

	if (!PyObject_IsSubclass( pObjNewClass, (PyObject *)&PyEntity::s_type_ ))
	{
		WARNING_MSG( "EntityType::reload: "
			"Class '%s' does not derive from Entity!", className );
		return;
	}

	PyTypeObject * pNewClass = (PyTypeObject*)pObjNewClass;

	// now find all old instances and replace with new ones
	// (we don't replace the class in existing method objects 'tho )
	/*
	// This would only work if Py_TRACE_REFS was on
	for (PyObject * i = Py_None->_ob_next; i != Py_None; i = i->_ob_next)
	{
		if (!PyInstance_Check( i )) continue;
		PyInstanceObject * pInst = (PyInstanceObject*)i;
		if (pInst->in_class == pClass) continue;

		Py_DECREF( pInst->in_class );
		pInst->in_class = pNewClass;
		Py_INCREF( pInst->in_class );
	}
	*/

	// Ask every entity to do it. No-one but the Entity objects themselves
	//  should actually have references to the instances that we're swizzling,
	//  so this is pretty thorough.
	for (Entity::Census::iterator iter = Entity::census_.begin();
		iter != Entity::census_.end();
		iter++)
	{
		Entity * pEntity = *iter;
		if (!pEntity->isDestroyed())
		{
			pEntity->pPyEntity()->swizzleClass( pOldClass, pNewClass );
		}
	}

	// use this new class from now on
	Py_DECREF( pOldClass );
	pOldClass = pNewClass;
}


/**
 *	Get all the preloads from all the classes, and return them
 *	as a list of model names.
 */
PyObject * EntityType::getPreloads( void )
{
	BW_GUARD;
	PyObject * pList = PyList_New(0);
	PyObject * pArgs = Py_BuildValue("(O)", pList );

	for (uint i=0; i < s_types_.size(); i++)
	{
		PyObject * pFn = PyObject_GetAttrString(
			s_types_[i]->pModule_, "preload" );
		if (pFn == NULL)
		{
			PyErr_Clear();
			continue;
		}

		Py_INCREF( pArgs );
		Script::call( pFn, pArgs, "EntityType::getPreloads: ", true );
	}

	Py_DECREF( pArgs );
	return pList;
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. If a data section is passed in, the values are read from
 *	it, otherwise entries get their default values.
 */
PyObject * EntityType::newDictionary( DataSectionPtr pSection,
		bool includeOwnClient ) const
{
	BW_GUARD;
	PyObject * pDict = PyDict_New();

	for (uint i=0; i < description_.propertyCount(); i++)
	{
		DataDescription * pDD = description_.property(i);

		if (!pDD->isClientServerData() ||
				(!includeOwnClient && pDD->isOwnClientData()))
		{
			// if we're not interested in it, don't set it.
			continue;
		}

		DataSectionPtr	pSubSection;

		PyObjectPtr pValue = NULL;

		// Can we get it from the DataSection?
		if (pSection &&
			(pSubSection = pSection->openSection( pDD->name() )))
		{
			ScriptDataSink sink;
			MF_VERIFY( pDD->createFromSection( pSubSection, sink ) );
			pValue = sink.finalise();
		}
		else	// ok, resort to the default then
		{
			pValue = pDD->pInitialValue();
		}

		PyDict_SetItemString( pDict,
			const_cast<char*>( pDD->name().c_str() ), pValue.getObject() );
	}

	return pDict;
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. It streams the properties from the input stream.
 *	This is only used for creating the player entity, and thus only the
 *	base-and-client data or the cell-and-[own|]client data is on the stream.
 */
PyObject * EntityType::newDictionary( BinaryIStream & stream,
		StreamContents contents ) const
{
	BW_GUARD;

	const bool includeOwnClient = (contents == TAGGED_CELL_PLAYER_DATA);

	// If there is no stream, return a default dictionary.
	if (stream.remainingLength() == 0)
	{
		return this->newDictionary( /* pSection */ NULL, includeOwnClient );
	}

	PyObject * pDict = PyDict_New();

	if (contents < TAGGED_CELL_PLAYER_DATA)
	{
		const int dataDomainsToRead =
			((contents == BASE_PLAYER_DATA) ?
				EntityDescription::FROM_BASE_TO_CLIENT_DATA :
				EntityDescription::FROM_CELL_TO_CLIENT_DATA);
		description_.readStreamToDict( stream, dataDomainsToRead, 
			ScriptDict( pDict, ScriptObject::FROM_BORROWED_REFERENCE ) );
	}
	else
	{
		description_.readTaggedClientStreamToDict( stream, ScriptDict( 
			pDict, ScriptObject::FROM_BORROWED_REFERENCE) , includeOwnClient );
	}

	return pDict;
}


/**
 *	This method returns the Python player class associated with this type. If no
 *	such class exists, NULL is returned.
 */
PyTypeObject * EntityType::pPlayerClass() const
{
	BW_GUARD;
	if (pPlayerClass_ == NULL)
	{
		BW::string playerClassName("Player");
		playerClassName.append( this->name() );

		PyObject * pNewClass = PyObject_GetAttrString(
			pModule_, const_cast<char*>( playerClassName.c_str() ) );

		if (!pNewClass)
		{
			return NULL;	// Cannot have this kind of player.
		}

		if (!PyObject_IsSubclass( pNewClass, (PyObject *)&PyEntity::s_type_ ))
		{
			Py_DECREF( pNewClass );
			ERROR_MSG( "EntityType::pPlayerClass: "
					"%s does not derive from BigWorld.Entity\n",
				playerClassName.c_str() );
			return NULL;
		}

		pPlayerClass_ = (PyTypeObject *)pNewClass;
	}

	return pPlayerClass_;
}


/**
 *	Fills the given memory stream with property values to be used
 *	by EntityManager when creating entity instances.
 */
void EntityType::prepareCreationStream( MemoryOStream& outputStream,
		const DataSectionPtr& pPropertiesDS ) const
{
	uint8 * pNumProp = (uint8*)outputStream.reserve( sizeof(uint8) );
	*pNumProp = 0;
	const EntityDescription & edesc = this->description();
	for (uint8 i = 0; i < edesc.clientServerPropertyCount(); i++)
	{
		// make sure this is a property we want
		const DataDescription * pDD = edesc.clientServerProperty( i );
		if (!pDD->isOtherClientData())
		{
			// If we have properties that will not be loaded, be verbose about
			// it.
			NOTICE_MSG( "EntityType::prepareCreationStream: "
				"Property %s.%s of type "
				"%s is not created for client-only entities.\n",
				this->name().c_str(), pDD->name().c_str(),
				pDD->getDataFlagsAsStr() );
			continue;
		}

		// open section and start with default value
		DataSectionPtr pPropSec = pPropertiesDS->openSection( pDD->name() );
		ScriptObject pPyProp = pDD->pInitialValue();

		// change to created value if it parses ok
		if (pPropSec)
		{
			ScriptDataSink sink;
			if(pDD->createFromSection( pPropSec, sink ))
			{
				ScriptObject pTemp = sink.finalise();
				if (pTemp)
				{
					pPyProp = pTemp;
				}
			}
		}

		// and add whichever to the stream
		outputStream << i;

		ScriptDataSource source( pPyProp );
		pDD->addToStream( source, outputStream, false );
		(*pNumProp)++;
	}
}

/**
 *	Fills the given memory stream with property values to be used
 *	when creating entity instances.
 */
void EntityType::prepareCreationStream( MemoryOStream & outputStream,
	const ScriptDict & props ) const
{
	uint8 * pNumProp = (uint8*)outputStream.reserve( sizeof(uint8) );
	*pNumProp = 0;
	const EntityDescription & edesc = this->description();
	for (uint8 i = 0; i < edesc.clientServerPropertyCount(); i++)
	{
		// make sure this is a property we want
		const DataDescription * pDD = edesc.clientServerProperty( i );
		if (!pDD->isOtherClientData())
		{
			// If we have properties that will not be loaded, be verbose about
			// it.
			NOTICE_MSG( "EntityType::prepareCreationStream: "
				"Property %s.%s of type "
				"%s is not created for client-only entities.\n",
				this->name().c_str(), pDD->name().c_str(),
				pDD->getDataFlagsAsStr() );
			continue;
		}

		// open section and start with default value
		ScriptObject prop =
			props.getItem( pDD->name().c_str(), ScriptErrorClear() );

		if (!prop)
		{
			 prop = pDD->pInitialValue();
		}

		// and add whichever to the stream
		outputStream << i;
		pDD->addToStream( ScriptDataSource( prop ), outputStream, false );
		(*pNumProp)++;
	}

}


/**
 *	Fills the given memory stream with property values to be used
 *	when creating base player entity instances.
 */
void EntityType::prepareBasePlayerStream( MemoryOStream & outputStream,
	const ScriptDict & props ) const
{
	description_.addDictionaryToStream(
		props, outputStream,
		EntityDescription::FROM_BASE_TO_CLIENT_DATA );
}


/**
 *	Fills the given memory stream with property values to be used
 *	when creating cell player entity instances.
 */
void EntityType::prepareCellPlayerStream( MemoryOStream & outputStream,
	const ScriptDict & props ) const
{
	description_.addDictionaryToStream(
		props, outputStream,
		EntityDescription::FROM_CELL_TO_CLIENT_DATA );
}


BW_END_NAMESPACE

// entity_type.cpp
