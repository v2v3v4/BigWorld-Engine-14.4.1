#include "dbappmgr.hpp"

#include "dbapp.hpp"
#include "dbappmgr_config.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "loginapp/login_int_interface.hpp"

#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/watcher.hpp"

#include "network/blocking_reply_handler.hpp"
#include "network/bundle.hpp"
#include "network/channel.hpp"
#include "network/channel_sender.hpp"
#include "network/machined_utils.hpp"
#include "network/watcher_nub.hpp"

#include "resmgr/bwresource.hpp"

#include "server/reviver_subject.hpp"
#include "server/server_app_option.hpp"
#include "server/signal_processor.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/// DBAppMgr Singleton.
BW_SINGLETON_STORAGE( DBAppMgr )


const double DBAppMgr::ADD_WAIT_TIME_SECONDS = 1.0;


/**
 *	This template specialisation implements the streaming for Rendezvous
 *	hashing so it can stream out the address of the channel, rather than
 *	try to stream out the channel owner.
 */
template<>
struct DBAppMgr::DBApps::MapStreaming
{
	
	/**
	 *	This static method adds the given map to the stream, overriding
	 *	the default template implementation for generic maps.
	 */
	static void addToStream( BinaryOStream & os, 
			const BW::map< DBAppID, DBAppPtr > & map )
	{
		os.writePackedInt( static_cast< int >( map.size() ) );

		for (BW::map< DBAppID, DBAppPtr >::const_iterator iter = map.begin();
				iter != map.end();
				++iter)
		{
			// Stream out the address of the channel.
			os << iter->first << iter->second->channelOwner().addr();
		}
	}

	// We don't support streaming in for DBAppMgr. By not defining it, it
	// should fail at compile-time if it is ever attempted.
	//static bool readFromStream( BinaryIStream & is,
	//		BW::map< DBAppID, DBAppPtr > & map );
};


// -----------------------------------------------------------------------------
// Section: DBAppMgr
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
DBAppMgr::DBAppMgr( Mercury::EventDispatcher & mainDispatcher,
	Mercury::NetworkInterface & interface ) :
	ManagerApp( mainDispatcher, interface ),
	tickTimer_(),
	isShuttingDown_( false ),
	baseAppMgr_( interface ),
	cellAppMgr_( interface ),
	dbApps_(),
	addressMap_(),
	dbAppAddStartWaitTime_( 0 ),
	lastDBAppID_( 0 ),
	loginApps_(),
	shouldAcceptLoginApps_( true ),
	gatherLoginAppsTimer_(),
	lastLoginAppID_( 0 ),
	startupState_( STARTUP_STATE_NOT_STARTED )
{
	baseAppMgr_.channel().isLocalRegular( false );
	baseAppMgr_.channel().isRemoteRegular( false );

	cellAppMgr_.channel().isLocalRegular( false );
	cellAppMgr_.channel().isRemoteRegular( false );
}


/**
 *	Destructor.
 */
DBAppMgr::~DBAppMgr()
{
	tickTimer_.cancel();
	gatherLoginAppsTimer_.cancel();

	interface_.processUntilChannelsEmpty();
}


/*
 * 	Override from ManagerApp.
 */
