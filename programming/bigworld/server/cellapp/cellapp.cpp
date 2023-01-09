#include "script/first_include.hpp"

#include "cellapp.hpp"

#include "ack_cell_app_death_helper.hpp"
#include "add_to_cellappmgr_helper.hpp"
#include "buffered_entity_messages.hpp"
#include "buffered_ghost_messages.hpp"
#include "buffered_input_messages.hpp"
#include "cell.hpp"
#include "cellapp_config.hpp"
#include "cell_app_channels.hpp"
#include "cell_chunk.hpp"
#include "cell_viewer_server.hpp"
#include "cellapp_interface.hpp"
#include "entity.hpp"
#include "entity_population.hpp"
#include "entity_type.hpp"
#include "id_config.hpp"
#include "py_entities.hpp"
#include "real_entity.hpp"
#include "replay_data_collector.hpp"
#include "space.hpp"
#include "spaces.hpp"
#include "witness.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "db/dbapp_interface.hpp"
#include "db/dbapp_interface_utils.hpp"

#include "common/py_network.hpp"
#ifdef BUILD_PY_URLREQUEST
#include "urlrequest/manager.hpp"
#endif
#include "common/space_data_types.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/user_data_object.hpp"
#include "chunk/user_data_object_type.hpp"

#include "chunk_loading/edge_geometry_mapping.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/memory_stream.hpp"

#include "delegate_interface/game_delegate.hpp"

#include "entitydef/constants.hpp"

#include "math/boundbox.hpp"

#include "network/blocking_reply_handler.hpp"
#include "network/bundle_sending_map.hpp"
#include "network/channel_finder.hpp"
#include "network/channel_sender.hpp"
#include "network/condemned_channels.hpp"
#include "network/interface_table.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machined_utils.hpp"
#include "network/portmap.hpp"
#include "network/udp_bundle.hpp"
#include "network/watcher_nub.hpp"

#include "physics2/worldtri.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_traceback.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/pywatcher.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"

#include "server/add_to_manager_helper.hpp"
#include "server/balance_config.hpp"
#include "server/cell_app_init_data.hpp"
#include "server/cell_properties_names.hpp"
#include "server/common.hpp"
#include "server/plugin_library.hpp"
#include "server/python_server.hpp"
#include "server/server_app_config.hpp"
#include "server/shared_data.hpp"
#include "server/shared_data_type.hpp"
#include "server/stream_helper.hpp"
#include "server/time_keeper.hpp"
#include "server/watcher_forwarding.hpp"

#include "terrain/manager.hpp"

#include "waypoint/chunk_navigator.hpp"
#include "waypoint/waypoint_stats.hpp"

#include <algorithm>
#include <sys/types.h>
#include <sys/wait.h> // for waitpid


// #define MF_USE_VALGRIND
#ifdef MF_USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

extern int Math_token;
extern int ResMgr_token;
extern int PyScript_token;
extern int PyChunk_token;
extern int PyPhysics2_token;
extern int PyURLRequest_token;
static int s_moduleTokens =
	Math_token | ResMgr_token | PyScript_token | 
	PyChunk_token | PyPhysics2_token | PyURLRequest_token;

extern int PyUserDataObject_token;
extern int UserDataObjectDescriptionMap_Token;
extern int AppScriptTimers_token;

static int s_tokenSet =
	PyUserDataObject_token |
	UserDataObjectDescriptionMap_Token |
	AppScriptTimers_token;
#ifndef CODE_INLINE
#include "cellapp.ipp"
#endif


/// CellApp Singleton.
BW_SINGLETON_STORAGE( CellApp )

namespace // (anonymous)
{
inline
const char * asString( bool value )
{
	return value ? "True" : "False";
}
const double MAX_ENTITY_MESSAGE_BUFFER_TIME = 5.0;
const int DEFAULT_PRELOADED_SPACE_ID = 0;


// -----------------------------------------------------------------------------
// Section: EntityChannelFinder
// -----------------------------------------------------------------------------

/**
 *	This class resolves ChannelIDs to entity channels.
 */
class EntityChannelFinder : public Mercury::ChannelFinder
{
public:
	virtual Mercury::UDPChannel * find( Mercury::ChannelID id,
		const Mercury::Address & srcAddr,
		const Mercury::Packet * pPacket,
		bool & rHasBeenHandled );
};


/**
 *	This method resolves ChannelIDs to entity channels.
 */
Mercury::UDPChannel * EntityChannelFinder::find( Mercury::ChannelID id,
	const Mercury::Address & srcAddr,
	const Mercury::Packet * pPacket, bool & rHasBeenHandled )
{
	Entity *pEntity = CellApp::instance().findEntity( (EntityID)id );
	if (pEntity != NULL)
	{
		if (pEntity->pReal() != NULL)
		{
			Mercury::UDPChannel * pChannel = &pEntity->pReal()->channel();

			if (pChannel->addr() == srcAddr)
			{
				return pChannel;
			}
			else
			{
				WARNING_MSG( "EntityChannelFinder::find: "
							"Found entity (%d) but from %s, not %s\n",
						id, srcAddr.c_str(), pChannel->addr().c_str() );
				rHasBeenHandled = true;
				return NULL;
			}
		}
		else
		{
			// If the entity is a ghost, then we forward the packet to the real
			// on the CellAppChannel to ensure that it doesn't arrive before the
			// offload data.
			Mercury::ChannelSender sender( pEntity->pRealChannel()->channel() );
			Mercury::Bundle & bundle = sender.bundle();
			bundle.startMessage( CellAppInterface::forwardedBaseEntityPacket );

			bundle << pEntity->id();
			bundle << srcAddr;
			bundle.addBlob( pPacket->data(), pPacket->totalSize() );

			rHasBeenHandled = true;

			return NULL;
		}
	}

	else
	{
		return NULL;
	}
}


const int abandonEntityTimeout = 60000000;


// -----------------------------------------------------------------------------
// Section: BaseRestoreConfirmHandler
// -----------------------------------------------------------------------------

/**
 *  This class is for keeping track of base entity restoration. Because there
 *  is no guarentee that simply because an entity has a cell part that its base
 *  has been backed up, some cell entities may be orphaned in the process.
 *  When this timer expires all cell entities with a recently restored base
 *  part that has not responded will be destroyed.
 */
class BaseRestoreConfirmHandler : public TimerHandler
{
public:
	BaseRestoreConfirmHandler();
	~BaseRestoreConfirmHandler();

	virtual void handleTimeout( TimerHandle handle, void * arg );

private:
	TimerHandle abandonEntitiesTimer_;
	static BaseRestoreConfirmHandler * s_pInstance_;
};

/**
 *	The constructor for BaseRestoreConfirmHandler.
 *
 */
BaseRestoreConfirmHandler::BaseRestoreConfirmHandler()
{
	delete s_pInstance_;
	s_pInstance_ = this;
	abandonEntitiesTimer_ =
		CellApp::instance().mainDispatcher().addTimer( abandonEntityTimeout,
			this, NULL, "BaseRestoreConfirmHandler" );
}


/**
 *	Destructor.
 */
BaseRestoreConfirmHandler::~BaseRestoreConfirmHandler()
{
	abandonEntitiesTimer_.cancel();
	s_pInstance_ = NULL;
}


/**
 *	When this structure times out, any entities that should have been
 *  restored on the BaseApp but have not received traffic are destroyed.
 */
void BaseRestoreConfirmHandler::handleTimeout( TimerHandle handle, void * arg )
{
	EntityPopulation::const_iterator iter;
	for (iter = Entity::population().begin();
		 iter != Entity::population().end(); iter++)
	{
		Entity & entity = *iter->second;
		if (entity.isReal() && !entity.isDestroyed() &&
			entity.pReal()->channel().version() != 0 &&
			entity.pReal()->channel().wantsFirstPacket())
		{
			WARNING_MSG( "BaseRestoreConfirmHandler::handleTimeout: entity %d "
						 "is assumed to have not been restored on its "
						 "baseapp.\n", iter->first );
			entity.destroy();
		}
	}
	delete this;
}


BaseRestoreConfirmHandler * BaseRestoreConfirmHandler::s_pInstance_;


// This set stores the id of the entity channels that are in danger of
// overflowing.
typedef BW::set< Mercury::ChannelID > OverflowIDs;
OverflowIDs s_overflowIDs;

// This callback function is called when an entity channel is in danger of
// overflowing.
void onSendWindowOverflow( const Mercury::UDPChannel & channel )
{
	IF_NOT_MF_ASSERT_DEV( channel.isIndexed() )
	{
		return;
	}

	s_overflowIDs.insert( channel.id() );
}

} // end namespace // (anonymous)

// -----------------------------------------------------------------------------
// Section: CellApp implementation
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
CellApp::CellApp( Mercury::EventDispatcher & mainDispatcher,
		Mercury::NetworkInterface & interface ) :
	EntityApp( mainDispatcher, interface ),
	cells_(),
	pSpaces_( new Spaces ),
	preloadedSpaces_(),
	idClient_(),
	cellAppMgr_( interface_ ),
	dbAppAlpha_( interface_ ),
	shutDownTime_( 0 ),
	pTimeKeeper_( NULL ),
	pPickler_( NULL ),

	timeoutPeriod_( 0.f ),
	lastGameTickTime_( 0 ),
	pCellAppData_( NULL ),
	pGlobalData_( NULL ),
	baseAppAddr_( 0, 0 ),
	backupIndex_( 0 ),
	gameTimer_(),
	loadingTimer_(),
	trimHistoryTimer_(),
	reservedTickTime_( 0 ),
	pViewerServer_( NULL ),
	id_( -1 ),
	persistentLoad_( 0.f ),
	transientLoad_( 0.f ),
	totalLoad_( 0.f ),
	totalEntityLoad_( 0.f ),
	totalAddedLoad_( 0.f ),
	perEntityLoadShare_( 0.f ),
	maxCellAppLoad_( 0.f ),
	shouldOffload_( true ),
	hasAckedCellAppMgrShutDown_( false ),
	hasCalledWitnesses_( false ),
	isReadyToStart_( false ),
	pBufferedEntityMessages_( new BufferedEntityMessages() ),
	pBufferedGhostMessages_( new BufferedGhostMessages() ),
	pBufferedInputMessages_( new BufferedInputMessages() ),
	pCellAppChannels_( NULL ),
	witnesses_(),
	pPyServicesMap_( NULL ),
	nextSpaceEntryIDSalt_( 0 )
{
	static EntityChannelFinder channelFinder;

	// Register the indexed channel finder for the interface
	interface_.registerChannelFinder( &channelFinder );

	dbAppAlpha_.channel().isLocalRegular( false );
	dbAppAlpha_.channel().isRemoteRegular( false );
}


/**
 *	The destructor for CellApp.
 */
CellApp::~CellApp()
{
	interface_.processUntilChannelsEmpty();

	Py_XDECREF( pPyServicesMap_ );
	pPyServicesMap_ = NULL;

	gameTimer_.cancel();
	loadingTimer_.cancel();
	trimHistoryTimer_.cancel();

	// Stop loading thread so that we can clean up without fear.
	INFO_MSG( "CellApp::~CellApp: Stopping background threads.\n" );
	bgTaskManager_.stopAll();
	fileIOTaskManager_.stopAll();
	INFO_MSG( "CellApp::~CellApp: Stopped background threads.\n" );

	// If we have lots of chunks loaded, this can take a long *LONG* time.  This
	// can be long enough in some cases that the tools will stop waiting for the
	// controlled shutdown to complete and start sending SIGQUITs, which means
	// coredumps, which is bad, so we make cleaning up all the chunks optional.
	// In "fastShutdown" mode we still need to correctly fre 
	// the delegate-related resources. More specifically, we need
	// to call pPhysicalSpace_->clear() (from within ~Space),
	// so it will remove all the worlds that have been created.
	if (!Config::fastShutdown() || IGameDelegate::instance() != NULL)
	{
		// Get spaces to mark loaded chunks as loaded, otherwise they won't
		// be cleaned up.
		pSpaces_->prepareNewlyLoadedChunksForDelete();

		PreloadedChunkSpaces::iterator it = preloadedSpaces_.begin();
		while (it != preloadedSpaces_.end())
		{
			it->second->prepareNewlyLoadedChunksForDelete();
			++it;
		}

		delete pTimeKeeper_;

		// TODO: Move this to the destructor of Cells. Presently, we must
		// delete cells here before we delete spaces below.
		cells_.destroyAll();

		this->timeQueue().clear();

		bw_safe_delete( pSpaces_ );

		it = preloadedSpaces_.begin();
		while (it != preloadedSpaces_.end())
		{
			bw_safe_delete( it->second );
			++it;
		}

		// This should only be done when retiring this app but not killing the
		// server.
#if 0
		// TODO: Should this be done outside the fastShutdown_ block?
		if (!this->isShuttingDown())
		{
			idClient_.returnIDs();
		}
#endif

		bw_safe_delete( pBufferedEntityMessages_ );
		bw_safe_delete( pBufferedGhostMessages_ );
		bw_safe_delete( pBufferedInputMessages_ );
		bw_safe_delete( pCellAppChannels_ );
		bw_safe_delete( pPickler_ );

		Py_XDECREF( pCellAppData_ );
		pCellAppData_ = NULL;

		Py_XDECREF( pGlobalData_ );
		pGlobalData_ = NULL;
	}

	if (IGameDelegate::instance())
	{
		IGameDelegate::instance()->shutDown();
	}

	EntityType::clearStatics();
	DataType::clearStaticsForReload();
	UserDataObjectType::clearStatics();

	this->checkPython();

	delete pViewerServer_;
}


