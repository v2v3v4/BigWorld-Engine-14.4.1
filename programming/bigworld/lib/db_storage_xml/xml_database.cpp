#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "xml_database.hpp"

#include "xml_billing_system.hpp"

#include "db/db_config.hpp"

#include "db_storage/db_entitydefs.hpp"
#include "db_storage/entity_auto_loader_interface.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/entity_description_map.hpp"

#include "network/event_dispatcher.hpp"

#include "pyscript/py_data_section.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"


DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

namespace // anonymous
{

const BW::string DATABASE_INFO_SECTION( "_BigWorldInfo" );
const BW::string DATABASE_LOGONMAPPING_SECTION( "LogOnMapping" );
const BW::string DATABASE_AUTO_LOAD_ENTITIES_SECTION( "AutoLoadedEntities" );
const BW::string DATABASE_AUTO_LOAD_SPACE_DATA_SECTION( "AutoLoadedSpaceData" );

enum TimeOutType
{
	TIMEOUT_SAVE,
	TIMEOUT_ARCHIVE
};

} // end namespace (anonymous)

// -----------------------------------------------------------------------------
// Section: XMLDatabase
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
XMLDatabase::XMLDatabase() :
	pDatabaseSection_( NULL ),
	nameToIdMaps_(),
	idToData_(),
	autoLoadEntityMap_(),
	maxID_( 0 ),
	activeSet_(),
	spareIDs_(),
	nextID_( FIRST_ENTITY_ID ),
	pEntityDefs_( NULL ),
	pNewEntityDefs_( NULL ),
	numArchives_( 5 ),
	archiveTimer_(),
	saveTimer_()
{
}


/**
 *	Destructor.
 */
XMLDatabase::~XMLDatabase()
{
	archiveTimer_.cancel();
	saveTimer_.cancel();

	this->shutDown();
}


/*
 *	Override from IDatabase.
 */
bool XMLDatabase::startup( const EntityDefs& entityDefs,
		Mercury::EventDispatcher & dispatcher,
		int /*numRetries*/ )
{
	// Create NameMaps for all entity types
	nameToIdMaps_.resize( entityDefs.getNumEntityTypes() );

	pDatabaseSection_ = BWResource::openSection(
		EntityDef::Constants::xmlDatabaseFile() );
	pEntityDefs_ = &entityDefs;

	if (!pDatabaseSection_)
	{
		NOTICE_MSG( "XMLDatabase::startup: Could not open %s: Creating it.\n",
				EntityDef::Constants::xmlDatabaseFile() );
		pDatabaseSection_ = BWResource::openSection(
				EntityDef::Constants::xmlDatabaseFile(), true );
	}

	this->initEntityMap();

	// add the DB as an attribute for Python - so executeRawDatabaseCommand()
	// can access the database.
	PyObjectPtr dbRoot( new PyDataSection( pDatabaseSection_ ),
						PyObjectPtr::STEAL_REFERENCE );
	PyObject* pBigWorldModule = PyImport_AddModule( "BigWorld" );
	PyObject_SetAttrString( pBigWorldModule, "dbRoot", dbRoot.get() );
	// Import BigWorld module as user cannot execute "import" using
	// executeRawDatabaseCommand().

	PyObject * pMainModule = PyImport_AddModule( "__main__" );

	if (pMainModule)
	{
		PyObject * pMainModuleDict = PyModule_GetDict( pMainModule );
		if (PyDict_SetItemString( pMainModuleDict, "BigWorld", pBigWorldModule)
				 != 0)
		{
			ERROR_MSG( "XMLDatabase::startup: Can't insert BigWorld module into"
						" __main__ module\n" );
		}
	}
	else
	{
		ERROR_MSG( "XMLDatabase::startup: Can't create Python __main__ "
					"module\n" );
		PyErr_Print();
	}

	const DBConfig::Config & config = DBConfig::get();

	const float archivePeriod = config.xml.archivePeriod();

	if (archivePeriod > 0.f)
	{
		numArchives_ = config.xml.numArchives();

		INFO_MSG( "XMLDatabase::startup: "
					"archivePeriod = %.1f seconds. numArchives = %d\n",
				archivePeriod, numArchives_ );

		int64 microSeconds = int64( archivePeriod * 1000000.0 );
		archiveTimer_ =
			dispatcher.addTimer( microSeconds,
					this, reinterpret_cast< void * >( TIMEOUT_ARCHIVE ),
					"XMLDB_Archive" );

	}

	const float savePeriod = config.xml.savePeriod();

	if (savePeriod > 0.f)
	{
		INFO_MSG( "XMLDatabase::startup: savePeriod = %.1f seconds\n",
				savePeriod );
		int64 microSeconds = int64( savePeriod * 1000000.0 );
		saveTimer_ = dispatcher.addTimer( microSeconds,
					this, reinterpret_cast< void * >( TIMEOUT_SAVE ),
					"XMLDB_Save" );

	}

	this->initAutoLoad();

	return true;
}


