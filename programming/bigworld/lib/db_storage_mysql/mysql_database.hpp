#ifndef MYSQL_DATABASE_HPP
#define MYSQL_DATABASE_HPP

#include "mappings/entity_type_mappings.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "network/frequent_tasks.hpp"
#include "network/interfaces.hpp"

#include "db_storage/idatabase.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

class BillingSystem;
class BufferedEntityTasks;
class GetEntityTask;
typedef SmartPointer< GetEntityTask > GetEntityTaskPtr;

class MySql;
class MySqlLockedConnection;

class EntityDefs;

namespace DBConfig
{
class ConnectionInfo;
}

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

/**
 *	This class is an implementation of IDatabase for the MySQL database.
 */
class MySqlDatabase : public IDatabase,
	public Mercury::FrequentTask
{
public:
	MySqlDatabase( Mercury::NetworkInterface & interface,
		Mercury::EventDispatcher & dispatcher );
	virtual ~MySqlDatabase();

	virtual bool startup( const EntityDefs&,
				Mercury::EventDispatcher & dispatcher, 
				int numRetries );
	virtual bool resetGameServerState();
	virtual bool shutDown();

	// IDatabase
	virtual BillingSystem * createBillingSystem();

	virtual void getEntity( const EntityDBKey & entityKey,
							BinaryOStream * pStream,
							bool shouldGetBaseEntityLocation,
							IDatabase::IGetEntityHandler & handler );

	virtual void getDatabaseIDFromName( const EntityDBKey & entityKey,
							IDatabase::IGetDbIDHandler & handler );

	virtual void lookUpEntities( EntityTypeID entityTypeID,
			const LookUpEntitiesCriteria & criteria,
			ILookUpEntitiesHandler & handler );

	virtual void putEntity( const EntityKey & ekey,
							EntityID entityID,
							BinaryIStream * pStream,
							const EntityMailBoxRef * pBaseMailbox,
							bool removeBaseMailbox,
							bool putExplicitID,
							UpdateAutoLoad updateAutoLoad,
							IPutEntityHandler& handler );

	virtual void delEntity( const EntityDBKey & ekey,
		EntityID entityID,
		IDatabase::IDelEntityHandler& handler );

	virtual void executeRawCommand( const BW::string & command,
		IDatabase::IExecuteRawCommandHandler& handler );

	virtual void putIDs( int numIDs, const EntityID * ids );
	virtual void getIDs( int numIDs, IDatabase::IGetIDsHandler& handler );

	// Backing up spaces.
	virtual void writeSpaceData( BinaryIStream& spaceData );

	virtual bool getSpacesData( BinaryOStream& strm );
	virtual void autoLoadEntities( IEntityAutoLoader & autoLoader );

	virtual void setGameTime( GameTime gameTime );

	virtual void getBaseAppMgrInitData(
			IGetBaseAppMgrInitDataHandler& handler );

	// BaseApp death handler
	virtual void remapEntityMailboxes( const Mercury::Address& srcAddr,
			const BackupHash & destAddrs );

	// Secondary database entries
	virtual bool shouldConsolidate() const { return shouldConsolidate_; }
	virtual void shouldConsolidate( bool shouldConsolidate )
		{ shouldConsolidate_ = shouldConsolidate; }
	virtual void addSecondaryDB( const IDatabase::SecondaryDBEntry& entry );
	virtual void updateSecondaryDBs( const SecondaryDBAddrs& addrs,
			IUpdateSecondaryDBshandler& handler );
	virtual void getSecondaryDBs( IDatabase::IGetSecondaryDBsHandler& handler );
	virtual uint32 numSecondaryDBs();
	virtual int clearSecondaryDBs();

	// DB locking
	virtual bool lockDB();
	virtual bool unlockDB();

	virtual void doTask();

	bool hasUnrecoverableError() const;

	void scheduleGetEntityTask( const GetEntityTaskPtr & pGetEntityTask );

private:
	bool syncTablesToDefs() const;
	void startBackgroundThreads(
		const DBConfig::ConnectionInfo & connectionInfo );

	void getAutoLoadSpacesFromEntities( SpaceIDSet & spaceIDs );

	TaskManager bgTaskManager_;

	int numConnections_;

	bool shouldConsolidate_;

	TimerHandle	reconnectTimerHandle_;
	size_t reconnectCount_;
	BufferedEntityTasks * pBufferedEntityTasks_;

	Mercury::NetworkInterface & interface_;
	Mercury::EventDispatcher & dispatcher_;

	const EntityDefs * pEntityDefs_;
	EntityTypeMappings entityTypeMappings_;

	MySqlLockedConnection * pConnection_;
};

BW_END_NAMESPACE

#endif // MYSQL_DATABASE_HPP
