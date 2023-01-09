#include "script/first_include.hpp"

#include "main_app.hpp"

#include "client_app.hpp"
#include "entity.hpp"
#include "patrol_graph.hpp"
#include "py_bots.hpp"

// Callable Watchers exposing MainApp::instance() methods
#include "add_bots_callable_watcher.hpp"
#include "del_bots_callable_watcher.hpp"
#include "del_tagged_bots_callable_watcher.hpp"
#include "update_movement_callable_watcher.hpp"

#include "connection/login_challenge_factory.hpp"
#include "connection/rsa_stream_encoder.hpp"

#include "entitydef/constants.hpp"

#include "network/machined_utils.hpp"
#include "network/misc.hpp"
#include "network/watcher_nub.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/py_traceback.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/python_server.hpp"
#include "server/server_info.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/string_builder.hpp"

#ifdef __unix__
#include <signal.h>
#include <dlfcn.h>
#endif

#include <memory>

DECLARE_DEBUG_COMPONENT2( "Bots", 0 )


BW_BEGIN_NAMESPACE

extern int Math_token;
extern int ResMgr_token;
extern int PyScript_token;

extern int PyUserDataObject_token;
extern int UserDataObjectDescriptionMap_Token;
extern int AppScriptTimers_token;

extern int ChunkTerrain_token;

namespace
{
// These options are related to splitting the sends up over each tick.
const int TICK_FRAGMENTS = 1; // Currently not used.
const int TICK_FREQUENCY = 10;
const int TICK_TIMEOUT = 1000000/TICK_FREQUENCY/TICK_FRAGMENTS;
const float TICK_PERIOD = 1.f/TICK_FREQUENCY;
const float MINI_TICK_PERIOD = 0.1f / TICK_FRAGMENTS;
const int FILE_IO_THREADS = 1;
const int BG_THREADS = 1;

// This estimated value needs to be safely over the actual value to prevent
// epoll_create in EpollPoller::EpollPoller()'s initializer list exceeding
// the file descriptor limit.
const int FD_PER_BOT = 10;

int s_moduleTokens =
	Math_token | ResMgr_token | PyScript_token | AppScriptTimers_token;
int s_udoTokens = PyUserDataObject_token | UserDataObjectDescriptionMap_Token;
int s_chunkTokens = ChunkTerrain_token;
}

// -----------------------------------------------------------------------------
// Section: Static data
// -----------------------------------------------------------------------------

/// Bots Application Singleton.
BW_SINGLETON_STORAGE( MainApp )

// -----------------------------------------------------------------------------
// Section: class MainAppTimeQueue
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MainAppTimeQueue::MainAppTimeQueue( int updateHertz, MainApp & mainApp ) :
		ScriptTimeQueue( updateHertz ),
		mainApp_( mainApp )
{
}


/**
 *	This methods returns the current time.
 */
GameTime MainAppTimeQueue::time() const
{
	return mainApp_.time();
}


// -----------------------------------------------------------------------------
// Section: class MainApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MainApp::MainApp( Mercury::EventDispatcher & mainDispatcher, 
			Mercury::NetworkInterface & networkInterface ) :
		ServerApp( mainDispatcher, networkInterface ),
		pLogOnParamsEncoder_( NULL ), // initialised below in the constructor
		localTime_( 0.0 ),
		gameTimer_(),
		chunkTimer_(),
		sendTimeReportThreshold_( 10.0 ),
		timeQueue_( TICK_FREQUENCY, *this ),
		pPythonServer_( NULL ),
		clientTickIndex_( bots_.end() ),
		appID_(),
		nextBotID_( 0 ),
		nextSpaceEntryIDSalt_( 0 ),
		fileIOTaskManager_(),
		terrainManager_(),
		spaces_(),
		loginChallengeFactories_(),
		condemnedInterfaces_()
{
	ScriptTimers::init( timeQueue_ );

	srand( (unsigned int)timestamp() );
}


/**
 *	Destructor.
 */
MainApp::~MainApp()
{
	// Stop loading thread so that we can clean up without fear.
	bgTaskManager_.stopAll();
	fileIOTaskManager_.stopAll();
	INFO_MSG( "MainApp::~MainApp: Stopped background threads.\n" );

	Spaces::iterator it = spaces_.begin();
	while (it != spaces_.end())
	{
		PreloadedChunkSpace * pSpace = it->second;
		++it;

		pSpace->prepareNewlyLoadedChunksForDelete();
		bw_safe_delete( pSpace );
	}
	spaces_.clear();

	gameTimer_.cancel();
	chunkTimer_.cancel();

	timeQueue_.clear();

	ScriptTimers::fini( timeQueue_ );

	bots_.clear();
	bw_safe_delete( pPythonServer_ );

	Script::fini();
}


/**
 *	This method initialises the application.
 *
 *	@return True on success, otherwise false.
 */
