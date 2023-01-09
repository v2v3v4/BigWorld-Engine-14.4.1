#ifndef ENTITY_TASK_WITH_ID_HPP
#define ENTITY_TASK_WITH_ID_HPP

#include "db_storage/idatabase.hpp"
#include "entity_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class encapsulates the MySqlDatabase::putEntity() operation so that
 *	it can be executed in a separate thread.
 */
class EntityTaskWithID : public EntityTask
{
public:
	EntityTaskWithID( const EntityTypeMapping & entityTypeMapping,
							DatabaseID databaseID,
							EntityID entityID,
							const char * taskName );

	virtual EntityID entityID() const	{ return entityID_; }

private:
	EntityID						entityID_;
};

BW_END_NAMESPACE

#endif // ENTITY_TASK_WITH_ID_HPP
