#ifndef CONSOLIDATE_ENTITY_TASK_HPP
#define CONSOLIDATE_ENTITY_TASK_HPP

#include "db_storage_mysql/tasks/put_entity_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class extends PutEntityTask to ensure that the entity is only written
 *	if the existing gameTime value is small enough.
 */
class ConsolidateEntityTask : public PutEntityTask
{
public:
	ConsolidateEntityTask( const EntityTypeMapping * pEntityTypeMapping,
							DatabaseID databaseID,
							BinaryIStream & stream,
							GameTime time,
							IDatabase::IPutEntityHandler & handler );

	virtual void performBackgroundTask( MySql & conn );

private:
	GameTime time_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_ENTITY_TASK_HPP
