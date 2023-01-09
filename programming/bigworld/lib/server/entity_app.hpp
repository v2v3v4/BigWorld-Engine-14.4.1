#ifndef ENTITY_APP_HPP
#define ENTITY_APP_HPP

#include "script_app.hpp"
#include "script_timers.hpp"

#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

class EntityApp;
class Watcher;

#define ENTITY_APP_HEADER SERVER_APP_HEADER

/**
 *	This ScriptTimeQueue subclass is specialised for EntityApp's time
 */
class EntityAppTimeQueue : public ScriptTimeQueue
{
public:
	EntityAppTimeQueue( int updateHertz, EntityApp & entityApp );
	virtual GameTime time() const;

private:
	EntityApp & entityApp_;
};


/**
 *	This class is a common base class for BaseApp and CellApp.
 */
class EntityApp : public ScriptApp
{
public:
	EntityApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~EntityApp();

	virtual bool init( int argc, char * argv[] );

	ScriptTimeQueue & timeQueue()	{ return timeQueue_; }

	virtual void onSignalled( int sigNum );

	BgTaskManager & bgTaskManager()		{ return bgTaskManager_; }

protected:
	void addWatchers( Watcher & watcher );
	void tickStats();

	void callTimers();

	virtual void onSetStartTime( GameTime oldTime, GameTime newTime );

	// Override from ServerApp
	virtual void onTickProcessingComplete();

	BgTaskManager bgTaskManager_;

private:

	EntityAppTimeQueue timeQueue_;

	uint32 tickStatsPeriod_;
};

BW_END_NAMESPACE

#endif // ENTITY_APP_HPP
