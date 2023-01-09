#include "baseappmgr.hpp"

#ifndef CODE_INLINE
#include "baseappmgr.ipp"
#endif

#include "baseapp.hpp"
#include "baseappmgr_config.hpp"
#include "login_conditions_config.hpp"
#include "reply_handlers.hpp"
#include "watcher_forwarding_baseapp.hpp"

#include "baseapp/address_load_pair.hpp"
#include "baseapp/baseapp_int_interface.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "loginapp/login_int_interface.hpp"

#include "db/dbapp_interface.hpp"

// #include "common/doc_watcher.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/timestamp.hpp"

#include "server/shared_data_type.hpp"

#include "network/channel_sender.hpp"
#include "network/event_dispatcher.hpp"
#include "network/machine_guard.hpp"
#include "network/machined_utils.hpp"
#include "network/nub_exception.hpp"
#include "network/portmap.hpp"
#include "network/udp_bundle.hpp"
#include "network/watcher_nub.hpp"

#include "server/backup_hash_chain.hpp"
#include "server/bwconfig.hpp"
#include "server/reviver_subject.hpp"
#include "server/stream_helper.hpp"
#include "server/time_keeper.hpp"

#include <limits>

DECLARE_DEBUG_COMPONENT( 0 )

namespace {

/**
 * This static method sanitises pickled representation of a global base key to
 * be used in diagnostic messages.
 */
BW::string globalBaseKeyToString( const BW::StringRef & pickledKey )
{
	static const BW::string::size_type MAX_LEN = 100;
	if (pickledKey.size() <= MAX_LEN)
	{
		return BW::sanitiseNonPrintableChars( pickledKey );
	}
	else
	{
		return BW::sanitiseNonPrintableChars(
			BW::StringRef( pickledKey.data(), MAX_LEN ) );
	}
}


} // anonymous namespace

BW_BEGIN_NAMESPACE


/**
 *	Constructor. Initialises the iterator to the beginning of the container.
 *	@param apps	Container to iterate over.
 *	@param pred	Optional predicate to iterate only over BaseApp's matching
 *				the predicate.
 */
BaseAppsIterator::BaseAppsIterator( const BaseApps & apps,
									const Predicate & pred )
	: iter_( apps.begin() ), end_( apps.end() ), predicate_( pred )
{
}


const BW::BaseAppPtr BaseAppsIterator::endOfContainer_;

/**
 *	This method returns the next element from the collection and advances to
 *	next element.
 *	@return	Next container element or @a BaseAppPtr().
 */
const BaseAppPtr & BaseAppsIterator::next()
{
	while (iter_ != end_)
	{
		const BaseAppPtr & pAppPtr = (iter_++)->second;
		if (!predicate_ || predicate_( *pAppPtr ))
		{
			return pAppPtr;
		}
	}
	return endOfContainer_;
}


/**
 *	Constructor.
 * @param apps			Collection of all managed BaseApp's.
 * @param cellAppMgr	@a CellAppMgr reference used to notify CellApp's about
 * 						base entity events.
 */
ManagedAppSubSet::ManagedAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr )
	: apps_( apps ), cellAppMgr_( cellAppMgr ), lastAppID_( 0 ),
	  size_( 0 ), hasIdealBackups_( false )
{
	MF_ASSERT( apps.size() == 0 && "Should start with an empty peer set" );
}


/**
 * This method counts the number of apps in the subset. Used for debugging only.
 */
uint ManagedAppSubSet::countApps() const
{
	BaseAppsIterator iter = this->iterator();
	uint result = 0;

	while (iter.next())
	{
		++result;
	}
	return result;
}


/**
 *	Constructor.
 *	@param apps			Collection of all managed BaseApp's.
 *	@param cellAppMgr	@a CellAppMgr reference used to notify CellApp's about
 * 						base entity events.
 */
BaseAppSubSet::BaseAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr )
	: ManagedAppSubSet( apps, cellAppMgr ), bestBaseAppAddr_( 0, 0 )
{
}


/**
 *	This method returns an iterator for enumerating "real" BaseApps (no
 *	ServiceApps).
 */
BaseAppsIterator BaseAppSubSet::iterator() const
{
	using namespace std::placeholders;
	return BaseAppsIterator( apps_,
		// returns !_1.isSrviceApp()
		std::bind( std::logical_not<bool>(),
						std::bind( &BaseApp::isServiceApp, _1 ) ) );
}


/**
 * Returns @a true if the server is configured to shut down in case a BaseApp
 * dies.
 */
bool BaseAppSubSet::shutDownOnAppDeath() const
{
	return Config::shutDownServerOnBaseAppDeath();
}


/**
 *	Constructor.
 *	@param apps			Collection of all managed BaseApp's.
 *	@param cellAppMgr	@a CellAppMgr reference used to notify CellApp's about
 * 						base entity events.
 */
ServiceAppSubSet::ServiceAppSubSet( BaseApps & apps, CellAppMgr & cellAppMgr )
	: ManagedAppSubSet( apps, cellAppMgr )
{
}


/**
 *	This method returns an iterator for enumerating ServiceApps.
 */
BaseAppsIterator ServiceAppSubSet::iterator() const
{
	using namespace std::placeholders;
	return BaseAppsIterator( apps_, std::bind(&BaseApp::isServiceApp, _1) );
}


/**
 *	This method serialises all service fragments int the provided bundle.
 *	@param bundle Bundle to put service fragments into.
 */
void ServiceAppSubSet::putServiceFragmentsInto( Mercury::Bundle & bundle )
{
	ServicesMap::const_iterator iFragment = services_.begin();

	while (iFragment != services_.end())
	{
		bundle.startMessage( BaseAppIntInterface::addServiceFragment );
		bundle << iFragment->first << iFragment->second;

		++iFragment;
	}
}


/**
 *	Returns @a true if the server is configured to shut down in case a
 *	ServiceApp dies.
 */
bool ServiceAppSubSet::shutDownOnAppDeath() const
{
	return Config::shutDownServerOnServiceAppDeath();
}


/**
*	This method returns the minimum number of required ServiceApps based on
*	@a badStateOnLastServiceAppDeath config option.
*/
uint ServiceAppSubSet::minimumRequiredApps() const
{
	return Config::isBadStateWithNoServiceApps() ? 1 : 0;
}


/**
 *	This method does nothing -- ServiceApps should never be considered for
 *	BigWorld.createBaseAnywhere.
 */
void ServiceAppSubSet::updateCreateBaseInfo()
{
}


/// BaseAppMgr Singleton.
BW_SINGLETON_STORAGE( BaseAppMgr )

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BaseAppMgr::BaseAppMgr( Mercury::EventDispatcher & mainDispatcher,
		Mercury::NetworkInterface & interface ) :
	ManagerApp( mainDispatcher, interface ),
	cellAppMgr_( interface ),
	dbAppAlpha_( interface ),
	dbApps_(),
	pendingApps_(),
	baseAndServiceApps_(),
	baseApps_( const_cast<BaseApps &>( baseAndServiceApps_ ), cellAppMgr_ ),
	serviceApps_( const_cast<BaseApps &>( baseAndServiceApps_ ), cellAppMgr_ ),
	pBackupHashChain_( new BackupHashChain() ),
	sharedBaseAppData_(),
	sharedGlobalData_(),
	globalBases_(),
	pTimeKeeper_( NULL ),
	syncTimePeriod_( 0 ),
	isRecovery_( false ),
	hasInitData_( false ),
	hasStarted_( false ),
	shouldShutDownOthers_( false ),
	deadBaseAppAddr_( Mercury::Address::NONE ),
	archiveCompleteMsgCounter_( 0 ),
	shutDownTime_( 0 ),
	shutDownStage_( SHUTDOWN_NONE ),
	baseAppOverloadStartTime_( 0 ),
	loginsSinceOverload_( 0 ),
	gameTimer_()
{
	cellAppMgr_.channel().isLocalRegular( false );
	cellAppMgr_.channel().isRemoteRegular( false );

	dbAppAlpha_.channel().isLocalRegular( false );
	dbAppAlpha_.channel().isRemoteRegular( false );

	PROC_IP_INFO_MSG( "Internal address = %s\n", interface_.address().c_str() );
}


/**
 *	Destructor.
 */
BaseAppMgr::~BaseAppMgr()
{
	gameTimer_.cancel();

	if (shouldShutDownOthers_)
	{
		BaseAppIntInterface::shutDownArgs	baseAppShutDownArgs = { false };

		{
			BaseApps::const_iterator iter = baseAndServiceApps_.begin();
			while (iter != baseAndServiceApps_.end())
			{
				iter->second->bundle() << baseAppShutDownArgs;
				iter->second->send();

				++iter;
			}
		}

		{
			BaseApps::const_iterator iter = pendingApps_.begin();
			while (iter != pendingApps_.end())
			{
				iter->second->bundle() << baseAppShutDownArgs;
				iter->second->send();

				++iter;
			}
		}

		if (cellAppMgr_.channel().isEstablished())
		{
			Mercury::Bundle	& bundle = cellAppMgr_.bundle();
			CellAppMgrInterface::shutDownArgs cellAppmgrShutDownArgs = 
				{ false };
			bundle << cellAppmgrShutDownArgs;
			cellAppMgr_.send();
		}
	}

	// Make sure channels shut down cleanly
	interface_.processUntilChannelsEmpty();

	if (pTimeKeeper_)
	{
		delete pTimeKeeper_;
		pTimeKeeper_ = NULL;
	}
}


/**
 *	This method initialises this object.
 *
 *	@return True on success, false otherwise.
 */