/**
 *	This method is a simple helper to convert from seconds to ticks.
 */
int CellApp::secondsToTicks( float seconds, int lowerBound )
{
	return std::max( lowerBound,
			int( floorf( seconds * Config::updateHertz() + 0.5f ) ) );
}



/**
 *	This method is used to initialise the application.
 */
bool CellApp::init( int argc, char * argv[] )
{
	if (!EntityApp::init( argc, argv ))
	{
		return false;
	}

	if (!interface_.isGood())
	{
		NETWORK_ERROR_MSG( "CellApp::init: "
				"Failed to create NetworkInterface on internal interface.\n" );
		return false;
	}

	if (Config::shouldResolveMailBoxes() && !Config::isProduction())
	{
		CONFIG_WARNING_MSG( "CellApp::init: shouldResolveMailBoxes is enabled. "
			"This can lead to writing script that makes bad assumptions "
			"as to locality of mailboxes.\n" );
	}

	if (BalanceConfig::demo() && Config::isProduction())
	{
		CONFIG_ERROR_MSG( "CellApp::init: "
				"Production Mode: Demo load balancing enabled!\n" );
	}

	reservedTickTime_ =
		int64( Config::reservedTickFraction()/ Config::updateHertz() *
				stampsPerSecondD() );

	// Set entity specific configuration.
	Entity::s_init();

	if (!AutoConfig::configureAllFrom( "resources.xml" ))
	{
		CRITICAL_MSG( "CellApp::init: Failed to load resources.xml\n" );
	}

	PROC_IP_INFO_MSG( "Internal address = %s\n", interface_.address().c_str() );

	// find the cell app manager.
	if (!cellAppMgr_.init( "CellAppMgrInterface", Config::numStartupRetries(),
			Config::maxMgrRegisterStagger() ))
	{
		NETWORK_DEBUG_MSG( "CellApp::init: Failed to find the CellAppMgr.\n" );
		return false;
	}

	// Register the fixed portion of our interface with the interface
	CellAppInterface::registerWithInterface( interface_ );

	pViewerServer_ = new CellViewerServer( *this );
	pViewerServer_->startup( this->mainDispatcher(), 0 );

	// load any extensions (in DLLs)
	if (!this->initExtensions())
	{
		ERROR_MSG( "CellApp::init: Failed to load plugins.\n" );
		return false;
	}

	if (IGameDelegate::instance() != NULL) 
	{
		BW::string resPaths;

		for (int i = 0; i < BWResource::getPathNum(); ++i)
		{
			if (i!=0)
			{
				resPaths += ";";
			}
			resPaths += BWResource::getPath( i );
		}

		INFO_MSG( "CellApp::init: Initializing IGameDelegate with resource paths: "
				" %s\n", resPaths.c_str() );
		
		if (!IGameDelegate::instance()->initialize( resPaths.c_str() ))
		{
			ERROR_MSG( "CellApp::init: Failed to initialize IGameDelegate.\n" );
			return false;
		}
	}

	fileIOTaskManager_.initWatchers( "FileIO" );
	fileIOTaskManager_.startThreads( "FileIO", 1 );

	bgTaskManager_.initWatchers( "CellApp" );
	bgTaskManager_.startThreads( "BGTask Manager", 1 );

	this->addWatchers();

	// start up the scripting system
	if (!this->initScript())
	{
		return false;
	}

	ProfileVal::setWarningPeriod( stampsPerSecond() / Config::updateHertz() );
	CellProfileGroup::init();

	if (!EntityType::init())
	{
		ERROR_MSG( "EntityType::init: Failed\n" );
		return false;
	}

	if (!UserDataObjectType::init())
	{
		ERROR_MSG( "EntityType::init: Failed\n" );
		return false;
	}

	pTerrainManager_ = TerrainManagerPtr( new Terrain::Manager() );
	if (!pTerrainManager_->isValid())
	{
		ERROR_MSG( "Terrain::Manager::init: Failed\n" );
		return false;
	}

	// TODO: This is a bit dodgy. We do not want to trim all histories at once.
	trimHistoryTimer_ =
		this->mainDispatcher().addTimer( 4 * 60 * 1000000, // 4 minutes
				this,
				reinterpret_cast<void *>(TIMEOUT_TRIM_HISTORIES),
				"TrimHistory" );

	int loadingTickMicroseconds =
		std::max( int( 1000000 * Config::chunkLoadingPeriod() ), 5000 );
	loadingTimer_ =
		CellApp::instance().mainDispatcher().addTimer( loadingTickMicroseconds,
			this,
			reinterpret_cast<void *>(TIMEOUT_LOADING_TICK),
			"LoadingTick" );

 	mainDispatcher_.clearSpareTime();

	pCellAppChannels_ = new CellAppChannels(
			int( 1000000 / Config::ghostUpdateHertz() ), mainDispatcher_ );

	DataSectionPtr pWatcherValues =
		BWConfig::getSection( "cellApp/watcherValues" );
	if (pWatcherValues)
	{
		pWatcherValues->setWatcherValues();
	}

	// Register ourselves with the CellAppMgr.  This post-registration init code
	// is implemented as a reply handler so that it will be done as soon as the
	// reply is received from the CellAppMgr.  This was formerly done using
	// BlockingReplyHandler, however, doing it that way means that any
	// out-of-order packets received before the reply would be processed before
	// the remainder of the init code had been executed.
	new AddToCellAppMgrHelper( *this, pViewerServer_->port() );

	Mercury::UDPChannel::sendWindowCallbackThreshold(
			Config::sendWindowCallbackThreshold() );
	Mercury::UDPChannel::setSendWindowCallback( onSendWindowOverflow );

	WaypointStats::addWatchers();

	this->preloadSpaces();

	return true;
}


/**
 *  This method handles the portion of init after registering with the
 *  CellAppMgr.
 */
bool CellApp::finishInit( const CellAppInitData & initData )
{
	// Make sure that nothing else is read in the main thread.
	BWResource::watchAccessFromCallingThread( true );

	if (int32( initData.id ) == -1)
	{
		ERROR_MSG( "CellApp::finishInit: "
				"CellAppMgr refused to let us join.\n" );
		return false;
	}

	id_ = initData.id;
	this->setStartTime( initData.time );
	baseAppAddr_ = initData.baseAppAddr;
	dbAppAlpha_.addr( initData.dbAppAlphaAddr );
	isReadyToStart_ = initData.isReady;

	// Attach ourselves to an ID server (in this case, the Alpha DBApp).
	if (!idClient_.init( &this->dbAppAlpha(),
			DBAppInterface::getIDs,
			DBAppInterface::putIDs,
			IDConfig::criticallyLowSize(),
			IDConfig::lowSize(),
			IDConfig::desiredSize(),
			IDConfig::highSize() ))
	{
		ERROR_MSG( "CellApp::finishInit: Failed to get IDs\n" );
		return false;
	}

	timeoutPeriod_ = initData.timeoutPeriod;

	// Send app id to loggers
	LoggerMessageForwarder::pInstance()->registerAppID( id_ );

	if (isReadyToStart_)
	{
		this->startGameTime();
	}
	else
	{
		// Let startup() message handler start the game timer.
		isReadyToStart_ = true;
	}

	CONFIG_INFO_MSG( "\tCellApp ID            = %d\n", id_ );
	CONFIG_INFO_MSG( "\tstarting time         = %.1f\n",
		this->gameTimeInSeconds() );

	CellAppInterface::registerWithMachined( this->interface(), id_ );

	// We can safely register a birth listener now since we have mapped the
	// interfaces we are serving.
	Mercury::MachineDaemon::registerBirthListener(
			this->interface().address(),
			CellAppInterface::handleCellAppMgrBirth, "CellAppMgrInterface" );

	// init the watcher stuff
	char	abrv[32];
	bw_snprintf( abrv, sizeof(abrv), "cellapp%02d", id_ );
	BW_REGISTER_WATCHER( id_, abrv, "cellApp",
			mainDispatcher_, this->interface().address() );

	int pythonPort = BWConfig::get( "cellApp/pythonPort",
						PORT_PYTHON_CELLAPP + id_ );
	this->startPythonServer( pythonPort, id_ );

	INFO_MSG( "CellApp::finishInit: CellAppMgr acknowledged our existence.\n" );

	return true;
}


/**
 *	This method preloads spaces.
 */
void CellApp::preloadSpaces()
{
	DataSectionPtr pTopSection =
		BWConfig::getSection( "cellApp/preloadedGeometryMappings" );

	if (!pTopSection)
	{
		return;
	}

	CONFIG_INFO_MSG( "cellApp/preloadedGeometryMappings (from bw.xml) are:\n" );

	BW::Matrix m;

	DataSectionIterator iter = pTopSection->begin();
	while (iter != pTopSection->end())
	{
		DataSectionPtr pSection = *iter;
		++iter;

		if (pSection->sectionName() == "preload")
		{
			BW::string path = pSection->asString();

			PreloadedChunkSpaces::iterator it = preloadedSpaces_.find( path );
			if (it != preloadedSpaces_.end())
			{
				WARNING_MSG( "CellApp::preloadSpaces: %s already created\n",
						path.c_str() );
				continue;
			}

			CONFIG_INFO_MSG( "  preload: %s\n", path.c_str() );

			m.setIdentity();

			PreloadedChunkSpace * pSpace = new PreloadedChunkSpace(
					m, path, this->nextSpaceEntryID(),
					DEFAULT_PRELOADED_SPACE_ID, *this );

			preloadedSpaces_[ path ] = pSpace;
		}
	}
}


/**
 *	This method handles the when a geometry mapping has been fully loaded.
 */
void CellApp::onSpaceGeometryLoaded( SpaceID spaceID, const BW::string & name )
{
	this->scriptEvents().triggerEvent(
			"onSpaceGeometryLoaded",
			Py_BuildValue( "is", spaceID, name.c_str() ) );
}


/**
 *	This method is called when we are ready to start the game timer.
 */
void CellApp::startGameTime()
{
	INFO_MSG( "CellApp is starting\n" );

	MF_ASSERT( !gameTimer_.isSet() && (pTimeKeeper_ == NULL) );
	MF_ASSERT( cellAppMgr_.isInitialised() );

	// start the game timer
	gameTimer_ = this->mainDispatcher().addTimer(
						1000000/Config::updateHertz(), this,
							reinterpret_cast< void * >( TIMEOUT_GAME_TICK ),
							"GameTick" );

	lastGameTickTime_ = timestamp();
	gettimeofday( &oldTimeval_, NULL );
	mainDispatcher_.clearSpareTime();
	this->calcTransientLoadTime();

	pTimeKeeper_ = new TimeKeeper( interface_, gameTimer_, time_,
		Config::updateHertz(), cellAppMgr_.addr(),
		&CellAppMgrInterface::gameTimeReading,
		id_, Config::maxTickStagger() );

	// Now we're sending load updates to the CellAppMgr regularly
	cellAppMgr_.isRegular( true );
}


namespace
{
int numCells()
{
	return CellApp::instance().cells().size();
}

uint64 runningTime()
{
	return ProfileGroup::defaultGroup().runningTime();
}


// Stats
float g_maxTickPeriod;
float g_allTimeMaxTickPeriod;

float getAndResetMaxTickPeriod()
{
	float result = g_maxTickPeriod;
	g_allTimeMaxTickPeriod = std::max( g_allTimeMaxTickPeriod,
			g_maxTickPeriod );

	g_maxTickPeriod = 0.0;

	return result;
}

} // namespace


/**
 *	This method adds watchers to the CellApp.
 */
void CellApp::addWatchers()
{
	Watcher & watcher = Watcher::rootWatcher();

	MF_WATCH( "stats/stampsPerSecond", &stampsPerSecond );
	MF_WATCH( "stats/runningTime", &runningTime );

	MF_WATCH( "resetOnRead/maxTickPeriod", &getAndResetMaxTickPeriod );
	MF_WATCH( "maxTickPeriod", g_allTimeMaxTickPeriod );

	MF_WATCH( "timeoutPeriod", timeoutPeriod_, Watcher::WT_READ_ONLY );


	// The cells (profiles should possibly go in cells too).

	MF_WATCH( "persistentLoad", persistentLoad_ );
	MF_WATCH( "transientLoad", transientLoad_ );
	MF_WATCH( "load", totalLoad_ );
	MF_WATCH( "entityLoad", totalEntityLoad_ );
	MF_WATCH( "perEntityLoadShare", perEntityLoadShare_ );
	MF_WATCH( "addedArtificialLoad", totalAddedLoad_ );

	MF_WATCH( "numCells", &numCells );
	MF_WATCH( "numSpaces", *this, &CellApp::numSpaces );

	MF_WATCH( "id", id_ );

	// OK, we want to make a watcher for the type 'EntityPopulation',
	// but EntityPopulation is a Map of Entity *, not of Entity. So
	// to do this we first make a map watcher...
	MapWatcher<EntityPopulation> * pWatchEntities =
										new MapWatcher<EntityPopulation>();
	// then we add a 'BaseDereference' watcher to it which passes
	// on all its functions to its child watcher, after dereferencing
	// any base address. Easy!
	pWatchEntities->addChild( "*", new BaseDereferenceWatcher(
		Entity::pWatcher() ) );
	watcher.addChild( "entities", pWatchEntities,
		(void*)&Entity::population() );

	watcher.addChild( "cells", Cells::pWatcher(), (void*)&cells_ );

	watcher.addChild( "spaces", pSpaces_->pWatcher() );

	watcher.addChild( "entityTypes", EntityType::pWatcher() );

	this->EntityApp::addWatchers( watcher );

	Entity::addWatchers();

	watcher.addChild( "throttle", EmergencyThrottle::pWatcher(), &throttle_ );

	cellAppMgr_.addWatchers( "cellAppMgr", Watcher::rootWatcher() );

	watcher.addChild( "dbAppAlpha", makeWatcher( &Mercury::ChannelOwner::addr ),
		&dbAppAlpha_ );
}


