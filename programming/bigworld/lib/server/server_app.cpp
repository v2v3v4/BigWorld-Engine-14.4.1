#include "server_app.hpp"

#include "manager_app_gateway.hpp"
#include "server_app_config.hpp"

#include "cstdmf/bw_util.hpp"
#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/profiler.hpp"
#include "cstdmf/watcher.hpp"

#include "network/event_dispatcher.hpp"
#include "network/network_interface.hpp"
#include "network/tcp_server.hpp"

#include "server/signal_processor.hpp"

#include <signal.h>
#include <dirent.h>
#include <sys/resource.h>

DECLARE_DEBUG_COMPONENT2( "Server", 0 );


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

class RetireAppCommand : public NoArgCallableWatcher
{
public:
	RetireAppCommand( ServerApp & app ) :
		// TODO: BWT-29273 Enable DBApp watcher forwarding 
		NoArgCallableWatcher( strcmp( app.getAppName(), "DBApp" ) == 0 ?
					CallableWatcher::LOCAL_ONLY : CallableWatcher::LEAST_LOADED,
				"Retire the least loaded app." ),
		app_( app )
	{
	}

private:
	virtual bool onCall( BW::string & output, BW::string & value )
	{
		INFO_MSG( "Requesting to retire this app.\n" );
		app_.requestRetirement();
		return true;
	}

	ServerApp & app_;
};


class ServerAppSignalHandler : public SignalHandler
{
public:
	ServerAppSignalHandler( ServerApp & serverApp ):
			serverApp_( serverApp )
	{}

	virtual ~ServerAppSignalHandler()
	{}

	virtual void handleSignal( int sigNum )
	{
		serverApp_.onSignalled( sigNum );
	}
private:
	ServerApp & serverApp_;
};

} // end namespace anonymous


u_int32_t ServerApp::discoveredInternalIP = 0;

/**
 *	Constructor.
 */
ServerApp::ServerApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface ) :
	time_( 0 ),
	mainDispatcher_( mainDispatcher ),
	interface_( interface ),
	startTime_( ::time( NULL ) ),
	buildDate_(),
	updatables_(),
	lastAdvanceTime_( 0 ),
	pSignalHandler_( NULL )
{
#if ENABLE_PROFILER
	g_profiler.init( 12*1024*1024, ServerAppConfig::profilerMaxThreads() );
	
	BW::string profileJsonDumpDir = \
						ServerAppConfig::profilerJsonDumpDirectory();
	if (!profileJsonDumpDir.empty())
	{
		g_profiler.setJsonDumpDir( profileJsonDumpDir );
	}
#endif

	MF_WATCH( "gameTimeInTicks", time_, Watcher::WT_READ_ONLY );
	MF_WATCH( "gameTimeInSeconds", *this, &ServerApp::gameTimeInSeconds );
	MF_WATCH( "uptime", *this, &ServerApp::uptime );
	MF_WATCH( "uptimeInSeconds", *this, &ServerApp::uptimeInSeconds );

	MF_WATCH( "dateBuilt", *this, &ServerApp::buildDate );
	MF_WATCH( "dateStarted", *this, &ServerApp::startDate );

	MF_WATCH( "pid", mf_getpid, "pid of this process" );
	MF_WATCH( "uid", getUserId, "uid of the user running this process" );

	interface_.pExtensionData( this );
}


/**
 *	Destructor.
 */
ServerApp::~ServerApp()
{}


/**
 *	Factory method for ServerApp subclasses to create their own signal
 *	handler instances. 
 *
 *	Subclasses should call enableSignalHandler() for each signal that this
 *	handler should handle. 
 *	
 *	The instance created is managed by the ServerApp instance.
 *
 *	The default ServerApp instance calls through to ServerApp::onSignalled().
 */
SignalHandler * ServerApp::createSignalHandler()
{
	return new ServerAppSignalHandler( *this );
}


/**
 *	This method adds the watchers associated with this class.
 */
void ServerApp::addWatchers( Watcher & watcher )
{
	watcher.addChild( "nub",
			Mercury::NetworkInterface::pWatcher(), &interface_ );

	if (this->pManagerAppGateway())
	{
		char buf[ 256 ];
		snprintf( buf, sizeof( buf ), "command/retire%s",
				this->getAppName() );

		watcher.addChild( buf, new RetireAppCommand( *this ) );
	}
}