/**
 *
 */
void XMLDatabase::initEntityMap()
{
	// We do two loops for backward compatibiliy, where none of the entities
	// had DBIDs stored in the xml file. The first loop reads all the entities
	// with DBIDs and then determines the max DBID. The second loop then
	// assigns DBIDs to the entities with no DBIDs.
	bool hasEntityWithNoDBID = false;
	bool shouldAssignDBIDs = false;
	for (int i = 0; i < 2; ++i)
	{
		for (DataSection::iterator sectionIter = pDatabaseSection_->begin();
				sectionIter != pDatabaseSection_->end(); ++sectionIter)
		{
			DataSectionPtr pCurr = *sectionIter;

			// Check that it is a valid entity type
			EntityTypeID typeID =
					pEntityDefs_->getEntityType( pCurr->sectionName() );

			if (typeID == INVALID_ENTITY_TYPE_ID)
			{
				// Print warning if it is not our info section and only in
				// the first loop
				if ((pCurr->sectionName() != DATABASE_INFO_SECTION) && (i == 0))
				{
					WARNING_MSG( "XMLDatabase::initEntityMap: "
							"'%s' is not a valid entity type - data ignored\n",
						pCurr->sectionName().c_str() );
				}
				continue;
			}

			DatabaseID id = pCurr->read( "databaseID", int64( 0 ) );
			if (id == 0)
			{
				if (shouldAssignDBIDs)
				{
					id = ++maxID_;
					pCurr->write( "databaseID", id );
				}
				else
				{
					hasEntityWithNoDBID = true;
					continue;
				}
			}
			else
			{
				if (shouldAssignDBIDs)
					// Since we have a DBID, we were loaded in the first loop.
					continue;
				else if (id > maxID_)
					maxID_ = id;
			}

			// Check for duplicate DBID
			IDMap::iterator idIter = idToData_.find( id );

			if (idIter != idToData_.end())
			{
				// HACK: Skip -1, -2, -3: ids for bots etc.
				if (id >= 0)
				{
					WARNING_MSG( "XMLDatabase::initEntityMap: '%s' and '%s' "
							"have same id "
							"(%" FMT_DBID ") - second entity ignored\n",
						idIter->second->sectionName().c_str(),
						pCurr->sectionName().c_str(),
						id );
				}

				continue;
			}
			else
			{
				idToData_[id] = pCurr;
			}

			// Find the name of this entity.
			const DataDescription *pIdentifier =
					pEntityDefs_->getIdentifierProperty( typeID );

			if (pIdentifier)
			{
				BW::string entityName =
						pCurr->readString( pIdentifier->name() );
				// Check that name is not already taken. This is
				// possible if a different property is chosen as the
				// name property.
				NameMap& nameMap = nameToIdMaps_[ typeID ];
				NameMap::const_iterator it = nameMap.find( entityName );

				if (it == nameMap.end())
				{
					nameMap[entityName] = id;
				}
				else
				{
					WARNING_MSG( "XMLDatabase::initEntityMap: Multiple "
							"entities of type '%s' have the same name: '%s' - "
							"second entity will not be retrievable by name\n",
						pCurr->sectionName().c_str(), entityName.c_str() );
				}
			}
		}

		if (hasEntityWithNoDBID)
			shouldAssignDBIDs = true;
		else
			break;	// Don't do second loop.
	}

	// Make sure watcher is initialised by now
	MF_WATCH( "maxID", maxID_ );
}


/**
 *	This method handles timers.
 */
void XMLDatabase::handleTimeout( TimerHandle handle, void * arg )
{
	switch (reinterpret_cast< uintptr >( arg ))
	{
	case TIMEOUT_SAVE:
		this->save();
		break;

	case TIMEOUT_ARCHIVE:
		this->archive();
		break;
	}
}