/**
 *	This method is called by different components. When all things we are
 *	waiting for are ready, we call the callback.
 */
void CellApp::onGetFirstCell( bool isFromDB )
{
	this->scriptEvents().triggerTwoEvents( "onAppReady", "onCellAppReady",
		Py_BuildValue( "(i)", isFromDB ) );
}


/**
 *	This method returns the number of spaces this CellApp handles.
 */
size_t CellApp::numSpaces() const
{
	return pSpaces_->size();
}


/**
 *	This method handles timeout events.
 */
void CellApp::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	switch (reinterpret_cast<uintptr>( arg ))
	{
		case TIMEOUT_GAME_TICK:
			this->handleGameTickTimeSlice();
			break;

		case TIMEOUT_TRIM_HISTORIES:
			this->handleTrimHistoriesTimeSlice();
			break;

		case TIMEOUT_LOADING_TICK:
		{
			// TODO: Could do this in an OpportunisticPoller
			bgTaskManager_.tick();
			fileIOTaskManager_.tick();

			{
				SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );

				PreloadedChunkSpaces::iterator it = preloadedSpaces_.begin();
				while (it != preloadedSpaces_.end())
				{
					it->second->chunkTick();
					++it;
				}
			}
			pSpaces_->tickChunks();
			break;
		}
	}
}


/**
 *
 */
void CellApp::tickShutdown()
{
	if (!hasAckedCellAppMgrShutDown_)
	{
		this->sendShutdownAck( SHUTDOWN_INFORM );

		// No longer regularly sending the load from now on.
		cellAppMgr_.isRegular( false );

		hasAckedCellAppMgrShutDown_ = true;
	}
}


/**
 *	This method returns the number of seconds since this method was last called.
 *	It is used for timing the real length of a game tick.
 */
uint64 CellApp::calcTickPeriod()
{
	uint64 newTime = timestamp();
	uint64 deltaTime = newTime - lastGameTickTime_;
	lastGameTickTime_ = newTime;

	timeval newTimeval;
	gettimeofday( &newTimeval, NULL );

	if (time_ % 20 == 0)
	{
		double lastTickPeriod = stampsToSeconds( deltaTime );

		double lastTickPeriodCheck =
			double(newTimeval.tv_sec - oldTimeval_.tv_sec) +
				double(newTimeval.tv_usec - oldTimeval_.tv_usec) / 1000000.0;

		if ((lastTickPeriodCheck < 0.9 * lastTickPeriod) ||
			(1.1 * lastTickPeriod < lastTickPeriodCheck))
		{
			WARNING_MSG( "CellApp::calcTickPeriod: "
				"Time calculation off by %.1f%%. %.3fs instead of %.3fs\n"
					"This may be caused by Speedstep technology that "
					"changes the CPU frequency, often found in laptops or "
					"unsynchronised Time Stamp Counter in dual core "
					"systems.\n"
					"See the Server Operations Guide for how to change to "
					"the gettimeofday timing method.\n",
				(lastTickPeriod/lastTickPeriodCheck)*100.f,
				lastTickPeriod, lastTickPeriodCheck );
		}
	}

	oldTimeval_ = newTimeval;

	return deltaTime;
}


/**
 *	This method calculates any temporary load time. This is the time in the last
 *	tick that we spent doing things that should not be counted towards
 *	persistent load.
 */
double CellApp::calcTransientLoadTime()
{
	static uint64 lastSumTime = 0;
	uint64 sumTime = TRANSIENT_LOAD_PROFILE.sumTime_;

	double time = stampsToSeconds( sumTime - lastSumTime );
	lastSumTime = sumTime;

	return time;
}


/**
 *	This method calculates the number of seconds spent idling since this was
 *	last called.
 */
double CellApp::calcSpareTime()
{
	double spareTime = stampsToSeconds( mainDispatcher_.getSpareTime() );
	mainDispatcher_.clearSpareTime();

	return spareTime;
}


/**
 *	This method calculates the number of seconds spent doing work that is
 *	influenced by the throttle.
 */
double CellApp::calcThrottledLoadTime()
{
	static uint64 lastSumTime = 0;
	uint64 sumTime = CLIENT_UPDATE_PROFILE.sumTime_;

	double time = stampsToSeconds( sumTime - lastSumTime );
	lastSumTime = sumTime;

	return time;
}


/**
 *	This method displays warnings if a game tick has taken too long.
 */
void CellApp::checkTickWarnings( double persistentLoadTime,
		double lastTickPeriod, double spareTime )
{
	// Did the last tick take more than 2 ticks amount of time?
	if (lastTickPeriod * Config::updateHertz() > 2.f)
	{
		WARNING_MSG( "CellApp::checkTickWarnings: "
				"Last game tick took %.2f seconds. "
				"Persistent load time = %.2f. Spare time %.2f.\n",
			lastTickPeriod, persistentLoadTime, spareTime );
	}

	if (!Config::allowInteractiveDebugging() &&
		lastTickPeriod > timeoutPeriod_)
	{
		CRITICAL_MSG( "CellApp::checkTickWarnings: Last game tick took %.2f "
			"seconds. cellAppMgr/cellAppTimeout is %.2f. "
			"This process should have been stopped by CellAppMgr. This "
			"process will now be terminated.\n",
			lastTickPeriod,
			timeoutPeriod_ );
	}

	{
		static uint64 scriptTime = SCRIPT_CALL_PROFILE.sumTime_;
		uint64 newScriptTime = SCRIPT_CALL_PROFILE.sumTime_;
		uint64 deltaScriptTime = newScriptTime - scriptTime;
		scriptTime = newScriptTime;
		if (deltaScriptTime * Config::updateHertz() / 2 > stampsPerSecond())
		{
			WARNING_MSG( "CellApp::checkTickWarnings: "
							"Last game tick, script took %.2f seconds\n",
						stampsToSeconds( deltaScriptTime ) );
		}
	}

	if ((persistentLoadTime < 0.f) || (lastTickPeriod < persistentLoadTime))
	{
		if (g_timingMethod == RDTSC_TIMING_METHOD)
		{
			CRITICAL_MSG( "CellApp::checkTickWarnings: "
					"Invalid timing result %.3f.\n"
					"This is likely due to running on a multicore system that "
					"does not have synchronised Time Stamp Counters. Refer to "
					"server_installation_guide.html regarding TimingMethod.\n",
				lastTickPeriod );
		}
		else
		{
			CRITICAL_MSG( "CellApp::checkTickWarnings: "
				"Invalid timing result %.3f.\n", lastTickPeriod );
		}
	}
}


/**
 *	This method returns the number of seconds since the current tick should have
 *	been triggered. This is a good measure if this process is overloaded and
 *	running late.
 */
float CellApp::numSecondsBehind() const
{
	uint64 now = timestamp();
	uint64 next = mainDispatcher_.timerDeliveryTime( gameTimer_ );
	uint64 toNext = next - now;

	// Check if toNext is 'negative' (unsigned arthimetic).
	return (2 * toNext < toNext) ?
		Config::expectedTickPeriod() + stampsToSeconds( now - next ) :
		Config::expectedTickPeriod() - stampsToSeconds( toNext );
}


/**
 *	This method factors in the time spent in the last tick to the load result.
 *
 *	@param timeSpent	The number of seconds spent in the last tick.
 *	@param result		The load property that will be updated.
 */
void CellApp::addToLoad( float timeSpent, float & result ) const
{
	float bias = Config::loadSmoothingBias();

	result = (1-bias) * result + bias * timeSpent * Config::updateHertz();
}


/**
 *	This method updates what the load is associated with the app.
 */
void CellApp::updateLoad()
{
	uint64 lastTickTimeInStamps = this->calcTickPeriod();
	double tickTime = stampsToSeconds( lastTickTimeInStamps );

	this->tickProfilers( lastTickTimeInStamps );

	double spareTime = this->calcSpareTime();
	double transientLoadTime = this->calcTransientLoadTime();

	double persistentLoadTime = tickTime - spareTime - transientLoadTime;

	double estimatedPersistentLoadTime = throttle_.estimatePersistentLoadTime(
												persistentLoadTime,
												this->calcThrottledLoadTime() );

	throttle_.update( this->numSecondsBehind(),
						spareTime, Config::expectedTickPeriod() );

	this->checkTickWarnings( persistentLoadTime, tickTime, spareTime );

	// Factor these times into the load values.
	float artificialTime = totalAddedLoad_ / Config::updateHertz();

	this->addToLoad( estimatedPersistentLoadTime + artificialTime,
			persistentLoad_ );
	this->addToLoad( transientLoadTime, transientLoad_ );
	this->addToLoad( tickTime - spareTime, totalLoad_ );


	int numRealEntities = this->numRealEntities();

	if (BalanceConfig::demo())
	{
		// oldstyle demo load balancing
		persistentLoad_ =
			(float)numRealEntities / BalanceConfig::demoNumEntitiesPerCell();
	}

	if (persistentLoad_ > totalEntityLoad_)
	{
		perEntityLoadShare_ = (persistentLoad_ - totalEntityLoad_) / numRealEntities;
	}
	else
	{
		perEntityLoadShare_ = 0.f;
	}

}


/**
 * This method ticks entity and entity type profilers
 */
void CellApp::tickProfilers( uint64 lastTickInStamps )
{
	cells_.tickProfilers( lastTickInStamps, Config::loadSmoothingBias() );

	EntityType::tickProfilers( totalEntityLoad_, totalAddedLoad_ );
}

/**
 *	This method lets the CellAppMgr know about our bounding rectangle.
 */
void CellApp::updateBoundary()
{
	AUTO_SCOPED_PROFILE( "calcBoundary" );
	// TODO: We could probably only send this at the load balancing rate or
	//	it could be part of informOfLoad?

	cellAppMgr_.updateBounds( cells_ );
}


/**
 *	This method backs up some of the entities. It should be called each tick.
 */
void CellApp::tickBackup()
{
	int backupPeriod = Config::backupPeriodInTicks();

	if (backupPeriod != 0)
	{
		AUTO_SCOPED_PROFILE( "backup" );
		cells_.backup( backupIndex_, backupPeriod );
			backupIndex_ = (backupIndex_ + 1) % backupPeriod;
	}
}


/**
 *	This method checks whether any entities should be offloaded or ghosts
 *	created or destroyed.
 */
void CellApp::checkOffloads()
{
	if ((time_ % Config::checkOffloadsPeriodInTicks()) == 0)
	{
		cells_.checkOffloads();
	}
}


/**
 *	This method is used to make sure that the time is synchronised between
 *	all processes.
 */
void CellApp::syncTime()
{
	// TODO: Move this to ServerApp
	if ((time_ % Config::timeSyncPeriodInTicks()) == 0)
	{
		if (pTimeKeeper_)
		{
			// send a reading of our time to the cell app manager...
			// ... and get a better one back to sync our tick with
			pTimeKeeper_->synchroniseWithMaster();
		}
	}
}


/**
 *	This method handles the game tick time slice.
 */
void CellApp::handleGameTickTimeSlice()
{
	AUTO_SCOPED_PROFILE( "gameTick" );

	if (this->inShutDownPause())
	{
		this->tickShutdown();
		return;
	}

	this->updateLoad();

	cellAppMgr_.informOfLoad( persistentLoad_ );

	this->updateBoundary();

	// Increment the time - we are now into the quantum of the next tick
	this->advanceTime();

	this->tickBackup();

	this->checkSendWindowOverflows();

	// ---- Jobs that can just take up remaining time should go last ----

	this->checkOffloads();

	pSpaces_->deleteOldSpaces();

	this->syncTime();

	this->tickStats();

	this->bufferedEntityMessages().playBufferedMessages( *this );

	this->bufferedInputMessages().playBufferedMessages( *this );

	cells_.tickRecordings();
}

/**
 *	This method is called on StartOfTick, before the Updaters are updated.
 * We use it here to execute the update step on all the cells (physics, etc.)
 */
void CellApp::onStartOfTick()
{
	this->EntityApp::onStartOfTick();

	if (IGameDelegate::instance() != NULL)
	{
		IGameDelegate::instance()->update();
	}	
}

/**
 *	This method updates and sends all the witnesses as the last action
 *	of the game tick, and ensures that cell-cell channels are flushed
 */
void CellApp::onEndOfTick()
{
	pCellAppChannels_->stopTimer();
	pCellAppChannels_->sendAll();

	this->callWitnesses( /* checkReceivedTime = */ false );
}


/**
 *	This method flushes cell-cell channels and re-enables the flush-timer
 *	now that the game tick procssing is complete.
 */