bool MainApp::init( int argc, char * argv[] )
{
	if (!this->ServerApp::init( argc, argv ))
	{
		return false;
	}

	if (!this->initLogOnParamsEncoder( BotsConfig::publicKey() ))
	{
		ERROR_MSG( "MainApp::init: "
				"LoginApp public key initialisation failed, "
				"using unencrypted logins instead.\n" );
	}

	this->parseCommandLine( argc, argv );

	if (BotsConfig::serverName().empty())
	{
		Mercury::Address addr;
		Mercury::Reason lookupStatus = Mercury::MachineDaemon::findInterface(
											"LoginInterface", 0, addr, 10 );
		if (lookupStatus == Mercury::REASON_SUCCESS)
		{
			BotsConfig::serverName.set( addr.c_str() );
			/* ignore port_ config as we've got that from LoginInterface */
			BotsConfig::port.set( 0 );
			INFO_MSG( "Found LoginInterface via bwmachined at %s.",
				BotsConfig::serverName().c_str() );
		}
		else
		{
			ERROR_MSG( "MainApp::init: Failed to find LoginApp Interface "
					"(%s)\n", Mercury::reasonToString( lookupStatus ) );
			return false;
		}
	}

	if (BotsConfig::port() != 0)
	{
		char portNum[10];
		snprintf( portNum, 10, ":%d", BotsConfig::port() );
		BW::string newServerName = BotsConfig::serverName() + 
			BW::string( portNum );

		BotsConfig::serverName.set( newServerName );
	}

	if (!terrainManager_.isValid())
	{
		ERROR_MSG( "MainApp::init: TerrainManager is not valid\n" );
		return false;
	}

	gameTimer_ = this->mainDispatcher().addTimer(
			TICK_TIMEOUT,
			this, 
			reinterpret_cast< void * >( TIMEOUT_GAME_TICK ),
			"BotsTick" );

	int loadingTickMicroseconds =
		std::max( int( 1000000 * Config::chunkLoadingPeriod() ), 5000 );
	chunkTimer_ = this->mainDispatcher().addTimer(
			loadingTickMicroseconds,
			this,
			reinterpret_cast< void * >( TIMEOUT_CHUNK_TICK ),
			"ChunkTick" );

	bgTaskManager_.initWatchers( "Bots" );
	bgTaskManager_.startThreads( "BGTask Manager", BG_THREADS );

	fileIOTaskManager_.initWatchers( "FileIO" );
	fileIOTaskManager_.startThreads( "FileIO", FILE_IO_THREADS );

	char buffer[256];
	StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	builder.append( ServerInfo().serverName() );
	builder.append( ":" );
	builder.append( ntohs( interface_.address().port ) );

	appID_ = builder.string();

	if (!this->initScript())
	{
		return false;
	}

	this->initWatchers();

	Script::initExceptionHook( &(this->mainDispatcher()) );

	Mercury::MachineDaemon::registerWithMachined( interface_.address(), "bots",
			0 /*ignoredID*/ );

	INFO_MSG( "MainApp::init: AppID = %s\n", appID_.c_str() );

	return true;
}


/**
 *	This method preloads a GeometryMapping into a new space.
 */
PreloadedChunkSpace * MainApp::addSpaceGeometryMapping( ChunkSpaceID spaceID,
	BW::Matrix & matrix, const BW::string & name )
{
	Spaces::iterator it = spaces_.find( spaceID );

	if (it != spaces_.end())
	{
		return NULL;
	}

	SpaceEntryID entryID = (const SpaceEntryID &)( interface_.address() );
	entryID.salt = ++nextSpaceEntryIDSalt_;

	PreloadedChunkSpace * pSpace =
		new PreloadedChunkSpace( matrix, name, entryID, spaceID, *this );

	spaces_[ spaceID ] = pSpace;

	INFO_MSG( "MainApp::addSpaceGeometryMapping: Mapped %s to space %u",
			name.c_str(), spaceID );

	return pSpace;
}


PyObject * MainApp::addSpaceGeometryMapping( ChunkSpaceID spaceID,
	MatrixProviderPtr pMapper, const BW::string & path )
{
	Matrix m = Matrix::identity;

	if (pMapper)
	{
		pMapper->matrix( m );

		float yaw = m.yaw();
		float pitch = m.pitch();
		float roll = m.roll();

		if (!almostZero( fmod( yaw, MATH_PI/2.f ) ) ||
			!almostZero( pitch ) ||
			!almostZero( roll ))
		{
			ERROR_MSG( "MainApp::addSpaceGeometryMapping: "
					"'%s'. Rotation needs to be axis aligned. "
					"yaw = %.2f. pitch = %.2f. roll = %.2f\n",
				path.c_str(),
				RAD_TO_DEG( yaw ), RAD_TO_DEG( pitch ), RAD_TO_DEG( roll ) );
		}
	}

	PreloadedChunkSpace * pSpace = this->addSpaceGeometryMapping(
		spaceID, m, path );

	if (!pSpace)
	{
		PyErr_Format( PyExc_AttributeError,
				"Space %u is already mapped", spaceID ); 
		return NULL;
	}

	return Script::getData( pSpace->entryID() );
}


/*
 *	Override from ServerGeometryMappingsHandler.
 */
