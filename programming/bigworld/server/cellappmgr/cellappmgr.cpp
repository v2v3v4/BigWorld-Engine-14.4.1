#include "cellappmgr.hpp"

#include "cellapp.hpp"
#include "cellappmgr_config.hpp"
#include "cellappmgr_viewer_server.hpp"
#include "cell_app_death_handler.hpp"
#include "cell_app_groups.hpp"
#include "cell_app_load_config.hpp"
#include "login_conditions_config.hpp"
#include "shutdown_handler.hpp"
#include "space.hpp"
#include "watcher_forwarding_cellapp.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cellapp/cellapp_interface.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"

#include "db/dbapp_interface.hpp"
#include "db/dbappmgr_interface.hpp"

#include "network/bundle.hpp"
#include "network/channel_sender.hpp"
#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/machine_guard.hpp"
#include "network/machined_utils.hpp"
#include "network/network_interface.hpp"
#include "network/nub_exception.hpp"
#include "network/portmap.hpp"
#include "network/watcher_nub.hpp"
#include "network/smart_bundles.hpp"

#include "server/bwconfig.hpp"
#include "server/cell_app_init_data.hpp"
#include "server/reviver_subject.hpp"
#include "server/shared_data_type.hpp"
#include "server/stream_helper.hpp"
#include "server/time_keeper.hpp"
#include "server/util.hpp"

#include <algorithm>
#include <functional>
#include <time.h>

#include <sys/types.h>

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "cellappmgr.ipp"
#endif

/// CellAppMgr Singleton.
BW_SINGLETON_STORAGE( CellAppMgr )


namespace // (anonymous)
{

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

bool g_shouldLoadBalance = true;

} // end namespace (anonymous)

bool CellAppMgr::shouldMetaLoadBalance_ = true;

/**
 *	Constructor.
 */
CellAppMgr::CellAppMgr( Mercury::EventDispatcher & mainDispatcher,
		Mercury::NetworkInterface & interface ) :
	ManagerApp( mainDispatcher, interface ),
	cellApps_( CellAppMgrConfig::updateHertz() ),
	pTimeKeeper_( NULL ),
	baseAppMgr_( interface ),
	dbAppMgr_( interface ),
	dbAppAlpha_( interface ),
	baseAppAddr_( Mercury::Address::NONE ),
	pCellAppMgrViewerServer_( NULL ),
	waitingFor_( READY_ALL ),
	lastCellAppID_( 0 ),
	lastSpaceID_( NULL_SPACE_ID ),
	allowNewCellApps_( true ),
	isRecovering_( false ),
	hasStarted_( false ),
	isShuttingDown_( false ),
	avgCellAppOverloadStartTime_( 0 ),
	maxCellAppOverloadStartTime_( 0 ),
	lastCellAppDeath_( Mercury::Address::NONE ),
	pShutDownHandler_( NULL ),
	cellAppDeathHandlers_(),
	servicesMap_()
{
	baseAppMgr_.channel().isLocalRegular( false );
	baseAppMgr_.channel().isRemoteRegular( false );

	dbAppMgr_.channel().isLocalRegular( false );
	dbAppMgr_.channel().isRemoteRegular( false );

	dbAppAlpha_.channel().isLocalRegular( false );
	dbAppAlpha_.channel().isRemoteRegular( false );
}


/**
 *	Destructor.
 */
CellAppMgr::~CellAppMgr()
{
	loadBalanceTimer_.cancel();
	metaLoadBalanceTimer_.cancel();
	overloadCheckTimer_.cancel();
	gameTimer_.cancel();
	recoveryTimer_.cancel();

	interface_.processUntilChannelsEmpty();

	delete pTimeKeeper_;
	pTimeKeeper_= NULL;

	// Delete all the spaces
	{
		Spaces::iterator spaceIter = spaces_.begin();

		while (spaceIter != spaces_.end())
		{
			delete spaceIter->second;

			spaceIter++;
		}
		spaces_.clear();
	}

	cellApps_.deleteAll();

	interface_.processUntilChannelsEmpty();

	delete pCellAppMgrViewerServer_;
	pCellAppMgrViewerServer_ = NULL;
}


/**
 *	The initialisation method.
 */
bool CellAppMgr::init( int argc, char * argv [] )
{
	if (!this->ManagerApp::init( argc, argv ))
	{
		return false;
	}

	if (!interface_.isGood())
	{
		NETWORK_ERROR_MSG( "CellAppMgr::init: "
			"Failed to create network interface. Unable to proceed.\n" );
		return false;
	}

	bool isRecovery = false;

	for (int i = 0; i < argc; ++i)
	{
		if (strcmp( argv[i], "-recover" ) == 0)
		{
			isRecovery = true;
		}
		else if (strcmp( argv[i], "-machined" ) == 0)
		{
			CONFIG_INFO_MSG( "CellAppMgr::init: Started from machined\n" );
		}
	}

	ReviverSubject::instance().init( &interface_, "cellAppMgr" );


	PROC_IP_INFO_MSG( "Internal address = %s\n", interface_.address().c_str() );
	CONFIG_INFO_MSG( "Is Recovery = %s\n",
		this->isRecovering() ? "True" : "False" );

	loadBalanceTimer_ =
		this->mainDispatcher().addTimer(
				int( Config::loadBalancePeriod() * 1000000.0 ),
				this, (void *)TIMEOUT_LOAD_BALANCE,
				"LoadBalance" );

	if (Config::metaLoadBalancePeriod() > 0.0)
	{
		metaLoadBalanceTimer_ = this->mainDispatcher().addTimer(
				int( Config::metaLoadBalancePeriod() * 1000000.0 ),
				this, (void *)TIMEOUT_META_LOAD_BALANCE,
				"MetaLoadBalance" );
	}

	overloadCheckTimer_ = this->mainDispatcher().addTimer(
				1000000, this, (void *)TIMEOUT_OVERLOAD_CHECK,
				"OverloadCheck" );

	pCellAppMgrViewerServer_ = new CellAppMgrViewerServer(*this);

	pCellAppMgrViewerServer_->startup( this->mainDispatcher(), 0 );

	// set up the watcher stuff
	BW_REGISTER_WATCHER( 0, "cellappmgr", "cellAppMgr",
			mainDispatcher_, this->interface().address() ); 
	// add the watchers
	this->addWatchers();

	Mercury::MachineDaemon::registerBirthListener( interface_.address(),
			CellAppMgrInterface::handleBaseAppMgrBirth, "BaseAppMgrInterface" );

	int numStartupRetries = Config::numStartupRetries();

	Mercury::MachineDaemon::registerBirthListener( interface_.address(),
			CellAppMgrInterface::handleDBAppMgrBirth, "DBAppMgrInterface" );

	Mercury::Address dbAppMgrAddr;
	Mercury::Reason reason = Mercury::MachineDaemon::findInterface(
			"DBAppMgrInterface", /*id*/ 0, dbAppMgrAddr, 0 );

	if (reason != Mercury::REASON_SUCCESS)
	{
		INFO_MSG( "CellAppMgr::init: DBAppMgr not ready yet.\n" );
	}

	dbAppMgr_.addr( dbAppMgrAddr );

	{
		CellAppMgrInterface::registerWithInterface( interface_ );

		Mercury::Reason reason =
			CellAppMgrInterface::registerWithMachined( interface_, 0 );

		if (reason != Mercury::REASON_SUCCESS)
		{
			NETWORK_ERROR_MSG( "CellAppMgr::init: "
					"Unable to register with interface (%s). "
					"Is machined running?\n",
				Mercury::reasonToString( reason ));
			return false;
		}
	}

	{
		Mercury::Address baseAppMgrAddr;
		int reason = Mercury::MachineDaemon::findInterface(
				"BaseAppMgrInterface", 0, baseAppMgrAddr, numStartupRetries );

		if (reason == Mercury::REASON_SUCCESS)
		{
			baseAppMgr_.addr( baseAppMgrAddr );
			this->ready( READY_BASE_APP_MGR );
		}
		else if (reason == Mercury::REASON_TIMER_EXPIRED)
		{
			NETWORK_INFO_MSG( "CellApp::init: BaseAppMgr not ready yet.\n" );
		}
		else
		{
			NETWORK_ERROR_MSG( "CellApp::init: Failed to find BaseAppMgr.\n" );
			return false;
		}
	}

	Mercury::MachineDaemon::registerBirthListener( interface_.address(),
			CellAppMgrInterface::handleCellAppMgrBirth, "CellAppMgrInterface" );
	Mercury::MachineDaemon::registerDeathListener( interface_.address(),
			CellAppMgrInterface::handleCellAppDeath, "CellAppInterface" );

	if (isRecovery)
	{
		this->startRecovery();
	}

	return true;
}