void CellApp::onTickProcessingComplete()
{
	this->EntityApp::onTickProcessingComplete();
	pCellAppChannels_->sendTickCompleteToAll( this->time() );
	pCellAppChannels_->startTimer( int( 1000000 / Config::ghostUpdateHertz() ),
		mainDispatcher_ );

	hasCalledWitnesses_ = false;

	this->callWitnesses( /* checkReceivedTime = */ true );
}


/**
 *	This method handles the trim histories time slice.
 *
 *	TODO: This is dodgy. We don't want to trim all of the histories in one go.
 */
void CellApp::handleTrimHistoriesTimeSlice()
{
	// Keep iterator in tact. This is necessary as trimEventHistory can call
	// onWitnessed (which it probably should not do).
	Entity::callbacksPermitted( false );
	{
		EntityPopulation::const_iterator iter = Entity::population().begin();

		while (iter != Entity::population().end())
		{
			// TODO: Could skip dead entities.
			iter->second->trimEventHistory( 0 );

			iter++;
		}
	}
	Entity::callbacksPermitted( true );

	Entity::population().expireRealChannels();
}


/**
 *	This method kills a cell.
 */
void CellApp::destroyCell( Cell * pCell )
{
	cells_.destroy( pCell );
}


/**
 *	Shut down this application.
 */
void CellApp::shutDown()
{
	this->ServerApp::shutDown();
}

// -----------------------------------------------------------------------------
// Section: Mercury Message Handlers
// -----------------------------------------------------------------------------


/**
 *	This method handles a shutDown message.
 */
void CellApp::shutDown( const CellAppInterface::shutDownArgs & /*args*/ )
{
	TRACE_MSG( "CellApp::shutDown: Told to shut down\n" );
	this->shutDown();
}


/**
 *	This method handles a message telling us to shut down in a controlled way.
 */
void CellApp::controlledShutDown(
		const CellAppInterface::controlledShutDownArgs & args )
{
	switch (args.stage)
	{
		case SHUTDOWN_INFORM:
		{
			if (shutDownTime_ != 0)
			{
				ERROR_MSG( "CellApp::controlledShutDown: "
						"Setting shutDownTime_ to %u when already set to %u\n",
					args.shutDownTime, shutDownTime_ );
			}

			shutDownTime_ = args.shutDownTime;

			hasAckedCellAppMgrShutDown_ = false;

			if (this->hasStarted())
			{
				this->scriptEvents().triggerTwoEvents(
					"onAppShuttingDown", "onCellAppShuttingDown",
					Py_BuildValue( "(d)",
						(double)shutDownTime_/Config::updateHertz() ) );

				// Don't send ack immediately to allow scripts to do stuff.
			}
			else
			{
				this->sendShutdownAck( SHUTDOWN_INFORM );
			}
		}
		break;

		case SHUTDOWN_PERFORM:
		{
			ERROR_MSG( "CellApp::controlledShutDown: "
					"CellApp does not do SHUTDOWN_PERFORM stage.\n" );
			// TODO: It could be good to call this so that we can call a script
			// method.
			break;
		}

		case SHUTDOWN_NONE:
		case SHUTDOWN_REQUEST:
		case SHUTDOWN_DISCONNECT_PROXIES:
		case SHUTDOWN_TRIGGER:
			break;
	}
}


/**
 * 	This method sends an ack to the CellAppMgr to indicate that we've
 * 	finished one of our shutdown stages.
 */
void CellApp::sendShutdownAck( ShutDownStage stage )
{
	cellAppMgr_.ackShutdown( stage );
}


/**
 *	This method handles a message to add a cell to this cell application.
 */
void CellApp::addCell( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	if (header.replyID != 0)
	{
		if (srcAddr == cellAppMgr_.addr())
		{
			cellAppMgr_.bundle().startReply( header.replyID );
			cellAppMgr_.send();
		}
		else
		{
			WARNING_MSG( "CellApp::addCell: "
							"Request came from %s instead of CellAppMgr %s\n",
						srcAddr.c_str(), cellAppMgr_.addr().c_str() );
			Mercury::UDPBundle bundle;
			bundle.startReply( header.replyID );
			this->interface().send( srcAddr, bundle );
		}
	}

	SpaceID spaceID;
	data >> spaceID;
	Space * pSpace = this->findSpace( spaceID );

	if (pSpace)
	{
		INFO_MSG( "CellApp::addCell: Re-using space %d\n", spaceID );
		pSpace->reuse();
	}
	else
	{
		pSpace = pSpaces_->create( spaceID );
	}

	INFO_MSG( "CellApp::addCell: Space = %u\n", spaceID );

	MF_ASSERT( pSpace );

	pSpace->updateGeometry( data );

	CellInfo * pCellInfo = pSpace->findCell( interface_.address() );

	if (pCellInfo)
	{
		Cell * pNewCell = pSpace->pCell();

		if (pNewCell)
		{
			WARNING_MSG( "CellApp::addCell: "
					"Cell did not fully remove; reusing.\n" );
			MF_ASSERT( pCellInfo == &pNewCell->cellInfo() );
			pNewCell->reuse();
		}
		else
		{
			pNewCell = new Cell( *pSpace, *pCellInfo );
			cells_.add( pNewCell );
		}

		bool isFirstCell;
		data >> isFirstCell;

		bool isFromDB;
		data >> isFromDB;

		if (data.remainingLength() > 0)
		{
			pSpace->allSpaceData( data );
		}

		// The first cell in the first space.
		if (isFirstCell && Config::useDefaultSpace() && spaceID == 1)
		{
			this->onGetFirstCell( isFromDB );
		}
	}
	else
	{
		CRITICAL_MSG( "CellApp::addCell: Failed to add a cell for space %u\n",
				spaceID );
	}
}


/**
 *	This method handles a message from the CellAppMgr telling us to set our
 * 	game time.
 */
void CellApp::setGameTime( const CellAppInterface::setGameTimeArgs & args )
{
	MF_ASSERT( !gameTimer_.isSet() );	// Not started yet.

	time_ = args.gameTime;

	INFO_MSG( "CellApp::setGameTime: gametime = %.1f\n",
			this->gameTimeInSeconds() );
}


/**
 *	This method handles a message from the CellAppMgr telling us to start.
 */
void CellApp::startup( const CellAppInterface::startupArgs & args )
{
	baseAppAddr_ = args.baseAppAddr;

	if (isReadyToStart_)
	{
		this->startGameTime();
	}
	else
	{
		// The cell app initialisation hasn't completed yet -- this may happen
		// when a nested dispatch loop is being executed for a blocking request
		// inside finishInit().
		isReadyToStart_ = true;
	}
}


/**
 *	This method handles a message that lets us know that there is a new cell
 *	manager.
 */
void CellApp::handleCellAppMgrBirth(
					const CellAppInterface::handleCellAppMgrBirthArgs & args )
{
	INFO_MSG( "CellApp::handleCellAppMgrBirth: %s\n", args.addr.c_str() );
	cellAppMgr_.onManagerRebirth( *this, args.addr );

	if (pTimeKeeper_)
	{
		pTimeKeeper_->masterAddress( cellAppMgr_.addr() );
	}
}


/**
 *
 */
void CellApp::addCellAppMgrRebirthData( BinaryOStream & stream )
{
	stream << interface_.address();
	stream << pViewerServer_->port();
	stream << id_ << time_;

	pCellAppData_->addToStream( stream );
	pGlobalData_->addToStream( stream );

	pSpaces_->writeRecoveryData( stream );
}


/**
 *	This method handles a message from the cell app manager informing us that a
 *	CellApp has died.
 */
void CellApp::handleCellAppDeath(
	const CellAppInterface::handleCellAppDeathArgs & args )
{
	NETWORK_INFO_MSG( "CellApp::handleCellAppDeath: %s\n", args.addr.c_str() );

	MF_ASSERT( args.addr.port != 0 );

	cells_.handleCellAppDeath( args.addr );

	// This helper object decides when it's safe to send the ACK back to the
	// CellAppMgr.  See the header comment for this class for more details on
	// the ins and outs of this problem.
	AckCellAppDeathHelper * pAckHelper = new AckCellAppDeathHelper( *this,
		args.addr );

	// Destroy the ghosts that are associated with real entities that are lost.
	// TODO: Probably don't want to do this. We should bring the reals back so
	// that these ghosts do not disappear.
	// TODO: More of a reason not to do this is that we cannot be sure that this
	// information is correct. We may not delete a ghost when we should, or
	// worse, delete a ghost when we should not have.

	typedef BW::vector< EntityPtr > EntityPtrs;
	EntityPtrs zombies;

	EntityPopulation::const_iterator iter = Entity::population().begin();

	while (iter != Entity::population().end())
	{
		Entity * pEntity = iter->second;

		// We must clear haunts on the dead app for each real
		if (pEntity->isReal())
		{
			RealEntity::Haunts::iterator iter = pEntity->pReal()->hauntsBegin();

			while (iter != pEntity->pReal()->hauntsEnd())
			{
				RealEntity::Haunt & haunt = *iter;

				if (haunt.addr() == args.addr)
				{
					iter = pEntity->pReal()->delHaunt( iter );
				}
				else
				{
					++iter;
				}
			}
		}
		else if (pEntity->checkIfZombied( args.addr ))
		{
			zombies.push_back( pEntity );
		}

		// Any real whose base channel is in a critical state must be
		// considered to be a recent onload.
		if (pEntity->isReal() &&
			pEntity->pReal()->channel().hasUnackedCriticals())
		{
			pAckHelper->addCriticalEntity( pEntity );
		}

		iter++;
	}

	// Send a message to the base entities just in case they still think
	// that we have the real entity.
	Mercury::BundleSendingMap bundles( interface_ );

	EntityPtrs::iterator ghostIter = zombies.begin();

	while (ghostIter != zombies.end())
	{
		EntityPtr pEntity = *ghostIter;

		if (!pEntity->isDestroyed())
		{
			if (pEntity->hasBase())
			{
				Mercury::Bundle & bundle = bundles[ pEntity->baseAddr() ];

				// We could be smarter about sending this. We only really
				// need to do it if we were the last ones to have the real
				// entity.  For simplicity, always sending it. Probably
				// doesn't save us enough.
				BaseAppIntInterface::setClientArgs setClientArgs =
					{ pEntity->id() };
				bundle << setClientArgs;

				bundle.startRequest(
					BaseAppIntInterface::emergencySetCurrentCell,
					pAckHelper,
					reinterpret_cast< void * >( pEntity->id() ) );

				bundle << pEntity->space().id();
				bundle << pEntity->realAddr();

				// Remember that we're expecting another reply.
				pAckHelper->addBadGhost();
			}

			// Buffered lifespans may revive a ghost, real, or zombie.
			do
			{
				pEntity->destroy();
			}
			while (pEntity->checkIfZombied( args.addr ));
		}

		++ghostIter;
	}

	Entity::population().notifyBasesOfCellAppDeath( args.addr, bundles, pAckHelper );

	// Send all the zombie info to the baseapps.
	bundles.sendAll();

	pAckHelper->startTimer();

	// Notify any listeners that the CellAppChannel is about to disappear.
	CellAppDeathListeners::instance().handleCellAppDeath( args.addr );

	// Now it's safe to delete the CellAppChannel to the dead app.
	pCellAppChannels_->remoteFailure( args.addr );
}


/**
 *	This method handles a message from the CellAppMgr informing us that a
 *	BaseApp has died.
 */
void CellApp::handleBaseAppDeath( BinaryIStream & data )
{
	Mercury::Address deadAddr;
	BackupHash backupHash;
	uint8 isService = 0;
	data >> deadAddr >> isService >> backupHash;

	if (isService)
	{
		INFO_MSG( "CellApp::handleBaseAppDeath: ServiceApp %s has died\n",
			deadAddr.c_str() );

		pPyServicesMap_->map().removeFragmentsForAddress( deadAddr );
	}
	else
	{
		INFO_MSG( "CellApp::handleBaseAppDeath: BaseApp %s has died. "
				"%u backups\n",
			deadAddr.c_str(), 
			backupHash.size() );
	}

	if (backupHash.empty())
	{
		ERROR_MSG( "CellApp::handleBaseAppDeath: "
				"Unable to recover from BaseApp death. Shutting down.\n" );
		this->shutDown();
		return;
	}


	const EntityPopulation & pop = Entity::population();
	EntityPopulation::const_iterator iter = pop.begin();
	// This object will free itself when it times out.
	new BaseRestoreConfirmHandler();

	while (iter != pop.end())
	{
		if (iter->second->baseAddr() == deadAddr)
		{
			iter->second->adjustForDeadBaseApp( backupHash );
		}

		++iter;
	}

	ServerEntityMailBox::adjustForDeadBaseApp( deadAddr, backupHash );

	this->scriptEvents().triggerEvent( 
		(isService ? "onServiceAppDeath" : "onBaseAppDeath"),
		Py_BuildValue( "((sH))", 
			deadAddr.ipAsString(),
			ntohs( deadAddr.port ) ) );
}


/**
 * This method receives a forwarded watcher set request from the CellAppMgr
 * and performs the set operation on the current CellApp.
 * parameters in 'data'.
 *
 * @param srcAddr  The address from which this request originated.
 * @param header   The mercury header.
 * @param data     The data stream containing the watcher data to set.
 */
void CellApp::callWatcher( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data)
{
	BW::string path;
	data >> path;

	DeferrableWatcherPathRequest * pPathRequest = 
		new DeferrableWatcherPathRequest( path, interface_, srcAddr,
				header.replyID );

	pPathRequest->setPacketData( data );
	pPathRequest->setWatcherValue();
}