bool BaseAppMgr::init( int argc, char * argv[] )
{
	if (!this->ManagerApp::init( argc, argv ))
	{
		return false;
	}

	if (!interface_.isGood())
	{
		NETWORK_ERROR_MSG( "Failed to open internal interface.\n" );
		return false;
	}

	ReviverSubject::instance().init( &interface_, "baseAppMgr" );

	for (int i = 0; i < argc; ++i)
	{
		if (strcmp( argv[i], "-recover" ) == 0)
		{
			isRecovery_ = true;
			break;
		}
	}

	CONFIG_INFO_MSG( "isRecovery = %s\n", isRecovery_ ? "True" : "False" );

	// register dead app callback with machined
	Mercury::MachineDaemon::registerDeathListener( interface_.address(),
				BaseAppMgrInterface::handleBaseAppDeath,
				"BaseAppIntInterface" );

	Mercury::MachineDaemon::registerDeathListener( interface_.address(),
				BaseAppMgrInterface::handleBaseAppDeath,
				"ServiceAppInterface" );

	int numStartupRetries = Config::numStartupRetries();

	BaseAppMgrInterface::registerWithInterface( interface_ );

	Mercury::Reason reason = 
		BaseAppMgrInterface::registerWithMachined( interface_, 0 );

	if (reason != Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "BaseAppMgr::init: Unable to register. "
				"Is machined running?\n" );
		return false;
	}

	{
		Mercury::MachineDaemon::registerBirthListener( interface_.address(),
			BaseAppMgrInterface::handleCellAppMgrBirth, "CellAppMgrInterface" );

		Mercury::Address cellAppMgrAddr;
		reason = Mercury::MachineDaemon::findInterface(
				"CellAppMgrInterface", 0, cellAppMgrAddr, numStartupRetries );

		if (reason == Mercury::REASON_SUCCESS)
		{
			cellAppMgr_.addr( cellAppMgrAddr );
		}
		else if (reason == Mercury::REASON_TIMER_EXPIRED)
		{
			NETWORK_INFO_MSG( "BaseAppMgr::init: CellAppMgr not ready yet.\n" );
		}
		else
		{
			NETWORK_ERROR_MSG( "BaseAppMgr::init: "
				"Failed to find CellAppMgr interface: %s\n",
				Mercury::reasonToString( (Mercury::Reason &)reason ) );

			return false;
		}

		Mercury::MachineDaemon::registerBirthListener( interface_.address(),
			BaseAppMgrInterface::handleBaseAppMgrBirth,
			"BaseAppMgrInterface" );
	}

	BW_REGISTER_WATCHER( 0, "baseappmgr", "baseAppMgr",
			mainDispatcher_, interface_.address() );

	this->addWatchers();

	return true;
}


// -----------------------------------------------------------------------------
// Section: Helpers
// -----------------------------------------------------------------------------


/**
 *	This method returns a subset instance reference to which the given app
 *	belongs.
 */
inline ManagedAppSubSet & BaseAppMgr::getSubSet( const BaseApp & app )
{
	return this->getSubSet( app.isServiceApp() );
}


/**
 *  This method returns the BaseApp for the given address, or NULL if none
 *  exists.
 */
BaseApp * BaseAppMgr::findBaseApp( const Mercury::Address & addr )
{
	BaseApps::const_iterator it = baseAndServiceApps_.find( addr );
	if (it != baseAndServiceApps_.end())
	{
		return it->second.get();
	}
	else
	{
		return NULL;
	}
}


/**
 *	This method finds the least loaded BaseApp.
 *
 *	@return The least loaded BaseApp. If none exists, NULL is returned.
 */
BaseApp * ManagedAppSubSet::findLeastLoadedApp() const
{
	BaseApp * pBest = NULL;

	float lowestLoad = 0.f;
	BaseAppsIterator iter = this->iterator();

	while (BaseApp * pCurr = iter.next().get())
	{
		const float currLoad = pCurr->load();

		if (!pCurr->isRetiring())
		{
			if (!pBest || (currLoad < lowestLoad))
			{
				lowestLoad = currLoad;
				pBest = pCurr;
			}
		}
	}

	return pBest;
}


/**
 *	This method is called when a BaseApp wants to retire.
 */
void BaseAppMgr::onBaseAppRetire( BaseApp & baseApp )
{
	if (baseApps_.bestBaseApp() == baseApp.addr())
	{
		baseApps_.updateBestBaseApp();
	}

	this->getSubSet( baseApp ).adjustBackupLocations( baseApp,
		ADJUST_BACKUP_LOCATIONS_OP_RETIRE );
}


/**
 *	This method updates the best BaseApp for creating bases through
 *	createBaseAnywhere calls. 
 */
void BaseAppSubSet::updateBestBaseApp()
{
	BaseApp * pBest = this->findLeastLoadedApp();

	// TODO:BAR Should consider the possibility that the last app is 
	// now being retired, which will result in pBest == NULL. Maybe
	// retiring the last BaseApp should be a special case where we 
	// trigger a controlled shutdown of the whole server.

	if ((pBest != NULL) &&
			(bestBaseAppAddr_ != pBest->addr()) &&
			cellAppMgr_.channel().isEstablished())
	{
		bestBaseAppAddr_ = pBest->addr();
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		CellAppMgrInterface::setBaseAppArgs & rSetBaseApp =
			CellAppMgrInterface::setBaseAppArgs::start( bundle );

		rSetBaseApp.addr = bestBaseAppAddr_;

		cellAppMgr_.send();
	}
}


/**
 *	This method returns the approximate number of bases on the server.
 */
int BaseAppMgr::numBases() const
{
	int count = 0;

	BaseApps::const_iterator iter = baseAndServiceApps_.begin();

	while (iter != baseAndServiceApps_.end())
	{
		count += iter->second->numBases();

		iter++;
	}

	return count;
}


/**
 *	This method returns the approximate number of proxies on the server.
 */
int BaseAppMgr::numProxies() const
{
	int count = 0;

	BaseApps::const_iterator iter = baseAndServiceApps_.begin();

	while (iter != baseAndServiceApps_.end())
	{
		count += iter->second->numProxies();

		iter++;
	}

	return count;
}


/**
 *	This method returns the minimum Base App load.
 */
float ManagedAppSubSet::minAppLoad() const
{
	float load = 2.f;

	BaseAppsIterator iter = this->iterator();

	while (const BaseAppPtr & pCurr = iter.next())
	{
		load = std::min( load, pCurr->load() );
	}

	return load;
}


/**
 *	This method returns the average Base App load.
 */
float ManagedAppSubSet::avgAppLoad() const
{
	float load = 0.f;

	BaseAppsIterator iter = this->iterator();

	while (const BaseAppPtr & pCurr = iter.next())
	{
		load += pCurr->load();
	}

	return this->size() == 0 ? 0.f : load / this->size();
}


/**
 *	This method returns the maximum Base App load.
 */
float ManagedAppSubSet::maxAppLoad() const
{
	float load = 0.f;

	BaseAppsIterator iter = this->iterator();

	while (const BaseAppPtr & pCurr = iter.next())
	{
		load = std::max( load, pCurr->load() );
	}

	return load;
}


/**
 *	This method returns an ID for a new BaseApp.
 */
BaseAppID ManagedAppSubSet::getNextAppID()
{
	lastAppID_ = (lastAppID_ + 1) & 0x0FFFFFFF; 	// arbitrary limit
	return lastAppID_;
}


namespace
{

/**
 *  This function sends a Mercury message to all known baseapps.  The message
 *  payload is taken from the provided MemoryOStream.  If pExclude is non-NULL,
 *  nothing will be sent to that app.  If pReplyHandler is non-NULL, we start a
 *  request instead of starting a regular message.
 */
void sendToBaseApps( const Mercury::InterfaceElement & ifElt,
	MemoryOStream & args, BaseAppsIterator iter,
	Mercury::ReplyMessageHandler * pHandler = NULL )
{
	while (const BaseAppPtr & pBaseApp = iter.next())
	{
		// Stream message onto bundle and send
		Mercury::Bundle & bundle = pBaseApp->bundle();

		if (!pHandler)
			bundle.startMessage( ifElt );
		else
			bundle.startRequest( ifElt, pHandler );

		// Note: This does not stream off from "args". This is so that we can
		// read the same data multiple times.
		bundle.addBlob( args.data(), args.size() );

		pBaseApp->send();
	}

	args.finish();
}

/**
 *	This class is a functor to be used as a predicate for @ref BaseAppsIterator.
 *	The predicate excludes the given app.
 */
class BaseAppsExcluding : public std::unary_function<const BaseApp &, bool>
{
public:
	BaseAppsExcluding( const BaseApp * exclude ) : exclude_( exclude ) { }
	bool operator()( const BaseApp & app ) const
	{
		return &app != exclude_;
	}
private:
	const BaseApp * const exclude_;
};

}

/**
 *	This method adds the watchers that are related to this object.
 */
void BaseAppMgr::addWatchers()
{
	Watcher * pRoot = &Watcher::rootWatcher();

	this->ServerApp::addWatchers( *pRoot );

	// number of local proxies
	MF_WATCH( "numBaseApps", *this, &BaseAppMgr::numBaseApps );
	MF_WATCH( "numServiceApps", *this, &BaseAppMgr::numServiceApps );

	MF_WATCH( "numBases", *this, &BaseAppMgr::numBases );
	MF_WATCH( "numProxies", *this, &BaseAppMgr::numProxies );

	MF_WATCH( "config/shouldShutDownOthers", shouldShutDownOthers_ );

	MF_WATCH( "baseAppLoad/min", baseApps_, &BaseAppSubSet::minAppLoad );
	MF_WATCH( "baseAppLoad/average", baseApps_, &BaseAppSubSet::avgAppLoad );
	MF_WATCH( "baseAppLoad/max", baseApps_, &BaseAppSubSet::maxAppLoad );
	
	MF_WATCH( "serviceAppLoad/min", serviceApps_, &ServiceAppSubSet::minAppLoad );
	MF_WATCH( "serviceAppLoad/average", serviceApps_, &ServiceAppSubSet::avgAppLoad );
	MF_WATCH( "serviceAppLoad/max", serviceApps_, &ServiceAppSubSet::maxAppLoad );

	WatcherPtr pBaseAppWatcher = BaseApp::pWatcher();

	// map of these for locals
	pRoot->addChild( "baseApps", new MapWatcher<BaseApps>(
						// MapWatcher doesn't modify it's container, but does
						// not handle constness correctly.
						const_cast<BaseApps &>( baseAndServiceApps_ ) ) );
	pRoot->addChild( "baseApps/*",
			new BaseDereferenceWatcher( pBaseAppWatcher ) );

	// other misc stuff
	MF_WATCH( "lastBaseAppIDAllocated", baseApps_, &BaseAppSubSet::lastAppID );
	MF_WATCH( "lastServiceAppIDAllocated", serviceApps_, &ServiceAppSubSet::lastAppID );

	pRoot->addChild( "cellAppMgr", Mercury::ChannelOwner::pWatcher(),
		&cellAppMgr_ );

	pRoot->addChild( "forwardTo", new BAForwardingWatcher() );

	pRoot->addChild( "dbApps", DBAppsGateway::pWatcher(), &dbApps_ );
}


/**
 *	This method calculates whether the BaseAppMgr is currently overloaded.
 */
bool BaseAppMgr::calculateOverloaded( bool areBaseAppsOverloaded )
{
	if (areBaseAppsOverloaded)
	{
		uint64 overloadTime;

		// Start rate limiting logins
		if (baseAppOverloadStartTime_ == 0)
		{
			baseAppOverloadStartTime_ = timestamp();
		}

		overloadTime = timestamp() - baseAppOverloadStartTime_;
		INFO_MSG( "BaseAppMgr::Overloaded for %" PRIu64 "ms\n",
			overloadTime/(stampsPerSecond()/1000) );

		if ((overloadTime >
			LoginConditionsConfig::minOverloadTolerancePeriodInStamps()) ||
			(loginsSinceOverload_ >= LoginConditionsConfig::overloadLogins()))
		{
			return true;
		}
		else
		{
			// If we're not overloaded
			loginsSinceOverload_++;

			INFO_MSG( "BaseAppMgr::Logins since overloaded " \
					"(allowing max of %d): %d\n",
					LoginConditionsConfig::overloadLogins(),
					loginsSinceOverload_ );
		}
	}
	else
	{
		// Not overloaded, clear the timer
		baseAppOverloadStartTime_ = 0;
		loginsSinceOverload_ = 0;
	}

	return false;
}