/**
 *	This method adds watchers which are used for debugging to look at data
 *	associated with this Cell App Manager.
 */
void CellAppMgr::addWatchers()
{
	Watcher & rootWatcher = Watcher::rootWatcher();
	this->ServerApp::addWatchers( rootWatcher );

	// Watch what we know about cells in each category

	// OK, we want to make a watcher for the type 'Spaces', but Spaces
	// are vectors of Space*, not of Space. So to do this
	// we first make a vector watcher...
	MapWatcher<Spaces> * pWatchSpaces = new MapWatcher<Spaces>();
	// then we add a 'BaseDereference' watcher to it which passes
	// on all its functions to its child watcher, after dereferencing
	// any base address. Easy!
	pWatchSpaces->addChild( "*", new BaseDereferenceWatcher(
		Space::pWatcher() ) );
	rootWatcher.addChild( "spaces", pWatchSpaces,
		(void*)&this->spaces_ );

	rootWatcher.addChild( "cellApps", cellApps_.pWatcher() );

	rootWatcher.addChild( "forwardTo", new CAForwardingWatcher() );

	// How many spaces we have got
	MF_WATCH( "numSpaces", *this, &CellAppMgr::numSpaces );

	// How many cellApps do we have
	MF_WATCH( "numCellApps", *this, &CellAppMgr::numCellApps );

	// How many cellApps are active(numCell > 0)
	MF_WATCH( "numActiveCellApps", *this, &CellAppMgr::numActiveCellApps );

	// And how many cells are running
	MF_WATCH( "numCells", *this, &CellAppMgr::numCells );

	MF_WATCH( "numSpacesWithLoadedGeometry", *this, 
			&CellAppMgr::numSpacesWithLoadedGeometry );
	
	// Watchers for load-balancing
	MF_WATCH( "loadBalancing/cellsPerSpaceMax", *this,
			&CellAppMgr::cellsPerSpaceMax );
	MF_WATCH( "loadBalancing/cellsPerMultiCellSpaceAvg", *this,
			&CellAppMgr::cellsPerMultiCellSpaceAvg );
	MF_WATCH( "loadBalancing/numPartitions", *this,
			&CellAppMgr::numPartitions );
	MF_WATCH( "loadBalancing/cellAppGroups", *this,
			&CellAppMgr::cellAppGroups );
	MF_WATCH( "loadBalancing/numCells", *this,
			&CellAppMgr::numCells );
	MF_WATCH( "loadBalancing/numSpaces", *this,
			&CellAppMgr::numSpaces );
	MF_WATCH( "loadBalancing/numMultiCellSpaces", *this,
			&CellAppMgr::numMultiCellSpaces );
	MF_WATCH( "loadBalancing/numMultiMachineSpaces", *this,
			&CellAppMgr::numMultiMachineSpaces );
	MF_WATCH( "loadBalancing/machinesPerMultiCellSpaceAvg", *this,
			&CellAppMgr::machinesPerMultiCellSpaceAvg );
	MF_WATCH( "loadBalancing/machinesPerMultiCellSpaceMax", *this,
			&CellAppMgr::machinesPerMultiCellSpaceMax );
	MF_WATCH( "loadBalancing/numMachinePartitions", *this,
			&CellAppMgr::numMachinePartitions );

	// and watch how many avatars we think there are
	MF_WATCH( "numEntities", *this, &CellAppMgr::numEntities );
	MF_WATCH( "cellAppLoad/min", *this, &CellAppMgr::minCellAppLoad );
	MF_WATCH( "cellAppLoad/average", *this, &CellAppMgr::avgCellAppLoad );
	MF_WATCH( "cellAppLoad/max", *this, &CellAppMgr::maxCellAppLoad );

	MF_WATCH( "viewer server port",
			*pCellAppMgrViewerServer_, &CellAppMgrViewerServer::port);
	MF_WATCH( "debugging/shouldLoadBalance", g_shouldLoadBalance );

	MF_WATCH( "debugging/shouldMetaLoadBalance", shouldMetaLoadBalance_ );
	MF_WATCH( "waitingFor", waitingFor_ );
	MF_WATCH( "hasStarted", hasStarted_ );

	/**
	 * This adds a health check watcher to display a decaying average of largest
	 * timeout from the known CellApps.
	 */
	MF_WATCH( "stats/maxCellAppTimeout", cellApps_, &CellApps::maxCellAppTimeout );

	rootWatcher.addChild( "baseAppMgr", Mercury::ChannelOwner::pWatcher(),
		&baseAppMgr_ );

	rootWatcher.addChild( "dbAppAlpha",
		makeWatcher( &Mercury::ChannelOwner::addr ),
		&dbAppAlpha_ );
}


/**
 *	This method initiates a controlled shutdown of the system.
 */
void CellAppMgr::controlledShutDown()
{
	// Stop sending to anonymous channels etc.
	interface_.stopPingingAnonymous();

	ShutDownHandler::start( *this );
}


/**
 *  Watcher helper method for numActiveCellApps(cellApps that has > 0 cells)
 */
inline int CellAppMgr::numActiveCellApps() const
{
	return cellApps_.numActive();
}


/**
 *  Watcher helper method for numCells
 */
inline int CellAppMgr::numCells() const
{
	int count = 0;
	Spaces::const_iterator cit = spaces_.begin();
	while (cit != spaces_.end())
	{
		count+=cit->second->numCells();
		cit++;
	}
	return count;
}


/**
 *	This method returns the number of cells in the biggest space.
 */
int CellAppMgr::cellsPerSpaceMax() const
{
	int max = 0;

	Spaces::const_iterator iter = spaces_.begin();

	while (iter != spaces_.end())
	{
		if (iter->second->numCells() > max)
		{
			max = iter->second->numCells();
		}

		++iter;
	}

	return max;
}


/**
 *	This method returns the average number of cells per space. It only takes
 *	multi-cell spaces into account.
 */
float CellAppMgr::cellsPerMultiCellSpaceAvg() const
{
	int numSpaces = 0;
	int numCells = 0;

	Spaces::const_iterator iter = spaces_.begin();

	while (iter != spaces_.end())
	{
		int numCellsThisSpace = iter->second->numCells();

		if (numCellsThisSpace >= 2)
		{
			++numSpaces;
			numCells += numCellsThisSpace;
		}

		++iter;
	}

	return (numSpaces > 0) ? float( numCells ) / numSpaces : 0;
}


/**
 *	This method returns the number of cell partitions among all spaces.
 */
int CellAppMgr::numPartitions() const
{
	return this->numCells() - this->numSpaces();
}


/**
 *	Thie method returns a string representation of the CellApp groups with
 *	multiple CellApps.
 */
BW::string CellAppMgr::cellAppGroups() const
{
	CellAppGroups appGroups( cellApps_ );
	return appGroups.asString();
}


/**
 *	This method returns the number of multi-cell spaces.
 */
int CellAppMgr::numMultiCellSpaces() const
{
	int count = 0;

	Spaces::const_iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		if (spaceIter->second->cells().size() > 1)
		{
			++count;
		}

		++spaceIter;
	}

	return count;
}


/**
 *	This method returns the number of spaces that have CellApps on multiple
 *	physical machines.
 */
int CellAppMgr::numMultiMachineSpaces() const
{
	int count = 0;

	Spaces::const_iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		if (spaceIter->second->numUniqueIPs() > 1)
		{
			++count;
		}

		++spaceIter;
	}

	return count;
}


/**
 *	This method returns the average number of distinct physical machines over
 *	which a space's cells are spread. It only takes multi-cell spaces into
 *	consideration.
 */
float CellAppMgr::machinesPerMultiCellSpaceAvg() const
{
	int machinesCount = 0;
	int spaceCount = 0;

	Spaces::const_iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		const Space & space = *spaceIter->second;

		if (space.cells().size() >= 2)
		{
			uint numMachines = space.numUniqueIPs();

			machinesCount += numMachines;
			++spaceCount;
		}
		++spaceIter;
	}

	return (spaceCount != 0) ? float( machinesCount )/spaceCount : 0.f;
}


/**
 *	This method returns the maximum number of distinct physical machines over
 *	which a space's cells are spread. It only takes multi-cell spaces into
 *	consideration.
 */
int CellAppMgr::machinesPerMultiCellSpaceMax() const
{
	uint maxMachines = 0;

	Spaces::const_iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		uint numMachines = spaceIter->second->numUniqueIPs();
		maxMachines = std::max( numMachines, maxMachines );

		++spaceIter;
	}

	return maxMachines;
}