bool DBAppMgr::init( int argc, char * argv[] ) /* override */
{
	if (!this->ManagerApp::init( argc, argv ))
	{
		return false;
	}

	if (!this->interface().isGood())
	{
		ERROR_MSG( "DBAppMgr::init: "
				"Failed to create Nub on internal interface.\n" );
		return false;
	}

	PROC_IP_INFO_MSG( "Internal address = %s\n",
		this->interface().address().c_str() );

	DBAppMgrInterface::registerWithInterface( interface_ );

	// Find CellAppMgr.
	Mercury::Address cellAppMgrAddress;
	Mercury::Reason reason = Mercury::MachineDaemon::findInterface( 
		"CellAppMgrInterface", 0, cellAppMgrAddress,
		Config::numStartupRetries() );

	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "DBAppMgr::init: Unable to find CellAppMgr: %s\n",
			Mercury::reasonToString( reason ) );
		return false;
	}

	cellAppMgr_.addr( cellAppMgrAddress );

	// Find and contact BaseAppMgr.
	Mercury::Address baseAppMgrAddress;
	reason = Mercury::MachineDaemon::findInterface( "BaseAppMgrInterface",
		0, baseAppMgrAddress, Config::numStartupRetries() );

	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "DBAppMgr::init: Unable to find BaseAppMgr: %s\n",
			Mercury::reasonToString( reason ) );
		return false;
	}

	baseAppMgr_.addr( baseAppMgrAddress );

	Mercury::BlockingReplyHandlerWithResult< bool > hasStartedHandler(
		this->interface() );
	baseAppMgr_.bundle().startRequest( BaseAppMgrInterface::requestHasStarted,
		&hasStartedHandler );
	baseAppMgr_.send();
	reason = hasStartedHandler.waitForReply( &baseAppMgr_.channel() );
	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "DBAppMgr::init: Failed to request start status "
			"from BaseAppMgr\n" );
		return false;
	}

	const bool hasStarted = hasStartedHandler.get();
	if (hasStarted)
	{
		// Wait for DBApps to inform us of their IDs and state. This will
		// prevent us from sending hashes prematurely until we have collected
		// some DBApps.
		dbAppAddStartWaitTime_ = TimeStamp( BW::timestamp() );

		// If we are recovering wait 2 seconds to collect all LoginApps and
		// DBApps
		gatherLoginAppsTimer_ = 
			mainDispatcher_.addOnceOffTimer( 2000000,
			this,
			reinterpret_cast< void * >( TIMEOUT_GATHER_LOGIN_APPS ),
			"gatherLoginAppsTimer" );

		shouldAcceptLoginApps_ = false;

		startupState_ = STARTUP_STATE_STARTED;
	}
	else
	{
		startupState_ = STARTUP_STATE_NOT_STARTED;
	}

	// Finish registration.

	reason =
		DBAppMgrInterface::registerWithMachined( interface_, 0 );

	if (reason != Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "DBAppMgr::init: "
				"Unable to register with interface. Is machined running?\n" );
		return false;
	}

	Mercury::MachineDaemon::registerBirthListener( this->interface().address(),
		DBAppMgrInterface::handleDBAppMgrBirth, "DBAppMgrInterface" );
	
	// Register dead app callback with machined
	Mercury::MachineDaemon::registerDeathListener( this->interface().address(),
		DBAppMgrInterface::handleDBAppDeath, "DBAppInterface" );

	Mercury::MachineDaemon::registerDeathListener( this->interface().address(),
		DBAppMgrInterface::handleLoginAppDeath, "LoginIntInterface" );

	// Register revived manager app callbacks with machined
	Mercury::MachineDaemon::registerBirthListener( this->interface().address(),
		DBAppMgrInterface::handleBaseAppMgrBirth, "BaseAppMgrInterface" );

	Mercury::MachineDaemon::registerBirthListener( this->interface().address(),
		DBAppMgrInterface::handleCellAppMgrBirth, "CellAppMgrInterface" );

	ReviverSubject::instance().init( &(this->interface()), "dbAppMgr" );

	tickTimer_ = mainDispatcher_.addTimer(
		1000000 / Config::updateHertz(),
		this, (void *)TIMEOUT_TICK,
		"TickTimer" );

	BW_REGISTER_WATCHER( 0, "dbappmgr", "dbAppMgr", mainDispatcher_,
		this->interface().address() );

	this->addWatchers();

	return true;
}


/**
 *	This method shut down the system in a controlled manner.
 */