/**
 *	This method overrides the TimerHandler method to handle timer events.
 */
void BaseAppMgr::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	// Are we paused for shutdown?
	if ((shutDownTime_ != 0) && (shutDownTime_ == time_))
		return;

	switch (reinterpret_cast<uintptr>( arg ))
	{
		case TIMEOUT_GAME_TICK:
		{
			this->advanceTime();

			if (time_ % Config::timeSyncPeriodInTicks() == 0)
			{
				pTimeKeeper_->synchroniseWithMaster();
			}

			this->checkForDeadBaseApps();

			if (time_ % Config::updateCreateBaseInfoPeriodInTicks() == 0)
			{
				baseApps_.updateCreateBaseInfo();
			}

			// TODO: Don't really need to do this each tick.
			baseApps_.updateBestBaseApp();
		}
		break;
	}
}


/**
 *	This method is called periodically to check whether or not any base
 *	applications have timed out.
 */
void BaseAppMgr::checkForDeadBaseApps()
{
	uint64 currTime = timestamp();
	uint64 lastHeardTime = 0;
	BaseApps::const_iterator iter = baseAndServiceApps_.begin();

	while (iter != baseAndServiceApps_.end())
	{
		lastHeardTime = std::max( lastHeardTime,
				iter->second->channel().lastReceivedTime() );
		++iter;
	}

	const uint64 timeSinceAnyHeard = currTime - lastHeardTime;


	// The logic behind the following block of code is that if we
	// haven't heard from any baseapp in a long time, the baseappmgr is
	// probably the misbehaving app and we shouldn't start forgetting
	// about baseapps.  If we want to shutdown our server on bad state,
	// we want to be able to return true when our last baseapp dies, so
	// relax the following check.
	if (timeSinceAnyHeard > Config::baseAppTimeoutInStamps()/2)
	{
		INFO_MSG( "BaseAppMgr::checkForDeadBaseApps "
			"Last inform time not recent enough %f\n",
			double((int64)timeSinceAnyHeard)/stampsPerSecondD() );
		return;
	}

	iter = baseAndServiceApps_.begin();

	while (iter != baseAndServiceApps_.end())
	{
		BaseApp * pApp = iter->second.get();
		const ManagedAppSubSet & peerApps = this->getSubSet( *pApp );

		if (pApp->hasTimedOut( currTime, Config::baseAppTimeoutInStamps() ) &&
			(!BaseAppMgrConfig::shutDownServerOnBadState() ||
				(peerApps.size() > peerApps.minimumRequiredApps())) &&
			!Mercury::UDPChannel::allowInteractiveDebugging())
		{
			// handleBaseAppDeath will destroy the BaseApp, and continue to use
			// the addr arg.
			Mercury::Address addr = pApp->addr();
			this->handleBaseAppDeath( addr );

			// Only handle one timeout per check because the above call will
			// likely change the collection we are iterating over.
			return;
		}

		iter++;
	}
}


/**
 *	This method handles a message from a Base App that informs us on its current
 *	load.
 */
void BaseAppMgr::informOfLoad( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppMgrInterface::informOfLoadArgs & args )
{
	if (BaseApp * baseApp = this->findBaseApp( addr ))
	{
		baseApp->updateLoad( args.load, args.numBases, args.numProxies );
	}
	else
	{
		NETWORK_ERROR_MSG( "BaseAppMgr::informOfLoad: "
					"No BaseApp with address %s\n",
				addr.c_str() );
	}
}


// -----------------------------------------------------------------------------
// Section: Handler methods
// -----------------------------------------------------------------------------

/**
 *	This method handles the createEntity message. It is called by DBApp when
 *	logging in.
 */
void BaseAppMgr::createEntity( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	Mercury::Address baseAppAddr( 0, 0 );

	BaseApp * pBest = baseApps_.findLeastLoadedApp();

	if (pBest == NULL)
	{
		ERROR_MSG( "BaseAppMgr::createEntity: Could not find a BaseApp.\n");
		baseAppAddr.port =
				BaseAppMgrInterface::CREATE_ENTITY_ERROR_NO_BASEAPPS;

		Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startReply( header.replyID );
		bundle << baseAppAddr;
		bundle << "No BaseApp could be found to add to.";

		return;
	}

	bool areBaseAppsOverloaded = (pBest->load() >
								LoginConditionsConfig::minLoad());

	if (this->calculateOverloaded( areBaseAppsOverloaded ))
	{
		INFO_MSG( "BaseAppMgr::createEntity: All baseapps overloaded "
				"(best load=%.02f > overload level=%.02f.\n",
			pBest->load(), LoginConditionsConfig::minLoad() );
		baseAppAddr.port =
			BaseAppMgrInterface::CREATE_ENTITY_ERROR_BASEAPPS_OVERLOADED;

		Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startReply( header.replyID );
		bundle << baseAppAddr;
		bundle << "All BaseApps overloaded.";

		return;
	}

	// Copy the client endpoint address
	baseAppAddr = pBest->externalAddr();

	CreateBaseReplyHandler * pHandler =
		new CreateBaseReplyHandler( srcAddr, header.replyID,
			baseAppAddr );

	// Tell the BaseApp about the client's new proxy
	Mercury::Bundle	& bundle = pBest->bundle();
	bundle.startRequest( BaseAppIntInterface::createBaseWithCellData,
			pHandler );

	bundle.transfer( data, data.remainingLength() );
	pBest->send();

	// Update the load estimate.
	pBest->addEntity();
}


/**
 *	This method handles an add message from a BaseApp. It returns the new ID
 *	that the BaseApp has.
 */
void BaseAppMgr::add( const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header,
	const BaseAppMgrInterface::addArgs & args )
{
	const Mercury::ReplyID replyID = header.replyID;

	MF_ASSERT( srcAddr == args.addrForCells );

	// If we're not allowing BaseApps to connect at the moment, just send back a
	// zero-length reply.
	if (!cellAppMgr_.channel().isEstablished() || !hasInitData_ || 
			dbApps_.empty())
	{
		PROC_IP_INFO_MSG( "BaseAppMgr::add: "
				"Not allowing BaseApp at %s to register yet (%s)\n",
			srcAddr.c_str(),
			!cellAppMgr_.channel().isEstablished() ? 
				"CellAppMgr has not started yet" :
				!hasInitData_ ? "No init data from DBApp Alpha" : 
					"DBApp hash has not been initialised (or is empty)" );

		Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr ) );
		sender.bundle().startReply( replyID );

		return;
	}

	if (shutDownStage_ != SHUTDOWN_NONE)
		return;	// just let it time out

	const BaseAppID id =
		this->getSubSet( args.isServiceApp ).getNextAppID();

	const BaseAppPtr pBaseApp( new BaseApp( *this, args.addrForCells,
		args.addrForClients, id, args.isServiceApp ) );
	
	pendingApps_.insert( std::make_pair( pBaseApp->addr(), pBaseApp ) );

	// Stream on the reply
	Mercury::Bundle & bundle = pBaseApp->bundle();
	bundle.startReply( replyID );

	BaseAppInitData initData;
	initData.id = pBaseApp->id();
	initData.time = time_;
	initData.isReady = hasStarted_;
	initData.timeoutPeriod = Config::baseAppTimeout();

	bundle << initData << dbApps_;

	pBaseApp->send();
}


/**
 *	This method returns whether there are multiple BaseApps with different IP
 *	addresses.
 */
bool ManagedAppSubSet::hasIdealBackups() const
{
	BaseAppsIterator iter = this->iterator();
	uint32 firstIP = 0;

	// We check if everything is on the same machine
	while (const BaseAppPtr & pCurr = iter.next())
	{
		uint32 currIP = pCurr->addr().ip;

		if (firstIP == 0)
		{
			firstIP = currIP;
		}
		else if (firstIP != currIP)
		{
			return true;
		}
	}

	return false;
}


/**
 *	This method updates information on the BaseApps about which other BaseApps
 *	they should create base entities on.
 */
void BaseAppSubSet::updateCreateBaseInfo()
{
	// Description of createBaseAnywhere scheme:
	// Currently a bit advanced scheme is implemented compared to the initial
	// one. This may also be modified with some additional ideas if it is not
	// effective enough.
	// A lot of the balancing occurs from players logging in and out. These
	// are always added to the least loaded BaseApp.
	//
	// The current scheme is that each BaseApp has a list of BaseApps assigned
	// to it where it should create Base entities. We try to have as much as
	// possible BaseApps in the destination list, in many cases it comprises all
	// the BaseApps but the amount is capped at 250 BaseApps by default.
	// When an entity is being created, a particular BaseApp is chosen randomly
	// with random weights based on the load.
	//
	// There are two configuration options: maxDestinationsInCreateBaseInfo and
	// updateCreateBaseInfoPeriod. The maxDestinationsInCreateBaseInfo is the
	// max number of destination BaseApps that each BaseApp will have pointing
	// to.
	//
	// updateCreateBaseInfoPeriod controls how often this information is
	// updated.
	//
	// Possible additions:
	// Instead of this information being updated to all BaseApps at a regular
	// period, this information could be updated as needed. The BaseApps could
	// be kept sorted and the destination set updated as the BaseApp loads
	// changed. Only some BaseApps would need to be updated.

	// Get all of the BaseApps in a vector.
	// TODO: Could consider maintaining this vector as an optimisation.
	BW::vector<BaseAppAddrLoadPair> infoPairs;
	infoPairs.reserve( this->size() );

	BaseAppsIterator iter = this->iterator();

	while (const BaseAppPtr & pCurr = iter.next())
	{
		infoPairs.push_back( BaseAppAddrLoadPair( pCurr->addr(), pCurr->load() ) );
	}

	int destSize = (int)infoPairs.size();

	if (destSize > Config::maxDestinationsInCreateBaseInfo())
	{
		destSize = Config::maxDestinationsInCreateBaseInfo();
		// Here the BaseApps are sorted so that we can find the least loaded
		// BaseApps. It only matters if baseApps_.size() becomes greater than
		// Config::maxDestinationsInCreateBaseInfo(), otherwise they all will
		// be sent.
		// TODO: Currently, this is only using the reported CPU load. We
		// probably want to consider the number of proxy and base entities on
		// the machine.
		using namespace std::placeholders;
		std::partial_sort(
			infoPairs.begin(), infoPairs.begin() + destSize, infoPairs.end(),
			std::bind( std::less<float>(),
					   std::bind( &BaseAppAddrLoadPair::load, _1 ),
					   std::bind( &BaseAppAddrLoadPair::load, _2 ) ) );
		infoPairs.erase( infoPairs.begin() + destSize, infoPairs.end() );
	}
	else
	{
		if (destSize == 0)
		{
			WARNING_MSG( "BaseAppSubSet::updateCreateBaseInfo: "
					"No valid destinations.\n" );
			return;
		}
	}

	// Initial stream size is guessed based on implementation of
	// << operator for BinaryOStream. It shouldn't hurt much if it's
	// a wrong guess but in theory it'll reduce the number of allocations
	// to 1
	MemoryOStream infoStream( infoPairs.size() * sizeof(BaseAppAddrLoadPair) +
								sizeof(uint32) );
	infoStream << infoPairs;
	// Send this information to all apps.
	sendToBaseApps( BaseAppIntInterface::setCreateBaseInfo, infoStream,
					BaseAppsIterator( apps_ ) );
}


