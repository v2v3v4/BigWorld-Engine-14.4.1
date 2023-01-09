#include "script/first_include.hpp"

#include "entity_type.hpp"
#include "cellapp.hpp"
#include "entity.hpp"

#include "real_entity.hpp"
#include "witness.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"

#include "cstdmf/md5.hpp"
#include "cstdmf/debug.hpp"

#include "entitydef/entity_description_map.hpp"
#include "entitydef/constants.hpp"

#include "resmgr/bwresource.hpp"

#include "pyscript/personality.hpp"

#include "delegate_interface/game_delegate.hpp"

#if defined( __GNUC__ )
#include <tr1/type_traits>
#else /* defined( __GNUC__ ) */
#include <type_traits>
#endif /* defined( __GNUC__ ) */

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity_type.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Static initialisers
// -----------------------------------------------------------------------------
PyObject * EntityType::s_pNewModules_;
EntityTypes EntityType::s_curTypes_, EntityType::s_newTypes_;
EntityType::NameToIndexMap EntityType::s_nameToIndexMap_;


// -----------------------------------------------------------------------------
// Section: EntityType
// -----------------------------------------------------------------------------

/**
 *	Constructor
 *
 *	@param entityDescription	Entity data description
 *	@param pType The Python class that is associated with this entity type.
 *					This object steals the reference from the caller
 */
EntityType::EntityType( const EntityDescription& entityDescription,
		PyTypeObject * pType ) :
	entityDescription_( entityDescription ),
	pPyType_( pType ),
#if !ENABLE_WATCHERS
	detailedPositionDescription_( "detailedPosition" ),
#endif
	expectsNearbyEntity_( false )
{
	MF_ASSERT( !pType ||
			(PyType_Check( pType ) &&
			PyObject_IsSubclass( (PyObject *)pType,
				(PyObject*)&Entity::s_type_ )) );

	propCountGhost_ = 0;
	for (uint i = 0; i < entityDescription_.propertyCount(); i++)
	{
		DataDescription * pDesc = entityDescription_.property( i );

		if (!pDesc->isCellData()) continue;
		if (pDesc->isComponentised()) continue;

		if (pDesc->isGhostedData())
		{
			propDescs_.insert( propDescs_.begin() + propCountGhost_++, pDesc );
		}
		else
		{
			propDescs_.push_back( pDesc );
		}
	}

	for (uint i = 0; i < propDescs_.size(); ++i)
	{
		propDescs_[i]->localIndex( i );
	}

	// Check if the __init__ method accepts 2 or more methods.
	int argCount = 0;
	const char * argCountPath[] =
		{ "__init__", "im_func", "func_code", "co_argcount", NULL };
	Script::getNestedValue( (PyObject *)pType, argCountPath, argCount );

	const char * flagsPath[] =
		{ "__init__", "im_func", "func_code", "co_flags", NULL };
	int flags = 0;
	Script::getNestedValue( (PyObject *)pType, flagsPath, flags );

	const bool hasVarArgs = ((flags & CO_VARARGS) != 0);

	expectsNearbyEntity_ = (argCount >= 2) || hasVarArgs;
}


/**
 *	Destructor
 */
EntityType::~EntityType()
{
	Py_XDECREF( pPyType_ );
}


/**
 *	This method creates an entity of this type.
 */
Entity * EntityType::newEntity() const
{
	if (pPyType_ == NULL)
	{
		ERROR_MSG( "EntityType::newEntity: %s is not a cell entity type.\n",
				this->description().name().c_str() );
		return NULL;
	}

	PyObject * pObject = PyType_GenericAlloc( pPyType_, 0 );

	if (!pObject)
	{
		PyErr_Print();
		ERROR_MSG( "EntityType::newEntity: Allocation failed\n" );
		return NULL;
	}

	MF_ASSERT( Entity::Check( pObject ) );

	BW_STATIC_ASSERT( std::tr1::is_polymorphic< Entity >::value == false,
		Entity_is_virtual_but_uses_PyType_GenericAlloc );

	Entity * pNewEntity =
		new (pObject) Entity( const_cast<EntityType *>( this ) );
	return pNewEntity;
}


/**
 *	This method returns the DataDescription associated with the attribute with
 *	the input name. If no such DataDescription exists, NULL is returned.
 */
DataDescription * EntityType::description( const char * attr, 
										   const char * component ) const
{
	return entityDescription_.findProperty( attr, component );
}


/**
 *	This method sets the equivalent old type for this type.
 */
void EntityType::old( EntityTypePtr pOldType )
{
	if (pOldSelf_ && pOldType)
	{
		MF_ASSERT( pOldSelf_ == pOldType );
		return;
	}

	pOldSelf_ = pOldType;

	if (pOldType)
	{
		EntityDescription & oldDesc = pOldType->entityDescription_;
		oldDesc.supersede( MethodDescription::CELL );
	}
}