/**
 *	This method returns the number of splits of all spaces over multiple
 *	machines. For example, a space that has cells on CellApps over three unique
 *	physical machines will have two machine partitions.
 */
int CellAppMgr::numMachinePartitions() const
{
	int cumulativeMachinesPerSpace = 0;

	Spaces::const_iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		cumulativeMachinesPerSpace += spaceIter->second->numUniqueIPs();

		++spaceIter;
	}

	return cumulativeMachinesPerSpace - numSpaces();
}


/**
 *	This method returns the number of entities that the Cell App Manager thinks
 *	are in the system.
 */
int CellAppMgr::numEntities() const
{
	return cellApps_.numEntities();
}


/**
 *	This method returns the minimum load on the active cell apps.
 */
float CellAppMgr::minCellAppLoad() const
{
	return cellApps_.minLoad();
}


/**
 *	This method returns the average load on the active cell apps.
 */
float CellAppMgr::avgCellAppLoad() const
{
	return cellApps_.avgLoad();
}


/**
 *	This method returns the average load on the active cell apps.
 */
float CellAppMgr::maxCellAppLoad() const
{
	return cellApps_.maxLoad();
}


// -----------------------------------------------------------------------------
// Section: Helper methods
// -----------------------------------------------------------------------------

/**
 *	Objects of this type handle createEntity reply messages from the cell, and
 *	forward them back to the requestor.
 */
class CreateEntityReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	CreateEntityReplyHandler( const Mercury::Address & addr,
			Mercury::ReplyID replyID ) :
		srcAddr_( addr ),
		replyID_( replyID )
	{
	}

private:
	void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data, void * /*arg*/ )
	{
		EntityID id;
		data >> id;

		if (id == NULL_ENTITY_ID)
		{
			WARNING_MSG( "CreateEntityReplyHandler::handleMessage: "
					"Error on cell %s\n", source.c_str() );
		}

		if (replyID_ != Mercury::REPLY_ID_NONE)
		{
			Mercury::ChannelSender sender( CellAppMgr::getChannel( srcAddr_ ) );
			Mercury::Bundle & bundle = sender.bundle();

			bundle.startReply( replyID_ );
			bundle << id;
		}

		delete this;
	}

	void handleException( const Mercury::NubException & ne, void* arg )
	{
		WARNING_MSG( "CreateEntityReplyHandler::handleException: "
					"Received exception (%s)\n",
				Mercury::reasonToString( ne.reason() ) );

		Mercury::ChannelSender sender( CellAppMgr::getChannel( srcAddr_ ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startReply( replyID_ );
		bundle << NULL_ENTITY_ID;

		delete this;
	}

	void handleShuttingDown( const Mercury::NubException & ne, void* arg )
	{
		INFO_MSG( "CreateEntityReplyHandler::handleShuttingDown: Ignoring\n" );
		delete this;
	}

	Mercury::Address 	srcAddr_;
	Mercury::ReplyID	replyID_;
};


// -----------------------------------------------------------------------------
// Section: Methods that handle messages
// -----------------------------------------------------------------------------

/**
 *	This method creates a new entity on the system. It finds the appropriate
 *	cell to create the entity on and does so.
 */
void CellAppMgr::createEntityInNewSpace( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	bool doesSpaceHavePreferredIP;

	data >> doesSpaceHavePreferredIP;

	uint32 preferredIP = (doesSpaceHavePreferredIP ? srcAddr.ip : 0);

	if (doesSpaceHavePreferredIP)
	{
		TRACE_MSG( "CellAppMgr::createEntityInNewSpace: "
					"Creating space with preferred IP %s\n",
				srcAddr.ipAsString() );
	}

	Space * pSpace = new Space( this->generateSpaceID(),
		/*isNewSpace*/ true, /*isFromDB*/ false,
		preferredIP );
	if (pSpace->addCell())
	{
		this->addSpace( pSpace );
	}
	else
	{
		ERROR_MSG( "CellAppMgr::createEntityInNewSpace: "
				"Unable to add a cell to space %u.\n", pSpace->id() );
		bw_safe_delete( pSpace );
	}

	//passing pSpace==NULL is needed here to send the errors (and is safe)
	this->createEntityCommon( pSpace, srcAddr, header, data );
}


/**
 *	This method checks whether there needs to be any migration of spaces between
 *	cell applications. If so, it will take action.
 */
void CellAppMgr::metaLoadBalance()
{
	// Identify the CellApp groups used in meta-load-balancing. These are the
	// groups of CellApps such that normal load-balancing can balance their
	// loads.
	CellAppGroups appGroups( cellApps_ );

	const float mergeThreshold =
		this->avgCellAppLoad() + CellAppMgrConfig::metaLoadBalanceTolerance();

	// Do meta-balance groups need to be joined?
	appGroups.checkForOverloaded( mergeThreshold );

	// Should more CellApps be added to help with loading?
	bool hasLoadingSpaces = this->checkLoadingSpaces();

	if (!hasLoadingSpaces)
	{
		// Are there underloaded groups who should have Cells retired?
		appGroups.checkForUnderloaded( CellAppLoadConfig::lowerBound() );
	}
}


/**
 *	This method performs load balancing on each of the spaces.
 */
void CellAppMgr::loadBalance()
{
	// Balance all spaces
	{
		Spaces::iterator iter = spaces_.begin();

		while (iter != spaces_.end())
		{
			if (g_shouldLoadBalance)
			{
				iter->second->loadBalance();
			}
			else
			{
				iter->second->informCellAppsOfGeometry( /*shouldSend*/ false );
			}

			iter++;
		}
	}

	// This is done after balancing each space so that the messages to each
	// CellApp are aggregated into a single bundle.
	cellApps_.sendToAll();
}


/**
 *	This method performs load balancing on each of the spaces.
 */
void CellAppMgr::updateRanges()
{
	// Balance all spaces
	{
		Spaces::iterator iter = spaces_.begin();

		while (iter != spaces_.end())
		{
			iter->second->updateRanges();

			iter++;
		}
	}

	// This is done after balancing each space so that the messages to each
	// CellApp are aggregated into a single bundle.
	cellApps_.sendToAll();
}


/**
 *	This method checks whether there are any spaces that are loading geometry
 *	that could benefit from more loading cells.
 */
bool CellAppMgr::checkLoadingSpaces()
{
	bool hasLoadingSpaces = false;

	Spaces::iterator spaceIter = spaces_.begin();

	while (spaceIter != spaces_.end())
	{
		Space * pSpace = spaceIter->second;

		// Work out whether more cells are needed to help do the initial chunk
		// loading.
		bool isLoading = !pSpace->hasLoadedRequiredChunks();
		hasLoadingSpaces |= isLoading;

		// There is an upper bound on the number of loading cells for a space
		// and the minimum size of these loading cells.
		bool needsMoreLoadingCells = isLoading &&
			(pSpace->numCells() < Config::maxLoadingCells()) &&
			(pSpace->spaceBounds().area()/pSpace->numCells() >
				 Config::minLoadingArea());

		if (needsMoreLoadingCells)
		{
			CellData * pCell = pSpace->addCell();

			if (pCell)
			{
				INFO_MSG( "CellAppMgr::checkLoadingSpaces: "
						"Added CellApp %u to Space %u.\n",
					pCell->cellApp().id(), pSpace->id() );

			}
		}

		++spaceIter;
	}

	return hasLoadingSpaces;
}


/**
 *	This method checks if we should report the CellApps as overloaded.
 */
void CellAppMgr::overloadCheck()
{
	// we need to call both isAvgOverloaded() and areAnyOverloaded()
	// every time in order to calculate timeouts correctly
	bool isAvgOverloaded = this->calculateOverload(
		cellApps_.isAvgOverloaded(),
		LoginConditionsConfig::avgOverloadTolerancePeriodInStamps(),
		avgCellAppOverloadStartTime_,
		"Average CellApp load has been exceeded" );

	bool areAnyOverloaded = this->calculateOverload(
		cellApps_.areAnyOverloaded(),
		LoginConditionsConfig::maxOverloadTolerancePeriodInStamps(),
		maxCellAppOverloadStartTime_,
		"Maximum CellApp load has been exceeded" );

	if (this->dbAppAlpha().channel().isEstablished())
	{
		// tell the DBApp whether cells are overloaded
		Mercury::Bundle & bundle = this->dbAppAlpha().bundle();
		DBAppInterface::cellAppOverloadStatusArgs::start(
								bundle ).hasOverloadedCellApps =
										areAnyOverloaded || isAvgOverloaded;
		this->dbAppAlpha().send();
	}
}


/**
 *	This method checks if CellApps overload exceeds tolerance period.
 */
bool CellAppMgr::calculateOverload( bool areCellAppsOverloaded,
									uint64 tolerancePeriodInStamps,
									uint64 &overloadStartTime,
									const char *warningMsg )
{
	bool isOverloaded = areCellAppsOverloaded;

	if (isOverloaded)
	{
		// Has it been overloaded for long enough?

		// Start rate limiting logins
		if (overloadStartTime == 0)
		{
			overloadStartTime = timestamp();
		}

		uint64 overloadTime = timestamp() - overloadStartTime;
		INFO_MSG( "CellAppMgr::overloadCheck: "
					"%s for %0.2f seconds. Min: %.2f. Avg: %.2f. Max: %.2f\n",
				warningMsg, overloadTime/stampsPerSecondD(),
				this->minCellAppLoad(),
				this->avgCellAppLoad(),
				this->maxCellAppLoad() );

		isOverloaded = (overloadTime >= tolerancePeriodInStamps);
	}
	else
	{
		overloadStartTime = 0;
	}

	return isOverloaded;
}

/**
 *	This method creates a new entity on the system. It finds the appropriate
 *	cell to create the entity on and does so.
 *
 *	@todo Currently adds the entity to the first space but the correct space
 *			will need to be specified in the message.
 */
void CellAppMgr::createEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	SpaceID spaceID;
	data >> spaceID;
	Space * pSpace = this->findSpace( spaceID );

	if (pSpace == NULL)
	{
		ERROR_MSG( "CellAppMgr::createEntity: Invalid space id %u\n", spaceID );
		// Rely on createEntityCommon to send the error reply.
	}
	else
	{
		pSpace->hasHadEntities( true );
	}

	this->createEntityCommon( pSpace, srcAddr, header, data );
}


