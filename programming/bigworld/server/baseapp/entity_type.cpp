#include "script/first_include.hpp"

#include "entity_type.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "proxy.hpp"
#include "py_cell_data.hpp"
#include "service.hpp"

#include "cstdmf/md5.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/entity_description_map.hpp"

#include "resmgr/bwresource.hpp"

#include "delegate_interface/game_delegate.hpp"

#if defined( __GNUC__ )
#include <tr1/type_traits>
#else /* defined( __GNUC__ ) */
#include <type_traits>
#endif /* defined( __GNUC__ ) */

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Utility functions
// -----------------------------------------------------------------------------
namespace
{
	/**
	 *	This function inserts default valued properties of the given entity
	 * 	type, into the given Python	dictionary.
	 *
	 * 	@param	pDict The dictionary to insert into.
	 * 	@param	description The entity type.
	 * 	@param	selector Functor object which decides which properties to insert
	 *	into the dictionary.
	 */
	template < class SELECTOR >
	inline void insertDefaultProperties( PyObject* pDict,
		const EntityDescription& description, const SELECTOR& selector )
	{
		for (uint i = 0; i < description.propertyCount(); i++)
		{
			DataDescription * pDD = description.property( i );
			if ( selector( *pDD ) )
			{
				if (PyDict_SetItemString( pDict,
						const_cast<char*>( pDD->fullName().c_str() ),
						pDD->pInitialValue().get() ) == -1)
				{
					ERROR_MSG( "insertDefaultProperties: Failed to add %s\n",
								pDD->fullName().c_str() );
					PyErr_Print();
				}
			}
		}
	}
	// Selectors for use with insertDefaultProperties()
	struct BaseNonPersistentSelector
	{
		bool operator()( const DataDescription& desc ) const
		{	return (desc.isBaseData() && !desc.isPersistent());	}
	};
	struct CellNonPersistentSelector
	{
		bool operator()( const DataDescription& desc ) const
		{	return (desc.isCellData() && !desc.isPersistent());	}
	};

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

			// TODO: Instead of always using Proxy, use Base when the Proxy is not needed.
			//       The condition for using Base or Proxy should be specified in
			//       some flag in EntityDescription or derived from a delegate data
			//       (a property of some Despair component?)
			PyTypeObject & type = desc.isService() ? 
				Service::s_type_ : Proxy::s_type_;

			INFO_MSG( "EntityType::importEntityClass: entity type '%s' has "
				"no base script, using base class '%s' instead\n", 
				name, type.tp_name );

			pClass = ScriptType( &type, ScriptType::FROM_BORROWED_REFERENCE );
		}

		if (!pClass)
		{
			ERROR_MSG( "EntityType::importEntityClass: Could not find class %s\n",
					name );
			return ScriptType();
		}
		
		const PyTypeObject & type =
			desc.isService() ? Service::s_type_ : Base::s_type_;

		if (!pClass.isSubClass( type, ScriptErrorPrint() ))
		{
			ERROR_MSG( "EntityType::init: %s should be derived from BigWorld.%s\n",
					name, type.tp_name );
			return ScriptType();
		}

		return pClass;
	}

} // end of anonymous namespace

// -----------------------------------------------------------------------------
// Section: Static initialisers
// -----------------------------------------------------------------------------
PyObject * EntityType::s_pNewModules_;
EntityType::EntityTypes EntityType::s_curTypes_, EntityType::s_newTypes_;
EntityType::NameToIndexMap EntityType::s_nameToIndexMap_;

MD5 EntityType::s_persistentPropertiesMD5_;
MD5::Digest EntityType::s_digest_;


// -----------------------------------------------------------------------------
// Section: EntityType - static methods
// -----------------------------------------------------------------------------

/**
 *	This static method is used to get the entity type associated with the input
 *	id.
 *
 *	@return EntityTypePtr on success, otherwise NULL.
 */
EntityTypePtr EntityType::getType( EntityTypeID typeID,
		bool shouldExcludeServices )
{
	if (typeID >= (int)s_curTypes_.size())
		return NULL;

	EntityTypePtr pType = s_curTypes_[ typeID ];

	if (shouldExcludeServices && pType && pType->isService())
	{
		return NULL;
	}

	return pType;
}

/**
 *	This static method is used to get the description of the 
 *  entity type associated with the input. It is safe to use from
 *  a different thread, since we avoid copying the smart pointer
 *  out of s_curTypes_, and just accessing it directly
 *
 *	@return EntityDescription* on success, otherwise NULL.
 */
const EntityDescription * EntityType::getDescription( EntityTypeID typeID,
		bool shouldExcludeServices )
{
	if (typeID >= (int)s_curTypes_.size())
		return NULL;

	EntityTypePtr& pType = s_curTypes_[ typeID ];

	if (shouldExcludeServices && pType && pType->isService())
	{
		return NULL;
	}

	return &pType->description();
}