namespace
{
/**
*	This utility method loads the entity class object
* 	Returns ScriptType() if there is an error in loading the class object.
*/
ScriptType importEntityClass( const EntityDescription& desc )
{
	const char * const name = desc.name().c_str();
	ScriptModule module = ScriptModule::import( name, ScriptErrorRetain() );

	ScriptType pClass;

	if (module)
	{
		pClass = module.getAttribute( name, ScriptErrorPrint() );
	}
	else
	{
		if (IGameDelegate::instance() == NULL) 
		{ 
			ERROR_MSG( "EntityType::importEntityClass: Could not load module "
				"%s\n", name );
			Script::printError();
			return ScriptType();
		}

		Script::clearError();
		INFO_MSG( "EntityType::importEntityClass: entity type %s has "
			"no cell script, using the class 'Entity' instead\n", name );


		pClass = ScriptType( &Entity::s_type_, ScriptType::FROM_BORROWED_REFERENCE );
	}

	if (!pClass)
	{
		ERROR_MSG( "EntityType::importEntityClass: Could not find class %s\n",
				name );
		return ScriptType();
	}

	if (!pClass.isSubClass( Entity::s_type_, ScriptErrorPrint() ))
	{
		ERROR_MSG( "Entity::init: %s does not derive from Entity\n",
				name );
		return ScriptType();
	}
		
	if (!desc.checkMethods( desc.cell().internalMethods(), pClass ))
	{
		return ScriptType();
	}

	return pClass;
}

} // end of anonymous namespace


// -----------------------------------------------------------------------------
// Section: EntityType static methods
// -----------------------------------------------------------------------------

/**
 *	This method needs to be called in order to initialise the different entity
 *	types.
 *
 *	@return True on success, false otherwise.
 */
bool EntityType::init( bool isReload )
{
	bool succeeded = true;

	if (isReload)
	{
		DataType::clearStaticsForReload();
	}

	EntityDescriptionMap entityDescriptionMap;

	if (!entityDescriptionMap.parse(
			BWResource::openSection( EntityDef::Constants::entitiesFile() ),
			&ClientInterface::Range::entityPropertyRange,
			&ClientInterface::Range::entityMethodRange,
			&BaseAppExtInterface::Range::baseEntityMethodRange,
			&BaseAppExtInterface::Range::cellEntityMethodRange ))
	{
		WARNING_MSG( "EntityType::init: "
				"Failed to initialise entityDescriptionMap\n" );
		return false;
	}

	// Print out the digest
	{
		MD5 md5;
		entityDescriptionMap.addToMD5( md5 );
		MD5::Digest digest;
		md5.getDigest( digest );
		BW::string quote = digest.quote();
		INFO_MSG( "Digest = %s\n", quote.c_str() );
	}

	EntityTypes & types = (!isReload) ? s_curTypes_ : s_newTypes_;
	types.resize( entityDescriptionMap.size() );

	for (int i = 0; i < entityDescriptionMap.size(); i++)
	{
		const EntityDescription& desc =
			entityDescriptionMap.entityDescription(i);
		const BW::string & name = desc.name();

		if (!isReload)
			s_nameToIndexMap_[ name.c_str() ] = i;


		ScriptType pClass;

		if (desc.canBeOnCell())
		{
			pClass = importEntityClass( desc );

			if (!pClass)
			{
				succeeded = false;
				continue;
			}
		}

		types[i] = new EntityType( desc, (PyTypeObject *)pClass.newRef() );
	}

	if (succeeded)
	{
		if (isReload)
		{
			DataType::reloadAllScript();

			// make a copy of the modules now before anything happens to them
			s_pNewModules_ = PyDict_Copy( PySys_GetObject( "modules" ) );
		}

		// call onInit callback function on personality script
		if (!CellApp::instance().triggerOnInit( isReload ))
		{
			ERROR_MSG( "EntityType::init: "
				"Personality.onInit listener failed\n" );
			succeeded = false;
		}
	}

	return succeeded;
}


/**
 *	This static method reloads all the Python scripts without reparsing the
 * 	entity definitions.
 *
 *	@param	isRecover	True if we are trying to reload the old scripts after
 * 						having trouble loading the new scripts.
 *	@return True if successful, false otherwise.
 */
