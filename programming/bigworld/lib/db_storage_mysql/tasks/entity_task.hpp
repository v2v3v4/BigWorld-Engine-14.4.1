#ifndef ENTITY_TASK_HPP
#define ENTITY_TASK_HPP

#include "background_task.hpp"

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class BufferedEntityTasks;
class EntityKey;
class EntityTypeMapping;


/**
 *	This class encapsulates the MySqlDatabase::putIDs() operation
 *	so that it can be executed in a separate thread.
 */
class EntityTask : public MySqlBackgroundTask
{
public:
	EntityTask( const EntityTypeMapping & entityTypeMapping,
			DatabaseID dbID, const char * taskName );

	virtual void performMainThreadTask( bool succeeded );
	virtual EntityID entityID() const	{ return 0; }

	DatabaseID dbID() const			{ return dbID_; }
	void dbID( DatabaseID dbID )	{ dbID_ = dbID; }

	EntityKey entityKey() const;

	void pBufferedEntityTasks( BufferedEntityTasks * pBufferedEntityTasks )
	{
		pBufferedEntityTasks_ = pBufferedEntityTasks;
	}

	void bufferTimestamp( uint64 bufferTimestamp )
	{
		bufferTimestamp_ = bufferTimestamp;
	}

	uint64 bufferTimestamp() const { return bufferTimestamp_; }

protected:
	virtual void performEntityMainThreadTask( bool succeeded ) = 0;

	const EntityTypeMapping & entityTypeMapping_;
	DatabaseID	dbID_;

private:
	BufferedEntityTasks * pBufferedEntityTasks_;
	uint64 bufferTimestamp_;
};

BW_END_NAMESPACE

#endif // ENTITY_TASK_HPP