/**
 *	This method is called to inform this BaseAppMgr about a base app during
 *	recovery from the death of an old BaseAppMgr.
 */
void BaseAppMgr::recoverBaseApp( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & /*header*/,
			BinaryIStream & data )
{
	if (!isRecovery_)
	{
		WARNING_MSG( "BaseAppMgr::recoverBaseApp: "
				"Recovering when we were not started with -recover\n" );
		isRecovery_ = true;
	}

	Mercury::Address		addrForCells;
	Mercury::Address		addrForClients;
	BaseAppID				id;
	bool					isServiceApp;

	data >> addrForCells >> addrForClients >> id >> isServiceApp >> time_;

	// hasStarted_ = true;
	this->startTimer();

	this->getSubSet( isServiceApp ).recoverBaseApp(
		srcAddr, *this, addrForCells, addrForClients, id, isServiceApp, data );
	this->recoverSharedDataFromStream( data );
	serviceApps_.recoverServiceFragments( srcAddr, data );
	this->recoverGlobalBasesFromStream( srcAddr, data );
}


/**
 *	This method reconstructs a @ref BaseApp after the death of an old BaseAppMgr.
 *	@param baseAppMgr		@ref BaseAppMgr used by @ref BaseApp.
 *	@param addrForCells		Internal address.
 *	@param addrForClients	External address.
 *	@param id				BaseApp ID.
 *	@param isServiceApp		BaseApp kind.
 *	@param data				Recovery data stream.
 */
void ManagedAppSubSet::recoverBaseApp( const Mercury::Address & /*srcAddr*/,
	BaseAppMgr & baseAppMgr, const Mercury::Address & addrForCells,
	const Mercury::Address & addrForClients, BaseAppID id, bool isServiceApp,
	BinaryIStream & data )
{
	PROC_IP_DEBUG_MSG( "ManagedAppSubSet::recoverBaseApp: %s, %s %u\n",
		addrForCells.c_str(),
		this->appName(),
		id );

	lastAppID_ = std::max( id, lastAppID_ );

	if (apps_.find( addrForCells ) != apps_.end())
	{
		ERROR_MSG( "ManagedAppSubSet::recoverBaseApp: "
				"Already know about %s at %s\n", this->appName(),
			addrForCells.c_str() );
		return;
	}

	BaseAppPtr pBaseApp( new BaseApp( baseAppMgr, addrForCells, addrForClients, 
		id, isServiceApp ) );
	this->addApp( pBaseApp );

	data >> pBaseApp->backupHash() >> pBaseApp->newBackupHash();
	hasIdealBackups_ = this->hasIdealBackups();
}


/**
 *	This method recovers shared (base and global) data after the death of an old
 *	BaseAppMgr.
 *	@param data	Recovery data stream.
 */
void BaseAppMgr::recoverSharedDataFromStream( BinaryIStream & data )
{
	// Read all of the shared BaseApp data
	{
		uint32 numEntries;
		data >> numEntries;

		BW::string key;
		BW::string value;

		for (uint32 i = 0; i < numEntries; ++i)
		{
			data >> key >> value;
			sharedBaseAppData_[ key ] = value;
		}
	}

	// TODO: This is mildly dodgy. It's getting its information from the
	// BaseApps but would probably be more accurate if it came from the
	// CellAppMgr. It may clobber a valid change that has been made by the
	// CellAppMgr.

	// Read all of the shared Global data
	{
		uint32 numEntries;
		data >> numEntries;

		BW::string key;
		BW::string value;

		for (uint32 i = 0; i < numEntries; ++i)
		{
			data >> key >> value;
			sharedGlobalData_[ key ] = value;
		}
	}
}

/**
 *	This method recovers service fragments after the death of an old BaseAppMgr.
 *	@param srcAddr	Address of tahe ServiceApp.
 *	@param data		Recovery data stream.
 */
void ServiceAppSubSet::recoverServiceFragments(
	const Mercury::Address & srcAddr, BinaryIStream & data )
{
	uint32 numServices;
	data >> numServices;

	for (uint32 i = 0; i < numServices; ++i)
	{
		this->addServiceFragmentFromStream( srcAddr, data, false );

	}
}


/**
 *	This method recovers global bases registry after the death of an old
 *	BaseAppMgr.
 *	@param srcAddr	Address of a BaseApp.
 *	@param data		Recovery data stream.
 */
void BaseAppMgr::recoverGlobalBasesFromStream( const Mercury::Address & srcAddr,
		BinaryIStream & data )
{
	while (data.remainingLength() > 0)
	{
		std::pair< BW::string, EntityMailBoxRef > value;
		data >> value.first >> value.second;

		MF_ASSERT( value.second.addr == srcAddr );

		if (!globalBases_.insert( value ).second)
		{
			WARNING_MSG( "BaseAppMgr::recoverGlobalBasesFromStream: "
					"Try to recover global base %s twice\n",
				globalBaseKeyToString( value.first ).c_str() );
		}
	}
}


/**
 *	This method handles the message from a BaseApp that it wants to be deleted.
 */
void BaseAppMgr::del( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		const BaseAppMgrInterface::delArgs & args )
{
	PROC_IP_TRACE_MSG( "BaseAppMgr::del: %u\n", args.id );

	// First try real Base Apps.

	if (this->onBaseAppDeath( addr ))
	{
		PROC_IP_DEBUG_MSG( "BaseAppMgr::del: now have %d base apps and %d"
				"service apps\n",
				baseApps_.size(), serviceApps_.size() );
	}
	else
	{
		PROC_IP_ERROR_MSG( "BaseAppMgr: Error deleting %s id = %u\n",
			addr.c_str(), args.id );
	}
}


/**
 *	This method handles the update from a BaseApp/ServiceApp after it 
 *	finishes initialization, which means it's ready to handle messages.
 */
void BaseAppMgr::finishedInit( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header )
{
	PROC_IP_TRACE_MSG( "BaseAppMgr::finishedInit: %s\n", addr.c_str() );

	BaseApps::iterator it = pendingApps_.find( addr );
	if (it != pendingApps_.end())
	{
		/** 
		 * The 'putServiceFragmentsInto' has to be here, after init is 
		 * done, otherwise any service fragment that registers between 
		 * 'add' and 'finishedInit' will be ignored and not able to be 
		 * populated to this BaseApp.
		 */
		serviceApps_.putServiceFragmentsInto( it->second->bundle() );

		ManagedAppSubSet & peerApps = this->getSubSet( *it->second );
		peerApps.addApp( it->second );

		this->synchronize( it->second );

		peerApps.adjustBackupLocations( *it->second,
			ADJUST_BACKUP_LOCATIONS_OP_ADD );
		pendingApps_.erase( it );

		PROC_IP_DEBUG_MSG( "BaseAppMgr::finishedInit: now have %d base "
				"apps and %d service apps ready\n",
			baseApps_.size(), serviceApps_.size() );
	}
	else
	{
		PROC_IP_ERROR_MSG( "BaseAppMgr::finishedInit "
			"Non existing app %s\n", addr.c_str() );
	}
}


/**
 *	This method synchronizes the new BaseApp with the existing BaseApps.
 *
 *	@param baseApp	The new added baseApp
 */
void BaseAppMgr::synchronize( BaseAppPtr baseApp )
{
	Mercury::Bundle & bundle = baseApp->bundle();
	// Now stream on globals as necessary
	for (GlobalBases::const_iterator iGlobalBase = globalBases_.begin();
			iGlobalBase != globalBases_.end(); ++iGlobalBase)
	{
		bundle.startMessage( BaseAppIntInterface::addGlobalBase );
		bundle << iGlobalBase->first << iGlobalBase->second;
	}

	for (SharedData::const_iterator iSharedData = sharedBaseAppData_.begin();
			iSharedData != sharedBaseAppData_.end(); ++iSharedData)
	{
		bundle.startMessage( BaseAppIntInterface::setSharedData );
		bundle << SharedDataType( SHARED_DATA_TYPE_BASE_APP ) <<
			iSharedData->first << iSharedData->second;
	}
	
	for (SharedData::const_iterator iSharedData = sharedGlobalData_.begin();
			iSharedData != sharedGlobalData_.end(); ++iSharedData)
	{
		bundle.startMessage( BaseAppIntInterface::setSharedData );
		bundle << SharedDataType( SHARED_DATA_TYPE_GLOBAL ) <<
			iSharedData->first << iSharedData->second;
	}

	for (BaseApps::const_iterator iter = baseAndServiceApps_.begin();
			iter != baseAndServiceApps_.end(); ++iter)
	{
		BaseAppPtr pOtherBaseApp = iter->second;
		if (pOtherBaseApp != baseApp)
		{
			bundle.startMessage( BaseAppIntInterface::handleBaseAppBirth );
			bundle << iter->first << pOtherBaseApp->externalAddr();

			pOtherBaseApp->bundle().startMessage(
				BaseAppIntInterface::handleBaseAppBirth );
			pOtherBaseApp->bundle() <<
				baseApp->addr() << baseApp->externalAddr();
			pOtherBaseApp->send();
		}
	}

	baseApp->send();
}


/**
 *	This method adjusts who each BaseApp is backing up to. It is called whenever
 *	BaseApp is added or removed.
 *
 *	This is used by the new-style backup.
 */