bool EntityType::reloadScript( bool isRecover )
{
	bool isOK = true;

	// Loop through entity types and reload their class objects.
	for ( EntityTypes::iterator it = s_curTypes_.begin();
			it != s_curTypes_.end(); ++it )
	{
		EntityType& 				entityType = **it;
		const EntityDescription& 	desc = entityType.description();
		ScriptType					pClass;

		if (desc.canBeOnCell())
		{
			pClass = importEntityClass( desc );

			if (!pClass)
			{
				isOK = false;
			}
		}

		entityType.setPyType( (PyTypeObject*)pClass.newRef() );
	}

	if (isOK)
	{
		DataType::reloadAllScript();

		// make a copy of the modules now before anything happens to them
		s_pNewModules_ = PyDict_Copy( PySys_GetObject( "modules" ) );

		if (!isRecover)
		{
			// call onInit callback function on personality script
			isOK = CellApp::instance().triggerOnInit( true );
		}
	}

	return isOK;
}

/**
 *	This static method migrates the collection of entity types to
 *	their reloaded version.
 */
void EntityType::migrate( bool isFullReload )
{
	PyObject * pModules = PySys_GetObject( "modules" );
	if (pModules != s_pNewModules_)
	{
		PyObjectPtr pSysModule = PyDict_GetItemString( pModules, "sys" );

		PyDict_Clear( pModules );
		PyDict_Update( pModules, s_pNewModules_ );

		PyThreadState_Get()->interp->builtins =
			PyModule_GetDict( PyDict_GetItemString( pModules, "__builtin__" ) );

		PyObject * pBigWorld = PyDict_GetItemString( pModules, "BigWorld" );
		Py_INCREF( pBigWorld ); // AddObject steals a reference
		PyModule_AddObject( PyDict_GetItemString( pModules, "__main__" ),
							"BigWorld", pBigWorld );

		// Restore original sys module since it has remapped stdout/stderr.
		if (pSysModule)
			PyDict_SetItemString( pModules, "sys", pSysModule.get() );
		// note this migration is still a bit dodgy but a lot better than it was
		// probably should migrate the whole interpreter state from the
		// reloading thread... although currently it is missing some stuff done
		// by init time jobs (e.g. stdout/stderr redirection).
	}

	if (isFullReload)
	{
		s_curTypes_ = s_newTypes_;

		s_nameToIndexMap_.clear();
		for (uint i = 0; i < s_curTypes_.size(); i++)
			s_nameToIndexMap_[ s_curTypes_[i]->name() ] = i;
	}
	// TODO: could match up all old and new types here...
}


/**
 *	This static method cleans up after a reload,
 *	when the old type info is no longer needed.
 */
void EntityType::cleanupAfterReload( bool isFullReload )
{
	if (isFullReload)
	{
		// throw away the links to the old types
		for (uint i = 0; i < s_curTypes_.size(); i++)
		{
			if (s_curTypes_[i])
				s_curTypes_[i]->old( NULL );
		}

		// we don't need a second copy of these pointers
		s_newTypes_.clear();
	}

	// and we certainly don't need this modules dictionary from another context
	Py_XDECREF( s_pNewModules_ );
	s_pNewModules_ = NULL;
}

/**
 *	Sets our type object. Consumes reference to pPyType.
 */
void EntityType::setPyType( PyTypeObject * pPyType )
{
	Py_XDECREF( pPyType_ );
	pPyType_ = pPyType;
}

/**
 *	This static method is used to get the entity type associated with the input
 *	id.
 */
EntityTypePtr EntityType::getType( EntityTypeID typeID )
{
	if (typeID >= (int)s_curTypes_.size())
	{
		CRITICAL_MSG( "EntityType::getType: "
					"No such type with ID = %d. %" PRIzu "\n",
				typeID, s_curTypes_.size() );

		return NULL;
	}

	return s_curTypes_[ typeID ];
}


/**
 *	This static method is used to get the entity type associated with the input
 *	name.
 */
EntityTypePtr EntityType::getType( const char * className )
{
	NameToIndexMap::iterator iter = s_nameToIndexMap_.find( className );

	if (iter == s_nameToIndexMap_.end())
		return NULL;
	else
		return s_curTypes_[iter->second];
}

/**
 *	This static method is used to get the description of the 
 *  entity type associated with the input. It is safe to use from
 *  a different thread, since we avoid copying the smart pointer
 *  out of s_curTypes_, and just accessing it directly
 *
 *	@return EntityDescription* on success, otherwise NULL.
 */
const EntityDescription * EntityType::getDescription( EntityTypeID typeID )
{
	if (typeID >= (int)s_curTypes_.size())
	{
		CRITICAL_MSG( "EntityType::getType: "
			"No such type with ID = %d. %" PRIzu "\n",
			typeID, s_curTypes_.size() );

		return NULL;
	}

	return &(s_curTypes_[ typeID ]->description());
}