/**
 *	This private method is used by createEntity and createSpace to implement
 *	their common functionality.
 */
void CellAppMgr::createEntityCommon( Space * pSpace,
		const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	Mercury::ChannelVersion channelVersion = Mercury::SEQ_NULL;
	data >> channelVersion;
	bool isRestore;
	data >> isRestore;

	StreamHelper::AddEntityData entityData;
	StreamHelper::removeEntity( data, entityData );

	const Vector3 & pos = entityData.position;
	CellData * pCellData = pSpace ? pSpace->findCell( pos.x, pos.z ) : NULL;

	if (pCellData)
	{
		Mercury::Bundle & bundle = pCellData->cellApp().bundle();

		bundle.startRequest( CellAppInterface::createEntity,
			new CreateEntityReplyHandler( srcAddr, header.replyID ) );

		bundle << pCellData->space().id();
		bundle << channelVersion;

		bundle << isRestore;
		StreamHelper::addEntity( bundle, entityData );

		bundle.transfer( data, data.remainingLength() );

		pCellData->cellApp().send();
	}
	else
	{
		ERROR_MSG( "CellAppMgr::createEntity: "
					"No cell found to place entity\n" );
		data.finish();

		Mercury::ChannelSender sender( CellAppMgr::getChannel( srcAddr ) );
		Mercury::Bundle & bundle = sender.bundle();

		bundle.startReply( header.replyID );
		bundle << NULL_ENTITY_ID;
	}
}


/**
 *	This method handles a message from the database to restore spaces that have
 *	been previously saved.
 */
void CellAppMgr::prepareForRestoreFromDB( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	MF_ASSERT( !hasStarted_ );

	data >> time_;

	int numSpaces;
	data >> numSpaces;

	INFO_MSG( "Restore from database: gametime=%.1f, "
			"numSpaces = %d\n", this->gameTimeInSeconds(), numSpaces );

	for (int i = 0; i < numSpaces; ++i)
	{
		SpaceID spaceID;
		data >> spaceID;

		Space * pSpace = new Space( this->generateSpaceID( spaceID ),
			/*isNewSpace*/ true, /*isFromDB*/ true );

		// Read the space data
		{
			int numData;
			data >> numData;

			while (numData > 0)
			{
				MF_ASSERT( !data.error() );
				SpaceEntryID spaceEntryID;
				uint16 key;
				BW::string spaceData;

				data >> spaceEntryID;
				data >> key;
				data >> spaceData;

				pSpace->addData( spaceEntryID, key, spaceData, 
					/* possibleRepeat */ false,
					/* setLastMappedGeometry */ true );

				--numData;
			}
		}
		if (pSpace->addCell())
		{
			this->addSpace( pSpace );
		}
		else
		{
			ERROR_MSG( "CellAppMgr::prepareForRestoreFromDB: "
					"Unable to add a cell to space %u.\n", pSpace->id() );
			bw_safe_delete( pSpace );
		}
	}

	// Send game time to CellApps
	cellApps_.sendGameTime( time_ );
}


/**
 *	This method shuts down this application.
 */
void CellAppMgr::shutDown( bool shutDownOthers )
{
	INFO_MSG( "CellAppMgr::shutDown: shutDownOthers = %d\n", shutDownOthers );
	if (shutDownOthers)
	{
		cellApps_.shutDownAll();
	}


	INFO_MSG( "CellAppMgr::shutDown: Told to shut down. shutDownOthers = %d\n",
			shutDownOthers );
	isShuttingDown_ = true;
	this->mainDispatcher().breakProcessing();
}


/**
 *	This method is called to shut down the CellAppMgr application.
 */
void CellAppMgr::shutDown( const CellAppMgrInterface::shutDownArgs & /*args*/ )
{
	this->shutDown( true );
}


/**
 *	This method handles a message to start shutting down in a controlled way.
 */
void CellAppMgr::controlledShutDown(
			const CellAppMgrInterface::controlledShutDownArgs & args )
{
	INFO_MSG( "CellAppMgr::controlledShutDown: stage = %s\n", 
		ServerApp::shutDownStageToString( args.stage ) );

	switch (args.stage)
	{
		case SHUTDOWN_TRIGGER:
			this->triggerControlledShutDown();
			break;

		case SHUTDOWN_REQUEST:
			this->controlledShutDown();
			break;

		default:
			ERROR_MSG( "CellAppMgr::controlledShutDown: "
					"Stage %s not handled.\n", 
				ServerApp::shutDownStageToString( args.stage ) );
			break;
	}
}


/**
 *	This method finds the space with the input id. If none is found, NULL is
 *	returned.  If we ask for the default space and it hasn't been created yet,
 *	it will be created here.
 */
Space * CellAppMgr::findSpace( SpaceID id )
{
	// If we don't have any spaces, maybe we should be creating the default
	// space.
	if (spaces_.empty() && id == 1 && !isRecovering_)
	{
		if (Config::useDefaultSpace())
		{
			INFO_MSG( "CellAppMgr::findSpace: Creating default space.\n" );
			Space * pSpace = new Space( this->generateSpaceID( 1 ) );
			if (pSpace->addCell())
			{
				this->addSpace( pSpace );
			}
			else
			{
				ERROR_MSG( "CellAppMgr::findSpace: "
						"Unable to add a cell to space %u.\n", pSpace->id() );
				bw_safe_delete( pSpace );
			}
			return pSpace;
		}
		else
		{
			// Skip SpaceID 1 if we do not have a default space.
			lastSpaceID_ = std::max( SpaceID(1), lastSpaceID_ );
			return NULL;
		}
	}

	Spaces::const_iterator iter = spaces_.find( id );

	Space * pSpace = ( iter == spaces_.end() ) ? NULL : iter->second;

	return pSpace;
}


/**
 *	This method finds the cell application with the input address. If no
 *	application is found, NULL is returned.
 */
CellApp * CellAppMgr::findApp( const Mercury::Address & addr ) const
{
	return cellApps_.find( addr );
}


/**
 *	This method returns a CellApp that is not the input one.
 */
CellApp * CellAppMgr::findAlternateApp( CellApp * pDeadApp ) const
{
	return cellApps_.findAlternateApp( pDeadApp );
}



/**
 * 	This method enables or disables offloading of entities on all cells.
 */
void CellAppMgr::shouldOffload(
		const CellAppMgrInterface::shouldOffloadArgs& args )
{
	Spaces::iterator iter = spaces_.begin();

	while (iter != spaces_.end())
	{
		iter->second->shouldOffload( args.enable );
		iter++;
	}
}


/**
 *	This method adds a Cell Application to the list of known cell applications.
 */