void ManagedAppSubSet::adjustBackupLocations( BaseApp & app,
	AdjustBackupLocationsOp op )
{
	// The current scheme is that every BaseApp backs up to every other BaseApp.
	// Ideas for improvement:
	// - May want to cap the number of BaseApps that a BaseApp backs up to.
	// - May want to limit how much the hash changes backups. Currently, all old
	//    backups are discarded but if an incremental hash is used, the amount
	//    of lost backup information can be reduced.
	// - Incremental hash could be something like the following: when we have a
	//    non power-of-2 number of backups, we assume that some previous ones
	//    are repeated to always get a power of 2.
	//    Let n be number of buckets and N be next biggest power of 2.
	//    bucket = hash % N
	//    if bucket >= n:
	//      bucket -= N/2;
	//    When another bucket is added, an original bucket that was managing two
	//    virtual buckets now splits the load with the new bucket. When a bucket
	//    is removed, a bucket that was previously managing one virtual bucket
	//    now handles two.

	const bool hadIdealBackups = hasIdealBackups_;

	hasIdealBackups_ = this->hasIdealBackups();

	const Mercury::Address & addr = app.addr();

	BaseAppsIterator iter = this->iterator();
	while (const BaseAppPtr & pExistingApp = iter.next())
	{
		if ((addr != pExistingApp->addr()) && !pExistingApp->isOffloading())
		{
			pExistingApp->adjustBackupLocations(
				addr, this->iterator(), op, hadIdealBackups, hasIdealBackups_ );

			if ((op == ADJUST_BACKUP_LOCATIONS_OP_ADD) &&
					!pExistingApp->isRetiring())
			{
				const bool isIdealBackup = (addr.ip != pExistingApp->addr().ip);

				if (!hasIdealBackups_ || isIdealBackup)
				{
					app.newBackupHash().push_back( pExistingApp->addr() );
				}
			}
		}
	}

	if (op == ADJUST_BACKUP_LOCATIONS_OP_ADD)
	{
		Mercury::Bundle & bundle = app.bundle();
		bundle.startMessage( BaseAppIntInterface::setBackupBaseApps );
		bundle << app.newBackupHash();
		app.send();
	}
}


/**
 *	This method sends SIGQUIT to a BaseApp via machined.
 *	@param baseApp	BaseApp to kill.
 */
void ManagedAppSubSet::killApp( const BaseApp & baseApp )
{
	const Mercury::Address & addr = baseApp.addr();
	INFO_MSG( "ManagedAppSubSet::killApp: Sending SIGQUIT to %s %s\n",
		this->appName(), addr.c_str() );

	if (!Mercury::MachineDaemon::sendSignalViaMachined( addr, SIGQUIT ))
	{
		ERROR_MSG( "ManagedAppSubSet::killApp: Failed to send SIGQUIT to %s %s\n",
			this->appName(), addr.c_str() );
	}
	this->removeApp( baseApp );
}


/**
 *	This method inserts a newly created BaseApp into the shared container and
 *	updates subset's cached size.
 *	@param pBaseApp	BaseApp to add.
 */
void ManagedAppSubSet::addApp( const BaseAppPtr & pBaseApp )
{
	size_ += apps_.insert( std::make_pair( pBaseApp->addr(), pBaseApp ) ).second;
	MF_ASSERT_DEBUG( size_ == this->countApps() );

	CONFIG_DEBUG_MSG( "ManagedAppSubSet::addApp:\n"
			"\tAllocated id\t\t= %u\n"
			"\t%ss\tin use\t= %d\n"
			"\tInternal address\t= %s\n"
			"\tExternal address\t= %s\n"
			"\tService App\t\t= %s\n",
		pBaseApp->id(),
		this->appName(),
		this->size(),
		pBaseApp->addr().c_str(),
		pBaseApp->externalAddr().c_str(),
		pBaseApp->isServiceApp() ? "true" : "false" );
}


/**
 *	This method removes a BaseApp into from the shared container and
 *	updates subset's cached size.
 *	@param pBaseApp	BaseApp to remove.
 */
void ManagedAppSubSet::removeApp( const BaseApp & baseApp )
{
	size_ -= apps_.erase( baseApp.addr() );
	MF_ASSERT_DEBUG( size_ == this->countApps() );
}


/**
 *	This method checks and handles the case where a BaseApp may have stopped.
 */
bool BaseAppMgr::onBaseAppDeath( const Mercury::Address & addr )
{
	BaseApps::const_iterator iter = baseAndServiceApps_.find( addr );

	if (iter == baseAndServiceApps_.end())
	{
		BaseApps::iterator pendingIter = pendingApps_.find( addr );
		if (pendingIter == pendingApps_.end())
		{
			return false;
		}
		pendingApps_.erase( pendingIter );
		return true;
	}
	// Keep the BaseApp instance around after it gets removed by
	// AppSubSet::onBaseAppDeath.
	BaseAppPtr pBaseApp = iter->second;
	const bool controlledShutDown =
		this->getSubSet( *pBaseApp ).onBaseAppDeath( *pBaseApp );
	if (!controlledShutDown)
	{
		pBackupHashChain_->adjustForDeadBaseApp( pBaseApp->addr(),
			pBaseApp->backupHash() );
		this->sendBaseAppDeathNotifications( *pBaseApp );
		this->redirectGlobalBases( *pBaseApp );
	}
	else
	{
		this->controlledShutDownServer();
	}
	return true;
}


/**
 *	This method processes an unexpected BaseApp death.
 *	@param baseApp	Died BaseApp.
 *	@return			@a true if the server should be shut down.
 */
bool ManagedAppSubSet::onBaseAppDeath( BaseApp & baseApp )
{

	const Mercury::Address & addr = baseApp.addr();
	bool controlledShutDown = false;
	INFO_MSG( "ManagedAppSubSet::onBaseAppDeath: %s%02d @ %s\n",
		this->appName(), baseApp.id(), addr.c_str() );

	const uint numAppsAlive = this->size() - 1;

	// Make sure it's really dead, otherwise backup will have
	// trouble taking over its address.
	this->killApp( baseApp );

	if (this->shutDownOnAppDeath())
	{
		controlledShutDown = true;
		NOTICE_MSG( "ManagedAppSubSet::onBaseAppDeath: "
				"shutDownServerOn%sDeath is enabled. "
				"Shutting down server\n", this->appName() );
	}
	else if (baseApp.backupHash().empty())
	{
		// TODO: What should be done if there is no backup or it's not
		// yet ready.
		if (baseApp.newBackupHash().empty())
		{
			ERROR_MSG( "ManagedAppSubSet::onBaseAppDeath: "
					"No backup for %s %s\n", this->appName(), addr.c_str() );
		}
		else
		{
			ERROR_MSG( "ManagedAppSubSet::onBaseAppDeath: "
					"Backup not ready for %s %s\n",
				this->appName(), addr.c_str() );
		}
		if (Config::shutDownServerOnBadState())
		{
			NOTICE_MSG( "ManagedAppSubSet::onBaseAppDeath: "
					"shutDownServerOnBadState is enabled. "
					"Shutting down server.\n" );
			controlledShutDown = true;
		}
	}

	// If the last BaseApp has died, then the server is in a bad state.
	if (numAppsAlive < this->minimumRequiredApps())
	{
		WARNING_MSG( "ManagedAppSubSet::onBaseAppDeath: "
				"There are no %s remaining.\n", this->appName() );

		if (Config::shutDownServerOnBadState())
		{
			NOTICE_MSG( "ManagedAppSubSet::onBaseAppDeath: "
					"shutDownServerOnBadState is enabled. "
					"Shutting down server.\n" );
			controlledShutDown = true;
		}
	}

	if (!controlledShutDown)
	{
		this->adjustBackupLocations( baseApp, ADJUST_BACKUP_LOCATIONS_OP_CRASH );
		this->updateCreateBaseInfo();
		this->stopBackupsTo( baseApp.addr() );
	}

	return controlledShutDown;
}


/**
 *	This method notifies CellAppMgr and the remaining BaseApps about BaseApp
 *	death.
 *	@param baseApp	Died BaseApp.
 */
void BaseAppMgr::sendBaseAppDeathNotifications( const BaseApp & baseApp )
{
	{
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( CellAppMgrInterface::handleBaseAppDeath );

		bundle << baseApp.addr() << uint8( baseApp.isServiceApp() );
		bundle << baseApp.backupHash();
		cellAppMgr_.send();
	}

	// Tell all other baseapps that the dead one is gone.
	if (baseAndServiceApps_.size() > 0)
	{
		MemoryOStream args;
		args << baseApp.addr() << uint8( baseApp.isServiceApp() )
			 << baseApp.backupHash();

		sendToBaseApps( BaseAppIntInterface::handleBaseAppDeath, args,
						BaseAppsIterator( baseAndServiceApps_,
										  BaseAppsExcluding( &baseApp ) ) );

		deadBaseAppAddr_ = baseApp.addr();
		archiveCompleteMsgCounter_ = baseAndServiceApps_.size();
	}

}


/**
 *	This method redirects global base mailboxes to the corresponding backup
 *	BaseApp after a BaseApp death.
 *	@param baseApp	Died BaseApp.
 */
void BaseAppMgr::redirectGlobalBases( const BaseApp & baseApp )
{
	// Adjust globalBases_ for new mapping
	{
		GlobalBases::iterator gbIter = globalBases_.begin();

		while (gbIter != globalBases_.end())
		{
			EntityMailBoxRef & mailbox = gbIter->second;

			if (mailbox.addr == baseApp.addr())
			{
				Mercury::Address newAddr =
					baseApp.backupHash().addressFor( mailbox.id );
				INFO_MSG( "BaseAppMgr::onBaseAppDeath: redirect base '%s' -> "
						"mailbox { entity=%d, addr=%s } to new address %s\n",
					globalBaseKeyToString( gbIter->first ).c_str(),
					mailbox.id, mailbox.addr.c_str(), newAddr.c_str() );
				mailbox.addr.ip = newAddr.ip;
				mailbox.addr.port = newAddr.port;
			}

			++gbIter;
		}
	}
}


/**
 *	This method processes an unexpected ServiceApp death. Checks for any
 *	lost service fragments to see if the server should be shut down.
 *	@param baseApp	Died ServiceApp.
 *	@return			@a true if the server should be shut down.
 */
bool ServiceAppSubSet::onBaseAppDeath( BaseApp & baseApp )
{
	bool controlledShutDown = ManagedAppSubSet::onBaseAppDeath( baseApp );
	ServicesMap::Names lostServices;
	int numRemoved = services_.removeFragmentsForAddress( baseApp.addr(),
		&lostServices );

	INFO_MSG( "ServiceAppSubSet::onBaseAppDeath: lost %d services on %s\n",
		numRemoved, baseApp.addr().c_str() );

	if (!lostServices.empty())
	{
		ServicesMap::Names::const_iterator iLostService = 
			lostServices.begin();

		while (iLostService != lostServices.end())
		{
			WARNING_MSG( "ServiceAppSubSet::onBaseAppDeath: "
					"Service %s has no remaining fragments on any "
					"ServiceApp\n",
				iLostService->c_str() );

			++iLostService;
		}

		if (Config::shutDownServerOnServiceDeath())
		{
			NOTICE_MSG( "ServiceAppSubSet::onBaseAppDeath: "
					"shutDownServerOnServiceDeath is enabled. "
					"Shutting down server\n" );
			controlledShutDown = true;
		}
	}
	return controlledShutDown;

}


/**
 *	This method request subset peer BaseApp to stop backing up to the given
 *	BaseApp.
 *	@param addr	Address of the BaseApp to which the backups should be stopped.
 */