void DBAppMgr::controlledShutDown(
		const DBAppMgrInterface::controlledShutDownArgs & args )
{
	DEBUG_MSG( "DBAppMgr::controlledShutDown: stage = %s\n", 
		ServerApp::shutDownStageToString( args.stage ) );

	switch (args.stage)
	{
	case SHUTDOWN_REQUEST:
	{
		// Make sure we no longer send to anonymous channels etc.
		interface_.stopPingingAnonymous();

		isShuttingDown_ = true;

		BaseAppMgrInterface::controlledShutDownArgs & args = 
			args.start( baseAppMgr_.bundle() );
		args.stage = SHUTDOWN_REQUEST;
		args.shutDownTime = 0;
		baseAppMgr_.send();

		break;
	}
	
	case SHUTDOWN_PERFORM:
	{
		INFO_MSG( "DBAppMgr::controlledShutDown: "
				"Telling %" PRIzu " DBApps to shut down\n",
			dbApps_.size() );
		for (DBApps::const_iterator iter = dbApps_.begin();
				iter != dbApps_.end();
				++iter)
		{
			iter->second->controlledShutDown( SHUTDOWN_PERFORM );
		}

		this->interface().processUntilChannelsEmpty();
		this->shutDown();
		break;
	}

	default:
		ERROR_MSG( "DBAppMgr::controlledShutDown: Stage %s not handled.\n",
			ServerApp::shutDownStageToString( args.stage ) );
		break;
	}
}


/**
 *	This method is called when a new DBAppMgr is started.
 */
void DBAppMgr::handleDBAppMgrBirth(
	const DBAppMgrInterface::handleDBAppMgrBirthArgs & args )
{
	if (args.addr != interface_.address())
	{
		PROC_IP_WARNING_MSG( "DBAppMgr::handleDBAppMgrBirth: %s\n",
			args.addr.c_str() );
		this->shutDown();
	}
}


/**
 *	This method is called when a LoginApp dies.
 */
void DBAppMgr::handleLoginAppDeath(
		const DBAppMgrInterface::handleLoginAppDeathArgs & args )
{
	loginApps_.erase( args.addr );
}


/** 
 *	This method is called when a BaseApp death dies.
 */
void DBAppMgr::handleBaseAppDeath( BinaryIStream & data )
{
	if (isShuttingDown_)
	{
		data.finish();
		return;
	}

	const int dataLength = data.remainingLength();
	const void * pData = data.retrieve( dataLength );

	// Forward to each DBApp so they can remap mailboxes.
	for (DBApps::const_iterator iter = dbApps_.begin();
			iter != dbApps_.end();
			++iter)
	{
		DBApp & dbApp = *(iter->second);
		
		Mercury::Bundle & bundle = dbApp.channelOwner().bundle();
		bundle.startMessage( DBAppInterface::handleBaseAppDeath );
		bundle.addBlob( pData, dataLength );
		dbApp.channelOwner().send();
	}
}


/**
 *	This method handles BaseAppMgr being revived.
 */
void DBAppMgr::handleBaseAppMgrBirth(
		const DBAppMgrInterface::handleBaseAppMgrBirthArgs & args )
{
	baseAppMgr_.addr( args.addr );
	this->sendDBAppHashUpdateToBaseAppMgr();
}


/**
 *	This method handles CellAppMgr being revived.
 */
void DBAppMgr::handleCellAppMgrBirth(
		const DBAppMgrInterface::handleCellAppMgrBirthArgs & args )
{
	cellAppMgr_.addr( args.addr );
	this->sendDBAppHashUpdateToCellAppMgr();
}

/**
 *  This method returns the DBApp for the given address, or NULL if none
 *  exists.
 */
DBApp * DBAppMgr::findApp( const Mercury::Address & addr )
{
	AddressMap::iterator iter = addressMap_.find( addr );
	return (iter != addressMap_.end()) ? iter->second.get() : NULL;
}


/**
 *	This method returns the DBApp Alpha, or NULL if there are no DBApps
 *	registered.
 */
DBApp * DBAppMgr::dbAppAlpha()
{
	if (dbApps_.empty())
	{
		return NULL;
	}

	return dbApps_.smallest().second.get();
}