void MainApp::onSpaceGeometryLoaded( SpaceID spaceID, const BW::string & name )
{
	ScriptObject personality = this->getPersonalityModule();

	if (personality)
	{
		personality.callMethod( "onSpaceGeometryLoaded",
			ScriptArgs::create( spaceID, name ),
			ScriptErrorPrint( "BWPersonality.onSpaceGeometryLoaded" ),
			/* allowNullMethod */ true );
	}
}


/**
 *	This method creates the new bot client with a specified username and
 *	password, performing any common initialisation and ads it to the Bot
 *	processes list of bots clients.
 *
 *	@param botName      The username of the bot to be created.
 *	@param botPassword  The password of the bot to be created.
 */
void MainApp::addBotWithName( const BW::string & botName,
	const BW::string & botPassword )
{
	int fdOpen = this->numFileDescriptorsOpen();
	int fdLimit = this->fileDescriptorLimit();

	if ((fdOpen + FD_PER_BOT) > fdLimit)
	{
		ERROR_MSG( "MainApp::addBotWithName: Failed to add %s. File descriptors"
				" limit will be exceeded: per bot=%d, opened=%d, limit=%d\n",
				botName.c_str(), FD_PER_BOT, fdOpen, fdLimit );
		return;
	}

	ClientApp * pClientApp = new ClientApp( botName, botPassword,
		ConnectionTransport( BotsConfig::transport() ), 
		BotsConfig::tag() );
	pClientApp->connectionSendTimeReportThreshold( sendTimeReportThreshold_ ); 
	bots_.push_back( ClientAppPtr( pClientApp, ScriptObject::FROM_NEW_REFERENCE ) ); 
}


/**
 *	This method adds another simulated client to this application.
 */
void MainApp::addBot()
{
	BW::string bname = BotsConfig::username();
	BW::string bpass = BotsConfig::password();
	if (BotsConfig::shouldUseRandomName())
	{
		char buffer[256];
		StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
		builder.append( "_" );
		builder.append( ++nextBotID_ );
		builder.append( "_" );
		builder.append( appID_ );

		bname.append( builder.string() );
	}

	this->addBotWithName( bname, bpass );
}


/**
 *	This method adds a number of simulated clients to this application.
 */
void MainApp::addBots( int num )
{
	for (int i = 0; i < num; ++i)
	{
		this->addBot();
	}
}


/**
 *	This method adds a set of new bot client applications to the Bots process
 *	using a pregenerated list of usernames and passwords.
 *
 *	@param pCredentialSequence  The Python list containing tuples of
 *	                            username/password pairs to be used in creating
 *	                            the ClientApps.
 */
void MainApp::addBotsWithName( PyObjectPtr pCredentialSequence )
{
	if (!pCredentialSequence || pCredentialSequence == Py_None)
	{
		PyErr_SetString( PyExc_TypeError,
			"Bots::addBotsWithName: Empty login info. "
			"Expecting a list of tuples containing username and password." );
		return;
	}

	if (!PySequence_Check( pCredentialSequence.get() ))
	{
		PyErr_SetString( PyExc_TypeError, "Bots::addBotsWithName: "
			"Expecting a list of tuples containing username and password." );
		return;
	}

	Py_ssize_t numCredentials = PySequence_Size(
		pCredentialSequence.get() );
	for (Py_ssize_t i = 0; i < numCredentials; ++i)
	{
		PyObject * pCredentials = PySequence_GetItem(
			pCredentialSequence.get(), i );
		if (!PyTuple_Check( pCredentials ) || PyTuple_Size( pCredentials ) != 2)
		{
			PyErr_Format( PyExc_TypeError,
				"Bots::addBotsWithName: Argument list item %" PRIzd " must "
				"be tuple of two strings.", i );

			Py_XDECREF( pCredentials );
			return;
		}

		PyObject * pClientName = PySequence_GetItem( pCredentials, 0 );
		PyObject * pClientPassword = PySequence_GetItem( pCredentials, 1 );

		Py_DECREF( pCredentials );

		if (!PyString_Check( pClientName ) ||
			!PyString_Check( pClientPassword ))
		{
			PyErr_Format( PyExc_TypeError, "Bots::addBotsWithName: "
				"Invalid credentials for element %" PRIzd ". Expecting a tuple "
				"containing a username and password.", i );

			Py_XDECREF( pClientName );
			Py_XDECREF( pClientPassword );

			return;
		}

		this->addBotWithName( BW::string( PyString_AsString( pClientName ) ),
			BW::string( PyString_AsString( pClientPassword ) ) );

		Py_DECREF( pClientName );
		Py_DECREF( pClientPassword );
	}
}


/**
 *	This method removes a number of simulated clients from this application.
 */
void MainApp::delBots( int num )
{
	while (num-- > 0 && !bots_.empty())
	{
		Bots::iterator iter = bots_.begin();
		if (iter == clientTickIndex_)
		{
			++clientTickIndex_;
		}
		bots_.front()->destroy();
		bots_.pop_front();
	}
}


/**
 *	This method updates the movement controllers of all bots matching the input
 *	tag based on the current default values.
 *
 *	If the input tag is empty, all bots are changed.
 */