void ManagedAppSubSet::stopBackupsTo( const Mercury::Address & addr )
{
	BaseAppsIterator iter = this->iterator();
	while (const BaseAppPtr & pCurr = iter.next())
	{
		pCurr->stopBackup( addr );
	}
}


/**
 *	This method handles a BaseApp finishing its controlled shutdown.
 */
void BaseAppMgr::removeControlledShutdownBaseApp(
		const Mercury::Address & addr )
{
	TRACE_MSG( "BaseAppMgr::removeControlledShutdownBaseApp: %s\n",
			addr.c_str() );

	BaseApp * const baseApp = this->findBaseApp( addr );
	if (baseApp)
	{
		this->getSubSet( *baseApp ).removeApp( *baseApp );
	}
}


/**
 *	This method shuts down this process.
 */
void BaseAppMgr::shutDown( bool shutDownOthers )
{
	INFO_MSG( "BaseAppMgr::shutDown: shutDownOthers = %d\n",
			shutDownOthers );
	shouldShutDownOthers_ = shutDownOthers;
	mainDispatcher_.breakProcessing();
}


/**
 *	This method responds to a shutDown message.
 */
void BaseAppMgr::shutDown( const BaseAppMgrInterface::shutDownArgs & args )
{
	this->shutDown( args.shouldShutDownOthers );
}


/**
 *	This method is called to inform CellAppMgr that the base apps are in a
 *	particular shutdown stage.
 *	@param stage	Shutdown stage.
 */
void BaseAppMgr::ackBaseAppsShutDownToCellAppMgr( ShutDownStage stage )
{
	DEBUG_MSG( "BaseAppMgr::ackBaseAppsShutDownToCellAppMgr: "
			"All BaseApps have shut down, informing CellAppMgr\n" );
	Mercury::Bundle & bundle = this->cellAppMgr().bundle();
	CellAppMgrInterface::ackBaseAppsShutDownArgs & rAckBaseAppsShutDown =
		CellAppMgrInterface::ackBaseAppsShutDownArgs::start( bundle );

	rAckBaseAppsShutDown.stage = stage;

	this->cellAppMgr().send();
}


/**
 *	This method informs base and service apps of the controlled shutdown process.
 *	@param args Shutdown stage and time.
 */
void BaseAppMgr::informBaseAppsOfShutDown(
	const BaseAppMgrInterface::controlledShutDownArgs & args )
{
	MF_ASSERT( !baseAndServiceApps_.empty() );
	SyncControlledShutDownHandler * pHandler = new SyncControlledShutDownHandler(
		args.stage, baseAndServiceApps_.size() );
	MemoryOStream payload;
	payload << args.stage << args.shutDownTime;
	sendToBaseApps( BaseAppIntInterface::controlledShutDown, payload,
		BaseAppsIterator( baseAndServiceApps_ ), pHandler );
}


/**
 *	This method responds to a message telling us what stage of the controlled
 *	shutdown process the server is at.
 */
void BaseAppMgr::controlledShutDown(
		const BaseAppMgrInterface::controlledShutDownArgs & args )
{
	INFO_MSG( "BaseAppMgr::controlledShutDown: stage = %s\n", 
		ServerApp::shutDownStageToString( args.stage ) );

	switch (args.stage)
	{
		case SHUTDOWN_REQUEST:
		{
			if (cellAppMgr_.channel().isEstablished())
			{
				Mercury::Bundle & bundle = cellAppMgr_.bundle();
				CellAppMgrInterface::controlledShutDownArgs args;
				args.stage = SHUTDOWN_REQUEST;
				bundle << args;
				cellAppMgr_.send();
			}
			break;
		}

		case SHUTDOWN_INFORM:
		{
			shutDownStage_ = args.stage;
			shutDownTime_ = args.shutDownTime;

			if (baseAndServiceApps_.empty())
			{
				this->ackBaseAppsShutDownToCellAppMgr( args.stage );
			}
			else
			{
				// Inform all base apps, once the requests are complete the
				// CellAppMgr is notified via ackBaseAppsShutDownToCellAppMgr().
				this->informBaseAppsOfShutDown( args );
			}

			break;
		}

		case SHUTDOWN_PERFORM:
		{
			this->startAsyncShutDownStage( SHUTDOWN_DISCONNECT_PROXIES );
			break;
		}

		case SHUTDOWN_TRIGGER:
		{
			this->controlledShutDownServer();
			break;
		}

		case SHUTDOWN_NONE:
		case SHUTDOWN_DISCONNECT_PROXIES:
			break;
	}
}


/**
 *
 */
void BaseAppMgr::startAsyncShutDownStage( ShutDownStage stage )
{
	BW::vector< Mercury::Address > addrs;
	addrs.reserve( baseAndServiceApps_.size() );

	BaseApps::const_iterator iter = baseAndServiceApps_.begin();

	while (iter != baseAndServiceApps_.end())
	{
		addrs.push_back( iter->first );

		++iter;
	}

	// This object deletes itself.
	new AsyncControlledShutDownHandler( stage, addrs );
}


/**
 *  Trigger a controlled shutdown of the entire server.
 */
