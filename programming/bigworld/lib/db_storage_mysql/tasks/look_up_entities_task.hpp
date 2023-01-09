#ifndef LOOKUP_ENTITIES_TASK_HPP
#define LOOKUP_ENTITIES_TASK_HPP

#include "background_task.hpp"

#include "db_storage/idatabase.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class EntityTypeMapping;


/**
 *	This class encapsulates the MySqlDatabase::lookUpEntities() operation so
 *	that it can be executed in a separate thread.
 */
class LookUpEntitiesTask : public MySqlBackgroundTask
{
public:
	LookUpEntitiesTask( const EntityTypeMapping & entityTypeMapping,
		const LookUpEntitiesCriteria & criteria,
		IDatabase::ILookUpEntitiesHandler & handler );

	virtual ~LookUpEntitiesTask() {}


	// Overrides from MySqlBackgroundTask

	virtual void performBackgroundTask( MySql & conn );

	virtual void performMainThreadTask( bool succeeded );

private:
	const EntityTypeMapping & 			mapping_;
	LookUpEntitiesCriteria 				criteria_;
	IDatabase::ILookUpEntitiesHandler & handler_;
};

BW_END_NAMESPACE

#endif // LOOKUP_ENTITIES_TASK_HPP