/**
 *	This method is called by the CellAppMgr to inform us that a shared value has
 *	changed. This value may be shared between CellApps or CellApps and BaseApps.
 */
void CellApp::setSharedData( BinaryIStream & data )
{
	// This code is nearly identical to the code in BaseApp::setSharedData.
	// We could look at sharing the code somehow.
	SharedDataType dataType;
	BW::string key;
	BW::string value;

	data >> dataType >> key >> value;

	Mercury::Address originalSrcAddr = Mercury::Address::NONE;

	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddr;
	}

	IF_NOT_MF_ASSERT_DEV( (data.remainingLength() == 0) && !data.error() )
	{
		ERROR_MSG( "CellApp::setSharedData: Invalid data!!\n" );
		return;
	}

	// The original change was made by this CellApp. Therefore, treat
	// this message as an ack.
	bool isAck =
		(originalSrcAddr == this->interface().address());

	switch (dataType)
	{
	case SHARED_DATA_TYPE_CELL_APP:
		pCellAppData_->setValue( key, value, isAck );
		break;

	case SHARED_DATA_TYPE_GLOBAL:
		pGlobalData_->setValue( key, value, isAck );
		break;

	default:
		ERROR_MSG( "CellApp::setValue: Invalid dataType %d\n", dataType );
		break;
	}
}


/**
 *	This method is called by the CellAppMgr to inform us that a shared value was
 *	deleted. This value may be shared between CellApps or CellApps and BaseApps.
 */
void CellApp::delSharedData( BinaryIStream & data )
{
	// This code is nearly identical to the code in BaseApp::delSharedData.
	// We could look at sharing the code somehow.
	SharedDataType dataType;
	BW::string key;

	data >> dataType >> key;

	Mercury::Address originalSrcAddr = Mercury::Address::NONE;

	if (data.remainingLength() == sizeof( Mercury::Address ))
	{
		data >> originalSrcAddr;
	}

	IF_NOT_MF_ASSERT_DEV( (data.remainingLength() == 0) && !data.error() )
	{
		ERROR_MSG( "CellApp::delSharedData: Invalid data!!\n" );
		return;
	}

	// The original change was made by this CellApp. Therefore, treat this
	// message as an ack.
	bool isAck =
		(originalSrcAddr == this->interface().address());

	switch (dataType)
	{
	case SHARED_DATA_TYPE_CELL_APP:
		pCellAppData_->delValue( key, isAck );
		break;

	case SHARED_DATA_TYPE_GLOBAL:
		pGlobalData_->delValue( key, isAck );
		break;

	default:
		ERROR_MSG( "CellApp::delValue: Invalid dataType %d\n", dataType );
		break;
	}
}


/**
 *	This method handles a message to add a service fragment.
 *
 *	@param data 	The message payload.
 */
void CellApp::addServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	EntityMailBoxRef fragmentMailBox;
	data >> serviceName >> fragmentMailBox;

	pPyServicesMap_->map().addFragment( serviceName, fragmentMailBox );
}


/**
 *	This method handles a message to delete a service fragment.
 *
 *	@param data 	The message payload.
 */
void CellApp::delServiceFragment( BinaryIStream & data )
{
	BW::string serviceName;
	Mercury::Address fragmentAddress;
	data >> serviceName >> fragmentAddress;

	pPyServicesMap_->map().removeFragment( serviceName, fragmentAddress );
}


/**
 *	This method is used to tell the CellApp which BaseApp it should use when
 *	BigWorld.createEntityOnBase is called.
 */
void CellApp::setBaseApp( const CellAppInterface::setBaseAppArgs & args )
{
	baseAppAddr_ = args.baseAppAddr;
}


/**
 *	This method handles notification from the CellAppMgr about which DBApp to
 *	use as the Alpha.
 */
void CellApp::setDBAppAlpha( const CellAppInterface::setDBAppAlphaArgs & args )
{
	TRACE_MSG( "CellApp::setDBAppAlpha: %s\n", args.addr.c_str() );
	dbAppAlpha_.addr( args.addr );
}


/**
 *	This method is used to teleport an entity. If the teleport fails, the
 *	accompanying onload message is sent back to the originator.
 */
void CellApp::onloadTeleportedEntity( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	EntityID nearbyID = 0;
	uint32 createGhostDataLen = 0;
	data >> nearbyID >> createGhostDataLen;

	EntityID teleportingID = 0;
	EntityPtr pNearbyEntity = CellApp::instance().findEntity( nearbyID );


	if (pNearbyEntity != NULL)
	{
		// We need to split up the messages because createGhost looks at the
		// remainingLength of the message data it is passed
		MemoryOStream createGhostData( createGhostDataLen + sizeof( SpaceID ) );
		data >> teleportingID;
		createGhostData << pNearbyEntity->space().id();
		createGhostData << teleportingID;
		createGhostData.transfer( data, 
							  createGhostDataLen - sizeof (teleportingID) );

		// This prevents an annoying case where a create ghost message is
		// waiting for a destroy ghost message to arrive, which will only be
		// sent once that ghost is created.
		if (srcAddr == this->interface().address())
		{
			Entity * pEntity = this->findEntity( teleportingID );

			if (pEntity)
			{
				pEntity->destroy();
			}
		}

		Mercury::UnpackedMessageHeader dummyHeader = header;
		dummyHeader.identifier = CellAppInterface::createGhost.id();
		dummyHeader.length = createGhostData.remainingLength();

		CellAppInterface::createGhost.pHandler()->
			handleMessage( srcAddr, dummyHeader, createGhostData );

		dummyHeader.identifier = CellAppInterface::onload.id();
		dummyHeader.length = data.remainingLength();

		CellAppInterface::onload.pHandler()->
			handleMessage( srcAddr, dummyHeader, data );

		Entity * pNewEntity = this->findEntity( teleportingID );

		MF_ASSERT( pNewEntity != NULL );

		pNewEntity->onTeleportSuccess( pNearbyEntity.get() );

		// If the entity is non-volatile, then the above call to the onload
		// handler will have added the detailed position
		if (pNewEntity->volatileInfo().hasVolatile( 0.f ))
		{
			pNewEntity->addDetailedPositionToHistory(
					/* isLocalOnly */ false );
		}

		if (pNewEntity->cell().pReplayData())
		{
			pNewEntity->cell().pReplayData()->addEntityState( *pNewEntity );
		}
	}
	else
	{
		// Stream off the rest of the createGhost message, it is discarded
		data.retrieve( createGhostDataLen );

		// Stream off the start of the offload message
		bool teleportFailure; 	// we discard this, as this should always be
								// false, because we set it from here
		data >> teleportingID >> teleportFailure;

		ERROR_MSG( "CellApp::onloadTeleportedEntity: "
				"nearby entity (id %u) does not exist on this CellApp, "
				"while teleporting entity %u (from %s)\n",
			nearbyID, teleportingID, srcAddr.c_str() );

		Mercury::Channel & srcChannel = CellApp::getChannel( srcAddr );
		Mercury::Bundle & bundle = srcChannel.bundle();

		bundle.startMessage( CellAppInterface::onload );

		// Send back the rest of the onload message to the originator
		bundle << teleportingID << true /*teleportFailure*/;
		bundle.transfer( data, data.remainingLength() );
	}
}


/**
 *	This method is called regularly by the CellAppMgr to inform the CellApp
 *	about general information it should know about.
 */
void CellApp::cellAppMgrInfo(
		const CellAppInterface::cellAppMgrInfoArgs & args )
{
	maxCellAppLoad_ = args.maxCellAppLoad;
}


/**
 * This method is called when a failure occurs in handling
 * onBaseOffloaded message
 */
void CellApp::handleOnBaseOffloadedFailure( EntityID entityID,
		const Mercury::Address &oldBaseAddr,
		const CellAppInterface::onBaseOffloadedArgs & args )
{
	INFO_MSG( "CellApp::handleOnBaseOffloadedFailure: "
		"Failed to deliver onBaseOffloaded message to "
		"non-existing entity %d\n", entityID );

	Mercury::UDPChannel * pChannel =
			interface_.condemnedChannels().find(
					Mercury::ChannelID ( entityID ) );
	if (pChannel != NULL)
	{
		INFO_MSG( "CellApp::handleOnBaseOffloadedFailure: "
				"Switching a dead entity's (%d) channel from "
				"address %s to %s\n",
				entityID,
				oldBaseAddr.c_str(),
				args.newBaseAddr.c_str() );

		pChannel->setAddress( args.newBaseAddr );
	}
	else
	{
		WARNING_MSG( "CellApp::handleOnBaseOffloadedFailure: "
				"Failed to find a dead entity's (%d) condemned channel",
				entityID );
	}

}


/**
 * This method receives the latest tick from a remote cellapp
 */
void CellApp::onRemoteTickComplete( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const CellAppInterface::onRemoteTickCompleteArgs & args )
{
	pCellAppChannels_->onRemoteTickComplete( srcAddr, args.gameTime );

	this->callWitnesses( /* checkReceivedTime = */ true );
}


// -----------------------------------------------------------------------------
// Section: Utility methods
// -----------------------------------------------------------------------------

/**
 * 	This method finds the entity with the given id.
 */
Entity * CellApp::findEntity( EntityID id ) const
{
	AUTO_SCOPED_PROFILE( "findEntity" );

	EntityPopulation::const_iterator iter = Entity::population().find( id );

	Entity * pEntity =
		(iter != Entity::population().end()) ? iter->second : NULL;

	return ((pEntity != NULL) && !pEntity->isDestroyed()) ? pEntity : NULL;
}


/**
 *	This method returns a list of all entity IDs in the app.
 */
void CellApp::entityKeys( PyObject * pList ) const
{
	EntityPopulation::const_iterator iter = Entity::population().begin();

	while (iter != Entity::population().end())
	{
		if (!iter->second->isDestroyed())
		{
			PyObject * pInt = PyInt_FromLong( iter->first );
			PyList_Append( pList, pInt );
			Py_DECREF( pInt );
		}

		iter++;
	}
}


/**
 *	This method returns a list of all entity objects in the app.
 */
void CellApp::entityValues( PyObject * pList ) const
{
	EntityPopulation::const_iterator iter = Entity::population().begin();

	while (iter != Entity::population().end())
	{
		if (!iter->second->isDestroyed())
		{
			PyList_Append( pList, iter->second );
		}

		iter++;
	}
}


/**
 *	This method returns a list of all entity id/object pairs in the app.
 */
void CellApp::entityItems( PyObject * pList ) const
{
	EntityPopulation::const_iterator iter = Entity::population().begin();

	while (iter != Entity::population().end())
	{
		if (!iter->second->isDestroyed())
		{
			PyObject * pTuple = PyTuple_New( 2 );
			PyTuple_SetItem( pTuple, 0, PyInt_FromLong( iter->first ) );
			Py_INCREF( iter->second );
			PyTuple_SetItem( pTuple, 1, iter->second );
			PyList_Append( pList, pTuple );
			Py_DECREF( pTuple );
		}

		iter++;
	}
}


/**
 *	This method finds the cell with the given id.
 *
 *	@param id		The id of the cell to search for.
 *
 *	@return		The cell with the input id. NULL if no such cell is found.
 */
Cell * CellApp::findCell( SpaceID id ) const
{
	Space * pSpace = this->findSpace( id );

	return pSpace ? pSpace->pCell() : NULL;
}


/**
 *	This method finds the space with the given id.
 *
 *	@param id		The id of the space to search for.
 *
 *	@return		The space with the input id. NULL if no such space is found.
 */
Space * CellApp::findSpace( SpaceID id ) const
{
	return pSpaces_->find( id );
}


/**
 *	This method adjusts the persistent load immediately for an entity offload or
 *	onload.
 */
void CellApp::adjustLoadForOffload( float entityLoad )
{
	persistentLoad_ -= entityLoad;

	if (persistentLoad_ < 0.f)
	{
		persistentLoad_ = 0.f;
	}
}


/**
 *	This method reloads the scripts.
 */
bool CellApp::reloadScript( bool isFullReload )
{
	SCRIPT_NOTICE_MSG( "CellApp::reloadScript: "
				"reloadScript should be used with caution, and never in a "
				"production environment.\n" );

	// Load entity defs in new Python interpreter.
	PyThreadState* pNewPyIntrep = Script::createInterpreter();
	PyThreadState* pOldPyIntrep = Script::swapInterpreter( pNewPyIntrep );

	bool isOK = (isFullReload) ? EntityType::init( true/*isReload*/ ) :
					EntityType::reloadScript();

	// Load UDO modules
	UserDataObjectTypes udoTypes;
	UserDataObjectTypesInDifferentDomain udoTypesInDifferentDomain;

	if (isOK)
		isOK = UserDataObjectType::load( udoTypes, udoTypesInDifferentDomain );

	if (isOK)
	{
		// Restore original Python interpreter. EntityType::init() got a copy
		// of the loaded modules. And EntityType::migrate() will replace the
		// modules in the original Python interpreter with new modules.
		// __kyl__ (8/2/2006) Can't simply throw away old interpreter and
		// use new interpreter since we are currently inside a Python call
		// so the old interpreter is still on the callstack.
		Script::swapInterpreter( pOldPyIntrep );
		Script::destroyInterpreter( pNewPyIntrep );

		EntityType::migrate( isFullReload );
		UserDataObjectType::migrate( udoTypes, udoTypesInDifferentDomain );
		ServerEntityMailBox::migrateMailBoxes();

		// Migrate entities
		for (EntityPopulation::const_iterator iter = Entity::population().begin();
				iter != Entity::population().end(); ++iter)
		{
			MF_VERIFY( iter->second->migrate() );
		}

		EntityType::cleanupAfterReload( isFullReload );

		return true;
	}
	else
	{
		// Restore original Python interpreter.
		Script::swapInterpreter( pOldPyIntrep );

		if (!isFullReload)
		{
			// Try using the old scripts again.
			// NOTE: This works (mostly) because when we try to import a module
			// that's already imported, it doesn't try to read it from the
			// disk.
			MF_VERIFY( EntityType::reloadScript( true ) );
		}

		EntityType::cleanupAfterReload( isFullReload );
		Script::destroyInterpreter( pNewPyIntrep );
		return false;
	}
}