void CellAppMgr::addApp( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	bool shouldAdd = true;
	bool shouldRetry = true;

	if (baseAppAddr_ == Mercury::Address::NONE)
	{
		// Don't let CellApps join until we know about at least one BaseApp.
		PROC_IP_INFO_MSG( "CellAppMgr::addApp: "
			"Not adding CellApp at %s because we don't have a BaseApp yet\n",
			srcAddr.c_str() );
		shouldAdd = false;
	}

	if (shouldAdd && (dbAppAlpha_.addr() == Mercury::Address::NONE))
	{
		// Don't let CellApps join until we know which DBApp is the Alpha.
		PROC_IP_INFO_MSG( "CellAppMgr::addApp: "
			"Not adding CellApp at %s because we don't have a DBApp yet\n",
			srcAddr.c_str() );
		shouldAdd = false;
	}

	Mercury::Address addr;
	uint16 viewerPort;
	data >> addr >> viewerPort;

	MF_ASSERT( addr == srcAddr );

#ifdef BW_EVALUATION
	if (shouldAdd && !cellApps_.empty())
	{
		WARNING_MSG( "CellAppMgr::addApp: Denying CellApp %s. "
				"The Evaluation build only allows one cell app.\n",
			addr.c_str() );

		shouldAdd = false;
		shouldRetry = false;
	}
	else
#endif
	if (shouldAdd && !allowNewCellApps_)
	{
		PROC_IP_WARNING_MSG( "CellAppMgr::addApp: "
			"Denying %s since in middle of update\n", addr.c_str() );

		shouldAdd = false;
		shouldRetry = false;
	}

	if (shouldAdd && this->isRecovering())
	{
		PROC_IP_WARNING_MSG( "CellAppMgr::addApp: "
			"Denying %s since in middle recovery.\n", addr.c_str() );

		shouldAdd = false;
		shouldRetry = false;
	}

	CellAppInitData initData;

	if (!shouldAdd)
	{
		Mercury::ChannelSender sender( CellAppMgr::getChannel( addr ) );
		Mercury::Bundle & bundle = sender.bundle();
		bundle.startReply( header.replyID );

		if (!shouldRetry)
		{
			initData.id = -1; // reader just checks id == -1 as a sign of error
			bundle << initData;
		}

		return;
	}

	this->ready( READY_CELL_APP );

	initData.id = ++lastCellAppID_;
	initData.time = this->time();
	initData.baseAppAddr = baseAppAddr_;
	initData.dbAppAlphaAddr = dbAppAlpha_.addr();
	initData.isReady = hasStarted_;
	initData.timeoutPeriod = Config::cellAppTimeout();

	PROC_IP_INFO_MSG( "CellAppMgr::addApp: %u %s\n",
		initData.id, addr.c_str() );

	CellApp * pApp = cellApps_.add( addr, viewerPort, initData.id );

	pendingApps_.push_back( pApp );

	Mercury::Bundle & bundle = pApp->bundle();
	bundle.startReply( header.replyID );
	bundle << initData;


	// Send shared cell data and global data
	if (!sharedCellAppData_.empty())
	{
		SharedData::iterator iter = sharedCellAppData_.begin();

		while (iter != sharedCellAppData_.end())
		{
			bundle.startMessage( CellAppInterface::setSharedData );
			bundle << SharedDataType( SHARED_DATA_TYPE_CELL_APP ) <<
				iter->first << iter->second;
			++iter;
		}
	}

	if (!sharedGlobalData_.empty())
	{
		SharedData::iterator iter = sharedGlobalData_.begin();

		while (iter != sharedGlobalData_.end())
		{
			bundle.startMessage( CellAppInterface::setSharedData );
			bundle << SharedDataType( SHARED_DATA_TYPE_GLOBAL ) <<
				iter->first << iter->second;
			++iter;
		}
	}

	if (!servicesMap_.empty())
	{
		ServicesMap::const_iterator iFragment = servicesMap_.begin();

		while (iFragment != servicesMap_.end())
		{
			bundle.startMessage( CellAppInterface::addServiceFragment );
			bundle << iFragment->first << iFragment->second;
			++iFragment;
		}
	}

	pApp->send();
}


/**
 *	This method lets the CellAppMgr know about a CellApp when it is recovering.
 */
void CellAppMgr::recoverCellApp( BinaryIStream & data )
{
	// If we get this message, we should have been started with -recover.
	// Here, we handle the situation where this was not true.
	if (!this->isRecovering())
	{
		if (!cellApps_.empty())
		{
			WARNING_MSG( "CellAppMgr::recoverCellApp: "
					"Recovering when we were not started with -recover\n" );
			// TODO: Only needed if we added a default space
			Spaces::iterator iter = spaces_.begin();

			while (iter != spaces_.end())
			{
				delete iter->second;
				++iter;
			}
			spaces_.clear();
		}

		this->startRecovery();
	}

	Mercury::Address addr;
	uint16 viewerPort;
	int32 cellAppID;
	GameTime time;

	// We could get the address from the sender info but we're putting it on
	// the stream.
	data >> addr >> viewerPort >> cellAppID >> time;

	NETWORK_INFO_MSG( "CellAppMgr::recoverCellApp: %s. CellApp = %u\n",
			addr.c_str(), cellAppID );

	CellApp * pCellApp = cellApps_.find( addr );

	if (!pCellApp)
	{
		// TODO: Do better with the time.
		time_ = time;
		lastCellAppID_ = std::max( cellAppID, lastCellAppID_ );

		pCellApp = cellApps_.add( addr, viewerPort, cellAppID );
	}
	else
	{
		ERROR_MSG( "CellAppMgr::recoverCellApp: Already recovered %s\n",
			addr.c_str() );
		return;
	}

	// Read all of the shared CellApp data
	{
		uint32 numEntries;
		data >> numEntries;

		BW::string key;
		BW::string value;

		for (uint32 i = 0; i < numEntries; ++i)
		{
			data >> key >> value;
			sharedCellAppData_[ key ] = value;
		}
	}

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

	// Read all of the space information that this Cell Application has.
	{
		uint32 numSpaces;
		data >> numSpaces;

		for (uint32 spaceIndex = 0; spaceIndex < numSpaces; ++spaceIndex)
		{
			if (data.remainingLength() == 0)
			{
				// TODO: This may occur if we have spaces that do not have
				// cells.
				WARNING_MSG( "CellAppMgr::recoverCellApp: "
						"Only read %u of %u spaces. "
						"Probably due to a space with no cell.\n",
					spaceIndex, numSpaces );
				break;
			}

			// Find or create the space.
			SpaceID spaceID;
			data >> spaceID;
			Space * pSpace = this->findSpace( spaceID );

			bool isNewSpace = false;

			if (pSpace == NULL)
			{
				pSpace = new Space( this->generateSpaceID( spaceID ), false );
				this->addSpace( pSpace );
				isNewSpace = true;
			}

			// Initially, we assume that we don't have any geometry mappings
			// that haven't finished loading when we died. But if one of our
			// CellApps tells us that there were, then we believe them.
			BW::string lastMappedGeometry;
			data >> lastMappedGeometry;
			if (!lastMappedGeometry.empty())
				pSpace->setLastMappedGeometry( lastMappedGeometry );

			// Read the space data
			uint32 dataSize;
			data >> dataSize;

			for (uint32 dataIndex = 0; dataIndex < dataSize; ++dataIndex)
			{
				SpaceEntryID spaceEntryID;
				uint16 key;
				BW::string spaceData;
				data >> spaceEntryID >> key >> spaceData;
				pSpace->addData( spaceEntryID, key, spaceData, 
						/* possibleRepeat */ !isNewSpace,
						/* setLastMappedGeometry */ false );
			}

			// Each cell application has a full description of the BSP of
			// each space that it has.
			// Initially, all of the cells in the space are marked as not
			// being recovered. As the different applications are recovered,
			// this mark is cleared.

			pSpace->recoverCells( data );

			// Let the space know that this cell application is back.
			pSpace->recoverCell( *pCellApp, addr );
		}
	}
}


/**
 *	This method removes a Cell Application from the list of known cell
 *	applications.
 */
void CellAppMgr::delApp( const CellAppMgrInterface::delAppArgs & args )
{
	cellApps_.delApp( args.addr );
}


/**
 *	This method is called by the BaseAppMgr to inform us of the address of a
 *	base application.
 */
void CellAppMgr::setBaseApp( const CellAppMgrInterface::setBaseAppArgs & args )
{
	// TRACE_MSG( "CellAppMgr::setBaseApp: addr = %s\n", args.addr.c_str() );
	// MF_ASSERT( baseAppAddr_.ip == 0 );

	baseAppAddr_ = args.addr;

	if (waitingFor_ == 0)
	{
		cellApps_.setBaseApp( baseAppAddr_ );
	}
	else
	{
		this->ready( READY_BASE_APP );
	}
}


/**
 *	This method is called to inform the Cell App Manager that another Cell App
 *	Manager has been started.
 */