/**
 *	This static method returns the index of the type with the input name. If no
 *	such type exists, -1 is returned.
 *
 *	@return	On success, the index of the type found, otherwise EntityType::INVALID_INDEX.
 */
EntityTypeID EntityType::nameToIndex( const char * name )
{
	NameToIndexMap::iterator iter = s_nameToIndexMap_.find( name );

	return (iter != s_nameToIndexMap_.end()) ? iter->second : INVALID_INDEX;
}

/**
 *	This static method is used to get the vector of entity types.
 */
EntityTypes& EntityType::getTypes()
{
	return s_curTypes_;
}

/**
 *	Clean up static data. Used during shutdown.
 */
void EntityType::clearStatics()
{
	s_nameToIndexMap_.clear();
	s_curTypes_.clear();
	s_newTypes_.clear();
}


/**
 * Tick profilers on all the current types
 */
void EntityType::tickProfilers( float & totalEntityLoad,
								float & totalAddedLoad )
{
	totalEntityLoad = 0.f;
	totalAddedLoad = 0.f;

	for ( EntityTypes::iterator it = s_curTypes_.begin();
			it != s_curTypes_.end(); ++it )
	{
		EntityType & entityType = **it;
		entityType.profiler_.tick();
		totalEntityLoad += entityType.profiler_.load();
		totalAddedLoad += entityType.profiler_.addedLoad();
	}
}


#if ENABLE_WATCHERS

/**
 * 	This method returns the generic watcher for EntityTypes.
 */
WatcherPtr EntityType::pWatcher()
{
	static WatcherPtr pMapWatcher = NULL;

	if (pMapWatcher == NULL)
	{
		DirectoryWatcherPtr pWatcher = new DirectoryWatcher();
		EntityType	*pNull = NULL;

		pWatcher->addChild( "methods", EntityDescription::pWatcher(),
				&pNull->entityDescription_ );

		pWatcher->addChild( "interface/typeID", 
				makeWatcher( &EntityType::typeID ) );

		pWatcher->addChild( "interface/clientTypeID",
				makeWatcher( &EntityType::clientTypeID ) );

		pWatcher->addChild( "interface/propCountGhost",
				makeWatcher( &EntityType::propCountGhost ) );

		pWatcher->addChild( "interface/propCountGhostPlusReal",
				makeWatcher( &EntityType::propCountGhostPlusReal ) );

		pWatcher->addChild( "interface/propCountClientServer",
				makeWatcher( &EntityType::propCountClientServer ) );

		pWatcher->addChild( "stats", Stats::pWatcher(), 
							&pNull->stats_ );

		pWatcher->addChild( "profile",
							EntityTypeProfiler::pWatcher(),
							&pNull->profiler_ );

		SequenceWatcher<BW::vector<DataDescription*> > * watchDataTypes =
			new SequenceWatcher< BW::vector< DataDescription * > >(
				pNull->propDescs_ );
		watchDataTypes->addChild( "*", new BaseDereferenceWatcher(
			DataDescription::pWatcher() ) );
		watchDataTypes->setLabelSubPath( "name" );
		pWatcher->addChild( "properties", watchDataTypes );

		pMapWatcher = new MapWatcher< NameToIndexMap >( s_nameToIndexMap_ );
		pMapWatcher->addChild( "*",
				new ContainerBounceWatcher< EntityTypes, EntityTypeID >(
					new SmartPointerDereferenceWatcher( pWatcher ),
					&s_curTypes_ ) );
	}

	return pMapWatcher;
}

WatcherPtr EntityType::Stats::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		Stats * pNull = NULL;
		WatcherPtr totals = new DirectoryWatcher();
		pWatcher->addChild( "all", totals );
		pNull->sentToOwnClient_.
			addWatchers( totals, "sentToOwnClient" );
		pNull->nonVolatileSentToOtherClients_.
			addWatchers( totals, "nonVolatileSentToOtherClients" );
		pNull->volatileSentToOtherClients_.
			addWatchers( totals, "volatileSentToOtherClients" );
		pNull->addedToHistoryQueue_.
			addWatchers( totals, "addedToHistoryQueue" );
		pNull->sentToGhosts_.
			addWatchers( totals, "sentToGhosts" );
		pNull->sentToBase_.
			addWatchers( totals, "sentToBase" );
		pWatcher->addChild( "detailedPosition", EntityMemberStats::pWatcher(),
							&pNull->detailedPositionDescription().stats() );
		pWatcher->addChild( "createEntity", EntityMemberStats::pWatcher(),
							&pNull->createEntity_ );
		pWatcher->addChild( "enterAoI", EntityMemberStats::pWatcher(),
							&pNull->enterAoI_ );
	}
	return pWatcher;
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// entity_type.cpp
