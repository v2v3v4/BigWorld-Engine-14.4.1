#ifndef CONSOLIDATE_DBS__PRIMARY_DATABASE_UPDATE_QUEUE_HPP
#define CONSOLIDATE_DBS__PRIMARY_DATABASE_UPDATE_QUEUE_HPP

#include "cstdmf/bgtask_manager.hpp"

#include "db_storage_mysql/mappings/entity_type_mappings.hpp"
#include "db_storage_mysql/wrapper.hpp"

#include "db_storage/idatabase.hpp"



BW_BEGIN_NAMESPACE

class BufferedEntityTasks;
class EntityDefs;
class EntityKey;
namespace DBConfig
{
	class ConnectionInfo;
}


/**
 *	This class implements job queue for entity update operations. Entity
 *	update operations will be serviced by multiple threads.
 */
class PrimaryDatabaseUpdateQueue : public IDatabase::IPutEntityHandler
{
public:
	PrimaryDatabaseUpdateQueue( int numConnections );
	~PrimaryDatabaseUpdateQueue();

	bool init( const DBConfig::ConnectionInfo & connectionInfo,
			const EntityDefs & entityDefs );

	void addUpdate( const EntityKey & key, BinaryIStream & data,
			GameTime time );
	void waitForUpdatesCompletion();

	bool hasError() const				{ return hasError_; }

private:
	// Called by ConsolidateEntityTask
	virtual void onPutEntityComplete( bool isOkay, DatabaseID dbID );

// Member data

	TaskManager				bgTaskMgr_;
	BufferedEntityTasks * 	pBufferedEntityTasks_;
	EntityTypeMappings 		entityTypeMappings_;

	bool					hasError_;
	int						numOutstanding_;
	int						numConnections_;

	typedef BW::map< EntityKey, GameTime > ConsolidatedTimes;
	ConsolidatedTimes		consolidatedTimes_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__PRIMARY_DATABASE_UPDATE_QUEUE_HPP