/**
 *	This method adds the input object to the collection of objects that
 *	regularly have their update method called.
 *
 *	Objects that are registered with a lower level are updated before those with
 *	a higher level.
 */
bool ServerApp::registerForUpdate( Updatable * pObject, int level )
{
	return updatables_.add( pObject, level );
}


/**
 *	This method removes the input object from the collection of objects that
 *	regularly have their update method called.
 */
bool ServerApp::deregisterForUpdate( Updatable * pObject )
{
	return updatables_.remove( pObject );
}


/**
 *	This method calls 'update' on all registered Updatable interfaces.
 */
void ServerApp::callUpdatables()
{
	AUTO_SCOPED_PROFILE( "callUpdates" );
	updatables_.call();
}


/**
 *	Initialisation function.
 *	
 *	This needs to be called from subclasses' overrides.
 */
bool ServerApp::init( int argc, char * argv[] )
{
	PROFILER_SCOPED( ServerApp_init );

	bool runFromMachined = false;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp( argv[i], "-machined" ))
		{
			runFromMachined = true;
		}
	}

	INFO_MSG( "ServerApp::init: Run from bwmachined = %s\n",
			watcherValueToString( runFromMachined ).c_str() );

#if ENABLE_PROFILER
	if (ServerAppConfig::hasHitchDetection())
	{
		g_profiler.setProfileMode( Profiler::SORT_BY_TIME, false );
	}
#endif

	pSignalHandler_.reset( this->createSignalHandler() );

	// Handle signals
	this->enableSignalHandler( SIGINT );

	this->raiseFileDescriptorLimit( ServerAppConfig::maxOpenFileDescriptors() );

	interface_.verbosityLevel( ServerAppConfig::isProduction() ?
		Mercury::NetworkInterface::VERBOSITY_LEVEL_NORMAL :
		Mercury::NetworkInterface::VERBOSITY_LEVEL_DEBUG );

	return true;
}


/**
 *	This is the default implementation of run. Derived classes to override
 *	this to implement their own functionality.
 */
bool ServerApp::run()
{
	mainDispatcher_.processUntilBreak();

	this->onRunComplete();

	return true;
}


/**
 *	This method runs this application.
 */
bool ServerApp::runApp( int argc, char * argv[] )
{
	// calculate the clock speed
	stampsPerSecond();

	bool result = false;

	if (this->init( argc, argv ))
	{
		INFO_MSG( "---- %s is running ----\n", this->getAppName() );
		result = this->run();
	}
	else
	{
		ERROR_MSG( "Failed to initialise %s\n", this->getAppName() );
	}

	this->fini();

	interface_.prepareForShutdown();

#if ENABLE_PROFILER
	g_profiler.fini();
#endif

	return result;
}


/*
 * This method gives subclasses the tick period each time advanceTime is
 * called
 */
void ServerApp::onTickPeriod( double tickPeriod )
{
	if (tickPeriod * ServerAppConfig::updateHertz() > 2.0)
	{
		WARNING_MSG( "ServerApp::onTickPeriod: "
					"Last game tick took %.2f seconds. Expected %.2f.\n",
				tickPeriod, 1.0/ServerAppConfig::updateHertz() );
	}

#if ENABLE_PROFILER
	if (ServerAppConfig::hasHitchDetection() &&
		((tickPeriod * ServerAppConfig::updateHertz()) >
			ServerAppConfig::hitchDetectionThreshold()))
	{
		WARNING_MSG( "Service::onTickPeriod: "
			"Server hitch detected, creating JSON dump.\n" );
		g_profiler.dumpThisFrame();
	}
#endif
}


/**
 *	This method increments the game time.
 */
void ServerApp::advanceTime()
{
	if (lastAdvanceTime_ != 0)
	{
		double tickPeriod = stampsToSeconds( timestamp() - lastAdvanceTime_ );

		this->onTickPeriod( tickPeriod );
	}

	lastAdvanceTime_ = timestamp();

	this->onEndOfTick();

	++time_;

#if ENABLE_PROFILER
	g_profiler.tick();
#endif

	this->onStartOfTick();

	this->callUpdatables();

	this->onTickProcessingComplete();
}


