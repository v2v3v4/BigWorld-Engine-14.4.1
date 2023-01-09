#include "script/first_include.hpp"

#include "entity_type.hpp"

#include "client_app.hpp"
#include "py_entity.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/entity_def_constants.hpp"
#include "connection/server_connection.hpp"

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
BW::vector< EntityType * > EntityType::s_types_;
EntityDescriptionMap		EntityType::entityDescriptionMap_;


/**
 *	Constructor (private)
 */
EntityType::EntityType( const EntityDescription & description,
		const ScriptType & type ):
	description_( description ),
	type_( type )
{
}


/**
 *	Destructor.
 */
EntityType::~EntityType()
{
}


/**
 *	This static method returns the EntityType instance for a given type id.
 */
EntityType * EntityType::find( uint type )
{
	if (type >= s_types_.size())
	{
		CRITICAL_MSG( "EntityType::find: "
			"No such type index %d\n", type );
		return NULL;
	}

	return s_types_[ type ];
}


/**
 *	This static method returns the EntityType instance for a given type name.
 */
EntityType * EntityType::find( const BW::string & name )
{
	EntityTypeID type = 0;

	if (!entityDescriptionMap_.nameToIndex( name, type ))
	{
		CRITICAL_MSG( "EntityType::find: "
			"No entity type named %s\n", name.c_str() );
		return NULL;
	}

	return EntityType::find( type );
}


/**
 *	This static method initialises the entity type stuff.
 */
