#include "script/first_include.hpp"

#include "entity_app.hpp"

#include "entity_app_config.hpp"

#include "entitydef/entity_member_stats.hpp"

#include "server/bwconfig.hpp"
#include "server/common.hpp"

#include <signal.h>

DECLARE_DEBUG_COMPONENT2( "Server", 0 );

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityAppTimeQueue
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EntityAppTimeQueue::EntityAppTimeQueue( int updateHertz,
		EntityApp & entityApp ) :
	ScriptTimeQueue( updateHertz ),
	entityApp_( entityApp )
{
}


GameTime EntityAppTimeQueue::time() const
{
	return entityApp_.time();
}


// -----------------------------------------------------------------------------
// Section: EntityApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EntityApp::EntityApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface ) :
	ScriptApp( mainDispatcher, interface ),
	timeQueue_( EntityAppConfig::updateHertz(), *this ),
	tickStatsPeriod_( 0 ) // Set below
{
	ScriptTimers::init( timeQueue_ );

	float tickStatsPeriod = BWConfig::get( "tickStatsPeriod", 2.f );
	tickStatsPeriod_ =
		uint32( tickStatsPeriod * EntityAppConfig::updateHertz() );
}


/**
 *	Destructor.
 */
EntityApp::~EntityApp()
{
	ScriptTimers::fini( timeQueue_ );
}


/**
 * 	Initialisation function.
 *
 *	This must be called from subclasses's init().
 */
bool EntityApp::init( int argc, char * argv[] )
{
	if (!this->ScriptApp::init( argc, argv ))
	{
		return false;
	}

	this->enableSignalHandler( SIGQUIT );
	return true;
}


/**
 *	This method adds the watchers associated with this class.
 */
void EntityApp::addWatchers( Watcher & watcher )
{
	this->ScriptApp::addWatchers( watcher );
}


/**
 *	This method ticks any stats that maintain a moving average.
 */
void EntityApp::tickStats()
{
	if (tickStatsPeriod_ > 0)
	{
		AUTO_SCOPED_PROFILE( "tickStats" );
		EntityMemberStats::Stat::tickSome( time_ % tickStatsPeriod_,
				tickStatsPeriod_,
				double( tickStatsPeriod_ ) / EntityAppConfig::updateHertz() );
	}
}


/**
 *	Signal handler.
 *
 *	This should be called from subclasses' onSignalled() overrides.
 */
void EntityApp::onSignalled( int sigNum )
{
	this->ScriptApp::onSignalled( sigNum );
	switch (sigNum)
	{
	case SIGQUIT:
		CRITICAL_MSG( "Received QUIT signal. This is likely caused by the "
				"%sMgr killing this %s because it has been "
				"unresponsive for too long. Look at the callstack from "
				"the core dump to find the likely cause.\n",
			strcmp( this->getAppName(), "ServiceApp" ) == 0 ?
				"BaseApp" : this->getAppName(),
			this->getAppName() );
	default: break;
	}
}


/**
 *	This method calls timers before updatables are run
 */
void EntityApp::onTickProcessingComplete()
{
	this->ServerApp::onTickProcessingComplete();
	this->callTimers();
}


/**
 *	This method calls any outstanding timers.
 */
void EntityApp::callTimers()
{
	AUTO_SCOPED_PROFILE( "callTimers" );
	timeQueue_.process( time_ );
}


/**
 *	This method responds to the game time being adjusted.
 */
void EntityApp::onSetStartTime( GameTime oldTime, GameTime newTime )
{
	if (oldTime != newTime)
	{
		if (!timeQueue_.empty())
		{
			NOTICE_MSG( "EntityApp::onSetStartTime: Adjusting %d timer%s\n",
					timeQueue_.size(), (timeQueue_.size() == 1) ? "" : "s" );
			timeQueue_.adjustBy( newTime - oldTime );
		}
	}
}

BW_END_NAMESPACE

// entity_app.hpp