/*
 *	Override from IDatabase.
 */
bool XMLDatabase::shutDown()
{
	this->save();

	return true;
}


void XMLDatabase::archive()
{
	if (!pDatabaseSection_)
	{
		return;
	}

	MultiFileSystemPtr pFS = BWResource::instance().fileSystem();

	BW::string filename( EntityDef::Constants::xmlDatabaseFile() );

	char buf[ 32 ];

	for (int i = numArchives_; i > 0; --i)
	{
		bw_snprintf( buf, sizeof( buf ), ".%d", i );
		BW::string newName = filename + buf;

		BW::string oldName = filename;

		if (i != 1)
		{
			bw_snprintf( buf, sizeof( buf ), ".%d", i - 1 );
			oldName += buf;
		}

		pFS->moveFileOrDirectory( oldName, newName );
	}

	this->timedSave();
}


/**
 *	This method saves the XML database to disk.
 */
void XMLDatabase::save()
{
	if (!pDatabaseSection_)
	{
		return;
	}

	BW::string oldName( EntityDef::Constants::xmlDatabaseFile() );
	BW::string newName = oldName + ".bak";

	BWResource::instance().fileSystem()->moveFileOrDirectory(
			oldName, newName );

	this->timedSave();
}


/**
 *	This method saves the XML information to disk. It warns if this operation
 *	takes too long.
 */
void XMLDatabase::timedSave()
{
	uint64 startTime = timestamp();

	// NOTE: This is done in the main thread. Ideally, this should be done in
	// an I/O thread.
	pDatabaseSection_->save( EntityDef::Constants::xmlDatabaseFile() );

	uint64 timeTaken = timestamp() - startTime;

	if (timeTaken > stampsPerSecond())
	{
		// Warn if greater than 1 second
		WARNING_MSG( "XMLDatabase::timedSave: Saving took %.2f seconds. "
					"The XML database is not suitable for large databases.\n",
				double( timeTaken )/stampsPerSecondD() );
	}
}


/**
 *	This method converts an entity id into a database id.
 */
DatabaseID XMLDatabase::dbIDFromEntityID( EntityID id ) const
{
	ActiveSet::const_iterator iter = activeSet_.begin();

	while (iter != activeSet_.end())
	{
		if (iter->second.baseRef.id == id)
		{
			return iter->first;
		}

		++iter;
	}

	WARNING_MSG( "XMLDatabase::dbIDFromEntityID: %d is not an active entity\n",
			id );

	return 0;
}


/**
 *	This method creates the default billing system associated with the XML
 *	database.
 */