/**
 *	This method handles the case where we detect that other cell apps are
 *	dead.
 */
void CellApp::detectDeadCellApps( const BW::vector<Mercury::Address> & addrs )
{
	BW::vector<Mercury::Address>::const_iterator iter = addrs.begin();

	while (iter != addrs.end())
	{
		INFO_MSG( "CellApp::detectDeadCellApps: %s has died\n",
				iter->c_str() );
		cellAppMgr_.handleCellAppDeath( *iter );

		++iter;
	}
}


/**
 *	This method returns the number of real entities on this application.
 */
int CellApp::numRealEntities() const
{
	return cells_.numRealEntities();
}


/**
 *	This method checks whether there are any entity channels that are close to
 *	overflowing. A callback is called on those that are in danger.
 */
void CellApp::checkSendWindowOverflows()
{
	OverflowIDs::iterator iter = s_overflowIDs.begin();

	while (iter != s_overflowIDs.end())
	{
		Entity * pEntity = this->findEntity( *iter );

		if (pEntity && pEntity->isReal())
		{
			Script::call( PyObject_GetAttrString( pEntity, "onWindowOverflow" ),
					PyTuple_New( 0 ), "onWindowOverflow", true );
		}
		else
		{
			if (pEntity)
			{
				NETWORK_WARNING_MSG( "CellApp::checkSendWindowOverflows: "
					"Entity %u is not real\n", *iter );
			}
			else
			{
				NETWORK_WARNING_MSG( "CellApp::checkSendWindowOverflows: "
					"Could not find entity %u\n", *iter );
			}
		}

		++iter;
	}

	s_overflowIDs.clear();
}


/**
 *	This method returns whether or not the next tick is pending. If the previous
 *	tick was over budget, then the next tick is considered pending. This is to
 *	break timer-handler loops where no network input is processed.
 */
bool CellApp::nextTickPending() const
{
	// If we haven't started yet, the gameTimer_ may not be set.
	return gameTimer_.isSet() &&
		((timestamp() + reservedTickTime_) >
					mainDispatcher_.timerDeliveryTime( gameTimer_ ));
}

/**
 *	This method triggers a controlled shutdown of the cluster.
 */
void CellApp::triggerControlledShutDown()
{
	CellAppMgrGateway & cellAppMgr = this->cellAppMgr();
	Mercury::Bundle & bundle = cellAppMgr.bundle();

	CellAppMgrInterface::controlledShutDownArgs &args =
		CellAppMgrInterface::controlledShutDownArgs::start( bundle );

	args.stage = SHUTDOWN_TRIGGER;

	cellAppMgr.send();
}


/*~ function BigWorld.isNextTickPending
 *	@components{ cell }
 *	The function isNextTickPending returns whether or not the current tick
 *	has completed. This allows your script code to determine whether it should
 *	relinquish control of expensive computations to allow the next tick to
 *	execute.
 *
 *	This can be controlled by the cellApp/reservedTickFraction setting in bw.xml.
 *
 *	@return isNextTickPending returns True if the next tick is pending, False
 *	otherwise.
 */
bool isNextTickPending()
{
	return CellApp::instance().nextTickPending();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isNextTickPending, END, BigWorld )


// -----------------------------------------------------------------------------
// Section: Script related
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle reply messages from BaseApps when a createBase
 *	message has been sent.
 */
class CreateBaseReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	CreateBaseReplyHandler( PyObjectPtr pCallbackFunc ) :
		pCallbackFunc_( pCallbackFunc )
	{
	}

private:
	void handleMessage( const Mercury::Address& /*srcAddr*/,
		Mercury::UnpackedMessageHeader& /*header*/,
		BinaryIStream& data, void * /*arg*/ )
	{
		EntityMailBoxRef baseRef;
		data >> baseRef;

		INFO_MSG( "CreateBaseReplyHandler::handleMessage: "
					"Base (%u) created at %s\n",
				baseRef.id, baseRef.addr.c_str() );

		if (pCallbackFunc_)
		{
			PyObjectPtr baseMb( BaseEntityMailBox::constructFromRef( baseRef ),
				PyObjectPtr::STEAL_REFERENCE );

			Py_INCREF( pCallbackFunc_.get() );
			Script::call( pCallbackFunc_.get(),
					Py_BuildValue( "(O)", baseMb.get() ),
					"BigWorld.createEntityOnBaseApp: " );
		}
		delete this;
	}

	void handleException( const Mercury::NubException& /*ne*/, void* /*arg*/ )
	{
		ERROR_MSG( "CreateBaseReplyHandler::handleException: "
			"Failed to create base\n" );

		if (pCallbackFunc_)
		{
			Py_INCREF( pCallbackFunc_.get() );
			Script::call( pCallbackFunc_.get(),
					Py_BuildValue( "(O)", Py_None ),
					"BigWorld.createEntityOnBaseApp: "  );
		}

		delete this;
	}

	PyObjectPtr pCallbackFunc_;
};


/*~ function BigWorld.createEntityOnBaseApp
 *	@components{ cell }
 *
 *	This method creates an entity on a BaseApp.
 *
 *  @param class class is the name of the class to instantiate. Note that this
 *	must be an entity class, as declared in the entities.xml file.
 *  @param state state is a dictionary describing the initial states of the
 *  entity's properties. A property will take on its default value from the
 *  .def file if it is not listed here.
 *	@param callback (optional) callback function taking a single argument
 *	of the newly created Base mailbox on success, or None on error.
 *
 *  @return None
 */
/**
 *	This method creates an entity on a base application.
 *
 *	@return True on success, false otherwise. If false is returned, the Python
 *				error state is set.
 */
static bool createEntityOnBaseApp( const BW::string & entityType,
	 PyObjectPtr pDict, PyObjectPtr pCallbackFunc )
{
	if (!pDict || pDict == Py_None)
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.createEntityOnBaseApp: Argument must be a dictionary" );
		return false;
	}

	if (!PyDict_Check( pDict.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.createEntityOnBaseApp: Argument must be a dictionary" );
		return false;
	}

	EntityTypePtr pEntityType = EntityType::getType( entityType.c_str() );
	if (!pEntityType)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntityOnBaseApp: "
			"Unknown entity type %s", entityType.c_str() );
		return false;
	}

	if (pCallbackFunc && !PyCallable_Check( pCallbackFunc.get() ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.createEntityOnBaseApp() "
			"callback must be a callable object if specified" );
		return false;
	}


	const EntityDescription & entityDesc = pEntityType->description();
	EntityTypeID entityTypeID = entityDesc.index();

	if (!entityDesc.canBeOnBase())
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntityOnBaseApp: "
			"Entity type %s has no base entity class", entityType.c_str() );
		return false;
	}

	std::auto_ptr< CreateBaseReplyHandler > pHandler(
		new CreateBaseReplyHandler( pCallbackFunc ) );

	Mercury::Channel & channel = CellApp::getChannel( 
		CellApp::instance().baseAppAddr() );

	// We don't use the channel's own bundle here because the streaming might
	// fail and the message might need to be aborted halfway through.
	std::auto_ptr< Mercury::Bundle > pBundle( channel.newBundle() );
	pBundle->startRequest( BaseAppIntInterface::createBaseWithCellData,
		pHandler.get() );

	*pBundle << EntityID( 0 ) << entityTypeID << DatabaseID( 0 );
	*pBundle << Mercury::Address( 0, 0 ); // dummy client address.
	*pBundle << BW::string(); // encryptionKey
	*pBundle << false;		// No logOnData
	*pBundle << false;		// Not persistent-only.

	if (!entityDesc.addDictionaryToStream( ScriptDict( pDict ), *pBundle,
			EntityDescription::BASE_DATA | EntityDescription::CELL_DATA ))
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.createEntityOnBaseApp: Bad dictionary" );

		return false;
	}

	if (entityDesc.canBeOnCell())
	{
		Vector3 position( 0, 0, 0 );
		Vector3 direction( 0, 0, 0 );
		SpaceID spaceID = 0;

		if (!Script::getValueFromDict( pDict.get(), POSITION_PROPERTY_NAME, position ) ||
			!Script::getValueFromDict( pDict.get(), DIRECTION_PROPERTY_NAME, direction ) ||
			!Script::getValueFromDict( pDict.get(), SPACE_ID_PROPERTY_NAME, spaceID ))
		{
			return false;
		}

		*pBundle << position << direction << spaceID;
	}

	channel.send( pBundle.get() );
	pHandler.release(); // handler deletes itself on callback

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, createEntityOnBaseApp,
			ARG( BW::string, ARG( PyObjectPtr,
				OPTARG( PyObjectPtr, NULL, END ) ) ),
			BigWorld )

// Deprecated name
PY_MODULE_FUNCTION_ALIAS( createEntityOnBaseApp, createEntityOnBase, BigWorld )

typedef SmartPointer<PyObject> PyObjectPtr;

/*~ function BigWorld createEntity
 *  @components{ cell }
 *  createEntity creates a new entity on the cell and places it in the world.
 *  This function also sets the new entity's class, location and facing,
 *  and may optionally set any of the entity's properties (as described in the
 *  entity's .def file).
 *
 *  Example:
 *  @{
 *  # create an open door entity at the same position as entity "thing"
 *  direction = ( 0, 0, thing.yaw )
 *  properties = { "open":1 }
 *  BigWorld.createEntity( "Door", thing.space, thing.position, direction,
 *                         properties )
 *  @}
 *  @param class class is the name of the class to instantiate. Note that this
 *  must be an entity class, as declared in the entities.xml file.
 *	@param spaceID spaceID is the id of the space in which to place the entity.
 *  @param position position is a sequence of 3 floats containing the position
 *  at which the new entity is to be spawned, in world coordinates.
 *  @param direction direction is a sequence of 3 floats containing the initial
 *  orientation of the new entity (roll, pitch, yaw), relative to the world
 *  frame.
 *  @param state state is a dictionary describing the initial states of the
 *  entity's properties. A property will take on its default value from the
 *  .def file if it is not listed here. It can also be a string, which will
 *  be handled as a TemplateID by an EntityDelegate, if any.
 *  @return The new entity.
 */
// TODO: Review this API. Should be args/kwargs based, like equivalent Base
// methods.
/**
 *	This method creates a new entity.
 */
static PyObject * createEntity( const BW::string & typeName, SpaceID spaceID,
	const Vector3 & position, const Vector3 & direction,
	const ScriptObject & state )
{
	// check that the space is OK
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (pSpace == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntity(): "
			"No space ID %d.", int(spaceID) );
		return NULL;
	}
	if (pSpace->pCell() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntity(): "
			"Space ID %d is not real.", int(spaceID) );
		return NULL;
	}

	// check that the class is OK
	EntityTypePtr pType = EntityType::getType( typeName.c_str() );

	if (!pType || !pType->canBeOnCell())
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntity(): "
			"Class '%s' is not a cell entity class", typeName.c_str() );
		return NULL;
	}

	bool isTemplate = false;
	ScriptDict properties;
	BW::string templateID;

	if (ScriptString::check( state ))
	{
		isTemplate = true;
		ScriptString( state ).getString( templateID );
	}
	else if (ScriptDict::check( state ))
	{
		properties = ScriptDict( state );
	}
	else if (state.isNone())
	{
		properties = ScriptDict::create();
	}
	else
	{
		PyErr_Format( PyExc_TypeError, "BigWorld.createEntity(): "
			"Entity properties argument must be a mapping, string or None." );
		return NULL;
	}

	// so make the creation stream then

	StreamHelper::AddEntityData entityData(
		NULL_ENTITY_ID, /* to allocate a unique id */
		position,
		/* isOnGround */ false,
		pType->description().index(),
		Direction3D( direction ), // Note: Vector3 taken as (Roll, Pitch, Yaw)
		isTemplate
		);
	MemoryOStream stream(8192);

	StreamHelper::addEntity( stream, entityData );

	// See Entity::readRealDataInEntityFromStreamForInitOrRestore
	// We must either provide a ScriptDict object, or set the
	// isTemplate flag and provide a template.
	// Alternatively a full EntityDescription::CELL_DATA stream would
	// work too, and simplify that code at the cost of streaming our
	// dictionary into the MemoryOStream and back
	MF_ASSERT( isTemplate || properties );

	if (isTemplate)
	{
		stream << templateID;
	}

	// The following is streamed off by RealEntity constructor.
	StreamHelper::addRealEntity( stream );

	stream << '-';	// no Witness

	stream.writePackedInt( 0 ); // no BASE_AND_CLIENT data.

	// And actually make the entity!
	EntityPtr pEntity =	pSpace->pCell()->createEntityInternal( 
		stream, properties );

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_SystemError, "BigWorld.createEntity(): "
			"Could not create entity of type %s: internal error",
			pType->name() );
		return NULL;
	}

	// Return it
	Py_INCREF( pEntity.get() );
	return pEntity.get();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createEntity,
	ARG( BW::string, ARG( SpaceID, ARG( Vector3, ARG( Vector3,
		OPTARG( ScriptObject, ScriptObject::none(), END ) ) ) ) ), BigWorld )