void CellAppMgr::handleCellAppMgrBirth(
				const CellAppMgrInterface::handleCellAppMgrBirthArgs & args )
{
	INFO_MSG( "CellAppMgr::handleCellAppMgrBirth: %s\n", args.addr.c_str() );

	if (args.addr != interface_.address())
	{
		this->shutDown( false );
	}
}


/**
 *	This method is called to inform the Cell App Manager that a new Base App
 *	Manager has been started.
 */
void CellAppMgr::handleBaseAppMgrBirth(
				const CellAppMgrInterface::handleBaseAppMgrBirthArgs & args )
{
	NETWORK_INFO_MSG( "CellAppMgr::handleBaseAppMgrBirth: %s\n",
		args.addr.c_str() );
	baseAppMgr_.addr( args.addr );

	// Should already be ready but just in case.
	this->ready( READY_BASE_APP_MGR );
}


/**
 *	This method is called to inform the Cell App Manager that a Cell App has
 *	died unexpectedly.
 */
void CellAppMgr::handleCellAppDeath( const Mercury::Address & srcAddr,
	   		const Mercury::UnpackedMessageHeader & header,
			const CellAppMgrInterface::handleCellAppDeathArgs & args )
{
	if (isShuttingDown_)
	{
		return;
	}

	CellApp * pApp = this->findApp( srcAddr );
	if (pApp != NULL)
	{
		WARNING_MSG( "CellAppMgr::handleCellAppDeathArgs: "
				"From CellApp %u %s\n", pApp->id(), pApp->addr().c_str() );

	}
	else
	{
		WARNING_MSG( "CellAppMgr::handleCellAppDeathArgs: "
				"From %s (machined?)\n", srcAddr.c_str() );
	}

	this->handleCellAppDeath( args.addr );
}


/**
 *	This method is called from a CellApp that has received and processed a
 *	handleCellAppDeath message.
 */
void CellAppMgr::ackCellAppDeath( const Mercury::Address & srcAddr,
	   		const Mercury::UnpackedMessageHeader & header,
			const CellAppMgrInterface::ackCellAppDeathArgs & args )
{
	CellAppDeathHandlers::iterator iter =
		cellAppDeathHandlers_.find( args.deadAddr );

	if (iter != cellAppDeathHandlers_.end())
	{
		// WARNING: This call may delete the handler.
		iter->second->clearWaiting( srcAddr, args.deadAddr );
	}
	else
	{
		BW::string srcAddrStr = srcAddr.c_str();
		ERROR_MSG( "CellAppMgr::ackCellAppDeath: No handler for %s from %s\n",
				args.deadAddr.c_str(), srcAddrStr.c_str() );
	}
}


/**
 *	This method handles a message from the BaseAppMgr informing us that a
 *	BaseApp has died. All the CellApps will now be informed.
 */
void CellAppMgr::handleBaseAppDeath( BinaryIStream & data )
{
	INFO_MSG( "CellAppMgr::handleBaseAppDeath:\n" );

	int length = data.remainingLength();
	const void * pBlob = data.retrieve( length );

	// Let all the CellApps know about it.
	cellApps_.handleBaseAppDeath( pBlob, length );

	// Read off the dead address and update our service map accordingly, in
	// case it was a ServiceApp.
	MemoryIStream temp( pBlob, length );
	Mercury::Address deadBaseAppAddress;
	uint8 isService = 0;
	temp >> deadBaseAppAddress >> isService;
	temp.finish();

	if (isService)
	{
		servicesMap_.removeFragmentsForAddress( deadBaseAppAddress );
	}

	// Let DBAppMgr know about it so it can notify DBApps to re-map.
	if (dbAppMgr_.channel().isEstablished())
	{
		Mercury::Bundle & bundle = dbAppMgr_.bundle();
		bundle.startMessage( DBAppMgrInterface::handleBaseAppDeath );
		bundle.addBlob( pBlob, length );
		dbAppMgr_.send();
	}
	else
	{
		WARNING_MSG( "CellAppMgr::handleBaseAppDeath: "
			"Not informing DBAppMgr since none known at the moment\n" );
	}
}


/**
 *	This method handles when a DBAppMgr has started.
 */
void CellAppMgr::handleDBAppMgrBirth(
		const CellAppMgrInterface::handleDBAppMgrBirthArgs & args )
{
	dbAppMgr_.addr( args.addr );
}


/**
 *	This method handles DBAppMgr telling us where the DBApp Alpha is.
 */
void CellAppMgr::setDBAppAlpha( 
		const CellAppMgrInterface::setDBAppAlphaArgs & args )
{
	dbAppAlpha_.addr( args.addr );

	cellApps_.setDBAppAlpha( args.addr );
}


/**
 *	This method is used to identify when all components are ready.
 */
void CellAppMgr::ready( int component )
{
	// The Cell App Manager is ready once it has a Cell App, Base App and the
	// Base App Manager.

	// Will this make it 0?
	if (component == waitingFor_)
	{
		if (!isRecovering_)
		{
			INFO_MSG( "CellAppMgr is ready to start.\n" );
		}
		else
		{
			this->startTimer();
		}
	}

	waitingFor_ &= ~component;
}


/**
 *	This method tells us to start.
 */
void CellAppMgr::startup( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	this->startTimer();

	// This forces the default space to be created if it isn't already.
	this->findSpace( 1 );

	INFO_MSG( "CellAppMgr is starting\n" );

	// Tell the cellApps to start
	cellApps_.startAll( baseAppAddr_ );
}


/**
 *	This method starts the timer.
 */
void CellAppMgr::startTimer()
{
	MF_ASSERT( !hasStarted_ );

	gameTimer_ = this->mainDispatcher().addTimer(
		1000000/Config::updateHertz(), this, (void*)TIMEOUT_GAME_TICK,
		"GameTick" );
	pTimeKeeper_ = new TimeKeeper( interface_, gameTimer_,
		time_, Config::updateHertz() );
	hasStarted_ = true;
}


/**
 *	This method triggers a controlled shutdown of the cluster.
 */
void CellAppMgr::triggerControlledShutDown()
{
	Mercury::Bundle & bundle = baseAppMgr_.bundle();
	BaseAppMgrInterface::controlledShutDownArgs &args =
		BaseAppMgrInterface::controlledShutDownArgs::start( bundle );
	args.stage = SHUTDOWN_TRIGGER;
	args.shutDownTime = 0; // unused on receiving side

	baseAppMgr_.send();
}

/**
 *	This method handles a cell application dying unexpectedly.  The address arg
 *	to this method is deliberately not a reference because the CellApp object
 *	can be deleted halfway through this, taking its channel and address with it.
 */
void CellAppMgr::handleCellAppDeath( Mercury::Address addr )
{
	if (!baseAppMgr_.channel().isEstablished())
	{
		ERROR_MSG( "CellAppMgr::handleCellAppDeath: "
			"BaseAppMgr was not registered.\n" );
		return;
	}

	CellApp * pDeadApp = cellApps_.find( addr );

	if (pDeadApp == NULL)
	{
		if (lastCellAppDeath_ != addr)
		{
			WARNING_MSG( "CellAppMgr::handleCellAppDeath: Could not find %s. "
				"(It may already have been removed)\n",
				addr.c_str() );
		}
		else
		{
			INFO_MSG( "CellAppMgr::handleCellAppDeath: %s already killed.\n",
					addr.c_str() );
		}
		return;
	}

	INFO_MSG( "CellAppMgr::handleCellAppDeath: Sending SIGQUIT to %s\n",
		addr.c_str() );
	if (!Mercury::MachineDaemon::sendSignalViaMachined( 
			addr, SIGQUIT ))
	{
		ERROR_MSG( "CellAppMgr::handleCellAppDeath: Failed to send "
				"SIGQUIT to %s\n", addr.c_str() );
	}

	lastCellAppDeath_ = addr;

	if (cellAppDeathHandlers_.find( addr ) != cellAppDeathHandlers_.end())
	{
		ERROR_MSG( "CellAppMgr::handleCellAppDeath: "
								"Still have handler for %s\n", addr.c_str() );
		return;
	}

	// Let the other handlers know that they may not receive a reply from the
	// CellApp.
	{
		CellAppDeathHandlers::iterator iter = cellAppDeathHandlers_.begin();
		while (iter != cellAppDeathHandlers_.end())
		{
			const Mercury::Address & deadAddr = iter->first;
			CellAppDeathHandler * pHandler = iter->second;
			++iter;

			// WARNING: This call may delete the handler.
			pHandler->clearWaiting( addr, deadAddr );
		}
	}

	// Max time to wait for replies from all CellApps.
	const float MAX_WAIT_ON_CELLAPP_DEATH = 2.f * Config::cellAppTimeout();

	// This object will send the notification to the BaseAppMgr once all
	// CellApps have replied.  This is to ensure that BaseApps do not begin
	// restoring with an incomplete picture of the entity population.
	// See AckCellAppDeathHelper in cellapp/cellapp.cpp for more info.
	CellAppDeathHandler * pCellAppDeathHandler =
		new CellAppDeathHandler( addr,
				Config::secondsToTicks( MAX_WAIT_ON_CELLAPP_DEATH, 1 ) );
	cellAppDeathHandlers_[ addr ] = pCellAppDeathHandler;

	pDeadApp->handleUnexpectedDeath( pCellAppDeathHandler->stream() );

	// Clean up the dead cell application.
	cellApps_.handleCellAppDeath( addr, pCellAppDeathHandler );

	bool shutDownServer = false;

	// If we've run out of CellApps we're in a very bad state, so shut down the
	// server before more hell breaks loose.
	if (cellApps_.empty() && Config::shutDownServerOnBadState())
	{
		ERROR_MSG( "CellAppMgr::handleCellAppDeath: "
			"All CellApps are dead, starting controlled shutdown!\n" );

		shutDownServer = true;
	}
	else if (Config::shutDownServerOnCellAppDeath())
	{
		NOTICE_MSG( "CellAppMgr::handleCellAppDeath: "
			"shutDownServerOnCellAppDeath is enabled. "
			"Shutting down server.\n" );
		shutDownServer = true;
	}

	if (shutDownServer)
	{
		this->triggerControlledShutDown();
	}
}