void MainApp::updateMovement( BW::string tag )
{
	Bots::iterator iter = bots_.begin();

	while (iter != bots_.end())
	{
		if (tag.empty() || ((*iter)->tag() == tag))
		{
			bool ok = (*iter)->setMovementController( 
				BotsConfig::controllerType(),
				BotsConfig::controllerData() );
			if (!ok)
			{
				PyErr_Print();
			}
		}
		++iter;
	}
}


/**
 *	This method deletes tagged entities.
 */
void MainApp::delTaggedEntities( BW::string tag )
{
	Bots::iterator iter = bots_.begin();
	Bots condemnedBots; //Call destructors when going out of scope

	while (iter != bots_.end())
	{
		Bots::iterator oldIter = iter++;
		if ((*oldIter)->tag() == tag)
		{
			if (oldIter == clientTickIndex_)
			{
				++clientTickIndex_;
			}
			condemnedBots.push_back(*oldIter);
			bots_.erase( oldIter );
		}
	}
}


/*
 *	Override from TimerExpiryHandler.
 */
void MainApp::handleTimeout( TimerHandle /*handle*/, void * arg )
{
	switch (reinterpret_cast< uintptr >( arg ))
	{
		case TIMEOUT_GAME_TICK:
			this->handleGameTickTimeSlice();
			break;

		case TIMEOUT_CHUNK_TICK:
		{
			this->handleChunkTickTimeSlice();
			break;
		}
	}
}


/**
 * 	This method ticks all the ClientApps.
 */
void MainApp::handleGameTickTimeSlice()
{
	static bool inTick = false;

	if (inTick)
	{
		// This can occur because the tick method of ClientApp can processInput
		// on the ServerConnection.
		WARNING_MSG( "MainApp::handleTimeout: Called recursively\n" );
		return;
	}

	inTick = true;

	ScriptObject personality = this->getPersonalityModule();

	if (personality)
	{
		// give Bots personality script a chance to handle

		personality.callMethod( "onTick",
			ScriptErrorPrint( "BWPersonality.onTick" ),
			/* allowNullMethod */ true );
	}

	static int remainder = 0;
	int numberToUpdate = (bots_.size() + remainder) / TICK_FRAGMENTS;
	remainder = (bots_.size() + remainder) % TICK_FRAGMENTS;

	localTime_ += MINI_TICK_PERIOD;

	while (numberToUpdate-- > 0 && !bots_.empty())
	{
		if (clientTickIndex_ == bots_.end())
		{
			clientTickIndex_ = bots_.begin();	
		}
		Bots::iterator iter = clientTickIndex_++;
		// tick may have called destroyBots(), which
		// could invalidate the current iterator and
		// free the ClientApp. Make a ptr so the bot holds on.
		ClientAppPtr pClientApp = *iter;
		if (!pClientApp->tick( TICK_PERIOD ))
		{
			if (!pClientApp->isDestroyed())
			{
				if (pClientApp->logOnRetryTime() != 0)
				{
					botsForLogOnRetry_.push_back( pClientApp );
				}
				bots_.erase( iter );
			}
		}
	}

	uint64 now = timestamp();

	Bots::iterator retryIter = botsForLogOnRetry_.begin();

	while (retryIter != botsForLogOnRetry_.end())
	{
		if ((*retryIter)->logOnRetryTime() < now)
		{
			(*retryIter)->logOn();
			bots_.push_back( (*retryIter) );
			retryIter = botsForLogOnRetry_.erase( retryIter );
		}
		else
		{
			++retryIter;
		}
	}

	this->advanceTime();

	condemnedInterfaces_.processOnce();

	inTick = false;
}


/**
 * 	This method ticks the chunk loading
 */
void MainApp::handleChunkTickTimeSlice()
{
	bgTaskManager_.tick();
	fileIOTaskManager_.tick();

	Spaces::iterator it = spaces_.begin();
	while (it != spaces_.end())
	{
		it->second->chunkTick();
		++it;
	}
}


/**
 *	This method calls timers
 */
void MainApp::onTickProcessingComplete()
{
	this->ServerApp::onTickProcessingComplete();
	timeQueue_.process( time_ );
}


/**
 *	Thie method returns personality module
 */
ScriptObject MainApp::getPersonalityModule() const
{
	return Personality::instance();
}


/** 
 *  Set the send time report threshold for new connections and existing 
 *  connections. If a client app takes longer than this threshold for 
 *  successive sends to the server, it will emit a warning message. 
 * 
 *  @param threshold    the new send time reporting threshold 
*/ 
void MainApp::sendTimeReportThreshold( double threshold ) 
{ 
	// Set it here for new connections 
	sendTimeReportThreshold_ = threshold; 
	// Also set it for any client apps we already have 
	Bots::iterator iBot = bots_.begin(); 
	while (iBot != bots_.end()) 
	{ 
		(*iBot)->connectionSendTimeReportThreshold( threshold ); 
		++iBot; 
	} 
}


namespace
{
typedef BW::map< BW::string, MovementFactory * > MovementFactories;
MovementFactories * s_pMovementFactories;
}

/**
 *	This method returns a default movement controller instance.
 */
MovementController * MainApp::createDefaultMovementController(
	float & speed, Vector3 & position )
{
	return this->createMovementController( speed, position,
		BotsConfig::controllerType(), BotsConfig::controllerData() );
}