/*~ function BigWorld.time
 *	@components{ cell }
 *
 *	This method returns the current game time in seconds as a floating
 *	point number. CellApps internally maintain a counter which will be increased
 *	by one on every tick. The game time is derived from
 *	gameTickCounter/updateHertz. When the CellApp is overloaded, the length of
 *	ticks will be longer. In this case the game time will elapse more slowly,
 *	which can affect gameplay functionality that is based on this method.
 *	As soon as the CellApp is back to its normal load, it will try to catch up
 *	with the other CellApps by shortening the interval of ticks util its game
 *	time fully synchronises with other CellApps.
 *
 *  @return The current game time in seconds as a double.
 */
/**
 *	This method returns the current game time in seconds as a floating
 *	point number.
 */
static double time()
{
	return (double)CellApp::instance().time()/CellAppConfig::updateHertz();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, time, END, BigWorld )

/*~ function BigWorld.timeInTicks
 *	@components{ cell }
 *
 *	This method returns the current game time in ticks.
 *
 *  @return The current game time in ticks.
 */
/**
 *	This method returns the current game time ticks.
 */
static double timeInTicks()
{
	return (double)CellApp::instance().time();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, timeInTicks, END, BigWorld )


/*~ function BigWorld.getLoad
 *	@components{ cell }
 *
 *	This method returns the current CellApp load.
 *
 *  @return The current CellApp load as a float.
 */
/**
 *	This method returns the current CellApp load as a floating
 *	point number.
 */
static float getLoad()
{
	return CellApp::instance().getLoad();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getLoad, END, BigWorld )


/**
 *	This method implements the BigWorld.executeRawDatabaseCommand function.
 *  See DBAppInterfaceUtils::executeRawDatabaseCommand for more details.
 */
static bool executeRawDatabaseCommand( const BW::string & command,
	PyObjectPtr pResultHandler )
{
	CellApp & app = CellApp::instance();

	// TODO: Scalable DB:
	// Propagate the full DBApp hash so we can use a partitionKey to determine
	// the actual DBApp instead of always asking DBApp Alpha.
	return DBAppInterfaceUtils::executeRawDatabaseCommand( command,
		pResultHandler, app.dbAppAlpha().channel() );
}
PY_AUTO_MODULE_FUNCTION( RETOK, executeRawDatabaseCommand,
	ARG( BW::string, OPTARG( PyObjectPtr, NULL, END ) ),
	BigWorld )


/*~ function BigWorld.debugDump
 *	@components{ cell }
 *
 *	This method is for debugging only.
 */
/**
 *	This method is for debugging.
 */
static void debugDump()
{
	CellApp::instance().cells().debugDump();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, debugDump, END, BigWorld )


#ifdef MF_USE_VALGRIND
static void valgrindDump()
{
	if (RUNNING_ON_VALGRIND)
	{
		VALGRIND_DO_LEAK_CHECK;
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, valgrindDump, END, BigWorld )
#endif


/*~ function BigWorld.reloadScript
 * @components{ cell }
 *
 *	This method reloads Python modules related to entities and custom
 *	data types. The class of current entities are set to the newly loaded
 *	classes. This method should be used for development only and is not
 *	suitable for production use. The following points should be noted:
 *
 *	1) Scripts are only reloaded on the component cellapp that
 *	this method is called on. The user is responsible for making sure that
 *	reloadScript is called on all server components.
 *
 *	2) The client doesn't have equivalent functionality in Python. Each
 *	BigWorld client needs to use CapsLock + F11 in order to reload the scripts.
 *
 *	3) Doesn't handle custom data types transparently. Instances of custom
 *	data types must have their __class__ reassigned to the proper class type
 *	after reloadScript() completes. For example, if we have a custom data type
 *	named "CustomClass":
 *
 *  @{
 *  for e in BigWorld.entities.values():
 *     if type( e ) is Avatar.Avatar:
 *        e.customData.__class__ = CustomClass
 *  @}
 *	BWPersonality.onInit( True ) is called when this method completes (where
 *	BWPersonality is the name of the personality module specified in
 *	&lt;root&gt;/&lt;personality&gt; in bw.xml).
 *
 *	@param fullReload Optional boolean parameter that specifies whether to
 * 	reload entity definitions as well. If this parameter is False, then
 * 	entities definitions are not reloaded. Default is True.
 *
 *  @return True if reload successful, False otherwise.
 */
/**
 *	This method implements the Python function that reloads the scripts.
 */
static bool reloadScript( bool isFullReload )
{
	return CellApp::instance().reloadScript( isFullReload );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, reloadScript, OPTARG( bool, true, END ),
		BigWorld )


/**
 *	This function implements the BigWorld.hasStarted script method.
 */
static PyObject * py_hasStarted( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
				"BigWorld.hasStarted: Excepted no arguments." );
		return NULL;
	}

	return Script::getData( CellApp::instance().hasStarted() );
}
/*~ function BigWorld.hasStarted
 *  @components{ cell }
 *	The function hasStarted returns whether or not the server has started. This
 *	can be useful for identifying entities that are being created from the
 *	database during a controlled start-up.
 */
PY_MODULE_FUNCTION( hasStarted, BigWorld );


/**
 *	This function implements the BigWorld.isShuttingDown script method.
 */
static PyObject * py_isShuttingDown( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
				"BigWorld.isShuttingDown: Accepts no arguments." );
		return NULL;
	}

	return Script::getData( CellApp::instance().isShuttingDown() );
}
/*~ function BigWorld.isShuttingDown
 *  @components{ cell }
 *	The function isShuttingDown returns whether or not the server is shutting
 *	down. This return True after the onCellAppShuttingDown personality callback
 *	has been called. It can be useful for not allowing operations to start when
 *	in the process of shutting down.
 */
PY_MODULE_FUNCTION( isShuttingDown, BigWorld );


namespace Util
{
/**
 *	This function sets a shared data value.
 */
void setSharedData( const BW::string & key, const BW::string & value,
		SharedDataType dataType )
{
	CellAppMgrGateway & cellAppMgr = CellApp::instance().cellAppMgr();
	Mercury::Bundle & bundle = cellAppMgr.bundle();
	bundle.startMessage( CellAppMgrInterface::setSharedData );
	bundle << dataType << key << value;

	cellAppMgr.send();
}


/**
 *	This method is called when a shared data value is set.
 */
void onSetSharedData( PyObject * pKey, PyObject * pValue,
		SharedDataType dataType )
{
	const char * methodName = (dataType == SHARED_DATA_TYPE_CELL_APP) ?
								"onCellAppData" : "onGlobalData";

	CellApp::instance().scriptEvents().triggerEvent( methodName,
			PyTuple_Pack( 2, pKey, pValue ) );
}


/**
 *	This function deletes a shared data value.
 */
void delSharedData( const BW::string & key, SharedDataType dataType )
{
	CellAppMgrGateway & cellAppMgr = CellApp::instance().cellAppMgr();
	Mercury::Bundle & bundle = cellAppMgr.bundle();
	bundle.startMessage( CellAppMgrInterface::delSharedData );
	bundle << dataType << key;

	cellAppMgr.send();
}


/**
 *	This method is called when a shared data value is set.
 */
void onDelSharedData( PyObject * pKey, SharedDataType dataType )
{
	const char * methodName = (dataType == SHARED_DATA_TYPE_CELL_APP) ?
								"onDelCellAppData" : "onDelGlobalData";
	CellApp::instance().scriptEvents().triggerEvent( methodName,
			PyTuple_Pack( 1, pKey ) );
}

} // namespace Util


/*~ function BigWorld.setBaseAppData
 *	@components{ cell }
 *	@param key The key of the value to be set.
 *	@param value The new value.
 *
 *	This function sets a data value that is accessible from all BaseApps. This
 *	can be accessed on BaseApps.
 *
 *	The BaseAppMgr is the authoritative copy of this data. If two or more
 *	components try to set the same value, the last one to reach the BaseAppMgr
 *	will be the one that is kept.
 *
 *	Note: Care should be taken using this functionality. It does not scale well
 *	and should be used sparingly.
 */