/**
 *	This method handles a message to set a shared data value. This may be
 *	data that is shared between all CellApps or all CellApps and BaseApps. The
 *	CellAppMgr is the authoritative copy of this information.
 */
void CellAppMgr::setSharedData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	SharedDataType dataType;
	BW::string key;
	BW::string value;

	data >> dataType >> key >> value;

	Mercury::Address originalSrcAddr = srcAddr;

	// Messages from the BaseAppMgr will contain the address of the original
	// sender of the message.
	if (data.remainingLength() == sizeof( Mercury::Address ) )
	{
		data >> originalSrcAddr;
	}

	IF_NOT_MF_ASSERT_DEV( data.remainingLength() == 0 )
	{
		return;
	}

	IF_NOT_MF_ASSERT_DEV( !data.error() )
	{
		return;
	}

	if ((dataType == SHARED_DATA_TYPE_CELL_APP) ||
		(dataType == SHARED_DATA_TYPE_GLOBAL))
	{
		if (dataType == SHARED_DATA_TYPE_CELL_APP)
		{
			sharedCellAppData_[ key ] = value;
		}
		else
		{
			sharedGlobalData_[ key ] = value;
		}

		cellApps_.setSharedData( dataType, key, value, originalSrcAddr );
	}

	if ((dataType == SHARED_DATA_TYPE_BASE_APP) ||
		(dataType == SHARED_DATA_TYPE_GLOBAL))
	{
		Mercury::Bundle & bundle = baseAppMgr_.bundle();
		bundle.startMessage( BaseAppMgrInterface::setSharedData );
		bundle << dataType << key << value << originalSrcAddr;

		baseAppMgr_.send();
	}
}


/**
 *	This method handles a message to delete a shared data value. This may be
 *	data that is shared between all CellApps or all CellApps and BaseApps. The
 *	CellAppMgr is the authoritative copy of this information.
 */
void CellAppMgr::delSharedData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	SharedDataType dataType;
	BW::string key;

	data >> dataType >> key;

	Mercury::Address originalSrcAddr = srcAddr;

	// Messages from the BaseAppMgr will contain the address of the original
	// sender of the message.
	if (data.remainingLength() == sizeof( Mercury::Address ) )
	{
		data >> originalSrcAddr;
	}

	IF_NOT_MF_ASSERT_DEV( data.remainingLength() == 0 )
	{
		return;
	}

	IF_NOT_MF_ASSERT_DEV( !data.error() )
	{
		return;
	}

	if ((dataType == SHARED_DATA_TYPE_CELL_APP) ||
		(dataType == SHARED_DATA_TYPE_GLOBAL))
	{
		// TODO: Check that this is successful.
		if (dataType == SHARED_DATA_TYPE_CELL_APP)
		{
			sharedCellAppData_.erase( key );
		}
		else
		{
			sharedGlobalData_.erase( key );
		}

		cellApps_.delSharedData( dataType, key, originalSrcAddr );
	}

	if ((dataType == SHARED_DATA_TYPE_BASE_APP) ||
		(dataType == SHARED_DATA_TYPE_GLOBAL))
	{
		Mercury::Bundle & bundle = baseAppMgr_.bundle();
		bundle.startMessage( BaseAppMgrInterface::delSharedData );
		bundle << dataType << key << originalSrcAddr;

		baseAppMgr_.send();
	}
}


/**
 *	This method handles a message for adding a service fragment.
 *
 *	@param data 	The message payload.
 */
void CellAppMgr::addServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	EntityMailBoxRef fragmentMailBox;

	data >> serviceName >> fragmentMailBox;

	servicesMap_.addFragment( serviceName, fragmentMailBox );
	cellApps_.addServiceFragment( serviceName, fragmentMailBox );
}


/**
 *	This method handles a message for deleting a service fragment.
 *
 *	@param data 	The message payload.
 */
void CellAppMgr::delServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	Mercury::Address fragmentAddress;

	data >> serviceName >> fragmentAddress;

	servicesMap_.removeFragment( serviceName, fragmentAddress );
	cellApps_.delServiceFragment( serviceName, fragmentAddress );
}


/**
 *	This method handles a message from a cell to update space data.
 */
void CellAppMgr::updateSpaceData( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	const int headerSize =
		sizeof( SpaceID ) + sizeof( SpaceEntryID ) + sizeof( uint16 );
	MF_ASSERT( data.remainingLength() >= headerSize );

	if (data.remainingLength() >= headerSize)
	{
		SpaceID spaceID;
		SpaceEntryID entryID;
		uint16 key;
		data >> spaceID >> entryID >> key;

		Space * pSpace = this->findSpace( spaceID );

		if (pSpace)
		{
			if (key == uint16( -1 ))
			{
				MF_ASSERT( data.remainingLength() == 0 );
				pSpace->delData( entryID );
				pSpace->sendSpaceDataUpdate( srcAddr, entryID, uint16( -1 ),
						NULL );
			}
			else
			{
				uint32 length = data.remainingLength();
				BW::string value( (char*)data.retrieve( length ), length );
				pSpace->addData( entryID, key, value );
				pSpace->sendSpaceDataUpdate( srcAddr, entryID, key, &value );
			}
		}
		else
		{
			ERROR_MSG( "CellAppMgr::updateSpaceData: Could not find space %u\n",
				spaceID );
		}
	}
}


/**
 *	This method handles a message informing us to shut down a space.
 */
void CellAppMgr::shutDownSpace(
		const CellAppMgrInterface::shutDownSpaceArgs & args )
{
	Space * pSpace = this->findSpace( args.spaceID );

	if (pSpace)
	{
		if (pSpace->hasHadEntities())
		{
			// Delay shutting down the space until the end of tick
			//	don't shutdown twice
			if (spacesShuttingDown_.insert( args.spaceID ).second)
			{
				pSpace->shutDown();
			}
		}
		else
		{
			NOTICE_MSG( "CellAppMgr::shutDownSpace: Not shutting down space "
								"%u since it has not had any entities\n",
							pSpace->id() );
		}
	}
	else
	{
		ERROR_MSG( "CellAppMgr::shutDownSpace: Could not find space %u\n",
			args.spaceID );
	}
}


/**
 *	This method is called to acknowledge that the base apps are in a particular
 *	shutdown stage.
 */
void CellAppMgr::ackBaseAppsShutDown(
			const CellAppMgrInterface::ackBaseAppsShutDownArgs & args )
{
	if (pShutDownHandler_)
	{
		pShutDownHandler_->ackBaseApps( args.stage );
	}
}


/**
 *	This request is called to query the status of this process.
 */