bool EntityType::init( const BW::string & standInEntity )
{
	// Initialise BigWorld module
	ScriptModule bigWorld = ScriptModule::import( "BigWorld",
		ScriptErrorPrint( "Failed to import BigWorld module" ) );

	ScriptErrorPrint errorPrint;

	if (PyType_Ready( &PyEntity::s_type_ ) < 0)
	{
		ERROR_MSG( "EntityType::init: Couldn't PyType_Ready for %s\n",
				PyEntity::s_type_.tp_name );
		Script::printError();
		return false;
	}

	// Add the "Entity" class to BigWorld
	if (!bigWorld.addObject( "Entity", &PyEntity::s_type_, errorPrint ))
	{
		return false;
	}

	ScriptType standInEntityType;

	// Load our stand-in entity first
	if (!standInEntity.empty())
	{
		ScriptModule standInEntityModule = ScriptModule::import(
				standInEntity.c_str(), errorPrint );

		if (standInEntityModule)
		{
			standInEntityType = standInEntityModule.getAttribute(
					standInEntity.c_str(), errorPrint );

			if (!standInEntityType.isSubClass( PyEntity::s_type_, errorPrint ))
			{
				standInEntityType = ScriptType();

				ERROR_MSG( "EntityType::init: "
						"Class %s is not derived from BigWorld.Entity\n",
					standInEntity.c_str() );
			}
		}	

		if (standInEntityType)
		{
			INFO_MSG( "EntityType::init: Loaded stand-in entity %s\n", 
				standInEntity.c_str() );
		}
		else
		{
			WARNING_MSG( "EntityType::init: Failed to load stand-in "
					"entity %s\n", standInEntity.c_str() );
		}
	}

	// Initialise the entity descriptions
	DataSectionPtr pEntitiesList =
		BWResource::openSection( EntityDef::Constants::entitiesFile() );

	if (!entityDescriptionMap_.parse( pEntitiesList,
			&ClientInterface::Range::entityPropertyRange,
			&ClientInterface::Range::entityMethodRange,
			&BaseAppExtInterface::Range::baseEntityMethodRange,
			&BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		ERROR_MSG( "EntityType::init: EntityDescriptionMap::parse failed\n" );
		return false;
	}

	if (entityDescriptionMap_.size() == 0)
	{
		ERROR_MSG( "EntityType::init: There are no entity descriptions!\n" );
		return false;
	}

	s_types_.resize( entityDescriptionMap_.size() );

	// Create an EntityType for each EntityDescription read in
	int numClientTypes = 0;

	for (EntityTypeID i = 0; i < entityDescriptionMap_.size(); ++i)
	{
		const EntityDescription & currDesc =
			entityDescriptionMap_.entityDescription( i );

		if (!currDesc.isClientType())
		{
			continue;
		}

		ScriptType entityType;

		// Import class
		if (currDesc.name() != standInEntity)
		{
			ScriptModule entityModule = ScriptModule::import(
					currDesc.name().c_str(), errorPrint );

			if (entityModule)
			{
				entityType = entityModule.getAttribute(
						currDesc.name().c_str(), ScriptErrorRetain() );
			}
			else
			{
				// Check if failure is due to scripting error
				BW::string path = "scripts/bot/" + currDesc.name();

				if (BWResource::fileExists( path + ".py" ) ||
					BWResource::fileExists( path + ".pyc" ))
				{
					Script::printError();
				}
			}
		}

		Script::clearError();

		// Ensure it subclasses Entity and has required methods
		if (entityType)
		{
			if (!entityType.isSubClass( PyEntity::s_type_, errorPrint ))
			{
				entityType = ScriptType();

 				ERROR_MSG( "EntityType::init: "
 						"Class %s is not derived from BigWorld.Entity\n",
 					currDesc.name().c_str() );
			}
			else if (!currDesc.checkMethods(
						currDesc.client().internalMethods(),
						entityType , false ))
			{
				WARNING_MSG(
					"EntityType::init: Class %s has missing method(s)\n",
					currDesc.name().c_str() );
			}
		}

		// Replace with stand-in entity type
		if (!entityType)
		{
			if (!standInEntityType)
			{
				ERROR_MSG( "EntityType::init: Could not load module %s.py "
						"and there is no stand-in entity available\n",
					currDesc.name().c_str() );
				return false;
			}

			entityType = standInEntityType;

			INFO_MSG( "Stand-in entity %s is replacing %s",
					standInEntity.c_str(), currDesc.name().c_str() );
		}

		s_types_[ currDesc.clientIndex() ] =
			new EntityType( currDesc, entityType );

		numClientTypes = std::max( numClientTypes, currDesc.clientIndex() + 1 );
	}

	s_types_.resize( numClientTypes );

	return true;
}


/**
 *	This static method should be called on shut down to clean up the entity type
 *	resources.
 */
bool EntityType::fini()
{
	for (uint i = 0; i < s_types_.size(); i++)
	{
		if (s_types_[ i ])
		{
			delete s_types_[ i ];
		}
	}

	s_types_.clear();

	return true;
}


/**
 *	This static method returns a collection of constants related to entity
 *	definitions.
 */
EntityDefConstants EntityType::entityDefConstants()
{
	return EntityDefConstants( entityDescriptionMap_.digest(),
			entityDescriptionMap_.maxExposedClientMethodCount(),
			entityDescriptionMap_.maxExposedBaseMethodCount(),
			entityDescriptionMap_.maxExposedCellMethodCount() );
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. If a data section is passed in, the values are read from
 *	it, otherwise entries get their default values.
 */
PyObject * EntityType::newDictionary( DataSectionPtr pSection,
		bool isPlayer ) const
{
	PyObject * pDict = PyDict_New();

	for (uint i=0; i < description_.propertyCount(); i++)
	{
		DataDescription * pDD = description_.property(i);

		if (!pDD->isClientServerData() ||
				(!isPlayer && pDD->isOwnClientData()))
		{
			// if we're not interested in it, don't set it.
			continue;
		}

		DataSectionPtr	pSubSection;

		ScriptObject pValue;

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

		MF_ASSERT( pValue.exists() );
		PyDict_SetItemString( pDict,
			const_cast<char*>( pDD->name().c_str() ), pValue.get() );
	}

	return pDict;
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. It streams the properties from the input stream.
 *	This is only used for creating the player entity.
 */
PyObject * EntityType::newDictionary( BinaryIStream & stream,
									 StreamContents contents ) const
{
	const bool isPlayer = (contents == TAGGED_CELL_PLAYER_DATA);

	// If there is no stream, return a default dictionary.
	if (stream.remainingLength() == 0)
	{
		return this->newDictionary( /* pSection */NULL, isPlayer );
	}

	ScriptDict pDict = ScriptDict::create();

	if (contents < TAGGED_CELL_PLAYER_DATA)
	{
		const int dataDomainsToRead =
			((contents == BASE_PLAYER_DATA) ?
				EntityDescription::FROM_BASE_TO_CLIENT_DATA :
				EntityDescription::FROM_CELL_TO_CLIENT_DATA);
		description_.readStreamToDict( stream, dataDomainsToRead, pDict );
	}
	else
	{
		uint8 size;
		stream >> size;

		for (uint8 i = 0; i < size; i++)
		{
			uint8 index;
			stream >> index;

			const DataDescription * pDD =
				description_.clientServerProperty( index );

			MF_ASSERT( pDD && (pDD->isOtherClientData() ||
				(isPlayer && pDD->isOwnClientData())) );

			if (pDD != NULL)
			{
				ScriptDataSink sink;
				MF_VERIFY( pDD->createFromStream( stream, sink,
					/* isPersistentOnly */ false ) );
				
				ScriptObject pValue = sink.finalise();

				MF_ASSERT( pValue.exists() );

				if (!pDict.setItem( pDD->name().c_str(), pValue, ScriptErrorRetain() ))
				{
					ERROR_MSG( "EntityType::newDictionary: "
						"Failed to set %s\n", pDD->name().c_str() );
					Script::printError();
				}
			}
		}
	}

	return pDict.newRef();
}


BW_END_NAMESPACE

// entity_type.cpp