BillingSystem * XMLDatabase::createBillingSystem()
{
	DataSectionPtr pLogonMapSection = this->findOrCreateInfoSection(
		DATABASE_LOGONMAPPING_SECTION );

	return new XMLBillingSystem( pLogonMapSection, *pEntityDefs_, this );
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::getEntity( const EntityDBKey & entityKey,
		BinaryOStream * pStream,
		bool shouldGetBaseEntityLocation,
		IDatabase::IGetEntityHandler & handler )
{
	const EntityDefs& entityDefs = *pEntityDefs_;

	// Take a copy so that it can be modified.
	EntityDBKey ekey = entityKey;

	MF_ASSERT( pDatabaseSection_ );

	bool isOK = true;

	const EntityDescription& desc =
		entityDefs.getEntityDescription( ekey.typeID );

	bool lookupByName = (!ekey.dbID);

	if (lookupByName)
	{
		ekey.dbID = this->findEntityByName( ekey.typeID, ekey.name );
	}

	isOK = (ekey.dbID != 0);

	EntityMailBoxRef baseEntityLocation;
	EntityMailBoxRef * pLocation = NULL;

	if (isOK)
	{
		// Get entity data
		IDMap::const_iterator iterData = idToData_.find( ekey.dbID );

		if (iterData != idToData_.end())
		{
			DataSectionPtr pData = iterData->second;

			if (pData->sectionName() != desc.name())
			{
				WARNING_MSG( "XMLDatabase::getEntity: "
						"Invalid entity type. Expected type '%s'. Got '%s'\n",
					desc.name().c_str(), pData->sectionName().c_str() );
				isOK = false;
			}

			if (isOK && !lookupByName)
			{
				// Set ekey.name
				const DataDescription *pIdentifier =
						entityDefs.getIdentifierProperty( ekey.typeID );
				if (pIdentifier)
				{
					ekey.name = pData->readString( pIdentifier->name() );
				}
			}

			if (isOK && (pStream != NULL))
			{
				BinaryOStream & stream = *pStream;

				desc.addSectionToStream( pData, stream,
					EntityDescription::BASE_DATA |
					EntityDescription::CELL_DATA |
					EntityDescription::ONLY_PERSISTENT_DATA	);

				if (desc.canBeOnCell())
				{
					Vector3 position = pData->readVector3( "position" );
					// This constructor takes its input as (roll, pitch, yaw)
					Direction3D direction( pData->readVector3( "direction" ) );
					SpaceID spaceID = pData->readInt( "spaceID" );
					stream << position << direction << spaceID;
				}
			}
		}
		else
		{
			isOK = false;
		}

		if (isOK && shouldGetBaseEntityLocation)
		{
			ActiveSet::iterator iter = activeSet_.find( ekey.dbID );

			if (iter != activeSet_.end())
			{
				baseEntityLocation = iter->second.baseRef;
				pLocation = &baseEntityLocation;
			}
		}
	}

	handler.onGetEntityComplete( isOK, ekey, pLocation );
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::getDatabaseIDFromName( const EntityDBKey & origEntityKey,
		IDatabase::IGetDbIDHandler & handler )
{
	MF_ASSERT( pDatabaseSection_ );

	// Take a copy so that it can be modified.
	EntityDBKey entityKey = origEntityKey;

	entityKey.dbID = this->findEntityByName( entityKey.typeID,
							entityKey.name );

	bool isOK = (entityKey.dbID > 0);

	handler.onGetDbIDComplete( isOK, entityKey );
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::putEntity( const EntityKey & entityKey,
					EntityID entityID,
					BinaryIStream * pStream,
					const EntityMailBoxRef * pBaseMailbox,
					bool removeBaseMailbox,
					bool putExplicitID, /* unused */
					UpdateAutoLoad updateAutoLoad,
					IDatabase::IPutEntityHandler& handler )
{
	MF_ASSERT( pDatabaseSection_ );

	const EntityDefs& entityDefs = *pEntityDefs_;
	const EntityDescription& desc =
		entityDefs.getEntityDescription( entityKey.typeID );
	const DataDescription *pIdentifier =
		entityDefs.getIdentifierProperty( entityKey.typeID );

	bool isOK = true;
	bool definitelyExists = false;
	DatabaseID dbID = entityKey.dbID;

	if (dbID == PENDING_DATABASE_ID)
	{
		dbID = this->dbIDFromEntityID( entityID );

		if (dbID == 0)
		{
			if (pStream)
			{
				pStream->finish();
			}

			WARNING_MSG( "XMLDatabase::putEntity: Could not get dbID for %d\n",
				   entityID	);
			handler.onPutEntityComplete( /*isOK:*/false, dbID );
			return;
		}
	}

	bool isExisting = (dbID != 0);

	if (pStream)
	{
		// Find the existing entity (if any)
		DataSectionPtr 	pOldProps;
		BW::string		oldName;

		if (isExisting)	// Existing entity
		{
			IDMap::const_iterator iterData = idToData_.find( dbID );
			if (iterData != idToData_.end())
			{
				pOldProps = iterData->second;
				if (pIdentifier)
				{
					oldName = pOldProps->readString( pIdentifier->name() );
				}
			}
			else
			{
				isOK = false;
			}
		}
		else
		{
			dbID = ++maxID_;
		}

		// Read stream into new data section
		DataSectionPtr pProps = pDatabaseSection_->newSection( desc.name() );;
		desc.readStreamToSection( *pStream,
			EntityDescription::BASE_DATA | EntityDescription::CELL_DATA |
				EntityDescription::ONLY_PERSISTENT_DATA,
			pProps );

		if (desc.canBeOnCell())
		{
			Vector3				position;
			Direction3D			direction;
			SpaceID				spaceID;

			*pStream >> position >> direction >> spaceID;
			pProps->writeVector3( "position", position );
			pProps->writeVector3( "direction", *(Vector3 *)&direction );
			pProps->writeInt( "spaceID", spaceID );
		}

		// Used in MySQL only
		GameTime gameTime;
		*pStream >> gameTime;

		// Check name if this type has a name property
		if (isOK && pIdentifier)
		{
			NameMap& 	nameMap = nameToIdMaps_[ entityKey.typeID ];
			BW::string newName = pProps->readString( pIdentifier->name() );

			// New entity or existing entity's name has changed.
			if (!isExisting || (oldName != newName))
			{
				// Check that entity name isn't already taken
				NameMap::const_iterator it = nameMap.find( newName );
				if (it == nameMap.end())
				{
					if (isExisting)	// Existing entity's name has changed
					{
					 	nameMap.erase( oldName );
					}
					nameMap[ newName ] = dbID;
				}
				else	// Name already taken.
				{
					WARNING_MSG( "XMLDatabase::putEntity: '%s' entity named"
						" '%s' already exists\n", desc.name().c_str(),
						newName.c_str() );
					isOK = false;
				}
			}
		}

		if (isOK)
		{
			pProps->write( "databaseID", dbID );

			if (isExisting)
			{
				pDatabaseSection_->delChild( pOldProps );
			}

			idToData_[dbID] = pProps;

			definitelyExists = true;
		}
		else
		{
			pDatabaseSection_->delChild( pProps );
		}
	}

	if (isOK &&
			(updateAutoLoad != UPDATE_AUTO_LOAD_RETAIN))
	{
		if (updateAutoLoad == UPDATE_AUTO_LOAD_TRUE)
		{
			this->addToAutoLoad( dbID );
		}
		else
		{
			this->removeFromAutoLoad( dbID );
		}
	}


	if (isOK &&
			((pBaseMailbox != NULL) || removeBaseMailbox))
	{
		// Update base mailbox.
		if (!definitelyExists)
		{
			isOK = (idToData_.find( dbID ) != idToData_.end());
		}

		if (isOK)
		{
			ActiveSet::iterator iter = activeSet_.find( dbID );

			if (pBaseMailbox)
			{
				if (iter != activeSet_.end())
				{
					iter->second.baseRef = *pBaseMailbox;
				}
				else
				{
					activeSet_[ dbID ].baseRef = *pBaseMailbox;
				}
			}
			else
			{
				this->removeFromAutoLoad( dbID );

				// Set base mailbox to null.
				if (iter != activeSet_.end())
				{
					activeSet_.erase( iter );
				}
			}
		}
	}

	handler.onPutEntityComplete( isOK, dbID );
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::delEntity( const EntityDBKey & ekey,
							EntityID entityID,
							IDatabase::IDelEntityHandler & handler )
{
	DatabaseID dbID = ekey.dbID;

	if (dbID == PENDING_DATABASE_ID)
	{
		dbID = this->dbIDFromEntityID( entityID );

		if (dbID == 0)
		{
			WARNING_MSG( "XMLDatabase::delEntity: Could not get dbID for %d\n",
				   entityID	);
			handler.onDelEntityComplete( /*isOkay:*/ false );
			return;
		}
	}

	// look up the id if we don't already know it
	if (dbID == 0)
	{
		dbID = findEntityByName( ekey.typeID, ekey.name );
	}

	bool isOK = (dbID != 0);

	if (isOK)
	{
		// try to delete it
		isOK = this->deleteEntity( dbID, ekey.typeID );

		// Remove from active set
		if (isOK)
		{
			ActiveSet::iterator afound = activeSet_.find( dbID );

			if (afound != activeSet_.end())
			{
				activeSet_.erase( afound );
			}
		}
	}

	handler.onDelEntityComplete(isOK);
}


/**
 *	Private delete method
 */
bool XMLDatabase::deleteEntity( DatabaseID id, EntityTypeID typeID )
{
	MF_ASSERT( pDatabaseSection_ );

	// find the record
	IDMap::iterator dfound = idToData_.find( id );
	if (dfound == idToData_.end())
	{
		return false;
	}

	DataSectionPtr pSect = dfound->second;

	// get rid of the name
	const DataDescription *pIdentifier =
		pEntityDefs_->getIdentifierProperty( typeID );
	if (pIdentifier)
	{
		BW::string name = pSect->readString( pIdentifier->name() );
		nameToIdMaps_[ typeID ].erase( name );
	}

	// get rid of the id
	idToData_.erase( dfound );

	// and finally get rid of the data section
	pDatabaseSection_->delChild( pSect );

	this->removeFromAutoLoad( id );

	return true;
}


/**
 *	Private find method
 */
DatabaseID XMLDatabase::findEntityByName( EntityTypeID entityTypeID,
		const BW::string & name ) const
{
	MF_ASSERT( pDatabaseSection_ );

	const NameMap& nameMap = nameToIdMaps_[entityTypeID];
	NameMap::const_iterator it = nameMap.find( name );

	return (it != nameMap.end()) ?  it->second : 0;
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::getBaseAppMgrInitData(
		IGetBaseAppMgrInitDataHandler& handler )
{
	// We don't remember game time.
	// Always return 0.
	handler.onGetBaseAppMgrInitDataComplete( 0 );
}


/**
 *	Override from IDatabase
 */
void XMLDatabase::executeRawCommand( const BW::string & command,
	IExecuteRawCommandHandler& handler )
{
	BW::string errMsg( "XML database does not support "
		   "executeRawDatabaseCommand" );

	handler.response() << errMsg;
	ERROR_MSG( "XMLDatabase::executeRawCommand: %s\n", errMsg.c_str() );
	PyErr_Print();
	handler.onExecuteRawCommandComplete();
}


/**
 *	Override from IDatabase.
 */
void XMLDatabase::putIDs( int count, const EntityID * ids )
{
	for (int i=0; i<count; i++)
		spareIDs_.push_back( ids[i] );
}


/**
 *	Override from IDatabase.
 */
void XMLDatabase::getIDs( int count, IGetIDsHandler& handler )
{
	BinaryOStream& strm = handler.idStrm();
	int counted = 0;

	while ((counted < count) && spareIDs_.size())
	{
		strm << spareIDs_.back();
		spareIDs_.pop_back();

		++counted;
	}

	while (counted < count)
	{
		strm << nextID_++;

		++counted;

		// It should be safe to just wrap
		// the ID around hoping nobody uses FIRST_ENTITY_ID
		// after ~2.1 bn generated IDs.
		if (nextID_ >= FIRST_LOCAL_ENTITY_ID)
		{
			nextID_ = FIRST_ENTITY_ID;
		}
	}

	handler.onGetIDsComplete();
}


/*
 *	Override frrom IDatabase. Archiving of SpaceData is not supported by the
 *	XMLDatabase.
 */
void XMLDatabase::writeSpaceData( BinaryIStream & spaceData )
{
	DataSectionPtr pSpaceDataSection = this->findOrCreateInfoSection(
		DATABASE_AUTO_LOAD_SPACE_DATA_SECTION );

	pSpaceDataSection->delChildren();

	uint32 numSpaces;
	spaceData >> numSpaces;

	for (uint32 spaceIndex = 0; spaceIndex < numSpaces; ++spaceIndex)
	{
		SpaceID spaceID;
		spaceData >> spaceID;

		DataSectionPtr pSpaceSection = pSpaceDataSection->newSection( "space" );
		pSpaceSection->writeUInt( "id", spaceID );

		DataSectionPtr pSpaceDataSection =
			pSpaceSection->newSection( "dataItems" );

		uint32 numData;
		spaceData >> numData;

		for (uint32 dataIndex = 0; dataIndex < numData; ++dataIndex)
		{
			uint64 spaceKey;
			uint16 dataKey;
			BW::string data;

			spaceData >> spaceKey;
			spaceData >> dataKey;
			spaceData >> data;

			DataSectionPtr pItemSection =
				pSpaceDataSection->newSection( "item" );
			pItemSection->writeUInt64( "spaceKey", spaceKey );
			pItemSection->writeUInt( "dataKey", dataKey );
			pItemSection->writeBlob( "data", data );
		}
	}
}


/**
 *	Override from IDatabase.
 */
bool XMLDatabase::getSpacesData( BinaryOStream & strm )
{
	SpaceIDSet spaceIDs;

	this->getAutoLoadSpacesFromEntities( spaceIDs );

	strm << int( spaceIDs.size() );

	DataSectionPtr pSpaceDataSection = this->findOrCreateInfoSection(
		DATABASE_AUTO_LOAD_SPACE_DATA_SECTION );

	typedef BW::map< SpaceID, DataSectionPtr > SpaceDatas;
	SpaceDatas spaceDatas;

	DataSection::iterator spaceSectionIter = pSpaceDataSection->begin();

	while (spaceSectionIter != pSpaceDataSection->end())
	{
		DataSectionPtr pSpaceSection = *spaceSectionIter;

		SpaceID spaceID = SpaceID( pSpaceSection->readUInt( "id" ) );
		DataSectionPtr pDataItems = pSpaceSection->findChild( "dataItems" );

		if (spaceID && pDataItems)
		{
			spaceDatas[ spaceID ] = pDataItems;
		}
		else
		{
			WARNING_MSG( "XMLDatabase::getSpacesData: Invalid <space> section. "
					"name = '%s'. spaceID = %d. pDataItems = %p\n",
				pSpaceSection->sectionName().c_str(),
				spaceID, pDataItems.get() );

		}

		++spaceSectionIter;
	}

	for (SpaceIDSet::const_iterator iSpaceID = spaceIDs.begin();
			iSpaceID != spaceIDs.end();
			++iSpaceID)
	{
		SpaceID spaceID = *iSpaceID;
		strm << spaceID;

		SpaceDatas::const_iterator iSpaceData = spaceDatas.find( spaceID );
		if (iSpaceData != spaceDatas.end())
		{
			DataSectionPtr pDataItems = iSpaceData->second;
			strm << int( pDataItems->countChildren() );

			DataSection::iterator dataItemsIter = pDataItems->begin();

			while (dataItemsIter != pDataItems->end())
			{
				DataSectionPtr pItemSection = *dataItemsIter;
				strm << pItemSection->readUInt64( "spaceKey" );
				strm << uint16( pItemSection->readUInt( "dataKey" ) );
				strm << pItemSection->readBlob( "data" );

				dataItemsIter++;
			}
		}
		else
		{
			strm << int( 0 );
		}
	}

	return true;
}


/**
 *	Override from IDatabase.
 */
void XMLDatabase::autoLoadEntities( IEntityAutoLoader & autoLoader )
{
	const EntityDefs & entityDefs = *pEntityDefs_;

	AutoLoadMap::const_iterator iAutoLoad = autoLoadEntityMap_.begin();
	while (iAutoLoad != autoLoadEntityMap_.end())
	{
		DatabaseID databaseID = iAutoLoad->first;

		AutoLoadMap::const_iterator iDataSection = idToData_.find( databaseID );

		if (iDataSection != idToData_.end())
		{
			BW::string entityTypeName = iDataSection->second->sectionName();
			EntityTypeID entityTypeID = 
				entityDefs.getEntityType( entityTypeName );

			if (entityTypeID == INVALID_ENTITY_TYPE_ID)
			{
				ERROR_MSG( "XMLDatabase::autoLoadEntities: "
						"could not find entity type for auto-loading entity "
						"with database ID = %" FMT_DBID ": %s\n",
					databaseID, entityTypeName.c_str() );
			}
			else
			{
				autoLoader.addEntity( entityTypeID, databaseID );
			}
		}
		else
		{
			WARNING_MSG( "XMLDatabase::autoLoadEntities: "
					"removing database ID for non-existent entity from "
					"auto-loading entities set: %" FMT_DBID "\n",
				databaseID );

			this->removeFromAutoLoad( databaseID );
		}

		++iAutoLoad;
	}

	autoLoader.start();
}


/**
 *	This method retrieves the spaceIDs from the auto-loaded entities.
 */
void XMLDatabase::getAutoLoadSpacesFromEntities( SpaceIDSet & spaceIDs )
{
	spaceIDs.clear();

	AutoLoadMap::const_iterator iAutoLoad = autoLoadEntityMap_.begin();
	while (iAutoLoad != autoLoadEntityMap_.end())
	{
		DatabaseID databaseID = iAutoLoad->first;

		IDMap::const_iterator iEntitySection = idToData_.find( databaseID );

		if (iEntitySection != idToData_.end())
		{
			SpaceID spaceID = iEntitySection->second->readInt( "spaceID" );

			if (spaceID)
			{
				spaceIDs.insert( spaceID );
			}
		}

		++iAutoLoad;
	}
}


/**
 *	Override from IDatabase.
 */
void XMLDatabase::remapEntityMailboxes( const Mercury::Address & srcAddr,
		const BackupHash & destAddrs )
{
	for (ActiveSet::iterator iter = activeSet_.begin();
			iter != activeSet_.end(); ++iter)
	{
		if (iter->second.baseRef.addr == srcAddr)
		{
			const Mercury::Address& newAddr =
					destAddrs.addressFor( iter->second.baseRef.id );
			// Mercury::Address::salt must not be modified.
			iter->second.baseRef.addr.ip = newAddr.ip;
			iter->second.baseRef.addr.port = newAddr.port;
		}
	}
}


/**
 *	Overrides IDatabase method
 */
void XMLDatabase::addSecondaryDB( const SecondaryDBEntry & entry )
{
	WARNING_MSG( "XMLDatabase::addSecondaryDb: Not implemented!\n" );
}


/**
 *	Overrides IDatabase method
 */
void XMLDatabase::updateSecondaryDBs( const SecondaryDBAddrs & addrs,
		IUpdateSecondaryDBshandler & handler )
{
	WARNING_MSG( "XMLDatabase::updateSecondaryDBs: Not implemented!\n" );
	handler.onUpdateSecondaryDBsComplete( SecondaryDBEntries() );
}


/**
 *	Overrides IDatabase method
 */
void XMLDatabase::getSecondaryDBs( IGetSecondaryDBsHandler& handler )
{
	WARNING_MSG( "XMLDatabase::getSecondaryDBs: Not implemented!\n" );
	handler.onGetSecondaryDBsComplete( SecondaryDBEntries() );
}


/**
 *	Overrides IDatabase method
 */
uint32 XMLDatabase::numSecondaryDBs()
{
	return 0;
}


/**
 *	Overrides IDatabase method
 */
int XMLDatabase::clearSecondaryDBs()
{
	// This always succeeds to simplify code from caller.
	// WARNING_MSG( "XMLDatabase::clearSecondaryDBs: Not implemented!\n" );
	return 0;
}


/**
 *	This method finds (or creates if missing) the server info data section.
 */
DataSectionPtr XMLDatabase::findOrCreateInfoSection(
		const BW::string & name )
{
	DataSectionPtr pInfoSection = pDatabaseSection_->findChild(
		DATABASE_INFO_SECTION );

	if (!pInfoSection)
	{
		pInfoSection = pDatabaseSection_->newSection( DATABASE_INFO_SECTION );
	}

	DataSectionPtr pInfoSectionChild = pInfoSection->findChild( name );
	if (!pInfoSectionChild)
	{
		pInfoSectionChild = pInfoSection->newSection( name );
	}

	return pInfoSectionChild;
}


/**
 *	This method reads the auto-loaded entities from the database section.
 */
void XMLDatabase::initAutoLoad()
{
	DataSectionPtr pAutoLoadSection = this->findOrCreateInfoSection(
		DATABASE_AUTO_LOAD_ENTITIES_SECTION );

	autoLoadEntityMap_.clear();
	for (int i = 0; i< pAutoLoadSection->countChildren(); ++i)
	{
		DataSectionPtr pChild = pAutoLoadSection->openChild( i );
		if (pChild->sectionName() == "databaseID" )
		{
			DatabaseID databaseID = pChild->asInt64();
			if (databaseID <= 0)
			{
				ERROR_MSG( "XMLDatabase::initAutoLoadSet: "
						"read invalid auto-load entity database ID in "
						"element %d\n",
					i );
			}
			else
			{
				autoLoadEntityMap_[databaseID] = pChild;
			}
		}
	}
}


/**
 *	This method adds a database ID to the auto-loaded entities.
 */
void XMLDatabase::addToAutoLoad( DatabaseID databaseID )
{
	if (autoLoadEntityMap_.count( databaseID ) != 0)
	{
		return;
	}

	DataSectionPtr pAutoLoadSection = this->findOrCreateInfoSection(
		DATABASE_AUTO_LOAD_ENTITIES_SECTION );

	DataSectionPtr pChild = pAutoLoadSection->newSection( "databaseID" );
	pChild->setUInt64( databaseID );

	autoLoadEntityMap_[databaseID] = pChild;
}


/**
 *	This method removes a database ID from the auto-loaded entities.
 */
void XMLDatabase::removeFromAutoLoad( DatabaseID databaseID )
{
	AutoLoadMap::iterator iDatabase = autoLoadEntityMap_.find( databaseID );

	if (iDatabase == autoLoadEntityMap_.end())
	{
		return;
	}

	DataSectionPtr pAutoLoadSection = this->findOrCreateInfoSection(
		DATABASE_AUTO_LOAD_ENTITIES_SECTION );

	pAutoLoadSection->delChild( iDatabase->second );

	autoLoadEntityMap_.erase( iDatabase );
}

BW_END_NAMESPACE

// xml_database.cpp