/**
 *	This method returns the current game time in seconds.
 */
double ServerApp::gameTimeInSeconds() const
{
	return double( time_ )/ServerAppConfig::updateHertz();
}


/**
 *	This method returns the uptime of this process in seconds.
 */
time_t ServerApp::uptimeInSeconds() const
{
	time_t now = ::time( NULL );

	return now - startTime_;
}


namespace
{

/*
 *	This is a simple helper function to print a component of an elapsed time to
 *	a string.
 */
char * addTimeToBuf( char * pBuf, char * pBufEnd,
		time_t & uptime, time_t unitPeriod, const char * unitsStr )
{
	time_t count = uptime / unitPeriod;
	uptime -= count * unitPeriod;

	if (count > 0)
	{
		int numPrinted = bw_snprintf( pBuf, pBufEnd - pBuf,
				"%d %s%s ",
				(int)count,
				unitsStr,
				(count == 1) ? "" : "s" );

		if (numPrinted >= 0)
		{
			pBuf += numPrinted;
		}
		else
		{
			ERROR_MSG( "addTimeToBuf: snprintf failed\n" );
		}
	}

	return std::min( pBuf, pBufEnd );
}

} // anonymous namespace


/**
 *	This method returns the uptime of this process as a string.
 */
const char * ServerApp::uptime() const
{
	static char buffer[ 128 ];

	time_t uptime = this->uptimeInSeconds();

	const time_t ONE_SECOND = 1;
	const time_t ONE_MINUTE = 60;
	const time_t ONE_HOUR = 60 * ONE_MINUTE;
	const time_t ONE_DAY = 24 * ONE_HOUR;
	const time_t ONE_YEAR = 365 * ONE_DAY;

	char * pBuf = buffer;
	char * pBufEnd = buffer + sizeof( buffer );

	pBuf = addTimeToBuf( pBuf, pBufEnd, uptime, ONE_YEAR, "year" );
	pBuf = addTimeToBuf( pBuf, pBufEnd, uptime, ONE_DAY, "day" );
	pBuf = addTimeToBuf( pBuf, pBufEnd, uptime, ONE_HOUR, "hour" );
	pBuf = addTimeToBuf( pBuf, pBufEnd, uptime, ONE_MINUTE, "minute" );
	pBuf = addTimeToBuf( pBuf, pBufEnd, uptime, ONE_SECOND, "second" );

	return buffer;
}


/**
 *	This method returns the start time as a string.
 */
const char * ServerApp::startDate() const
{
	static char buf[26] = "";

	if (buf[0] == 0)
	{
		ctime_r( &startTime_, buf );

		// Remove trailing '\n'
		char * tail = strchr( buf, '\n' );

		if (tail)
		{
			*tail = '\0';
		}
	}

	return buf;

	// return ctime( &startTime_ );
}


/**
 *	Convert a shutdown stage to a readable string.
 */
const char * ServerApp::shutDownStageToString( ShutDownStage value )
{
	switch( value )
	{
	case SHUTDOWN_NONE:
		return "SHUTDOWN_NONE";
	case SHUTDOWN_REQUEST:
		return "SHUTDOWN_REQUEST";
	case SHUTDOWN_INFORM:
		return "SHUTDOWN_INFORM";
	case SHUTDOWN_DISCONNECT_PROXIES:
		return "SHUTDOWN_DISCONNECT_PROXIES";
	case SHUTDOWN_PERFORM:
		return "SHUTDOWN_PERFORM";
	case SHUTDOWN_TRIGGER:
		return "SHUTDOWN_TRIGGER";
	default:
		return "(unknown)";
	}
}


/**
 *	Enables or disables the handling of a given signal by the ServerApp
 *	instance's signal handler.
 */
void ServerApp::enableSignalHandler( int sigNum, bool enable )
{
	if (pSignalHandler_.get() == NULL)
	{
		ERROR_MSG( "ServerApp::enableSignalHandler: no signal handler set\n" );
		return;
	}

	if (enable)
	{
		SignalProcessor::instance().addSignalHandler( sigNum, 
			pSignalHandler_.get() );
	}
	else
	{
		SignalProcessor::instance().clearSignalHandler( sigNum, 
			pSignalHandler_.get() );
	}
}