/**
 *	This static method is used to get the base type associated with the input
 *	Python class name.
 */
EntityTypePtr EntityType::getType( const char * className,
		bool shouldExcludeServices )
{
	NameToIndexMap::iterator iter = s_nameToIndexMap_.find( className );

	if (iter == s_nameToIndexMap_.end())
	{
		return NULL;
	}

	return EntityType::getType( iter->second, shouldExcludeServices );
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
 *	This method needs to be called in order to initialise the different base
 *	types. It is called by Base::init.
 *
 *	@return True on success, otherwise false.
 */
bool EntityType::init( bool isReload, bool isServiceApp )
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

	EntityType::s_digest_ = entityDescriptionMap.digest();

	EntityTypes & fillTypes = (!isReload) ? s_curTypes_ : s_newTypes_;
	fillTypes.resize( entityDescriptionMap.size() );

	for (int i = 0; i < entityDescriptionMap.size(); i++)
	{
		const EntityDescription & desc =
			entityDescriptionMap.entityDescription( i );
		const BW::string & name = desc.name();

		if (!isReload)
			s_nameToIndexMap_[ name.c_str() ] = i;

		if (desc.canBeOnBase() && (isServiceApp || !desc.isService()))
		{
			ScriptType pClass = importEntityClass( desc );

			if (!pClass)
			{
				succeeded = false;
				continue;
			}

			const bool isProxy = 
				pClass.isSubClass( Proxy::s_type_, ScriptErrorPrint() );

			fillTypes[ i ] =
				new EntityType( desc, (PyTypeObject *)pClass.newRef(), isProxy );
		}
		else
		{
			fillTypes[ i ] = new EntityType( desc, NULL, false );
		}
	}

	if (succeeded)
	{
		if (isReload)
		{
			DataType::reloadAllScript();

			s_pNewModules_ = PyDict_Copy( PySys_GetObject( "modules" ) );
		}

		// call onInit callback function on personality script
		succeeded = BaseApp::instance().triggerOnInit( isReload );
	}

	entityDescriptionMap.addPersistentPropertiesToMD5(
			s_persistentPropertiesMD5_ );

	return succeeded;
}

/**
 *	This static method reloads all the Python scripts without reparsing the
 * 	entity definitions.
 *
 *	@param	isRecover	True if we are trying to reload the old scripts after
 * 						having trouble loading the new scripts.
 *	@return True on success, otherwise false.
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

		if (desc.canBeOnBase())
		{
			pClass = importEntityClass( desc );

			if (!pClass)
			{
				isOK = false;
				continue;
			}

			const bool isProxy = 
				pClass.isSubClass( Proxy::s_type_, ScriptErrorPrint() );

			if (isProxy != entityType.isProxy())
			{
				ERROR_MSG( "EntityType::reloadScript: Cannot change type of "
						"entity '%s' from Proxy to non-Proxy and vice versa\n",
						desc.name().c_str() );
				isOK = false;
				continue;
			}

		}

		entityType.setClass( (PyTypeObject*)pClass.newRef() );
	}

	if (isOK)
	{
		DataType::reloadAllScript();

		s_pNewModules_ = PyDict_Copy( PySys_GetObject( "modules" ) );

		if (!isRecover)
		{
			// call onInit callback function on personality script
			isOK = BaseApp::instance().triggerOnInit( /* isReload */true );
		}
	}

	return isOK;
}

/**
 *	This static method migrates all our existing types to the new types
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
		// reloading thread... altho' currently it is missing some stuff done
		// by init time jobs (e.g. stdout/stderr redirection).
	}

	if (isFullReload)
	{
		// Set the "old" type for each "new" type.
		for ( EntityTypes::iterator i = s_newTypes_.begin();
				i != s_newTypes_.end(); ++i )
		{
			(*i)->old( EntityType::getType( (*i)->name() ) );
		}

		s_curTypes_ = s_newTypes_;

		s_nameToIndexMap_.clear();
		for (uint i = 0; i < s_curTypes_.size(); i++)
			s_nameToIndexMap_[ s_curTypes_[i]->name() ] = i;
	}
}

/**
 *	This static method cleans up after reloading
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
void EntityType::tickProfilers()
{
	for ( EntityTypes::iterator it = s_curTypes_.begin();
			it != s_curTypes_.end(); ++it )
	{
		EntityType & entityType = **it;
		entityType.profiler_.tick();
	}
}

// -----------------------------------------------------------------------------
// Section: EntityType
// -----------------------------------------------------------------------------

/**
 *	Constructor
 *
 *	@param desc		The entity description associated with this entity type.
 *	@param pType	The Python type object that is associated with this entity
 *					type. This object steals the reference from the caller.
 *	@param isProxy	Whether or not this type can be a proxy that clients
 *					attach to.
 */