namespace // (anonymous)
{

/**
 *	This class handles the reply to requestHasStarted request to BaseAppMgr.
 */
class HasStartedRequestHandler :
		public Mercury::ShutdownSafeReplyMessageHandler
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param dbAppMgr 	The DBAppMgr instance.
	 */
	HasStartedRequestHandler( DBAppMgr & dbAppMgr ) :
		dbAppMgr_( dbAppMgr )
	{}


	/* Override from Mercury::ShutdownSafeReplyMessageHandler */
	void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			void * arg ) /* override */
	{
		bool hasStarted = false;
		// TODO: Probably should change BaseAppMgr to send it as uint8 to be
		// explicit.
		data >> hasStarted;

		dbAppMgr_.onBaseAppMgrStartStatusReceived( hasStarted );
		delete this;
	}


	/* Override from Mercury::ShutdownSafeReplyMessageHandler */
	void handleException( const Mercury::NubException & exception,
			void * arg ) /* override */
	{
		ERROR_MSG( "HasStartedRequestHandler::handleException: "
				"Failed requesting start status from BaseAppMgr: %s\n",
			Mercury::reasonToString( exception.reason() ) );

		delete this;
	}

private:
	DBAppMgr & dbAppMgr_;
};

} // end namespace (anonymous)


/**
 *	This method is called when we receive notification from BaseAppMgr
 *	regarding the server startup status.
 */
void DBAppMgr::onBaseAppMgrStartStatusReceived( bool hasStarted )
{
	this->serverHasStarted( hasStarted );
}


/**
 *	This method handles an add message from a DBApp. It replies with the
 *	current DBApp hash with the new DBApp added. It may elect to wait some time
 *	to reply while it gathers up other DBApps that may have been added
 *	together, allowing for information for multiple DBApps to be aggregated
 *	into a single update.
 *
 *	@param srcAddr 	The message source, that is, the new DBApp address.
 *	@param header 	The message header.
 */
void DBAppMgr::addDBApp( const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header )
{
	const bool wasEmpty = dbApps_.empty();
	
	if (startupState_ == STARTUP_STATE_INDETERMINATE)
	{
		MF_ASSERT( !wasEmpty );
		// The Alpha DBApp has not finished initialization yet, other DBApps
		// should retry. 
		Mercury::Channel & channel = 
			this->getChannel< DBAppMgr >( srcAddr );
		channel.bundle().startReply( header.replyID );
		channel.send();
		return;
	}

	++lastDBAppID_;

	DBAppPtr pDBApp = new DBApp( interface_, srcAddr, lastDBAppID_,
		header.replyID );
	dbApps_.insert( std::make_pair( lastDBAppID_, pDBApp ) );
	addressMap_[ srcAddr ] = pDBApp;

	CONFIG_DEBUG_MSG( "DBAppMgr::addDBApp:\n"
			"\tAllocated id   = %u\n"
			"\tDBApps in use  = %" PRIzu "\n"
			"\tAddress        = %s\n"
			"\tAlpha          = %s\n",
		pDBApp->id(),
		dbApps_.size(),
		srcAddr.c_str(),
		wasEmpty ? "true" : "false" );

	if (wasEmpty)
	{
		// Tell interested processes about the first DBApp (which will be
		// DBApp Alpha) immediately.
		this->sendDBAppHashUpdate( /* haveNewAlpha */ true );
	}
	else
	{
		// We batch up the additions for non-alpha DBApps, and reply
		// ADD_WAIT_TIME_SECONDS after the first DBApp addition.
		dbAppAddStartWaitTime_ = TimeStamp( BW::timestamp() );
	}
}


/**
 *	This method handles a message from an already running DBApp to recover its
 *	state.
 */