/**
 *	This method requests that this application should be retired.
 */
void ServerApp::requestRetirement()
{
	if (!this->pManagerAppGateway())
	{
		ERROR_MSG( "ServerApp::requestRetirement: "
				"%s has no manager app gateway configured\n",
			this->getAppName() );
		return;
	}

	this->pManagerAppGateway()->retireApp();
}


/**
 *	Shutdown the server application.
 *
 *	Can be overridden by subclasses to do app-specific shutdown logic.
 */
void ServerApp::shutDown()
{
	INFO_MSG( "ServerApp::shutDown: shutting down\n" );
	mainDispatcher_.breakProcessing();
}


/**
 *	Default signal handling.
 */
void ServerApp::onSignalled( int sigNum )
{
	switch (sigNum)
	{
	case SIGINT:
		// This is really just to make sure that Python does not install its
		// own SIGINT handler.
		// It would probably be better to just revert to SIG_DFL straight after
		// Py_Initialize in Script::init().
		signal( sigNum, SIG_DFL );
		kill( getpid(), sigNum );
		break;

	default:
		break;
	}

}


/**
 *	This method attempts to bind the given network interface's UDP and TCP
 *	server to the same numeric port from a list of prescribed ports.
 *
 *	@param networkInterface		The network interface with the UDP socket to bind.
 *	@param tcpServer			The TCP server with the TCP socket to bind.
 *	@param externalInterfaceAddress
 *								The IP address of the external interface for
 *								binding.
 *	@param ports				The list of prescibed ports to attempt to bind to.
 */
bool ServerApp::bindToPrescribedPort( Mercury::NetworkInterface & networkInterface, 
		Mercury::TCPServer & tcpServer,
		const BW::string & externalInterfaceAddress, const Ports & ports )
{
	// Try binding with what the network interface currently has.
	if (networkInterface.isGood() && tcpServer.init())
	{
		return true;
	}

	if (ports.empty())
	{
		return false;
	}

	Ports::const_iterator iPort = ports.begin();

	// Skip first prescribed port, it's already been tried.
	++iPort;

	while (iPort != ports.end())
	{
		uint16 port = 0;
		if ((*iPort <= 0) || (*iPort >= 65536))
		{
			NETWORK_ERROR_MSG( "ServerApp::bindToPrescribedPort: "
					"Invalid port specified, ignoring: %d\n",
				*iPort );
			++iPort;
			continue;
		}
		else
		{
			port = static_cast<uint16>( *iPort );
		}

		NETWORK_INFO_MSG( "ServerApp::bindToPrescribedPort: "
				"Attempting to re-bind external interface to %s:%hu\n",
			externalInterfaceAddress.c_str(), port );

		if (networkInterface.recreateListeningSocket( htons( port ), 
				externalInterfaceAddress.c_str() ))
		{
			// OK, we have a UDP port, try binding the TCP port to the same
			// numeric port now.
			if (tcpServer.initWithPort( htons( port ) ))
			{
				return true;
			}
		}

		++iPort;
	}


	NETWORK_ERROR_MSG( "ServerApp::bindToPrescribedPort: "
			"Could not bind to any of the prescribed ports (%" PRIzu ") "
			"for external address: %s\n",
		ports.size(), externalInterfaceAddress.c_str() );

	return false;
}


/**
 *	This method attempts to bind the UDP socket of the given network interface
 *	and the TCP server to the same random port.
 *
 *	@param networkInterface		The network interface with the UDP socket to bind.
 *	@param tcpServer			The TCP server with the TCP socket to bind.
 *	@param externalInterfaceAddress
 *								The IP address of the external interface for
 *								binding.
 *
 */
bool ServerApp::bindToRandomPort( Mercury::NetworkInterface & networkInterface, 
		Mercury::TCPServer & tcpServer,
		const BW::string & externalInterfaceAddress )
{
	static const uint RANDOM_PORT_RETRIES = 20;
	uint numRetriesLeft = RANDOM_PORT_RETRIES;

	NETWORK_NOTICE_MSG( "Binding to random port\n" );

	while (numRetriesLeft)
	{
		if (networkInterface.recreateListeningSocket( 0, 
				externalInterfaceAddress.c_str() ))
		{
			// OK, we have a UDP port, try binding the TCP port to the same
			// numeric port now.
			if (tcpServer.init())
			{
				return true;
			}
		}

		--numRetriesLeft;
	}

	NETWORK_ERROR_MSG( "ServerApp::bindToRandomPort: "
			"Could not bind to random port after %u retries "
			"for external address: %s\n",
		RANDOM_PORT_RETRIES, externalInterfaceAddress.c_str() );
	return false;
}