void BaseAppMgr::controlledShutDownServer()
{
	if (shutDownStage_ != SHUTDOWN_NONE)
	{
		DEBUG_MSG( "BaseAppMgr::controlledShutDownServer: "
			"Already shutting down, ignoring duplicate shutdown request.\n" );
		return;
	}

	// First try to trigger controlled shutdown via the loginapp
	Mercury::Address loginAppAddr;
	Mercury::Reason reason = Mercury::MachineDaemon::findInterface(
				"LoginIntInterface", -1, loginAppAddr );

	if (reason == Mercury::REASON_SUCCESS)
	{
		Mercury::ChannelSender sender( BaseAppMgr::getChannel( loginAppAddr ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startMessage( LoginIntInterface::controlledShutDown );

		INFO_MSG( "BaseAppMgr::controlledShutDownServer: "
			"Triggering server shutdown via LoginApp @ %s\n",
			loginAppAddr.c_str() );

		return;
	}
	else
	{
		ERROR_MSG( "BaseAppMgr::controlledShutDownServer: "
			"Couldn't find a LoginApp to trigger server shutdown\n" );
	}

	// Next try to trigger shutdown via the DBApp.
	// TODO: Scalable DB. Talk directly to DBAppMgr, instead of DBApp. DBApp
	// will forward to DBAppMgr currently.
	if (dbAppAlpha_.channel().isEstablished())
	{
		Mercury::Bundle & bundle = dbAppAlpha_.bundle();
		DBAppInterface::controlledShutDownArgs::start( bundle ).stage =
			SHUTDOWN_REQUEST;
		dbAppAlpha_.send();

		INFO_MSG( "BaseAppMgr::controlledShutDownServer: "
				"Triggering server shutdown via the Alpha DBApp%02d (%s)\n",
			dbApps_.alpha().id(), 
			dbAppAlpha_.addr().c_str() );
		return;
	}
	else
	{
		ERROR_MSG( "BaseAppMgr::controlledShutDownServer: "
			"Couldn't find the DBApp to trigger server shutdown\n" );
	}

	// Alright, the shutdown starts with me then
	BaseAppMgrInterface::controlledShutDownArgs args;
	args.stage = SHUTDOWN_REQUEST;
	INFO_MSG( "BaseAppMgr::controlledShutDownServer: "
		"Starting controlled shutdown here (no LoginApp or DBApp found)\n" );
	this->controlledShutDown( args );
}

/**
 *	This method replies whether if the server has been started.
 */
void BaseAppMgr::requestHasStarted( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader& header )
{
	Mercury::ChannelSender sender( BaseAppMgr::getChannel( srcAddr ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( header.replyID );
	bundle << hasStarted_;

	return;
}

/**
 *	This method processes the initialisation data from DBApp.
 */
void BaseAppMgr::initData( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	if (hasInitData_)
	{
		CONFIG_ERROR_MSG( "BaseAppMgr::initData: "
				"Ignored subsequent initialisation data from %s\n",
			addr.c_str() );
		return;
	}

	// Save DBApp time for BaseApps
	GameTime gameTime;
	data >> gameTime;
	if (time_ == 0)
	{
		// __kyl__(12/8/2008) XML database always sends 0 as the game time.
		this->setStartTime( gameTime );
		CONFIG_INFO_MSG( "BaseAppMgr::initData: game time = %.1f\n",
				this->gameTimeInSeconds() );
	}
	// else
		// Recovery case. We should be getting the game time from BaseApps.


	hasInitData_ = true;
}

/**
 *	This method processes a message from the DBApp that restores the spaces
 * 	(and space data). This comes via the BaseAppMgr mainly because
 * 	DBApp doesn't have a channel to CellAppMgr and also because BaseAppMgr
 * 	tells DBApp when to "start" the system.
 */
void BaseAppMgr::spaceDataRestore( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	MF_ASSERT( !hasStarted_ && hasInitData_ );

	// Send spaces information to CellAppMgr
	{
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( CellAppMgrInterface::prepareForRestoreFromDB );
		bundle << time_;
		bundle.transfer( data, data.remainingLength() );
		cellAppMgr_.send();
	}
}


/**
 *	This method handles a message to set a shared data value. This may be
 *	data that is shared between all BaseApps or all BaseApps and CellApps. The
 *	BaseAppMgr is the authoritative copy of BaseApp data but the CellAppMgr is
 *	the authoritative copy of global data (i.e. data shared between all BaseApps
 *	and all CellApps).
 */
void BaseAppMgr::setSharedData( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	bool shouldSendToBaseApps = true;
	SharedDataType dataType;
	BW::string key;
	BW::string value;
	data >> dataType >> key >> value;

	Mercury::Address originalSrcAddr = srcAddr;

	// If the message contains an address, it will have come from the
	// CellAppMgr. The address is the original invoker of the change
	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddr;
	}

	if (dataType == SHARED_DATA_TYPE_BASE_APP)
	{
		sharedBaseAppData_[ key ] = value;
	}
	else if (dataType == SHARED_DATA_TYPE_GLOBAL)
	{
		sharedGlobalData_[ key ] = value;
	}
	else if ((dataType == SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP) ||
		(dataType == SHARED_DATA_TYPE_CELL_APP))
	{
		if (dataType == SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP)
		{
			dataType = SHARED_DATA_TYPE_GLOBAL;
		}

		// Because BaseApps don't have channels to the CellAppMgr
		// we will forward these messages on its behalf
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( CellAppMgrInterface::setSharedData );
		bundle << dataType << key << value << srcAddr;
		cellAppMgr_.send();

		// Make sure we don't tell the BaseApps about this yet, wait
		// for CellAppMgr to notify us.
		shouldSendToBaseApps = false;
	}
	else
	{
		ERROR_MSG( "BaseAppMgr::setSharedData: Invalid dataType %d\n",
				dataType );
		return;
	}

	if (shouldSendToBaseApps)
	{
		MemoryOStream payload;
		payload << dataType << key << value << originalSrcAddr;

		sendToBaseApps( BaseAppIntInterface::setSharedData, payload,
			BaseAppsIterator( baseAndServiceApps_ ) );
	}
}


/**
 *	This method handles a message to delete a shared data value. This may be
 *	data that is shared between all BaseApps or all BaseApps and CellApps. The
 *	BaseAppMgr is the authoritative copy of BaseApp data but the CellAppMgr is
 *	the authoritative copy of global data (i.e. data shared between all BaseApps
 *	and all CellApps).
 */
void BaseAppMgr::delSharedData( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	bool shouldSendToBaseApps = true;
	SharedDataType dataType;
	BW::string key;
	data >> dataType >> key;

	Mercury::Address originalSrcAddr = srcAddr;

	// If the message contains an address, it will have come from the
	// CellAppMgr. The address is the original invoker of the change
	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddr;
	}

	if (dataType == SHARED_DATA_TYPE_BASE_APP)
	{
		sharedBaseAppData_.erase( key );
	}
	else if (dataType == SHARED_DATA_TYPE_GLOBAL)
	{
		sharedGlobalData_.erase( key );
	}
	else if ((dataType == SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP) ||
		(dataType == SHARED_DATA_TYPE_CELL_APP))
	{
		if (dataType == SHARED_DATA_TYPE_GLOBAL_FROM_BASE_APP)
		{
			dataType = SHARED_DATA_TYPE_GLOBAL;
		}

		// Because BaseApps don't have channels to the CellAppMgr
		// we will forward these messages on its behalf
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( CellAppMgrInterface::delSharedData );
		bundle << dataType << key << srcAddr;
		cellAppMgr_.send();

		// Make sure we don't tell the BaseApps about this yet, wait
		// for CellAppMgr to notify us.
		shouldSendToBaseApps = false;
	}
	else
	{
		ERROR_MSG( "BaseAppMgr::delSharedData: Invalid dataType %d\n",
				dataType );
		return;
	}

	if (shouldSendToBaseApps)
	{
		MemoryOStream payload;
		payload << dataType << key << originalSrcAddr;

		sendToBaseApps( BaseAppIntInterface::delSharedData, payload,
			BaseAppsIterator( baseAndServiceApps_ ) );
	}
}


/**
 *	This method handles a message from a BaseApp informing us that it is ready
 *	to use its new backup hash.
 */
void BaseAppMgr::useNewBackupHash( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	BaseApp * pBaseApp = this->findBaseApp( addr );

	if (pBaseApp)
	{
		pBaseApp->useNewBackupHash( data );
	}
	else
	{
		WARNING_MSG( "BaseAppMgr::useNewBackupHash: "
				"No BaseApp %s. It may have just died.\n", addr.c_str() );
		data.finish();
	}
}


/**
 *	This method handles a message from a BaseApp informing us that it has
 *	completed a full archive cycle. Only BaseApps with secondary databases
 *	enabled should send this message.
 */
void BaseAppMgr::informOfArchiveComplete( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	BaseApp * pBaseApp = this->findBaseApp( addr );

	if (!pBaseApp)
	{
		ERROR_MSG( "BaseAppMgr::informOfArchiveComplete: No BaseApp with "
				"address %s\n",	addr.c_str() );
		return;
	}

	Mercury::Address deadBaseAppAddr;
	data >> deadBaseAppAddr;

	// Only interested in the last death
	if (deadBaseAppAddr != deadBaseAppAddr_)
	{
		return;
	}

	--archiveCompleteMsgCounter_;

	if (archiveCompleteMsgCounter_ == 0)
	{
		// Tell DBApp which secondary databases are still active
		Mercury::Bundle & bundle = dbAppAlpha_.bundle();
		bundle.startMessage( DBAppInterface::updateSecondaryDBs );

		bundle << uint32(baseAndServiceApps_.size());

		for (BaseApps::const_iterator iter = baseAndServiceApps_.begin();
				iter != baseAndServiceApps_.end(); ++iter)
		{
			bundle << iter->first;
		}

		dbAppAlpha_.send();
	}
}


/**
 *	This method handles a request to send back the backup hash chain to the
 *	requesting address. 
 */
void BaseAppMgr::requestBackupHashChain( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	Mercury::UDPBundle bundle; 

	// TODO: Make the requesting app retry until it gets a response, as it is
	// usually the Web Integration module making the requests, and it can't do
	// processing until after it's received its reply, so we can't make this
	// reliable.
	bundle.startReply( header.replyID, Mercury::RELIABLE_NO );
	bundle << *pBackupHashChain_;

	this->interface().send( addr, bundle );
}


/**
 *	This method handles an update to the DBApp map from the DBAppMgr.
 *
 *	@param data 	The streamed DBApp hash.
 */
void BaseAppMgr::updateDBAppHash( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	if (data.remainingLength() == 0)
	{
		// The assumption is that the message is sent by DBMgr instead of
		// DBAppMgr. Otherwise, it should be critical.
		WARNING_MSG( "BaseAppMgr::updateDBAppHash: Got corrupted DBApp hash"
			"map from %s\n", addr.c_str() );
		return;
	}

	MemoryOStream stream( data.remainingLength() );
	stream.transfer( data, data.remainingLength() );

	if (!dbApps_.updateFromStream( stream ))
	{
		CRITICAL_MSG( "BaseAppMgr::updateDBAppHash: "
			"Failed to de-stream DBApp hash\n" );
		return;
	}

	Mercury::Address dbAppAlphaAddress = dbApps_.alpha().address();
	if (dbAppAlphaAddress != dbAppAlpha_.addr())
	{
		DEBUG_MSG( "BaseAppMgr::updateDBAppHash: Got new DBApp Alpha %s\n",
			dbAppAlphaAddress.c_str() );
			
		dbAppAlpha_.addr( dbAppAlphaAddress );
	}

	for (BaseApps::const_iterator iter = baseAndServiceApps_.begin();
			iter != baseAndServiceApps_.end();
			++iter)
	{
		stream.rewind();
		iter->second->updateDBAppHashFromStream( stream );
	}
}


/**
 *	This method responds to a message from the DBApp that tells us to start.
 */
void BaseAppMgr::startup( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	if (hasStarted_)
	{
		WARNING_MSG( "BaseAppMgr::startup: Already ready.\n" );
		return;
	}

	bool didAutoLoadEntitiesFromDB;
	data >> didAutoLoadEntitiesFromDB;

	INFO_MSG( "BaseAppMgr is starting\n" );

	this->startTimer();

	// Start the CellAppMgr
	{
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( CellAppMgrInterface::startup );
		cellAppMgr_.send();
	}

	// Start the BaseApps.
	{
		if (baseAndServiceApps_.empty())
		{
			CRITICAL_MSG( "BaseAppMgr::startup: "
				"No Base apps running when started.\n" );
		}

		// Tell all the baseapps to start up, but only one is the bootstrap
		bool shouldSendBootstrap = true;
		for (BaseApps::const_iterator it = baseAndServiceApps_.begin();
				it != baseAndServiceApps_.end(); ++it)
		{
			this->startupBaseApp( *it->second, shouldSendBootstrap,
				didAutoLoadEntitiesFromDB );
		}
		
		for (BaseApps::const_iterator it = pendingApps_.begin();
				it != pendingApps_.end(); ++it)
		{
			this->startupBaseApp( *it->second, shouldSendBootstrap,
				didAutoLoadEntitiesFromDB );
		}
	}
}


/**
 *	This method sends the startup message to a BaseApp.
 * 
 *	@param baseApp						The BaseApp to send the message to.
 *	@param shouldSendBootstrap			Whether this BaseApp is the bootstrap.
 *	@param didAutoLoadEntitiesFromDB	Starting from auto-loaded state.
 */
void BaseAppMgr::startupBaseApp( BaseApp & baseApp, bool & shouldSendBootstrap, 
	bool didAutoLoadEntitiesFromDB )
{
	Mercury::Bundle & bundle = baseApp.bundle();
	BaseAppIntInterface::startupArgs & args =
		BaseAppIntInterface::startupArgs::start( bundle );
	if (shouldSendBootstrap && !baseApp.isServiceApp())
	{
		args.bootstrap = true;
		shouldSendBootstrap = false;
	}
	else
	{
		args.bootstrap = false;
	}
	args.didAutoLoadEntitiesFromDB = didAutoLoadEntitiesFromDB;

	baseApp.send();
}


/**
 *	This method starts the game timer.
 */
void BaseAppMgr::startTimer()
{
	if (!hasStarted_)
	{
		hasStarted_ = true;
		gameTimer_ = mainDispatcher_.addTimer( 1000000/Config::updateHertz(),
				this,
				reinterpret_cast< void * >( TIMEOUT_GAME_TICK ),
				"GameTick" );
		pTimeKeeper_ = new TimeKeeper( interface_, gameTimer_, time_,
				Config::updateHertz(), cellAppMgr_.addr(),
				&CellAppMgrInterface::gameTimeReading );
	}
}


/**
 *	This method handles a request from the DBApp for our status. The status from
 *	the CellAppMgr is retrieved and then both returned.
 */
void BaseAppMgr::checkStatus( const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	if (cellAppMgr_.channel().isEstablished())
	{
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startRequest( CellAppMgrInterface::checkStatus,
			   new CheckStatusReplyHandler( addr, header.replyID ) );
		bundle.transfer( data, data.remainingLength() );
		cellAppMgr_.send();
	}
	else
	{
		IF_NOT_MF_ASSERT_DEV( this->dbAppAlpha().addr() == addr )
		{
			return;
		}

		const int serviceAppCount = serviceApps_.size();
		const int baseAppCount = baseApps_.size();

		Mercury::Bundle & bundle = this->dbAppAlpha().bundle();
		bundle.startReply( header.replyID );
		bundle << uint8(false) << baseAppCount << serviceAppCount << 0;
		bundle << "No CellAppMgr";
		this->dbAppAlpha().send();
	}
}


/**
 *	This method is called to let the BaseAppMgr know that there is a new
 *	CellAppMgr.
 */
void BaseAppMgr::handleCellAppMgrBirth(
	const BaseAppMgrInterface::handleCellAppMgrBirthArgs & args )
{
	INFO_MSG( "BaseAppMgr::handleCellAppMgrBirth: %s\n", args.addr.c_str() );

	if (!cellAppMgr_.channel().isEstablished() && (args.addr.ip != 0))
	{
		INFO_MSG( "BaseAppMgr::handleCellAppMgrBirth: "
					"CellAppMgr is now ready.\n" );
	}

	cellAppMgr_.addr( args.addr );

	if (pTimeKeeper_)
	{
		pTimeKeeper_->masterAddress( cellAppMgr_.addr() );
	}
	// Reset the bestBaseAppAddr to allow the CellAppMgr to be
	// notified next game tick.
	baseApps_.clearBestBaseApp();
}


/**
 *	ManagedAppSubSet overrides
 */
void BaseAppSubSet::addApp( const BaseAppPtr & pBaseApp )
{
	// Let the Cell App Manager know about the first base app. This is so that
	// the cell app can know about a base app. We will probably improve this
	// later.
	if (this->size() == 0)
	{
		bestBaseAppAddr_ = pBaseApp->internalAddr();
		Mercury::Bundle	& bundle = cellAppMgr_.bundle();
		CellAppMgrInterface::setBaseAppArgs & rSetBaseApp =
			CellAppMgrInterface::setBaseAppArgs::start( bundle );
		rSetBaseApp.addr = bestBaseAppAddr_;
		cellAppMgr_.send();
	}
	return ManagedAppSubSet::addApp( pBaseApp );
}


/**
 *	This method resets the current best BaseApp address.
 */
void BaseAppSubSet::clearBestBaseApp()
{
	bestBaseAppAddr_.ip = 0;
	bestBaseAppAddr_.port = 0;
}


/**
 *	This method is called when another BaseAppMgr is started.
 */
void BaseAppMgr::handleBaseAppMgrBirth(
	const BaseAppMgrInterface::handleBaseAppMgrBirthArgs & args )
{
	if (args.addr != interface_.address())
	{
		WARNING_MSG( "BaseAppMgr::handleBaseAppMgrBirth: %s\n",
				args.addr.c_str() );
		this->shutDown( false );
	}
}


/**
 *	This method is called when a cell application has died unexpectedly.
 */
void BaseAppMgr::handleCellAppDeath( const Mercury::Address & /*addr*/,
		const Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data )
{
	TRACE_MSG( "BaseAppMgr::handleCellAppDeath:\n" );

	// Make a local memory stream with the data so we can add it to the bundle
	// for each BaseApp.
	MemoryOStream payload;
	payload.transfer( data, data.remainingLength() );

	sendToBaseApps( BaseAppIntInterface::handleCellAppDeath, payload,
			BaseAppsIterator( baseAndServiceApps_ ) );
}


/**
 *  This method is called by machined to inform us of a base application that
 *  has died unexpectedly.
 */
void BaseAppMgr::handleBaseAppDeath(
				const BaseAppMgrInterface::handleBaseAppDeathArgs & args )
{
	this->handleBaseAppDeath( args.addr );
}


/**
 *	This method handles a BaseApp dying unexpectedly.
 */
void BaseAppMgr::handleBaseAppDeath( const Mercury::Address & addr )
{
	if (shutDownStage_ != SHUTDOWN_NONE)
	{
		INFO_MSG( "BaseAppMgr::handleBaseAppDeath: "
			"dead app on %s received during shutdown!\n",
			addr.c_str() );

		BaseApp * const baseApp = this->findBaseApp( addr );
		if (baseApp)
		{
			this->getSubSet( *baseApp ).killApp( *baseApp );
		}

		return;
	}

	INFO_MSG( "BaseAppMgr::handleBaseAppDeath: dead app on %s\n",
			addr.c_str() );

	this->onBaseAppDeath( addr );
}


/**
 *	This method attempts to add a global base.
 */
void BaseAppMgr::registerBaseGlobally( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	// Figure out which baseapp sent this message
	BaseApp * pSender = this->findBaseApp( srcAddr );
	IF_NOT_MF_ASSERT_DEV( pSender )
	{
		ERROR_MSG( "BaseAppMgr::registerBaseGlobally: "
			"Got message from unregistered app @ %s, registration aborted\n",
			srcAddr.c_str() );

		return;
	}

	std::pair< BW::string, EntityMailBoxRef > value;
	data >> value.first >> value.second;

	INFO_MSG( "BaseAppMgr::registerBaseGlobally: Registered base "
			"'%s' -> mailbox { entity=%d, addr=%s } from %s\n",
		globalBaseKeyToString( value.first ).c_str(), value.second.id,
		value.second.addr.c_str(), srcAddr.c_str() );

	int8 successCode = 0;

	if (globalBases_.insert( value ).second)
	{
		successCode = 1;

		MemoryOStream args;
		args << value.first << value.second;

		sendToBaseApps( BaseAppIntInterface::addGlobalBase, args,
						BaseAppsIterator( baseAndServiceApps_,
										  BaseAppsExcluding( pSender ) ) );
	}
	else
	{
		INFO_MSG( "BaseAppMgr::registerBaseGlobally: "
				"Failed for key '%s' and base %d\n",
			globalBaseKeyToString( value.first ).c_str(), value.second.id );
	}

	// Send the ack back to the sender.
	Mercury::Bundle & bundle = pSender->bundle();
	bundle.startReply( header.replyID );
	bundle << successCode;
	pSender->send();
}


/**
 *	This method attempts to remove a global base.
 */
void BaseAppMgr::deregisterBaseGlobally( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & /*header*/,
			BinaryIStream & data )
{
	BW::string label;
	data >> label;

	const GlobalBases::iterator it = globalBases_.find( label );

	if (it != globalBases_.end())
	{
		const EntityMailBoxRef & mailbox = it->second;
		INFO_MSG( "BaseAppMgr::deregisterBaseGlobally: '%s' -> "
				"mailbox { entity=%d, addr=%s } from %s\n",
			globalBaseKeyToString( label ).c_str(), mailbox.id,
			mailbox.addr.c_str(), srcAddr.c_str() );
		globalBases_.erase( it );
		BaseApp * pSrc = this->findBaseApp( srcAddr );
		MemoryOStream payload;
		payload << label;

		sendToBaseApps( BaseAppIntInterface::delGlobalBase, payload,
			BaseAppsIterator( baseAndServiceApps_, BaseAppsExcluding( pSrc ) ) );
	}
	else
	{
		ERROR_MSG( "BaseAppMgr::deregisterBaseGlobally: Failed to erase '%s' "
				"from %s\n", globalBaseKeyToString( label ).c_str(),
			srcAddr.c_str() );
	}
}


/**
 *	This method attempts to add a service fragment.
 */
void BaseAppMgr::registerServiceFragment( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	serviceApps_.addServiceFragmentFromStream( srcAddr, data, true );
}


/**
 *	This method adds a service fragment from a description on a stream.
 */
void ServiceAppSubSet::addServiceFragmentFromStream( const Mercury::Address & srcAddr,
		BinaryIStream & data,
		bool shouldInformOthers )
{
	// Figure out which baseapp sent this message
	BaseApps::const_iterator it = apps_.find( srcAddr );
	IF_NOT_MF_ASSERT_DEV( it != apps_.end() )
	{
		ERROR_MSG( "ServiceAppSubSet::registerServiceFragment: "
			"Got message from unregistered app @ %s, registration aborted\n",
			srcAddr.c_str() );

		return;
	}
	BaseApp * const pSender = it->second.get();

	BW::string serviceName;
	EntityMailBoxRef mailBox;

	data >> serviceName >> mailBox;

	MF_ASSERT_DEV( srcAddr == mailBox.addr );

	IF_NOT_MF_ASSERT_DEV( pSender->isServiceApp() )
	{
		WARNING_MSG( "ServiceAppSubSet::registerServiceFragment: "
				"Registering fragment %s from %s that is not a ServiceApp\n",
				serviceName.c_str(), srcAddr.c_str() );
	}

	INFO_MSG( "ServiceAppSubSet::registerServiceFragment: Registered %s from %s\n",
		serviceName.c_str(), srcAddr.c_str() );

	services_.addFragment( serviceName, mailBox );

	if (shouldInformOthers)
	{
		MemoryOStream args;
		args << serviceName << mailBox;

		sendToBaseApps( BaseAppIntInterface::addServiceFragment, args,
			BaseAppsIterator( apps_ ) );

		args.rewind();
		Mercury::Bundle & bundle = cellAppMgr_.bundle();
		bundle.startMessage( 
			CellAppMgrInterface::addServiceFragment );
		bundle.transfer( args, args.remainingLength() );
		cellAppMgr_.send();
	}
}


/**
 *	This method attempts to remove a service fragment.
 *	@return	@a true if the server should be shut down.
 */
bool ServiceAppSubSet::deregisterServiceFragment(
	const Mercury::Address & srcAddr, BinaryIStream & data,
	ShutDownStage shutDownStage )
{
	BW::string serviceName;
	data >> serviceName;

	INFO_MSG( "ServiceAppSubSet::deregisterServiceFragment: %s %s\n",
			srcAddr.c_str(), serviceName.c_str() );

	bool didLoseService = false;
	int numDeleted = services_.removeFragment( serviceName, srcAddr, 
		&didLoseService );

	if (numDeleted != 1)
	{
		ERROR_MSG( "ServiceAppSubSet::deregisterServiceFragment: "
					"Service %s@%s. numDeleted = %d, not 1\n",
				serviceName.c_str(), srcAddr.c_str(), numDeleted );
	}

	MemoryOStream payload;
	payload << serviceName << srcAddr;
	sendToBaseApps( BaseAppIntInterface::delServiceFragment, payload,
		BaseAppsIterator( apps_ ) );

	payload.rewind();
	Mercury::Bundle & bundle = cellAppMgr_.bundle();
	bundle.startMessage( 
		CellAppMgrInterface::delServiceFragment );
	bundle.transfer( payload, payload.remainingLength() );
	cellAppMgr_.send();

	if (didLoseService && (shutDownStage == SHUTDOWN_NONE))
	{
		INFO_MSG( "ServiceAppSubSet::deregisterServiceFragment: "
				"Service %s has no remaining fragments on any ServiceApp\n",
			serviceName.c_str() );

		if (Config::shutDownServerOnServiceDeath())
		{
			return true;
		}
	}
	return false;

}


/**
 *	This method forwards service removal request to ServiceAppSubSet.
 */
void BaseAppMgr::deregisterServiceFragment( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & /*header*/,
			BinaryIStream & data )
{
	const bool controlledShutDown = serviceApps_.deregisterServiceFragment(
		srcAddr, data, shutDownStage_ );
	if (controlledShutDown)
	{
		NOTICE_MSG( "BaseAppMgr::deregisterServiceFragment: "
				"shutDownServerOnServiceDeath is enabled. "
				"Shutting down server\n" );
		this->controlledShutDownServer();
	}
}


/**
 *	This method returns the BaseApp or BackupBaseApp associated with the input
 *	address.
 */
Mercury::ChannelOwner *
		BaseAppMgr::findChannelOwner( const Mercury::Address & addr )
{
	return this->findBaseApp( addr );
}


/**
 *	This static method returns a channel to the input address. If one does not
 *	exist, it is created.
 */
Mercury::UDPChannel & BaseAppMgr::getChannel( const Mercury::Address & addr )
{
	return BaseAppMgr::instance().interface().findOrCreateChannel( addr );
}

BW_END_NAMESPACE

// baseappmgr.cpp
