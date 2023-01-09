#include "pch.hpp"

#include "entity_type.hpp"

#include "entity.hpp"
#include "py_entity.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/entity_def_constants.hpp"
#include "connection/server_connection.hpp"

#include "cstdmf/debug.hpp"

#include "entitydef/script_data_sink.hpp"

#include "pyscript/script.hpp"

#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


// Static variable instantiations
EntityType::Types			EntityType::types_;
BW::EntityDescriptionMap	EntityType::entityDescriptionMap_;


BW_BEGIN_NAMESPACE

extern int Math_token;
extern int ResMgr_token;
extern int PyScript_token;
static int s_moduleTokens =
	Math_token | ResMgr_token | PyScript_token;

extern int PyUserDataObject_token;
extern int UserDataObjectDescriptionMap_Token;
static int s_udoTokens = PyUserDataObject_token | UserDataObjectDescriptionMap_Token;

BW_END_NAMESPACE


/**
 *	Constructor (private)
 */
EntityType::EntityType( const BW::EntityDescription & description,
						PyTypeObject * pType ):
	description_( description ),
	pType_( pType )
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
EntityType * EntityType::find( BW::EntityTypeID entityTypeID )
{
	if (entityTypeID >= types_.size())
	{
		CRITICAL_MSG( "EntityType::find: "
			"No such type index %d\n", entityTypeID );
		return NULL;
	}
	return types_[ entityTypeID ];
}


/**
 *	This static method returns the EntityType instance for a given type name.
 */
EntityType * EntityType::find( const BW::string & name )
{
	BW::EntityTypeID	type = 0;

	if (!entityDescriptionMap_.nameToIndex( name, type ))
	{
		CRITICAL_MSG( "EntityType::find: "
			"No entity type named %s\n", name.c_str() );
		return NULL;
	}

	return EntityType::find(type);
}


/**
 *	This static method initialises the entity type stuff.
 */
bool EntityType::init( const BW::string & clientPath )
{
	// initialise BigWorld module
	PyObject * pBWModule = PyImport_AddModule( "BigWorld" );

	// add the "Entity" class to bigworld
	if (PyObject_SetAttrString( pBWModule, "Entity",
			(PyObject *)&BW::PyEntity::s_type_ ) == -1)
	{
		return false;
	}

	// Initialise the entity descriptions.
	BW::DataSectionPtr	pEntitiesList =
		BW::BWResource::openSection( clientPath );

	if (entityDescriptionMap_.parse( pEntitiesList,
			&BW::ClientInterface::Range::entityPropertyRange,
			&BW::ClientInterface::Range::entityMethodRange,
			&BW::BaseAppExtInterface::Range::baseEntityMethodRange,
			&BW::BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		// can check that entity definition is up to date here
	}
	else
	{
		CRITICAL_MSG( "EntityType::init: "
				"EntityDescriptionMap::parse failed\n" );
	}

	// Create an EntityType for each EntityDescription read in.
	int numEntityTypes = entityDescriptionMap_.size();
	types_.resize( numEntityTypes );

	int numClientTypes = 0;

	if (numEntityTypes <= 0)
	{
		ERROR_MSG( "EntityType::init: There are no entity descriptions!\n" );
		return false;
	}

	bool succeeded = true;

	for (int i = 0; i < numEntityTypes; i++)
	{
		types_[i] = NULL;

		const BW::EntityDescription & currDesc =
			entityDescriptionMap_.entityDescription( i );

		// Ignore server only types
		if (!currDesc.isClientType())
		{
			continue;
		}

		const char * name = currDesc.name().c_str();

		PyObject * pModule = PyImport_ImportModule( const_cast<char*>(name) );
		if (pModule == NULL)
		{
			ERROR_MSG( "EntityType::init: Could not load module\n" );
			PyErr_Print();
			succeeded = false;
			// warning: early dropout
			continue;
		}
		PyObject * pClass = PyObject_GetAttrString( pModule,
			const_cast<char *>( name ) );
		if (pClass == NULL)
		{
			ERROR_MSG( "EntityType::init: Could not find class %s\n", name );
			succeeded = false;
			continue;
		}
		else if (PyObject_IsSubclass( pClass,
					(PyObject *)&BW::PyEntity::s_type_ ) != 1)
		{
			ERROR_MSG( "EntityType::init: "
					"Class %s is not derived from BigWorld.Entity\n",
				name );
			succeeded = false;
			continue;
		}
		else if (!currDesc.checkMethods( currDesc.client().internalMethods(),
			BW::ScriptObject( pClass,
					BW::ScriptObject::FROM_BORROWED_REFERENCE ) ))
		{
			ERROR_MSG( "EntityType::init: "
					"Class %s does not have correct methods\n",
				name );
			// don't fail here because it's not *completely* critical that client
			// does not have all the methods (but it might be for, say, a real client)
			// succeeded = false;
			// continue;
		}

		MF_ASSERT( PyType_Check( pClass ) );
		types_[ currDesc.clientIndex() ] =
				new EntityType( currDesc, (PyTypeObject *)pClass );
		numClientTypes++;
	}

	types_.resize( numClientTypes );

	return succeeded;
}


/**
 *	This static method should be called on shutdown to clean up the entity type
 *	resources.
 */
bool EntityType::fini()
{
	Types::iterator iter = types_.begin();

	while (iter != types_.end())
	{
		// May be NULL.
		delete *iter;
		++iter;
	}

	types_.clear();

	return true;
}


/**
 *	This static method returns a collection of constants related to entity
 *	definitions.
 */
const BW::EntityDefConstants & EntityType::entityDefConstants()
{
	static BW::EntityDefConstants s_constants( entityDescriptionMap_.digest(),
			entityDescriptionMap_.maxExposedClientMethodCount(),
			entityDescriptionMap_.maxExposedBaseMethodCount(),
			entityDescriptionMap_.maxExposedCellMethodCount() );

	return s_constants;
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. If a data section is passed in, the values are read from
 *	it, otherwise entries get their default values.
 */
PyObject * EntityType::newDictionary( BW::DataSectionPtr pSection ) const
{
	PyObject * pDict = PyDict_New();

	for (BW::uint i=0; i < description_.propertyCount(); i++)
	{
		BW::DataDescription * pDD = description_.property(i);

		if (!pDD->isClientServerData())
		{
			// if we're not interested in it, don't set it.
			continue;
		}

		BW::DataSectionPtr	pSubSection;

		BW::ScriptObject pValue;

		// Can we get it from the DataSection?
		if (pSection &&
			(pSubSection = pSection->openSection( pDD->name() )))
		{
			BW::ScriptDataSink sink;
			pDD->createFromSection( pSubSection, sink );
			pValue = sink.finalise();
		}
		else	// ok, resort to the default then
		{
			pValue = pDD->pInitialValue();
		}

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
PyObject * EntityType::newDictionary( BW::BinaryIStream & stream,
	int dataDomains ) const
{
	BW::ScriptDict pDict = BW::ScriptDict::create();

	description_.readStreamToDict( stream, dataDomains, pDict );

	return pDict.newRef();
}


/**
 *	This function returns a brand new instance of a dictionary associated with
 *	this entity type. It streams the properties from the tagged input stream.
 *	This is used for creating new 'other' entities from the cell.
 */
PyObject * EntityType::newDictionary( BW::BinaryIStream & stream ) const
{
	BW::ScriptDict pDict = BW::ScriptDict::create();

	description_.readTaggedClientStreamToDict( stream, pDict, false );

	return pDict.newRef();
}


// entity_type.cpp