#ifdef _WIN32
/**
 *	Attempt to raise our file descriptor limit
 */
void ServerApp::raiseFileDescriptorLimit( long limit )
{
	if (limit < 0)
	{
		return;
	}
	// According to http://msdn.microsoft.com/en-us/library/6e3b887c(VS.80).aspx
	// _setmaxstdio is limited to the range [_IOB_ENTRIES, 2048]
#define HARD_FD_LIMIT 2048L
	if (limit > HARD_FD_LIMIT)
	{
		WARNING_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Maximum file descriptor is limited to %lu, "
				"requested maximum file descriptor is %ld. Setting "
				"maximum file descriptor to %lu.\n",
			HARD_FD_LIMIT, limit, HARD_FD_LIMIT );
		limit = HARD_FD_LIMIT;
	}
#undef HARD_FD_LIMIT
	if (limit < _IOB_ENTRIES)
	{
		WARNING_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Maximum file descriptor must be greater than or "
				"equal to %u, requested maximum file descriptor is "
				"%ld. Setting maximum file descriptor to %u.\n",
			_IOB_ENTRIES, limit, _IOB_ENTRIES );
		limit = _IOB_ENTRIES;
	}

	if (limit <= _getmaxstdio())
		return;

	if (_setmaxstdio( (int)limit ) == -1)
	{
		ERROR_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Error setting maximum file descriptor: %s\n",
			strerror( errno ) );
	}
}
#else // !_WIN32
/**
 *	Attempt to raise our file descriptor limit
 */
void ServerApp::raiseFileDescriptorLimit( long limit )
{
	if (limit < 0)
	{
		return;
	}
	rlimit nofile;
	if (getrlimit( RLIMIT_NOFILE, &nofile ) == -1)
	{
		ERROR_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Error checking limit of maximum file descriptor: %s\n",
			strerror( errno ) );
		return;
	}

	if ((unsigned long)limit > nofile.rlim_max)
	{
		WARNING_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Maximum file descriptor is limited to %lu, "
				"requested maximum file descriptor is %ld. Setting "
				"maximum file descriptor to %lu.\n",
			nofile.rlim_max, limit, nofile.rlim_max );
		limit = nofile.rlim_max;
	}

	if ((unsigned long)limit <= nofile.rlim_cur)
		return;

	nofile.rlim_cur = limit;
	if (setrlimit( RLIMIT_NOFILE, &nofile ) == -1)
	{
		ERROR_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Error setting maximum file descriptor: %s\n",
			strerror( errno ) );
	}
}


/**
 *	This method returns the process file descriptor limit.
 *	
 *	@return The process file descriptor limit.
 */
int ServerApp::fileDescriptorLimit() const
{
	rlimit nofile;

	if (getrlimit( RLIMIT_NOFILE, &nofile ) == -1)
	{
		ERROR_MSG( "ServerApp::raiseFileDescriptorLimit: "
				"Error checking limit of maximum file descriptor: %s\n",
			strerror( errno ) );
		return -1;
	}

	return nofile.rlim_cur;
}


/**
 *	This method returns the number of file descriptors opened by this process.
 *	
 *	@return The number of file descriptors opened by this process.
 */
int ServerApp::numFileDescriptorsOpen() const
{
	// Subtract '..' and '.'
	int fds = -2;

	// procfs creates a symlink for each open file descriptor
	DIR *dir = opendir( "/proc/self/fd/" );

	if (!dir)
	{
		ERROR_MSG( "ServerApp::numFileDescriptorsLimit: "
				"Error opening /proc/self/fd/: %s\n",
			strerror( errno ) );
		return -1;
	}

	while (readdir( dir ) != NULL)
	{
		++fds;
	}

	closedir( dir );

	return fds;
}
#endif // _WIN32

BW_END_NAMESPACE

// server_app.cpp