EntityType::EntityType( const EntityDescription & desc, PyTypeObject * pType,
		bool isProxy ):
	entityDescription_( desc ),
	pClass_( pType ),
	isProxy_( isProxy )
#if ENABLE_WATCHERS
	,backupSize_( 0 ),
	archiveSize_( 0 ),
	numInstancesBackedUp_( 0 ),
	numInstancesArchived_( 0 ),
	numInstances_( 0 )
#endif
{
	if (pClass_)
	{
		if (!entityDescription_.checkMethods(
				entityDescription_.base().internalMethods(),
				ScriptObject( this->pClass(), 
					ScriptObject::FROM_BORROWED_REFERENCE ) ))
		{
			ERROR_MSG( "EntityType::EntityType: "
					"Script for %s is missing a method.\n",
				desc.name().c_str() );
		}

		if (!entityDescription_.checkExposedMethods( isProxy ))
		{
			WARNING_MSG( "EntityType::EntityType: "
					"One or more methods for %s have invalid <Exposed> tag.\n",
				desc.name().c_str() );
		}
}
}


/**
 *	Destructor
 */
EntityType::~EntityType()
{
	Py_XDECREF( pClass_ );
}

/**
 *	Sets the class object for this entity type. Consumes reference to pClass.
 */
void EntityType::setClass( PyTypeObject * pClass )
{
	Py_XDECREF( pClass_ );
	pClass_ = pClass;
}

BW_END_NAMESPACE
#include "cstdmf/memory_stream.hpp"
BW_BEGIN_NAMESPACE

/**
 *	This method creates the actual python object for a Base or Proxy.
 */
Base * EntityType::newEntityBase( EntityID id, DatabaseID dbID )
{
	if (!this->canBeOnBase())
	{
		CRITICAL_MSG( "EntityType::newEntityBase: Invalid base type %d!\n",
				   this->id() );
		return NULL;
	}

	PyObject * pObject = PyType_GenericAlloc( pClass_, 0 );

	Base * pNewBase = NULL;

	BW_STATIC_ASSERT( std::tr1::is_polymorphic< Base >::value == false,
		Base_is_virtual_but_uses_PyType_GenericAlloc );

	BW_STATIC_ASSERT( std::tr1::is_polymorphic< Proxy >::value == false,
		Proxy_is_virtual_but_uses_PyType_GenericAlloc );

	if (this->isProxy())
	{
		pNewBase = new (pObject) Proxy( id, dbID, this );
	}
	else
	{
		pNewBase = new (pObject) Base( id, dbID, this );
	}

	return pNewBase;
}


/**
 *	This method creates a new Base object for this type.
 */
BasePtr EntityType::create( EntityID id, DatabaseID dbID, BinaryIStream & data,
		bool hasPersistentDataOnly, const BW::string * pLogOnData )
{
	MF_ASSERT( pLogOnData == NULL || this->isProxy() );

	if (!pClass_)
	{
		ERROR_MSG( "EntityType::create: "
				"Cannot create object that does not have a class. "
				"isService = %d\n",
			this->isService() );
		data.finish();
		return NULL;
	}

	BasePtr pNewBase;

	const EntityDescription& description = this->description();

	ScriptDict pBaseData = ScriptDict::create();
	int dataDomain = EntityDescription::BASE_DATA;

	if (hasPersistentDataOnly)
	{
		dataDomain |= EntityDescription::ONLY_PERSISTENT_DATA;
	}

	if (description.readStreamToDict( data, dataDomain, pBaseData ))
	{
		if (hasPersistentDataOnly)
		{
			// Insert default values for non-persistent properties.
			insertDefaultProperties( pBaseData.get(), description,
									BaseNonPersistentSelector() );
		}

		PyObject * pCellData = NULL;

		if (entityDescription_.canBeOnCell())
		{
			pCellData = new PyCellData( this, data, hasPersistentDataOnly );
		}

		pNewBase = this->newEntityBase( id, dbID );

		if (pNewBase)
		{
			pNewBase->init( pBaseData.get(), pCellData, pLogOnData );
		}

		Py_XDECREF( pCellData );

		if (pNewBase->isDestroyed())
		{
			pNewBase = NULL; // This will destruct the newly formed Base.
		}
	}
	else
	{
		pNewBase = NULL;
	}

	return pNewBase;
}

BasePtr EntityType::create( EntityID id,
							const BW::string & templateID )
{
	if (!pClass_)
	{
		ERROR_MSG( "EntityType::create: "
				   "Cannot create object that does not have a class. "
				   "isService = %d\n",
				   this->isService() );
		return NULL;
	}

	const BasePtr pNewBase = this->newEntityBase( id, DatabaseID( 0 ) );
	MF_ASSERT( pNewBase );

	if (!pNewBase->init( templateID ))
	{
		ERROR_MSG( "EntityType::create: "
				   "Failed to initialise entity with templateID '%s'\n",
				   templateID.c_str());
		return NULL;
	}
	return pNewBase;
}