/**
 *	This method creates a movement controller corresponding to the input
 *	arguments.
 */
MovementController *
	MainApp::createMovementController( float & speed, Vector3 & position,
							   const BW::string & controllerTypeIn,
							   const BW::string & controllerData )
{
	BW::string controllerType = controllerTypeIn;

	if (controllerType == "None") return NULL;

#ifdef __unix__
	uint soPos = controllerType.find( ".so:" );
   	if (soPos < controllerType.length())
	{
		BW::string soName = controllerType.substr( 0, soPos+3 );
		static BW::set<BW::string> loadedLibs;
		if (loadedLibs.find( soName ) == loadedLibs.end())
		{
			loadedLibs.insert( soName );
			BW::string soPath = "bots-extensions/"+soName;
			void * handle = dlopen( soPath.c_str(), RTLD_LAZY | RTLD_GLOBAL );
			if (handle == NULL)
			{
				ERROR_MSG( "MainApp::createMovementController: "
					"Failed to load dyn lib '%s' since %s\n",
					soName.c_str(), dlerror() );
			}
			else
			{
				INFO_MSG( "MainApp::createMovementController: "
					"Loaded dyn lib '%s'\n", soName.c_str() );
			}
		}

		controllerType = controllerType.substr( soPos+4 );
	}
#endif

	if (s_pMovementFactories != NULL)
	{
		MovementFactories::iterator iter =
				s_pMovementFactories->find( controllerType );

		if (iter != s_pMovementFactories->end())
		{
			return iter->second->create( controllerData, speed,
						position );
		}
	}

	PyErr_Format( PyExc_TypeError, "No such controller type '%s'",
		controllerType.c_str() );
	return NULL;
}


/**
 *	This static method registers a MovementFactory.
 */
void MainApp::addFactory( const BW::string & name, MovementFactory & factory )
{
	if (s_pMovementFactories == NULL)
	{
		s_pMovementFactories = new MovementFactories;
	}

	(*s_pMovementFactories)[ name ] = &factory;
}


// -----------------------------------------------------------------------------
// Section: Script related
// -----------------------------------------------------------------------------

/**
 *	This method returns the client application with the input id.
 */
ClientAppPtr MainApp::findApp( EntityID id ) const
{
	if (id == NULL_ENTITY_ID)
	{
		// Cannot find domant bots by ID
		return ClientAppPtr();
	}

	// This is inefficient. Could look at making a map of these but it should
	// not be used this way very often.(??)
	Bots::const_iterator iter = bots_.begin();

	while (iter != bots_.end())
	{
		ClientAppPtr pApp = *iter;

		// When deleting multiple bots, there may be holes in the bots_ vector
		// temporarily, so jump over any NULL pointers we encounter
		if (pApp && pApp->id() == id)
		{
			return pApp;
		}

		++iter;
	}

	return ClientAppPtr();
}


/**
 *	This method populates a list with the IDs of available apps.
 */
void MainApp::appsKeys( PyObject * pList ) const
{
	Bots::const_iterator iter = bots_.begin();

	while (iter != bots_.end())
	{
		if ((*iter)->id() != NULL_ENTITY_ID)
		{
			PyObject * pInt = PyInt_FromLong( (*iter)->id() );
			PyList_Append( pList, pInt );
			Py_DECREF( pInt );
		}

		++iter;
	}
}


/**
 *	This method populates a list with available apps.
 */
void MainApp::appsValues( PyObject * pList ) const
{
	Bots::const_iterator iter = bots_.begin();

	while (iter != bots_.end())
	{
		if ((*iter)->id() != NULL_ENTITY_ID)
		{
			PyList_Append( pList, 
				const_cast< ClientApp* >( iter->get() ));
		}

		++iter;
	}
}


/**
 *	This method populates a list with id, value pairs of the available apps.
 */
void MainApp::appsItems( PyObject * pList ) const
{
	Bots::const_iterator iter = bots_.begin();

	while (iter != bots_.end())
	{
		if ((*iter)->id() != NULL_ENTITY_ID)
		{
			PyObject * pTuple = PyTuple_New( 2 );
			PyTuple_SetItem( pTuple, 0, PyInt_FromLong( (*iter)->id() ) );
			Py_INCREF( (*iter).get() );
			PyTuple_SetItem( pTuple, 1, 
				const_cast< ClientApp* >( iter->get() ) );
			PyList_Append( pList, pTuple );
			Py_DECREF( pTuple );
		}

		++iter;
	}
}


/**
 *	This method parses the command line for options used by the bots process.
 */