PyObject * py_setBaseAppData( PyObject * args )
{
	PyObject * pKey = NULL;
	PyObject * pValue = NULL;

	if (!PyArg_ParseTuple( args, "OO:setBaseAppData", &pKey, &pValue ))
	{
		return NULL;
	}

	BW::string key = CellApp::instance().pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (key.empty())
	{
		PyErr_SetString( PyExc_TypeError, "key could not be pickled" );
		return NULL;
	}

	BW::string value = CellApp::instance().pickle( 
		ScriptObject( pValue, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (value.empty())
	{
		PyErr_SetString( PyExc_TypeError, "value could not be pickled" );
		return NULL;
	}

	Util::setSharedData( key, value, SHARED_DATA_TYPE_BASE_APP );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( setBaseAppData, BigWorld )


/*~ function BigWorld.delBaseAppData
 *	@components{ cell }
 *	@param key The key of the value to be deleted.
 *
 *	This function deletes a data value that is accessible from all BaseApps.
 *	This can be accessed on BaseApps.
 *
 *	The BaseAppMgr is the authoritative copy of this data. If two or more
 *	components try to set the same value, the last one to reach the BaseAppMgr
 *	will be the one that is kept.
 *
 *	Note: Care should be taken using this functionality. It does not scale well
 *	and should be used sparingly.
 */
PyObject * py_delBaseAppData( PyObject * args )
{
	PyObject * pKey = NULL;

	if (!PyArg_ParseTuple( args, "O:delBaseAppData", &pKey ))
	{
		return NULL;
	}

	BW::string key = CellApp::instance().pickle( 
		ScriptObject( pKey, ScriptObject::FROM_BORROWED_REFERENCE ) );

	if (key.empty())
	{
		PyErr_SetString( PyExc_TypeError, "key could not be pickled" );
		return NULL;
	}

	Util::delSharedData( key, SHARED_DATA_TYPE_BASE_APP );

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( delBaseAppData, BigWorld )


/*~ function BigWorld.maxCellAppLoad
 *	@components{ cell }
 *
 *	This function returns the maximum current load of any CellApp. This can be
 *	useful for deciding to disallow some script actions when the server is
 *	overloaded.
 */
float maxCellAppLoad()
{
	return CellApp::instance().maxCellAppLoad();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, maxCellAppLoad, END, BigWorld )


/*~ function BigWorld.load
 *	@components{ cell }
 *
 *	This function returns the current load of this CellApp. This can be useful
 *	for deciding to disallow some script actions when the server is overloaded.
 */
float load()
{
	return CellApp::instance().getLoad();
}
PY_AUTO_MODULE_FUNCTION( RETDATA, load, END, BigWorld )


/*~ function BigWorld.shutDownApp
 *	@components{ cell }
 *
 *	This method induce a controlled shutdown of this CellApp.
 */
void shutDownApp()
{
	CellApp::instance().shutDown();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, shutDownApp, END, BigWorld )


/*~ function BigWorld.controlledShutDown
 *	@components{ cell }
 *
 *	This method induces a controlled shutdown of the cluster.
 */
void controlledShutDown()
{
	CellApp::instance().triggerControlledShutDown();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, controlledShutDown, END, BigWorld )


/**
 *	This method loads any extensions (which live in DLLs)
 */
bool CellApp::initExtensions()
{
	return PluginLibrary::loadAllFromDirRelativeToApp( true, "-extensions" );
}


/* The following comments reside here because it doesn't seem right to have
 * declaration comments in executable code.
 */
/*~ attribute BigWorld entities
 *  @components{ cell }
 *  entities contains a list of all the entities which currently reside on the cell,
 *  including ghost entities.
 *  @type PyEntities
 */
/*~ attribute BigWorld services                                                                
 *  @components{ cell }                                                                             
 *  services is a map that keys a service name to a corresponding service
 *  fragment on one of the ServiceApps that offer it.
 *  @type PyBases                                                                                   
 */
/*~ attribute BigWorld VOLATILE_ALWAYS
 *  @components{ cell }
 *	This constant is to be used with the Entity.volatileInfo attribute to
 *	indicate that the value should always be sent to an interested client.
 */
/*~ attribute BigWorld VOLATILE_NEVER
 *  @components{ cell }
 *	This constant is to be used with the Entity.volatileInfo attribute to
 *	indicate that the value should never be sent to a client.
 */
/*~ attribute BigWorld NEXT_ONLY
 *	@components{ cell }
 *	This constant is used with the Entity.shouldAutoBackup property. This value
 *	indicates that the entity should be backed up next time it is considered
 *	and then this property is automatically set to False (0).
 */
/*~ attribute BigWorld INVALID_POSITION
 *	@components{ cell }
 *	This constant is used to represent an invalid entity position. An entity
 *	that has been teleported via Base.teleport will have this position when
 *	Entity.onTeleportSuccess is called. It is expected that the correct position
 *	is set during this callback.
 */
/*~ attribute BigWorld cellAppData
 *	@components{ cell }
 *	This property contains a dictionary-like object that is automatically
 *	duplicated between all CellApps. Whenever a value in the dictionary is
 *	modified, this change is propagated to all CellApps. The CellAppMgr keeps
 *	the authoritative copy of this information and resolves race conditions.
 *
 *	For example:
 *	@{
 *	BigWorld.cellAppData[ "hello" ] = "there"
 *	@}
 *
 *	Another CellApp can access this as follows:
 *	@{
 *	print BigWorld.cellAppData[ "hello" ]
 *	@}
 *
 *	The key and value can be any type that can be pickled and unpickled on all
 *	destination components.
 *
 *	When a value is changed or deleted, a callback function is called on all
 *	components.
 *	See: BWPersonality.onCellAppData and BWPersonality.onDelCellAppData.
 *
 *	NOTE: Only top-level values are propagated, if you have a value that is
 *	mutable (for example, a list) and it changed internally (for example,
 *	changing just one member), this information is NOT propagated.
 *
 *	Do NOT do the following:
 *	@{
 *	BigWorld.cellAppData[ "list" ] = [1, 2, 3]
 *	BigWorld.cellAppData[ "list" ][1] = 7
 *	@}
 *	Locally, this would look like [1, 7, 3] and remotely [1, 2, 3].
 */
/*~ attribute BigWorld globalData
 *	@components{ cell }
 *	This property contains a dictionary-like object that is automatically
 *	duplicated between all BaseApps and CellApps. Whenever a value in the
 *	dictionary is modified, this change is propagated to all BaseApps and
 *	CellApps. The CellAppMgr keeps the authoritative copy of this information
 *	and resolves race conditions.
 *
 *	For example:
 *	@{
 *	BigWorld.globalData[ "hello" ] = "there"
 *	@}
 *
 *	Another CellApp or BaseApp can access this as follows:
 *	@{
 *	print BigWorld.globalData[ "hello" ]
 *	@}
 *
 *	The key and value can be any type that can be pickled and unpickled on all
 *	destination components.
 *
 *	When a value is changed or deleted, a callback function is called on all
 *	components.
 *	See: BWPersonality.onGlobalData and BWPersonality.onDelGlobalData.
 *
 *	NOTE: Only top-level values are propagated, if you have a value that is
 *	mutable (for example, a list) and it changed internally (for example,
 *	changing just one member), this information is NOT propagated.
 *
 *	Do NOT do the following:
 *	@{
 *	BigWorld.globalData[ "list" ] = [1, 2, 3]
 *	BigWorld.globalData[ "list" ][1] = 7
 *	@}
 *	Locally, this would look like [1, 7, 3] and remotely [1, 2, 3].
 */
/*~ attribute BigWorld EXPOSE_CELL_APPS
 *	@components{ cell }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	all CellApp components owned by the manager.
 */
/*~ attribute BigWorld EXPOSE_LEAST_LOADED
 *	@components{ cell }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	the least loaded CellApp component owned by the manager.
 */
/*~ attribute BigWorld TRIANGLE_WATER
 *	@components{ cell }
 *	This flag is used to identify water triangles.
 */
/*~ attribute BigWorld TRIANGLE_TERRAIN
 *	@components{ cell }
 *	This flag is used to identify terrain triangles.
 */

template <class T>
void addToModule( PyObject * pModule, T value, const char * pName )
{
	PyObject * pObj = Script::getData( value );
	if (pObj)
	{
		PyObject_SetAttrString( pModule, pName, pObj );
		Py_DECREF( pObj );
	}
	else
	{
		PyErr_Clear();
	}
}


/**
 * 	This method is used to initialise the script associated with this
 * 	application.
 */
bool CellApp::initScript()
{
	this->scriptEvents().createEventType( "onAllSpaceGeometryLoaded" );
	this->scriptEvents().createEventType( "onAppReady" );
	this->scriptEvents().createEventType( "onAppShuttingDown" );
	this->scriptEvents().createEventType( "onBaseAppDeath" );
	this->scriptEvents().createEventType( "onCellAppData" );
	this->scriptEvents().createEventType( "onCellAppReady" );
	this->scriptEvents().createEventType( "onCellAppShuttingDown" );
	this->scriptEvents().createEventType( "onDelCellAppData" );
	this->scriptEvents().createEventType( "onDelGlobalData" );
	this->scriptEvents().createEventType( "onGlobalData" );
	this->scriptEvents().createEventType( "onServiceAppDeath" );
	this->scriptEvents().createEventType( "onSpaceData" );
	this->scriptEvents().createEventType( "onSpaceDataDeleted" );
	this->scriptEvents().createEventType( "onSpaceGeometryLoaded" );
	this->scriptEvents().createEventType( "onRecordingStarted" );
	this->scriptEvents().createEventType( "onRecordingTickData" );
	this->scriptEvents().createEventType( "onRecordingStopped" );

	if (!this->ScriptApp::initScript( "cell",
				EntityDef::Constants::entitiesCellPath() ) ||
#ifdef BUILD_PY_URLREQUEST
			!URL::Manager::init( this->mainDispatcher() ) ||
#endif
			!PyNetwork::init( this->mainDispatcher(), interface_ ))
	{
		return false;
	}

	Script::initExceptionHook( &mainDispatcher_ );

	// Pickler must be initialised after Python.
	pPickler_ = new Pickler();

	DataSectionPtr pSection =
		BWResource::openSection( EntityDef::Constants::entitiesFile() );

	if (!pSection)
	{
		ERROR_MSG("CellApp::initScript: Could not load entities.xml\n");
		return false;
	}

	// Initialise the CellApp module
	PyObject * pModule = PyImport_AddModule( "BigWorld" );

	PyObjectPtr pEntities( new PyEntities(), PyObjectPtr::STEAL_REFERENCE );

	if (PyObject_SetAttrString( pModule,
			"entities",
			pEntities.get() ) == -1)
	{
		ERROR_MSG( "CellApp::initScript: Failed to set entities\n" );

		return false;
	}

	// Initalize the service map now that python has started
	pPyServicesMap_ = new PyServicesMap();

	if (PyObject_SetAttrString( pModule,
			"services",
			pPyServicesMap_ ) == -1)
	{
		ERROR_MSG( "CellApp::initScript: Failed to set services\n" );

		return false;
	}

	PyObject_SetAttrString( pModule, "VOLATILE_ALWAYS",
			ScriptObject::createFrom( VolatileInfo::ALWAYS ).get() );
	PyObject_SetAttrString( pModule, "VOLATILE_NEVER", Py_None );

	PyObject_SetAttrString( pModule, "SPACE_DATA_FIRST_USER_KEY",
			ScriptObject::createFrom( SPACE_DATA_FIRST_USER_KEY ).get() );
	PyObject_SetAttrString( pModule, "SPACE_DATA_FINAL_USER_KEY",
			ScriptObject::createFrom( SPACE_DATA_FINAL_USER_KEY ).get() );
	PyObject_SetAttrString( pModule, "SPACE_DATA_FIRST_CELL_ONLY_KEY",
			ScriptObject::createFrom( SPACE_DATA_FIRST_CELL_ONLY_KEY ).get() );

	AutoBackupAndArchive::addNextOnlyConstant( pModule );

	// Add the watcher forwarding hints
	addToModule( pModule,
			ForwardingWatcher::CELL_APPS,          "EXPOSE_CELL_APPS" );
	addToModule( pModule,
			ForwardingWatcher::LEAST_LOADED, "EXPOSE_LEAST_LOADED" );
	// TODO: Add the following hints when the CellAppMgr support is
	//       implemented.
	//addToModule( pModule,
	//		ForwardingWatcher::WITH_ENTITY,  "EXPOSE_WITH_ENTITY" );
	//addToModule( pModule,
	//		ForwardingWatcher::WITH_SPACE,   "EXPOSE_WITH_SPACE" );

	addToModule( pModule, Entity::INVALID_POSITION, "INVALID_POSITION" );

	addToModule( pModule, (uint16) TRIANGLE_WATER, "TRIANGLE_WATER" );
	addToModule( pModule, (uint16) TRIANGLE_TERRAIN, "TRIANGLE_TERRAIN" );

	if (PyObject_SetAttrString( pModule,
			"Entity",
			reinterpret_cast<PyObject *>(&Entity::s_type_) ) == -1)
	{
		return false;
	}
	if (PyObject_SetAttrString( pModule,
			"UserDataObject",
			reinterpret_cast<PyObject *>(&UserDataObject::s_type_) ) == -1)
	{
		return false;
	}

	MF_ASSERT( pCellAppData_ == NULL );
	pCellAppData_ = new SharedData( SHARED_DATA_TYPE_CELL_APP,
		   Util::setSharedData, Util::delSharedData,
		   Util::onSetSharedData, Util::onDelSharedData,
		   pPickler_ );

	if (PyObject_SetAttrString( pModule, "cellAppData", pCellAppData_ ) == -1)
	{
		PyErr_Print();
		return false;
	}

	MF_ASSERT( pGlobalData_ == NULL );
	pGlobalData_ = new SharedData( SHARED_DATA_TYPE_GLOBAL,
		   Util::setSharedData, Util::delSharedData,
		   Util::onSetSharedData, Util::onDelSharedData,
		   pPickler_ );

	if (PyObject_SetAttrString( pModule, "globalData", pGlobalData_ ) == -1)
	{
		PyErr_Print();
		return false;
	}

	// Initialise the personality module
	if (!this->initPersonality())
	{
		return false;
	}

	return true;
}


/**
 * 	This is a helper method pickles the input Python object.
 */
BW::string CellApp::pickle( ScriptObject args )
{
	AUTO_SCOPED_PROFILE( "pickle" );
	return pPickler_->pickle( args );
}


/**
 *  This is a helper method used to unpickle the input data.
 */
ScriptObject CellApp::unpickle( const BW::string & str )
{
	AUTO_SCOPED_PROFILE( "unpickle" );
	return pPickler_->unpickle( str );
}


/**
 *	This is a helper method used to create an instance of a class without
 *	calling the __init__ method.
 */
PyObject * CellApp::newClassInstance( PyObject * pyClass,
		PyObject * pDictionary )
{
	// This code was inspired by new_instance function in Modules/newmodule.c in
	// the Python source code.

	PyInstanceObject * pNewObject =
		PyObject_New( PyInstanceObject, &PyInstance_Type );

	Py_INCREF( pyClass );
	Py_INCREF( pDictionary );
	pNewObject->in_class = (PyClassObject *)pyClass;
	pNewObject->in_dict = pDictionary;

	PyObject_GC_Init( pNewObject );

	return (PyObject *)pNewObject;
}

/**
 * 	Check the sanity of the python object chain.
 * 	This only compiles if Py_DEBUG is defined.
 */
void CellApp::checkPython()
{
#ifdef Py_DEBUG
	PyObject* head = PyInt_FromLong(1000000);
	PyObject* p = head;

	SCRIPT_INFO_MSG( "Checking python objects..." );

	while (p && p->_ob_next != head)
	{
		if ((p->_ob_prev->_ob_next != p) || (p->_ob_next->_ob_prev != p))
		{
			SCRIPT_CRITICAL_MSG( "Python object %0.8X is screwed\n", p );
		}

		p = p->_ob_next;
	}

	Py_DECREF(head);
	SCRIPT_INFO_MSG( "Python objects are Ok.\n" );
#endif
}


/**
 *	Signal handler.
 */
void CellApp::onSignalled( int sigNum )
{
	if (sigNum == SIGQUIT)
	{
		// Just print out some information, and pass it up to EntityApp to dump
		// core.

		ERROR_MSG( "CellApp::onSignalled: "
				"load = %f. emergencyThrottle = %f. "
				"Time since tick = %f seconds\n",
			this->getLoad(), this->emergencyThrottle(),
			stampsToSeconds( timestamp() - this->lastGameTickTime() ) );
	}

	this->EntityApp::onSignalled( sigNum );
}


/**
 *	This method adds the input Witness to the collection of objects that
 *	regularly have their update method called.
 *
 *	Objects that are registered with a lower level are updated before those with
 *	a higher level.
 */
bool CellApp::registerWitness( Witness * pObject, int level )
{
	return witnesses_.add( pObject, level );
}


/**
 *	This method removes the input Witness from the collection of objects that
 *	regularly have their update method called.
 */
bool CellApp::deregisterWitness( Witness * pObject )
{
	return witnesses_.remove( pObject );
}


/**
 *	This method calls 'update' on all registered Witnesses
 */
void CellApp::callWitnesses( bool checkReceivedTime )
{
	if (hasCalledWitnesses_)
	{
		return;
	}

	if (checkReceivedTime &&
			!pCellAppChannels_->haveAllChannelsReceivedTime( this->time() ))
	{
		return;
	}

	AUTO_SCOPED_PROFILE( "callWitnesses" );
	witnesses_.call();
	hasCalledWitnesses_ = true;
}


/**
 *	This method returns the next space entry ID
 *	
 *	SpaceIDs are per CellApp instance due to issues caused by cells offloading
 *	then being loaded back onto the same CellApp.
 *
 *	TODO: Handle case where address/port could be reused when a cellapp
 *		is retired then a new instance created.
 */
SpaceEntryID CellApp::nextSpaceEntryID()
{
	SpaceEntryID entryID = (const SpaceEntryID &)( interface_.address() );
	entryID.salt = ++nextSpaceEntryIDSalt_;
	return entryID;
}


BW_END_NAMESPACE

// cellapp.cpp
