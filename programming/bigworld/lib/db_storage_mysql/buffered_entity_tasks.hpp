#ifndef BUFFERED_ENTITY_TASKS_HPP
#define BUFFERED_ENTITY_TASKS_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class TaskManager;
class EntityKey;
class EntityTask;

typedef SmartPointer< EntityTask > EntityTaskPtr;

/**
 *	This class is responsible for ensuring there is only a single database
 *	operation outstanding for a specific entity. If there are two writeToDB
 *	calls for the same entity outstanding, these cannot be sent to different
 *	threads as the order these are applied to the database are not guaranteed.
 */
class BufferedEntityTasks
{
public:
	BufferedEntityTasks( TaskManager & bgTaskManager );

	void addBackgroundTask( const EntityTaskPtr & pTask );

	void onFinished( const EntityTaskPtr & pTask );

	bool shouldDelayAdds() const
	{
		return shouldDelayAdds_;
	}

	void shouldDelayAdds( bool shouldDelayAdds )
	{
		shouldDelayAdds_ = shouldDelayAdds;
	}

	unsigned int size() const { return tasks_.size(); }

private:
	bool grabLock( const EntityTaskPtr & pTask );
	void buffer( const EntityTaskPtr & pTask );
	void doTask( const EntityTaskPtr & pTask );

	void onFinishedNewEntity( const EntityTaskPtr & pFinishedTask );

	template < class MAP, class ID >
	bool playNextTask( MAP & tasks, ID id );

	TaskManager & bgTaskManager_;

	typedef BW::multimap< EntityKey, EntityTaskPtr > EntityKeyMap;
	typedef BW::multimap< EntityID, EntityTaskPtr > EntityIDMap;

	EntityKeyMap tasks_;
	EntityIDMap priorToDBIDTasks_;

	typedef BW::map< EntityID, DatabaseID > NewEntityMap;
	NewEntityMap newEntityMap_;

	bool shouldDelayAdds_;
};

BW_END_NAMESPACE

#endif // BUFFERED_ENTITY_TASKS_HPP