void MainApp::parseCommandLine( int argc, char * argv[] )
{
	// Get any command line arguments
	for (int i = 0; i < argc; i++)
	{
		if (strcmp( "-serverName", argv[i] ) == 0)
		{
			i++;
			BotsConfig::serverName.set( ( i < argc ) ? argv[ i ] : "" );
			INFO_MSG( "Server name is %s\n", BotsConfig::serverName().c_str() );
		}
		else if (strcmp( "-username", argv[i] ) == 0)
		{
			i++;
			BotsConfig::username.set( 
				( i < argc ) ? argv[ i ] : BotsConfig::username() );
			INFO_MSG( "Username is %s\n", BotsConfig::username().c_str() );
		}
		else if (strcmp( "-password", argv[i] ) == 0)
		{
			i++;
			BotsConfig::password.set( 
				( i < argc ) ? argv[ i ] : BotsConfig::password() );
		}
		else if (strcmp( "-port", argv[i] ) == 0)
		{
			i++;
			BotsConfig::port.set( 
				( i < argc ) ? atoi(argv[ i ]) : BotsConfig::port() );
		}
		else if (strcmp( "-randomName", argv[i] ) == 0)
		{
			BotsConfig::shouldUseRandomName.set( true );
		}
		else if (strcmp( "-scripts", argv[i] ) == 0)
		{
			BotsConfig::shouldUseScripts.set( true );
		}
	}
}


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
 *	Initialise Python scripting.
 */
bool MainApp::initScript()
{
	PyImportPaths paths;
	paths.addResPath( EntityDef::Constants::botScriptsPath() );
	paths.addResPath( EntityDef::Constants::serverCommonPath() );

	Script::init( paths, "bot" );

	PyObject * pModule = PyImport_AddModule( "BigWorld" );

	if (pModule)
	{
		PyObject * pBots = new PyBots;
		addToModule( pModule, pBots, "bots" );
		Py_DECREF( pBots );

		addToModule( pModule, CallableWatcher::LOCAL_ONLY, "EXPOSE_LOCAL_ONLY" );
		addToModule( pModule, (uint16) TRIANGLE_TERRAIN, "TRIANGLE_TERRAIN" );
	}

	// Initialise the entity descriptions.
	// Read entities scripts anyway as we need to create player entities
	// for logoff purpose
	if (!EntityType::init( BotsConfig::standinEntity() ))
	{
		ERROR_MSG( "MainApp::init: "
				"Could not initialise entity data. Abort!\n" );
		return false;
	}

	Personality::import(
		BWConfig::get( "personality", Personality::DEFAULT_NAME ).c_str() );

	ScriptObject personality = this->getPersonalityModule();

	if (personality)
	{
		personality.callMethod( "onBotsReady", ScriptErrorPrint( "onBotsReady" ),
			/* allowNullMethod */ true );
	}

	char buffer[256];
	StringBuilder builder( buffer, ARRAY_SIZE( buffer ) );
	builder.append( "Welcome to Bots " );
	builder.append( appID_ );

	pPythonServer_ = new PythonServer( builder.string() );
	pPythonServer_->startup( mainDispatcher_, 0,
							 interface_.address().port, 0 );
	PyRun_SimpleString( "import BigWorld" );

	return true;
}


void MainApp::initWatchers()
{
	// Register the watcher
	BW_REGISTER_WATCHER( 0, "bots", "bots", this->mainDispatcher(),
						 interface_.address() );

	Watcher & root = Watcher::rootWatcher();
	this->ServerApp::addWatchers( root );

	root.addChild( "command/addBots", new AddBotsCallableWatcher() );
	root.addChild( "command/delBots", new DelBotsCallableWatcher() );

	MF_WATCH( "tag", BotsConfig::tag.getRef() );
	root.addChild( "command/delTaggedEntities",
		new DelTaggedBotsCallableWatcher() );

	MF_WATCH( "numBots", bots_, &Bots::size );

	MF_WATCH( "pythonServerPort", *pPythonServer_, &PythonServer::port );

	/* */
	MF_WATCH( "defaultControllerType", BotsConfig::controllerType.getRef() );
			// MF_ACCESSORS( BW::string, MainApp, controllerType ) );
	MF_WATCH( "defaultControllerData", BotsConfig::controllerData.getRef() );
			// MF_ACCESSORS( BW::string, MainApp, controllerData ) );
	MF_WATCH( "defaultStandinEntity", BotsConfig::standinEntity.getRef() );
	root.addChild( "command/updateMovement",
		new UpdateMovementCallableWatcher() );

	MF_WATCH( "sendTimeReportThreshold", *this, 
			MF_ACCESSORS( double, MainApp, sendTimeReportThreshold ) ); 
}


/**
 *	This method initialises the log on parameter encoder with the LoginApp key
 *	file specified by the given resource path.
 *
 *	@param path		The resource path to the key.
 *	@return 		True if the encoder was initialised, false otherwise.
 */
bool MainApp::initLogOnParamsEncoder( const BW::string & path )
{
	std::auto_ptr< RSAStreamEncoder > pEncoder( 
		new RSAStreamEncoder( /* keyIsPrivate */ false ) );

	DataSectionPtr pPublicKey = BWResource::openSection( path );
	if (!pPublicKey)
	{
		return false;
	}

	BinaryPtr pBinData = pPublicKey->asBinary();
	BW::string keyString( pBinData->cdata(), pBinData->len() );

	if (!pEncoder->initFromKeyString( keyString ))
	{
		return false;
	}

	pLogOnParamsEncoder_.reset( pEncoder.release() );

	return true;
}


// -----------------------------------------------------------------------------
// Section: BigWorld script functions
// -----------------------------------------------------------------------------

