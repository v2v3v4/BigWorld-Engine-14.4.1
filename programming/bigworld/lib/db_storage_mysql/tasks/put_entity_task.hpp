#ifndef PUT_ENTITY_TASK_HPP
#define PUT_ENTITY_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "entity_task_with_id.hpp"

#include "cstdmf/memory_stream.hpp"

#include "server/common.hpp"


BW_BEGIN_NAMESPACE

class BufferedEntityTasks;
class EntityTypeMapping;

/**
 *	This class encapsulates the MySqlDatabase::putEntity() operation so that
 *	it can be executed in a separate thread.
 */
class PutEntityTask : public EntityTaskWithID
{
public:
	PutEntityTask( const EntityTypeMapping * pEntityTypeMapping,
							DatabaseID databaseID,
							EntityID entityID,
							BinaryIStream * pStream,
							const EntityMailBoxRef * pBaseMailbox,
							bool removeBaseMailbox,
							bool putExplicitID,
							UpdateAutoLoad updateAutoLoad,
							IDatabase::IPutEntityHandler & handler,
							GameTime * pGameTime = NULL );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performEntityMainThreadTask( bool succeeded );

protected:
	virtual void onRetry();

private:
	bool							writeEntityData_;
	bool							writeBaseMailbox_;
	bool							removeBaseMailbox_;
	bool							putExplicitID_;
	UpdateAutoLoad 					updateAutoLoad_;

	MemoryOStream					stream_;
	EntityMailBoxRef				baseMailbox_;

	IDatabase::IPutEntityHandler &	handler_;

	GameTime * pGameTime_;
};

BW_END_NAMESPACE

#endif // PUT_ENTITY_TASK_HPP
