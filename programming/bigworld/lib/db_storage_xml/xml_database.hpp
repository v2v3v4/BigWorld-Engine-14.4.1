#ifndef XML_DATABASE_HPP
#define XML_DATABASE_HPP

#include "db_storage/idatabase.hpp"

#include "cstdmf/timer_handler.hpp"
#include "resmgr/datasection.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class EntityDescriptionMap;
class EntityDefs;

/**
 *	This class implements the XML database functionality.
 */
class XMLDatabase : public IDatabase,
					public TimerHandler
{
public:
	XMLDatabase();
	~XMLDatabase();

	// IDatabase
	virtual bool startup( const EntityDefs&,
			Mercury::EventDispatcher & dispatcher,
			int numRetries );
	virtual bool shutDown();

	virtual BillingSystem * createBillingSystem();
	virtual bool supportsMultipleDBApps() const { return false; }

	virtual void getEntity( const EntityDBKey & entityKey,
		BinaryOStream * pStream,
		bool shouldGetBaseEntityLocation,
		IDatabase::IGetEntityHandler & handler );

	virtual void getDatabaseIDFromName( const EntityDBKey & entityKey,
		IDatabase::IGetDbIDHandler & handler );

	/*
	 *	Override from IDatabase to look up entities of a certain type based on
	 *	a match on all of the property queries.
	 *
	 *	@param entityTypeID 		The entity type's ID.
	 *	@param queries				The property queries to match.
	 *	@param handler 				The handler to call back on.
	 */
	virtual void lookUpEntities( EntityTypeID entityTypeID,
			const LookUpEntitiesCriteria & criteria,
			ILookUpEntitiesHandler & handler )
	{
		// TODO: implement me!
		ERROR_MSG( "XMLDatabase::lookUpEntities: "
			"not implemented for XML databases\n" );
		handler.onLookUpEntitiesEnd( /* hasError: */ true );
	}

	virtual void putEntity( const EntityKey & entityKey,
			EntityID entityID,
			BinaryIStream * pStream,
			const EntityMailBoxRef * pBaseMailbox,
			bool removeBaseMailbox,
			bool putExplicitID, /* unused */
			UpdateAutoLoad updateAutoLoad,
			IDatabase::IPutEntityHandler& handler );

	virtual void delEntity( const EntityDBKey & ekey,
		EntityID entityID,
		IDatabase::IDelEntityHandler& handler );

	virtual void  getBaseAppMgrInitData(
			IGetBaseAppMgrInitDataHandler& handler );

	virtual void executeRawCommand( const BW::string & command,
		IExecuteRawCommandHandler& handler );

	virtual void putIDs( int count, const EntityID * ids );
	virtual void getIDs( int count, IGetIDsHandler& handler );
	virtual void writeSpaceData( BinaryIStream& spaceData );

	virtual bool getSpacesData( BinaryOStream& strm );
	virtual void autoLoadEntities( IEntityAutoLoader & autoLoader );

	virtual void remapEntityMailboxes( const Mercury::Address& srcAddr,
			const BackupHash & destAddrs );

	// Secondary database stuff.
	virtual bool shouldConsolidate() const { return false; }
	virtual void shouldConsolidate( bool /*shouldConsolidate*/ ) { }
	virtual void addSecondaryDB( const SecondaryDBEntry& entry );
	virtual void updateSecondaryDBs( const SecondaryDBAddrs& addrs,
			IUpdateSecondaryDBshandler& handler );
	virtual void getSecondaryDBs( IGetSecondaryDBsHandler& handler );
	virtual uint32 numSecondaryDBs();
	virtual int clearSecondaryDBs();

	virtual void handleTimeout( TimerHandle handle, void * arg );

	// DB locking
	virtual bool lockDB() 	{ return true; };	// Not implemented
	virtual bool unlockDB()	{ return true; };	// Not implemented

	DatabaseID findEntityByName( EntityTypeID, const BW::string & name ) const;

private:
	void initEntityMap();

	bool deleteEntity( DatabaseID, EntityTypeID );

	void archive();
	void save();
	void timedSave();

	DatabaseID dbIDFromEntityID( EntityID id ) const;

	DataSectionPtr findOrCreateInfoSection( const BW::string & name );

	void initAutoLoad();
	void addToAutoLoad( DatabaseID databaseID );
	void removeFromAutoLoad( DatabaseID databaseID );

	void getAutoLoadSpacesFromEntities( SpaceIDSet & spaceIDs );

	//typedef StringMap< DatabaseID > NameMap;
	  // StringMap cannot handle characters > 127, use BW::map instead for
	  // wide string/unicode compatiblity
	typedef BW::map< BW::string, DatabaseID > NameMap;
	typedef BW::vector< NameMap >				NameMapVec;
	typedef BW::map< DatabaseID, DataSectionPtr > IDMap;

	DataSectionPtr	pDatabaseSection_;
	NameMapVec		nameToIdMaps_;
	IDMap			idToData_;

	typedef BW::map< DatabaseID, DataSectionPtr > AutoLoadMap;
	AutoLoadMap 	autoLoadEntityMap_;

	/// Stores the maximum of the used player IDs. Used to allocate
	/// new IDs to new players if allowed.
	DatabaseID		maxID_;

	class ActiveSetEntry
	{
	public:
		ActiveSetEntry()
			{
				baseRef.addr.ip = 0;
				baseRef.addr.port = 0;
				baseRef.id = 0;
			}
		EntityMailBoxRef	baseRef;
	};

	typedef BW::map< DatabaseID, ActiveSetEntry > ActiveSet;
	ActiveSet activeSet_;
	BW::vector<EntityID> spareIDs_;
	EntityID nextID_;

	const EntityDefs*	pEntityDefs_;
	const EntityDefs*	pNewEntityDefs_;

	int numArchives_;

	TimerHandle	archiveTimer_;
	TimerHandle	saveTimer_;
};

BW_END_NAMESPACE

#endif // XML_DATABASE_HPP