void DBAppMgr::recoverDBApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const DBAppMgrInterface::recoverDBAppArgs & args )
{
	DBAppID id = args.id;
	if (id > lastDBAppID_)
	{
		lastDBAppID_ = id;
	}

	DBAppPtr pDBApp = new DBApp( interface_, srcAddr, id,
		Mercury::REPLY_ID_NONE );

	dbApps_.insert( std::make_pair( id, pDBApp ) );
	addressMap_[ srcAddr ] = pDBApp;
	
	DEBUG_MSG( "DBAppMgr::recoverDBApp: id = %d, addr = %s\n",
		pDBApp->id(), srcAddr.c_str() );
}


/**
 *  This method is called by machined to inform us of that a DBApp has
 *	died unexpectedly.
 *
 *	@param srcAddr 	The message source.
 *	@param header 	The message header.
 *	@param args 	The message arguments.
 */
void DBAppMgr::handleDBAppDeath( const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header,
	const DBAppMgrInterface::handleDBAppDeathArgs & args )
{
	this->handleDBAppDeath( args.addr );
}


/**
 *	This method handles a DBApp dying unexpectedly.
 *
 *	@param addr 	The address of the dead DBApp.
 */
void DBAppMgr::handleDBAppDeath( const Mercury::Address & addr )
{
	AddressMap::iterator iter = addressMap_.find( addr );

	if (iter == addressMap_.end())
	{
		NOTICE_MSG( "DBAppMgr::handleDBAppDeath: "
				"Informed about an unknown DBApp: %s\n",
			addr.c_str() );
		return;
	}

	const bool wasAlpha = (addr == this->dbAppAlpha()->address());

	DBAppPtr pDeadDBApp = iter->second;
	NETWORK_INFO_MSG( "DBAppMgr::handleDBAppDeath: %s DBApp%02d%s\n",
		pDeadDBApp->address().c_str(),
		iter->second->id(),
		wasAlpha ? " (alpha)" : "" );

	MF_VERIFY( dbApps_.erase( iter->second->id() ) );
	addressMap_.erase( iter );

	if (isShuttingDown_)
	{
		return;
	}

	if (wasAlpha && (startupState_ == STARTUP_STATE_INDETERMINATE))
	{
		// We were waiting on DBApp-Alpha to tell us that it had finished
		// starting the rest of the server. Instead it died before it could
		// tell us. Now we have to ask BaseAppMgr if it was actually done or
		// not. If not, then we need to retry with the next DBApp-Alpha.

		Mercury::Bundle & bundle = baseAppMgr_.bundle();
		bundle.startRequest( BaseAppMgrInterface::requestHasStarted,
			new HasStartedRequestHandler( *this ) );
		baseAppMgr_.send();

		return;
	}

	if (dbApps_.empty())
	{
		ERROR_MSG( "DBAppMgr::handleDBAppDeath: No DBApps left\n" );
		// TODO: Scalable DB: Should trigger a controlled shutdown here.
	}

	DBAppPtr pAlphaApp = this->dbAppAlpha();

	if (wasAlpha && pAlphaApp)
	{
		DEBUG_MSG( "DBAppMgr::handleDBAppDeath: new DBApp Alpha: %d (%s)\n",
			pAlphaApp->id(), pAlphaApp->address().c_str() );		
	}

	this->sendDBAppHashUpdate( /* haveNewAlpha */ wasAlpha );
}


/**
 *	This method sends out the current DB App hash state to all interested
 *	processes.
 *
 *	@param haveNewAlpha 	Whether there is a new DBApp Alpha since the last
 *							update.
 */
