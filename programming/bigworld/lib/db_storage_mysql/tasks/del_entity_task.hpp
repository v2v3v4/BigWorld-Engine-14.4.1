#ifndef DEL_ENTITY_TASK_HPP
#define DEL_ENTITY_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "entity_task_with_id.hpp"


BW_BEGIN_NAMESPACE

class EntityDBKey;
class EntityTypeMapping;
class MySqlDatabase;

/**
 *	This class encapsulates the MySqlDatabase::delEntity() operation so that
 *	it can be executed in a separate thread.
 */
class DelEntityTask : public EntityTaskWithID
{
public:
	DelEntityTask( const EntityTypeMapping * pEntityMapping,
					const EntityDBKey & ekey,
					EntityID entityID,
					IDatabase::IDelEntityHandler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performEntityMainThreadTask( bool succeeded );

private:
	EntityDBKey entityKey_;

	IDatabase::IDelEntityHandler &	handler_;
};

BW_END_NAMESPACE

#endif // DEL_ENTITY_TASK_HPP