namespace // anonymous
{


static PyObject * addSpaceGeometryMapping( ChunkSpaceID spaceID,
	MatrixProviderPtr pMapper, const BW::string & path )
{
	MainApp & app = MainApp::instance();
	return app.addSpaceGeometryMapping( spaceID, pMapper, path );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, addSpaceGeometryMapping, ARG( SpaceID,
	ARG( MatrixProviderPtr,
	ARG( BW::string, END )) ), BigWorld )


void addBots( int count )
{
	MainApp::instance().addBots( count );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, addBots, ARG( int, END ), BigWorld )


void addBotsWithName( PyObjectPtr logInfoData )
{
	// validate input log info. it should be a list of tuples of
	// user name and password.
	// eg: [ ("user1", "pass1"), ("user2", "pass2"), ... ]
	MainApp::instance().addBotsWithName( logInfoData );
	if (PyErr_Occurred())
	{
		PyErr_Print();
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, addBotsWithName, ARG( PyObjectPtr, END ), 
	BigWorld )


void delBots( int count )
{
	MainApp::instance().delBots( count );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, delBots, ARG( int, END ), BigWorld )


void delTaggedEntities( BW::string tag )
{
	MainApp::instance().delTaggedEntities( tag );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, delTaggedEntities, ARG( BW::string, END ),
	BigWorld )


#define DEFAULT_ACCESSOR( N1, N2 )											\
BW::string getDefault##N2()												\
{																			\
	return BotsConfig::N1();												\
}																			\
PY_AUTO_MODULE_FUNCTION( RETDATA, getDefault##N2, END, BigWorld )			\
																			\
void setDefault##N2( const BW::string & v )								\
{																			\
	BotsConfig::N1.set( v );												\
}																			\
																			\
PY_AUTO_MODULE_FUNCTION( RETVOID,											\
		setDefault##N2, ARG( BW::string, END ), BigWorld )					\

DEFAULT_ACCESSOR( serverName, Server )
DEFAULT_ACCESSOR( username, Username )
DEFAULT_ACCESSOR( password, Password )
DEFAULT_ACCESSOR( tag, Tag )
DEFAULT_ACCESSOR( controllerType, ControllerType )
DEFAULT_ACCESSOR( controllerData, ControllerData )


class BotAdder : public TimerHandler
{
public:
	/**
	 *	Constructor.
	 *	@param total 	Number of bots to add.
	 *	@param period 	The tick period to add bots.
	 *	@param perTick	How many bots to add each tick.
	 */
	BotAdder( int total, float period, int perTick ) :
		remaining_( total ),
		perTick_( perTick )
	{
		timer_ = MainApp::instance().mainDispatcher().addTimer(
				int(period * 1000000), this, NULL, "BotAdder" );
	}

	/**
	 *	Destructor.
	 */
	virtual ~BotAdder()
	{
		timer_.cancel();
	}

	/**
	 *	Override from TimerHandler.
	 */
	virtual void handleTimeout( TimerHandle handle, void * )
	{
		MainApp::instance().addBots( std::min( remaining_, perTick_ ) );
		remaining_ -= perTick_;

		if (remaining_ <= 0)
		{
			timer_.cancel();
			delete this;
		}
	}

private:
	int remaining_;
	int perTick_;

	TimerHandle timer_;
};

void addBotsSlowly( int count, float period, int perTick )
{
	new BotAdder( count, period, perTick );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, addBotsSlowly,
		ARG( int, OPTARG( float, 1.f, OPTARG( int, 1, END ) ) ), BigWorld )


static PyObject * py_time( PyObject * args )
{
	BW_GUARD;
	return PyFloat_FromDouble( MainApp::instance().localTime() );
}
PY_MODULE_FUNCTION( time, BigWorld )


} // namespace (anonymous)

BW_END_NAMESPACE


/*~ function BigWorld.addSpaceGeometryMapping
 *	@components{ bots }
 *
 *	This function maps geometry into the given space ID. There can be only
 *	one geometry mapped to a space. It cannot be used with spaces created by
 *	the server since the server	controls the geometry mappings in those spaces.
 *
 *	The given transform must be aligned to the chunk grid. That is, it should
 *	be a translation matrix whose position is in multiples of the space's chunkSize
 *	on the X and Z axis. Any other transform will result in undefined behaviour. 
 *
 *	Any extra space mapped in must use the same terrain system as the first,
 *	with the same settings, the behaviour of anything else is undefined.
 *
 *	Raises a ValueError if the space already has a geometry mapped.
 *
 *	@param spaceID 		The ID of the space
 *	@param matrix 		The transform to apply to the geometry. None may be
 *						passed in if no transform is required (the identity
 *						matrix).
 *	@param filepath 	The path to the directory containing the space data
 *	@return 			(integer) handle that is used when removing mappings
 *						using BigWorld.delSpaceGeometryMapping().
 */
/*~ function BigWorld.addBots
 *	@components{ bots }
 *
 *	This function immediately creates a specified number of simulated clients. 
 *	These clients will log into an existing BigWorld server known to the 
 *	Bots application.
 *
 *	@param	num		Number of simulated clients to be added.
 */
/*~ function BigWorld.addBotsSlowly
 *	@components{ bots }
 *
 *	This function creates a specified number of simulated clients (similar to 
 *	BigWorld.addBots) in a slower and controlled fashion (in groups) to 
 *	prevent sudden surge of load that would overwhelm the server.
 *
 *	@param	num		Number of simulated clients to be added (will be broken up 
 *					into groups of size groupSize).
 *	@param	delay	a delay (in seconds) between groups
 *	@param	groupSize	group size. default is 1.
 *
 *	For example:
 *	@{
 *	BigWorld.addBotsSlowly( 200, 5, 20 )
 *	@}
 *	This example will add 200 clients in total in groups of 20 with 5
 *	seconds intervals between adding groups.
 */

/*~ function BigWorld.addBotsWithName
 *	@components{ bots }
 *
 *	This function creates a number of simulated clients with specific
 *	login names and passwords.
 *
 *	@param	PyObject	list of tuples of login name and password
 *
 *	For example:
 *	@{
 *	BigWorld.addBotsWithName( [('tester01', 'tp1'),('tester02', 'tp2'),('tester03', 'tp3')] )
 *	@}
 */
/*~ function BigWorld.delBots
 *	@components{ bots }
 *
 *	This function immediately removes a specified number of simulated
 *	clients from the Bots application. These clients will be logged
 *	off the server gracefully.
 *
 *	@param	num		Number of simulated clients to be removed.
 */
/*~ function BigWorld.delTaggedEntities
 *	@components{ bots }
 *
 *	This function immediately removes clients that match a specified tag.
 *	These clients will be logged off the server gracefully.
 *
 *	@param	tag		String tag to match.
 */
/*~ function BigWorld.setDefaultServer
 *	@components{ bots }
 *
 *	This function sets the default login server for the simulated
 *	clients.
 *
 *	@param	host		string in format of 'host:port'.
 */
/*~ function BigWorld.getDefaultServer
 *	@components{ bots }
 *
 *	This function returns the name of the default login server for 
 *	the simulated clients.
 *
 *	@return	host		string in format of 'host:port'.
 */
/*~ function BigWorld.setDefaultUsername
 *	@components{ bots }
 *
 *	This function sets the default login user name for 
 *	the simulated clients to use.
 *
 *	@param	username		string.
 */
/*~ function BigWorld.getDefaultUsername
 *	@components{ bots }
 *
 *	This function returns the default login user name for 
 *	the simulated clients to use.
 *
 *	@return	username		string.
 */
/*~ function BigWorld.setDefaultPassword
 *	@components{ bots }
 *
 *	This function sets the default login password for 
 *	the simulated clients to use.
 *
 *	@param	password		string.
 */
/*~ function BigWorld.getDefaultPassword
 *	@components{ bots }
 *
 *	This function returns the default login password for 
 *	the simulated clients to use.
 *
 *	@return	password		string.
 */
/*~ function BigWorld.setDefaultTag
 *	@components{ bots }
 *
 *	This function sets the default tag name for simulated clients.
 *
 *	@param	tag		string.
 */
/*~ function BigWorld.getDefaultTag
 *	@components{ bots }
 *
 *	This function returns the default tag name for the simulated
 *	clients.
 *
 *	@return	tag		string.
 */
/*~ function BigWorld.setDefaultControllerType
 *	@components{ bots }
 *
 *	This function sets the default movement controller to be used by
 *	the simulated clients. The input parameter should be the name of a
 *	compiled shared object file (with .so extension) containing a
 *	movement controller, residing under the bot-extensions
 *	subdirectory of your Bots installation.  Note: we recommend you to
 *	use the builtin default movement controller and create custom data
 *	for it instead of creating your own movement controller.
 *
 *	@param	controllerType		string.
 */
/*~ function BigWorld.getDefaultControllerType
 *	@components{ bots }
 *
 *	This function returns the default movement controller to
 *	be used by the simulated clients.
 *
 *	@return	controllerType		string.
 */
/*~ function BigWorld.setDefaultControllerData
 *	@components{ bots }
 *
 *	This function sets the default data file used by the current
 *	default movement controller. The input parameter should be the
 *	relative path (with respect to the BigWorld res path) to a data
 *	file. Make sure the data file is can be understood by your
 *	movement controller, otherwise the movement controller may cause
 *	an error and bring down the Bots application.
 *
 *	@param	controllerData		string.
 */
/*~ function BigWorld.getDefaultControllerData
 *	@components{ bots }
 *
 *	This function returns the default movement controller to
 *	be used by the simulated clients.
 *
 *	@return	controllerData		string.
 */
/*~ function BigWorld.time
 *	@components{ bots }
 *
 *	This function returns the bots process time. This is the elapsed time in
 *	seconds since this process has started.
 *
 *	@return		a float.  The time since start up.
 */
/*~ attribute BigWorld bots
 *  @components{ bots }
 *
 *  bots contains a list of all simulated clients currently instanced on the
 *  Bots application.  
 *
 *  @type Read-only PyBots
 */
/*~ attribute BigWorld TRIANGLE_TERRAIN
 *	@components{ bots }
 *	This flag is used to identify terrain triangles.
 */
// main_app.cpp