/**
 *	This method creates a dictionary of cell properties from a stream.
 */
PyObjectPtr EntityType::createCellDict( BinaryIStream & data,
	bool strmHasPersistentDataOnly )
{
	ScriptDict pCellData = ScriptDict::create();

	int dataDomain = EntityDescription::CELL_DATA;

	if (strmHasPersistentDataOnly)
	{
		dataDomain |= EntityDescription::ONLY_PERSISTENT_DATA;
	}

	const EntityDescription& description = this->description();

	if (description.readStreamToDict( data, dataDomain, pCellData ))
	{
		if (strmHasPersistentDataOnly)
		{
			// Insert default values for non-persistent properties.
			insertDefaultProperties( pCellData.get(), description,
									CellNonPersistentSelector() );
		}

		Vector3 pos;
		Direction3D dir;
		SpaceID spaceID;
		data >> pos >> dir >> spaceID;
		PyCellData::addSpatialData( pCellData, pos, dir, spaceID );
	}
	else
	{
		ERROR_MSG( "EntityType::createCellDict: Failed to create cellData!" );
		pCellData = ScriptDict();
	}

	return pCellData;
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
		oldDesc.supersede( MethodDescription::BASE );
	}
}

#if ENABLE_WATCHERS
void EntityType::updateBackupSize( const Base & instance, uint32 newSize )
{
	backupSize_ = backupSize_ + newSize - instance.backupSize();
	if (instance.backupSize() == 0)
	{
		numInstancesBackedUp_++;
	}
	if (newSize == 0)
	{
		numInstancesBackedUp_--;
	}
}

void EntityType::updateArchiveSize( const Base & instance, uint32 newSize )
{
	archiveSize_ = archiveSize_ + newSize - instance.archiveSize();

	if (instance.archiveSize() == 0)
	{
		++numInstancesArchived_;
	}

	if (newSize == 0)
	{
		--numInstancesArchived_;
	}
}

void EntityType::countNewInstance( const Base & instance )
{
	numInstances_++;
}

void EntityType::forgetOldInstance( const Base & instance )
{
	numInstances_--;
	this->updateBackupSize( instance, 0 );
	this->updateArchiveSize( instance, 0 );
}

uint32 EntityType::averageBackupSize() const
{
	if (!numInstancesBackedUp_)
	{
		return 0;
	}
	return backupSize_ / numInstancesBackedUp_;
}

uint32 EntityType::averageArchiveSize() const
{
	if (!numInstancesArchived_)
	{
		return 0;
	}
	return archiveSize_ / numInstancesArchived_;
}

uint32 EntityType::totalBackupSize() const
{
	return this->averageBackupSize() * numInstances_;
}

uint32 EntityType::totalArchiveSize() const
{
	return archiveSize_;
}


/**
 * 	This method returns the generic watcher for EntityTypes.
 */
WatcherPtr EntityType::pWatcher()
{
	static WatcherPtr pMapWatcher = NULL;

	if (pMapWatcher == NULL)
	{
		DirectoryWatcherPtr pWatcher = new DirectoryWatcher();

		pWatcher->addChild( "typeID", makeWatcher( &EntityType::id ) );
		pWatcher->addChild( "isProxy", makeWatcher( &EntityType::isProxy ) );
		pWatcher->addChild( "backupSize",
								makeWatcher( &EntityType::totalBackupSize ) );
		pWatcher->addChild( "totalArchiveSize",
								makeWatcher( &EntityType::totalArchiveSize ) );
		pWatcher->addChild( "averageBackupSize",
				makeWatcher( &EntityType::averageBackupSize ) );
		pWatcher->addChild( "averageArchiveSize",
				makeWatcher( &EntityType::averageArchiveSize ) ); 
		pWatcher->addChild( "numberOfInstances",
				makeWatcher( &EntityType::numInstances_ ) );

		EntityType	*pNull = NULL;
		pWatcher->addChild( "methods", EntityDescription::pWatcher(),
						   &pNull->entityDescription_ );

		pMapWatcher = new MapWatcher< NameToIndexMap >( s_nameToIndexMap_ );
		pMapWatcher->addChild( "*",
				new ContainerBounceWatcher< EntityTypes, EntityTypeID >(
					new SmartPointerDereferenceWatcher( pWatcher ),
					&s_curTypes_ ) );

		pWatcher->addChild( "profile",
							EntityTypeProfiler::pWatcher(),
							&pNull->profiler_ );

	}

	return pMapWatcher;
}

#endif // ENABLE_WATCHERS

BW_END_NAMESPACE

// entity_type.cpp