void DBAppMgr::sendDBAppHashUpdate( bool haveNewAlpha )
{
	// Inform BaseAppMgr.
	this->sendDBAppHashUpdateToBaseAppMgr();

	// Inform each DBApp.
	for (DBApps::const_iterator iter = dbApps_.begin();
			iter != dbApps_.end();
			++iter)
	{
		DBAppPtr pDBApp = iter->second;

		const bool shouldAlphaResetGameServerState = 
			(iter == dbApps_.begin()) &&
				(startupState_ == STARTUP_STATE_NOT_STARTED);

		pDBApp->updateDBAppHash( dbApps_, shouldAlphaResetGameServerState );

		if (shouldAlphaResetGameServerState)
		{
			DEBUG_MSG( "DBAppMgr::sendDBAppHashUpdate: "
					"Directing DBApp-Alpha %d (%s) to perform reset of game "
					"server state\n",
				pDBApp->id(),
				pDBApp->address().c_str() );

			// If we told a new DBApp-Alpha to start the server, we need to
			// wait for notification that this succeeded before trying again.
			startupState_ = STARTUP_STATE_INDETERMINATE;
		}
	}

	if (haveNewAlpha)
	{
		const Mercury::Address newAlpha = (!dbApps_.empty() ?
				this->dbAppAlpha()->address() :
				Mercury::Address::NONE);
	
		// Inform CellAppMgr.
		this->sendDBAppHashUpdateToCellAppMgr();

		// Inform LoginApps.
		for (LoginApps::iterator iter = loginApps_.begin();
				iter != loginApps_.end();
				++iter)
		{
			Mercury::Channel & channel = 
				this->getChannel< DBAppMgr >( *iter );

			LoginIntInterface::notifyDBAppAlphaArgs & args = 
				args.start( channel.bundle() );

			args.addr = newAlpha;


			channel.send();
		}
	}

	dbAppAddStartWaitTime_ = TimeStamp( 0 );
}


/**
 *	This method sends the DBApp hash update to BaseAppMgr.
 */
void DBAppMgr::sendDBAppHashUpdateToBaseAppMgr()
{
	baseAppMgr_.bundle().startMessage( BaseAppMgrInterface::updateDBAppHash );
	baseAppMgr_.bundle() << dbApps_;
	baseAppMgr_.send();

}


/**
 *	This method sends the DBApp hash update to CellAppMgr.
 */
void DBAppMgr::sendDBAppHashUpdateToCellAppMgr()
{
	const Mercury::Address alphaAddress = (!dbApps_.empty() ?
			this->dbAppAlpha()->address() :
			Mercury::Address::NONE);

	CellAppMgrInterface::setDBAppAlphaArgs & args =
		args.start( cellAppMgr_.bundle() );
	args.addr = alphaAddress;
	cellAppMgr_.send();
}


/**
 *	This method handles the registration of a new LoginApp.
 *
 *	@param srcAddr 	The message source, that is, the new LoginApp address.
 *	@param header 	The message header.
 *	@param data 	The message data.
 */
void DBAppMgr::addLoginApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & /* data */ )
{
	Mercury::ChannelSender sender( this->getChannel< DBAppMgr >( srcAddr ) );
	Mercury::Bundle & bundle = sender.bundle();

	if (header.replyID == Mercury::REPLY_ID_NONE)
	{
		NOTICE_MSG( "DBAppMgr::addLoginApp: "
				"Got non-request message from %s, ignoring\n",
			srcAddr.c_str() );
		return;
	}

	bundle.startReply( header.replyID );

	if (!shouldAcceptLoginApps_ || dbApps_.empty())
	{
		// Zero length replies mean to try again later
		return;
	}

	loginApps_.insert( srcAddr );

	Mercury::Address dbAppAlphaAddress = !dbApps_.empty() ? 
		this->dbAppAlpha()->address() : Mercury::Address::NONE;

	bundle << ++lastLoginAppID_ << dbAppAlphaAddress;
}


/**
 *	This method is called by each LoginApp when we are recovering to rebuild
 *	the LoginApp set.
 *
 *	@param srcAddr 	The message source.
 *	@param header 	The message header.
 *	@param data 	The message data.
 */
void DBAppMgr::recoverLoginApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const DBAppMgrInterface::recoverLoginAppArgs & args )
{
	loginApps_.insert( srcAddr );

	if (args.id > lastLoginAppID_)
	{
		lastLoginAppID_ = args.id;
	}
}


/**
 *	This method is used handle when we get notification about whether the rest
 *	of the server has started running.
 */