void CellAppMgr::checkStatus( const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
{
	IF_NOT_MF_ASSERT_DEV( addr == baseAppMgr_.addr() )
	{
		return;
	}

	Mercury::Bundle & reply = baseAppMgr_.bundle();
	reply.startReply( header.replyID );

	reply << uint8( waitingFor_ == 0 ) << cellApps_.size();

	if (waitingFor_ & READY_CELL_APP)
	{
		reply << "CellAppMgr is waiting for a CellApp";
	}

	if (waitingFor_ & READY_BASE_APP_MGR)
	{
		reply << "CellAppMgr is waiting for the BaseAppMgr";
	}

	if (waitingFor_ & READY_BASE_APP)
	{
		reply << "CellAppMgr is waiting for a BaseApp";
	}

	baseAppMgr_.send();
}


/**
 *	This message handles someone telling us what their game time is and asking
 *	for our version of it, since we are the time master (hmmm time lord?)
 */
void CellAppMgr::gameTimeReading( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const CellAppMgrInterface::gameTimeReadingArgs & args )
{
	double reading = 0.0;

	if (pTimeKeeper_)
	{
		// tell the requester what we think the game time should be....
		reading = pTimeKeeper_->readingNow();
	}
	else
	{
		ERROR_MSG( "CellAppMgr::gameTimeReading: pTimeKeeper_ is NULL.\n" );
		reading = args.gameTimeReadingContribution;
	}

	Mercury::AutoSendBundle autoBundle( this->interface(), srcAddr );
	Mercury::Bundle & bundle = *autoBundle;
	bundle.startReply( header.replyID );
	bundle << reading;
}


// -----------------------------------------------------------------------------
// Section: Private Functions
// -----------------------------------------------------------------------------

/**
 * This method generates space ID or checks validity of the passed space ID
 * @param spaceID space ID to use 
 */
SpaceID CellAppMgr::generateSpaceID( SpaceID spaceID )
{
	// TODO: Get a better scheme for allocating space ids.
	if (spaceID == NULL_SPACE_ID)
	{
		spaceID = ++lastSpaceID_;
	}
	else
	{
		MF_ASSERT( spaces_.find( spaceID ) == spaces_.end() );
		lastSpaceID_ = std::max( spaceID, lastSpaceID_ );
	}

	return spaceID;
}

/**
 *	This method adds a new space to the system.
 */
void CellAppMgr::addSpace( Space * pNewSpace )
{
	spaces_[pNewSpace->id()] = pNewSpace;
}

/**
 *	This method sets the Cell App Manager to be in recovery mode. In this time,
 *	currently running cell apps should inform us about their details.
 */
bool CellAppMgr::startRecovery()
{
	if (!isRecovering_)
	{
		// Should look at how long this should be.
		recoveryTimer_ =
			this->mainDispatcher().addOnceOffTimer( 2000000, // 2 seconds
				this,
				(void *)TIMEOUT_END_OF_RECOVERY );
		isRecovering_ = true;

		this->ready( waitingFor_ );

		return true;
	}

	ERROR_MSG( "CellAppMgr::startRecovery: Already in recovery mode.\n" );
	return false;
}


/**
 *	This method is called at the end of recovery.
 */
void CellAppMgr::endRecovery()
{
	MF_ASSERT( isRecovering_ );

	INFO_MSG( "CellAppMgr::endRecovery: End of Recovery\n" );
	isRecovering_ = false;

	Space::endRecovery();
	this->updateRanges();
}


/**
 *	This method responds to timeout events.
 */
void CellAppMgr::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	if (pShutDownHandler_ &&
			pShutDownHandler_->isPaused())
	{
		pShutDownHandler_->checkStatus();
		return;
	}

	if (isShuttingDown_)
	{
		return;
	}

	switch ((uintptr)arg)
	{
#ifndef BW_EVALUATION
		case TIMEOUT_LOAD_BALANCE:
		{
			if (!this->isRecovering() && hasStarted_)
			{
				this->loadBalance();
			}
			break;
		}

		case TIMEOUT_META_LOAD_BALANCE:
		{
			
			if (shouldMetaLoadBalance_ && hasStarted_)
			{
				this->metaLoadBalance();
			}
			break;
		}
#endif

		case TIMEOUT_OVERLOAD_CHECK:
			if (hasStarted_)
			{
				this->overloadCheck();
			}
			break;

		case TIMEOUT_GAME_TICK:
		{
			this->advanceTime();

			if (Config::archivePeriodInTicks() > 0)
			{
				if ((time_ % Config::archivePeriodInTicks()) == 1)
				{
					// It might be good to spread this out over time?
					this->writeGameTimeToDB();
					this->writeSpacesToDB();
				}
			}

			cellApps_.updateCellAppTimeout();

			this->checkForDeadCellApps();

			{
				CellAppDeathHandlers::iterator iter =
					cellAppDeathHandlers_.begin();
				while (iter != cellAppDeathHandlers_.end())
				{
					CellAppDeathHandler * pCellAppDeathHandler = iter->second;
					++iter;

					// WARNING: This may delete this object.
					pCellAppDeathHandler->tick();
				}
			}

			{
				SpaceIDs::iterator iter = spacesShuttingDown_.begin();
				while (iter != spacesShuttingDown_.end())
				{
					Spaces::iterator found = spaces_.find( *iter );
					if (found != spaces_.end())
					{
						delete found->second;
						spaces_.erase( found );
					}
					++iter;
				}
				spacesShuttingDown_.clear();
			}

			while (!pendingApps_.empty())
			{
				// take it off the queue
				CellApp * pApp = pendingApps_.back();
				pendingApps_.pop_back();

				// add a cell for every space (that we know about) that doesn't
				// have a cell yet. (Should only be needed for first space.
				Spaces::iterator iter = spaces_.begin();

				while (iter != spaces_.end())
				{
					if (iter->second->numCells() == 0)
					{
						iter->second->addCell( *pApp );
					}

					iter++;
				}
			}

			cellApps_.sendCellAppMgrInfo( this->maxCellAppLoad() );

			break;
		}

		case TIMEOUT_END_OF_RECOVERY:
			recoveryTimer_.clearWithoutCancel();
			this->endRecovery();
			break;
	}
}


/**
 *	This method is called periodically to check whether or not any cell
 *	applications have timed out.
 */
void CellAppMgr::checkForDeadCellApps()
{
	CellApp * pDeadApp =
		cellApps_.checkForDeadCellApps( Config::cellAppTimeoutInStamps() );

	if (pDeadApp)
	{
		// Only handle one timeout per check because the above call will
		// likely change the collection we are iterating over.
		INFO_MSG( "CellAppMgr::checkForDeadCellApps: %s has timed out.\n",
				pDeadApp->addr().c_str() );
		this->handleCellAppDeath( pDeadApp->addr() );
	}
}


/**
 *	This method clears the death handler associated with the input address.
 */
void CellAppMgr::clearCellAppDeathHandler( const Mercury::Address & addr )
{
	if (!cellAppDeathHandlers_.erase( addr ))
	{
		ERROR_MSG( "CellAppMgr::clearCellAppDeathHandler: No handler for %s\n",
				addr.c_str() );
	}
}


/**
 *	This method writes space information to the database.
 */
void CellAppMgr::writeSpacesToDB()
{
	if (Config::archivePeriodInTicks() == 0 ||
		!Config::shouldArchiveSpaceData())
	{
		return;
	}

	if (this->dbAppAlpha().channel().isEstablished())
	{
		Mercury::Bundle & bundle = this->dbAppAlpha().bundle();
		bundle.startMessage( DBAppInterface::writeSpaces );
		bundle << uint32( spaces_.size() );

		Spaces::const_iterator iter = spaces_.begin();

		while (iter != spaces_.end())
		{
			iter->second->sendToDB( bundle );
			++iter;
		}

		this->dbAppAlpha().send();
	}
	else
	{
		WARNING_MSG( "CellAppMgr::writeSpacesToDB: "
			"No known DBApp, not writing to DB.\n" );
	}
}


/**
 *	This method writes the game time to the database.
 */
void CellAppMgr::writeGameTimeToDB()
{
	if (this->dbAppAlpha().channel().isEstablished())
	{
		Mercury::Bundle & bundle = this->dbAppAlpha().bundle();
		DBAppInterface::writeGameTimeArgs & rWriteGameTime =
			DBAppInterface::writeGameTimeArgs::start( bundle );

		rWriteGameTime.gameTime = time_;

		this->dbAppAlpha().send();
	}
	else
	{
		WARNING_MSG( "CellAppMgr::writeGameTimeToDB: "
			"No known DBApp, not writing to DB.\n" );
	}
}


/**
 * Predicate that returns the number of spaces in which have loaded
 * their respective mapped geometry.
 *
 * @sa numSpaces()
 * @sa Space::cellsHaveLoadedMappedGeometry()
 */
int CellAppMgr::numSpacesWithLoadedGeometry() const
{
	int count = 0;
	Spaces::const_iterator spaceIter;
	for (spaceIter = spaces_.begin(); spaceIter != spaces_.end(); spaceIter++)
	{
		if (spaceIter->second->cellsHaveLoadedMappedGeometry())
		{
			count++;
		}
	}
	return count;
}

BW_END_NAMESPACE

// cellappmgr.cpp