void DBAppMgr::serverHasStarted( bool hasStarted /* = true */ )
{
	// The good case is when we are called as a result of DBApp-Alpha
	// succeeding in starting the rest of the server.

	// We can also get called as a result of BaseAppMgr if DBApp-Alpha dies
	// while we are waiting for it to start the rest of the server, in which
	// case we ask BaseAppMgr whether it did succeed or not.
	// Apart from startup, when we ask BaseAppMgr synchronously, we only send
	// requestHasStarted (asynchronously) after a DBApp-Alpha has registered
	// and dies while doing the first-time initialisation.

	MF_ASSERT( startupState_ == STARTUP_STATE_INDETERMINATE );

	startupState_ = (hasStarted ?
		STARTUP_STATE_STARTED : STARTUP_STATE_NOT_STARTED);
	
	DEBUG_MSG( "DBAppMgr::serverHasStarted: %s\n",
		hasStarted ? "true" : "false" );

	if (!dbApps_.empty())
	{
		// These have come in since our dead DBApp-Alpha first registered,
		// one of these will be the new Alpha, and can retry if !hasStarted.
		this->sendDBAppHashUpdate( /* haveNewAlpha */ true );

		// If we had a DBApp to become the new Alpha, then it will be told to
		// retry first-time initialisation.
		MF_ASSERT( (startupState_ == STARTUP_STATE_STARTED) || 
			(startupState_ == STARTUP_STATE_INDETERMINATE) );
	}
//	else if (startupState_ == STARTUP_STATE_NOT_STARTED)
// 	{
		// Otherwise, we just have to wait for a new DBApp Alpha to register
		// that we can direct to retry starting up the rest of the server. This
		// assumes that the extra initialisation for first-time startup is
		// idempotent, which is not really true, but it should be a small
		// enough window hopefully.
		// TODO: Scalable DB: This may be a problem, if e.g. auto-loads are
		// half-done when DBApp-Alpha dies. The next DBApp-Alpha will start the
		// auto-load again, resulting in duplicate entities. Hopefully this is
		// a small window, but we might want to fix this up in the future.
//	}
}


/**
 *	This method adds the watchers that are related to this object.
 */
void DBAppMgr::addWatchers()
{
	Watcher & root = Watcher::rootWatcher();

	this->ServerApp::addWatchers( root );

	MF_WATCH( "numDBApps", *this, &DBAppMgr::numDBApps );

	Watcher * dbAppsWatcher = new MapWatcher< DBApps >();
	dbAppsWatcher->addChild( "*", new SmartPointerDereferenceWatcher(
		DBApp::pWatcher() ) );

	root.addChild( "dbApps", dbAppsWatcher, &dbApps_ );
	
	root.addChild( "alphaAppID", makeWatcher( &DBAppMgr::alphaID ),
		this );
}


/*
 *	Override from TimerHandler.
 */
void DBAppMgr::handleTimeout( TimerHandle handle, void * arg ) /* override */
{
	switch (uintptr( arg ))
	{
	case TIMEOUT_TICK:
		this->advanceTime();
		break;

	case TIMEOUT_GATHER_LOGIN_APPS:
		gatherLoginAppsTimer_.clearWithoutCancel();
		shouldAcceptLoginApps_ = true;
		break;
	}
}


/*
 * 	Override from ManagerApp.
 */
void DBAppMgr::onSignalled( int sigNum ) /* override */
{
	this->ServerApp::onSignalled( sigNum );

	if (sigNum == SIGUSR1)
	{
		this->shutDown();
	}
}


/*
 *	Override from ManagerApp.
 */
void DBAppMgr::onStartOfTick()
{
	if ((dbAppAddStartWaitTime_ != 0) &&
			(dbAppAddStartWaitTime_.ageInSeconds() >= ADD_WAIT_TIME_SECONDS))
	{
		this->sendDBAppHashUpdate( /* haveNewAlpha */ false );
	}
}


BW_END_NAMESPACE

// dbappmgr.cpp
