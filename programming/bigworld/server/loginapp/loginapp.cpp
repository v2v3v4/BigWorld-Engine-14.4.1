#include "loginapp.hpp"

#include "add_to_dbappmgr_helper.hpp"
#include "bw_config_login_challenge_config.hpp"
#include "database_reply_handler.hpp"
#include "login_int_interface.hpp"
#include "login_stream_filter_factory.hpp"
#include "loginapp_config.hpp"
#include "status_check_watcher.hpp"

#include "connection/log_on_status.hpp"
#include "connection/login_challenge.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/login_interface.hpp"
#include "connection/rsa_stream_encoder.hpp"
#include "connection/client_server_protocol_version.hpp"

#include "db/dbapp_interface.hpp"
#include "db/dbappmgr_interface.hpp"

#include "network/encryption_filter.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machined_utils.hpp"
#include "network/nub_exception.hpp"
#include "network/once_off_packet.hpp"
#include "network/portmap.hpp"
#include "network/symmetric_block_cipher.hpp"
#include "network/udp_bundle.hpp"
#include "network/watcher_nub.hpp"

#include "resmgr/bwresource.hpp"

#include "server/bwconfig.hpp"
#include "server/nat_config.hpp"
#include "server/server_app_option_macros.hpp"
#include "server/util.hpp"
#include "server/writedb.hpp"

#include <pwd.h>
#include <signal.h>

#include <sys/types.h>


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

/// Timer period when updating login statistics
const uint32 LoginApp::UPDATE_STATS_PERIOD = 1000000;

/// LoginApp Singleton.
BW_SINGLETON_STORAGE( LoginApp )

namespace // anonymous
{
uint gNumLogins = 0;
uint gNumLoginFailures = 0;
uint gNumLoginAttempts = 0;

const int WARN_BUNDLE_PACKETS = 3;


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------


bool commandStopServer( BW::string & output, BW::string & value )
{
	if (LoginApp::pInstance())
	{
		LoginApp::pInstance()->controlledShutDown();
	}

	return true;
}

bool commandClearIPAddressBans( BW::string & output, BW::string & value )
{
	if (LoginApp::pInstance())
	{
		LoginApp::pInstance()->clearIPAddressBans();
	}
	output.assign("Done.");
	return true;
}

BW::string serverProtocolVersionString()
{
	return ClientServerProtocolVersion::currentVersion().c_str();
}

uint16 getExternalPort()
{
	int port = BWConfig::get( "loginApp/externalPorts/port", PORT_LOGIN );

	if (LoginAppConfig::shouldOffsetExternalPortByUID())
	{
		port += getUserId();
	}

	return htons(static_cast< uint16 >(port));
}

} // namespace (anonymous)


// -----------------------------------------------------------------------------
// Section: LoginApp
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
LoginApp::LoginApp( Mercury::EventDispatcher & mainDispatcher,
		Mercury::NetworkInterface & intInterface ) :
	ServerApp( mainDispatcher, intInterface ),
	pLogOnParamsEncoder_( NULL ),
	extInterface_( &mainDispatcher,
			Mercury::NETWORK_INTERFACE_EXTERNAL,
			getExternalPort(),
			Config::externalInterface().c_str() ),
	pStreamFilterFactory_( BWConfig::get( "shouldUseWebSockets", true ) ?
		new LoginStreamFilterFactory : NULL ),
	tcpServer_( extInterface_, Config::tcpServerBacklog() ),
	systemOverloaded_( 0 ),
	systemOverloadedTime_( 0 ),
	loginRequests_(),
	dbAppAlpha_( interface_ ),
	dbAppMgr_(),
	repliedFailsCounterResetTime_( 0 ),
	numFailRepliesLeft_( 0 ),
	lastRateLimitCheckTime_( 0 ),
	numAllowedLoginsLeft_( 0 ),
	nextIPAddressBanMapCleanupTime_( 0 ),
	id_(-1),
	loginStats_(),
	statsTimer_(),
	tickTimer_()
{
	extInterface_.pExtensionData( static_cast< ServerApp * >( this ) );

	BW::string extInterfaceConfig = Config::externalInterface();
	// TODO: this code is duplicated from BaseApp (see r124025).
	//       Make it a common function.
	DataSectionPtr pPorts = BWConfig::getSection( "loginApp/externalPorts" );

	Ports externalPorts;
	if (pPorts)
	{
		pPorts->readInts( "port", externalPorts );
	}

	bool didBind = this->bindToPrescribedPort( extInterface_, tcpServer_,
			extInterfaceConfig, externalPorts );

	if (!didBind &&
			(externalPorts.empty() || !Config::shouldShutDownIfPortUsed()))
	{
		this->bindToRandomPort( extInterface_, tcpServer_,
			extInterfaceConfig );
	}

	extInterface_.isVerbose( Config::verboseExternalInterface() );

	dbAppAlpha_.channel().isLocalRegular( false );
	dbAppAlpha_.channel().isRemoteRegular( false );
}


/**
 *	Destructor.
 */
LoginApp::~LoginApp()
{
	this->extInterface().prepareForShutdown();
	statsTimer_.cancel();
	tickTimer_.cancel();
}


/*
 *	Override from ServerApp.
 */
bool LoginApp::init( int argc, char * argv[] ) /* override */
{
	if (!this->ServerApp::init( argc, argv ))
	{
		return false;
	}

	if (!extInterface_.isGood())
	{
		ERROR_MSG( "Loginapp::init: "
			"Unable to bind external interface to any specified ports.\n" );
		return false;
	}

	if (!tcpServer_.isGood())
	{
		ERROR_MSG( "LoginApp::init: Unable to bind TCP server.\n" );
		return false;
	}

	tcpServer_.pStreamFilterFactory( pStreamFilterFactory_.get() );

	NETWORK_INFO_MSG( "Server protocol version: %s\n",
			ClientServerProtocolVersion::currentVersion().c_str() );

	if (!this->initLogOnParamsEncoder())
	{
		return false;
	}

	if (!this->intInterface().isGood())
	{
		ERROR_MSG( "Failed to create Nub on internal interface.\n" );
		return false;
	}

	if ((extInterface_.address().ip == 0) ||
			(this->intInterface().address().ip == 0))
	{
		ERROR_MSG( "LoginApp::init: Failed to open UDP ports. "
				"Maybe another process already owns it\n" );
		return false;
	}

	if (!NATConfig::postInit())
	{
		return false;
	}

	if (NATConfig::isInternalIP( this->intInterface().address().ip ) ||
		NATConfig::isInternalIP( extInterface_.address().ip ))
	{
		NETWORK_INFO_MSG( "Local subnet: %s\n",
				NATConfig::localNetMask().c_str() );
		NETWORK_INFO_MSG( "NAT external addr: %s\n",
				NATConfig::externalAddress().c_str() );
	}
	else
	{
		ERROR_MSG( "LoginApp::LoginApp: "
					"localNetMask %s does not match local ip %s\n",
				NATConfig::localNetMask().c_str(),
				this->intInterface().address().c_str() );

		return false;
	}

	if (Config::maxRepliesOnFailPerSecond() < 2)
	{
		ERROR_MSG("LoginApp::LoginApp: "
		"maxRepliesOnFailPerSecond in bw.xml should be >= 2");
		return false;
	}

	MF_WATCH( "numLogins", gNumLogins );
	MF_WATCH( "numLoginFailures", gNumLoginFailures );
	MF_WATCH( "numLoginAttempts", gNumLoginAttempts );
	MF_WATCH( "clientServerProtocol", serverProtocolVersionString );
	MF_WATCH( "numBannedIPAddresses", ipAddressBanMap_,
				&IPAddressBanMap::size );

	// ---- What used to be in loginsvr.cpp

	ReviverSubject::instance().init( &this->intInterface(), "loginApp" );

	// make sure the interface came up ok
	if (extInterface_.address().ip == 0)
	{
		NETWORK_CRITICAL_MSG( "LoginApp::init: "
			"Failed to listen to external interface.\n"
			"\tLogin port is probably in use.\n"
			"\tIs there another LoginApp server running on this machine?\n" );
		return false;
	}

	PROC_IP_INFO_MSG( "External address = %s\n",
		extInterface_.address().c_str() );
	PROC_IP_INFO_MSG( "Internal address = %s\n",
		this->intInterface().address().c_str() );

	int numStartupRetries = Config::numStartupRetries();

	if (!BW_INIT_ANONYMOUS_CHANNEL_CLIENT( dbAppMgr_, this->intInterface(),
			LoginIntInterface, DBAppMgrInterface, numStartupRetries ))
	{
		ERROR_MSG( "LoginApp::init: Could not find DBAppMgr\n" );
		return false;
	}

	LoginInterface::registerWithInterface( extInterface_ );
	LoginIntInterface::registerWithInterface( this->intInterface() );

	MF_WATCH( "systemOverloaded", systemOverloaded_ );

	if (Config::allowProbe() && Config::isProduction())
	{
		CONFIG_ERROR_MSG( "Production Mode: bw.xml/loginApp/allowProbe "
			"is enabled. This is a development-time feature only and should "
			"be disabled during production.\n" );
	}

	// Enable latency / loss on the external interface
	extInterface_.setLatency( Config::externalLatencyMin(),
			Config::externalLatencyMax() );
	extInterface_.setLossRatio( Config::externalLossRatio() );

	if (extInterface_.hasArtificialLossOrLatency())
	{
		NETWORK_WARNING_MSG( "LoginApp::init: "
			"External Nub loss/latency enabled\n" );
	}

	extInterface_.maxSocketProcessingTime(
		Config::maxExternalSocketProcessingTime() );

	// Set up the rate limiting parameters
	if (Config::rateLimitEnabled())
	{
		NETWORK_INFO_MSG( "LoginApp::init: "
				"Login rate limiting enabled: period = %u, limit = %d\n",
			Config::rateLimitDuration(), Config::loginRateLimit() );
	}

	numAllowedLoginsLeft_ = Config::loginRateLimit();
	lastRateLimitCheckTime_ = timestamp();

	extInterface_.rateLimitPeriod( Config::rateLimitDuration() );
	extInterface_.perIPAddressRateLimit( Config::ipAddressRateLimit() );
	extInterface_.perIPAddressPortRateLimit( Config::ipAddressPortRateLimit() );

	// This calls back on finishInit().
	new AddToDBAppMgrHelper( *this );

	if (!challengeFactories_.configureFactories(
			*(BWConfigLoginChallengeConfig::root()) ))
	{
		return false;
	}

	if (!Config::challengeType().empty() && 
			!challengeFactories_.getFactory( Config::challengeType() ))
	{
		CONFIG_ERROR_MSG( "LoginApp::init: "
				"No such challenge type: \"%s\"\n",
			Config::challengeType().c_str() );

		return false;
	}

	MF_WATCH( BW_OPTION_WATCHER_DIR "challengeType", *this,
			&LoginApp::challengeType,
			&LoginApp::challengeType );

	return true;
}


/**
 *	This method handles a DBAppMgr birth.
 */
void LoginApp::handleDBAppMgrBirth(
	const LoginIntInterface::handleDBAppMgrBirthArgs & args )
{
	// by this moment we should have gotten an id
	MF_ASSERT( id_ != -1 );
	DEBUG_MSG( "LoginApp::handleDBAppMgrBirth: "
		"Already got an ID, just notify DBAppMgr of our existence\n" );
	Mercury::Bundle	& bundle = this->dbAppMgr().bundle();
	DBAppMgrInterface::recoverLoginAppArgs & notifyArgs =
		DBAppMgrInterface::recoverLoginAppArgs::start( bundle );

	notifyArgs.id = id_;

	this->dbAppMgr().send();
}


/**
 *	This method completes initialisation after registration to DBAppMgr.
 *
 *	@param appID 				The LoginApp ID.
 *	@param dbAppAlphaAddress 	The address of DBApp Alpha.
 */
bool LoginApp::finishInit( LoginAppID appID,
		const Mercury::Address & dbAppAlphaAddress )
{
	DEBUG_MSG( "LoginApp::finishInit: id %d (DBApp Alpha: %s)\n",
		appID, dbAppAlphaAddress.c_str() );

	id_ = appID;
	dbAppAlpha_.addr( dbAppAlphaAddress );

	Mercury::Reason reason =
		LoginIntInterface::registerWithMachined( this->intInterface(), id_ );

	if (reason != Mercury::REASON_SUCCESS)
	{
		NETWORK_ERROR_MSG( "LoginApp::finishInit: "
			"Unable to register with interface. Is machined running?\n");
		return false;
	}

	if (Config::registerExternalInterface())
	{
		LoginInterface::registerWithMachined( extInterface_, id_ );
	}

	LoggerMessageForwarder::pInstance()->registerAppID( id_ );

	Mercury::MachineDaemon::registerBirthListener( interface_.address(),
		LoginIntInterface::handleDBAppMgrBirth, "DBAppMgrInterface" );

	// Handle USR1 for controlled shutdown.
	this->enableSignalHandler( SIGUSR1 );

	// Start up watchers
	BW_REGISTER_WATCHER( 0, "loginapp", "loginApp",
			mainDispatcher_, this->intInterface().address() );

	Watcher & root = Watcher::rootWatcher();
	this->ServerApp::addWatchers( root );
	root.addChild( "nubExternal", Mercury::NetworkInterface::pWatcher(),
		&extInterface_ );

	root.addChild( "command/statusCheck", new StatusCheckWatcher() );
	root.addChild( "command/shutDownServer",
			new NoArgFuncCallableWatcher( commandStopServer,
				CallableWatcher::LOCAL_ONLY,
				"Shuts down the entire server" ) );
	root.addChild( "command/clearIPAddressBans",
			new NoArgFuncCallableWatcher( commandClearIPAddressBans,
				CallableWatcher::LOCAL_ONLY,
				"Clears the list of banned IP addresses" ) );

	root.addChild( "dbAppMgr", Mercury::ChannelOwner::pWatcher(),
			&this->dbAppMgr() );

	WatcherPtr pStatsWatcher = new DirectoryWatcher();
	pStatsWatcher->addChild( "rateLimited",
			makeWatcher( loginStats_, &LoginStats::rateLimited ) );
	pStatsWatcher->addChild( "repeatedForAlreadyPending",
			makeWatcher( loginStats_, &LoginStats::pending ) );
	pStatsWatcher->addChild( "failures",
			makeWatcher( loginStats_, &LoginStats::fails ) );
	pStatsWatcher->addChild( "successes",
			makeWatcher( loginStats_, &LoginStats::successes ) );
	pStatsWatcher->addChild( "all",
			makeWatcher( loginStats_, &LoginStats::all ) );

	{
		// watcher doesn't like const-ness
		static uint32 s_updateStatsPeriod = LoginApp::UPDATE_STATS_PERIOD;
		pStatsWatcher->addChild( "updatePeriod",
				makeWatcher( s_updateStatsPeriod ) );
	}

	root.addChild( "averages", pStatsWatcher );

	WatcherPtr pChallengeStatWatcher = new DirectoryWatcher();
	
	pChallengeStatWatcher->addChild( "calculationTime",
			makeWatcher( loginStats_,
				&LoginStats::challengeCalculationAverage ) );
	pChallengeStatWatcher->addChild( "verificationTime",
			makeWatcher( loginStats_,
				&LoginStats::challengeVerificationAverage ) );

	WatcherPtr pChallengeFactoryConfigRoot = new DirectoryWatcher();
	challengeFactories_.addWatchers( pChallengeFactoryConfigRoot );

	root.addChild( "challenges/config", pChallengeFactoryConfigRoot );
	root.addChild( "challenges/stats", pChallengeStatWatcher );

	root.addChild( "dbAppAlpha", makeWatcher( &Mercury::ChannelOwner::addr ),
		&dbAppAlpha_ );

	MF_WATCH( "id", id_ );

	statsTimer_ =
		mainDispatcher_.addTimer( UPDATE_STATS_PERIOD, &loginStats_, NULL,
		"UpdateStats" );

	tickTimer_ = mainDispatcher_.addTimer(
					1000000/Config::updateHertz(),
					this, (void *)TIMEOUT_TICK,
					"TickTimer" );

	if (!this->isDBReady())
	{
		INFO_MSG( "Database is not ready yet\n" );
	}

	return true;
}


/**
 *	This method handles the notifyDBAppAlpha message.
 */
void LoginApp::notifyDBAppAlpha(
		const LoginIntInterface::notifyDBAppAlphaArgs & args )
{
	INFO_MSG( "LoginApp::notifyDBAppAlpha: %s\n", args.addr.c_str() );
	dbAppAlpha_.addr( args.addr );
}


/**
 *	This method initialises the LoginApp private key for decrypting log on
 *	parameters.
 *  @returns true if the initialisation finished correctly, false otherwise.
 */
bool LoginApp::initLogOnParamsEncoder()
{
	// If we allow unencrypted logins, then a failure to load the
	// encryption private key shouldn't kill the server
	bool shouldSucceedOnKeyLoadFailure = Config::allowUnencryptedLogins();
	BW::string privateKeyPath = Config::privateKey();

	if (privateKeyPath.empty())
	{
		ERROR_MSG( "LoginApp::initLogOnParamsEncoder: "
			"You must specify a private key to use with the "
			"<loginApp/privateKey> option in bw.xml\n" );

		return shouldSucceedOnKeyLoadFailure;
	}

	DataSectionPtr pSection = BWResource::openSection( privateKeyPath );
	if (!pSection)
	{
		ERROR_MSG( "LoginApp::initLogOnParamsEncoder: "
			"Couldn't load private key from non-existent file %s\n",
			privateKeyPath.c_str() );
		return shouldSucceedOnKeyLoadFailure;
	}

	BinaryPtr pBinData = pSection->asBinary();
	BW::string keyString( pBinData->cdata(), pBinData->len() );

	RSAStreamEncoder * pEncoder = 
		new RSAStreamEncoder( /* keyIsPrivate: */ true );

	if (!pEncoder->initFromKeyString( keyString ))
	{
		delete pEncoder;
		return shouldSucceedOnKeyLoadFailure;
	}

	pLogOnParamsEncoder_.reset( pEncoder ); 

	return true;
}


/*
 *	Override from ServerApp.
 */
void LoginApp::onRunComplete() /* override */
{
	INFO_MSG( "LoginApp::run: Terminating normally.\n" );
	this->ServerApp::onRunComplete();
	bool sent = false;

	if (this->isDBAppMgrReady())
	{
		Mercury::Bundle	& dbMgrBundle = dbAppMgr_.pChannelOwner()->bundle();
		DBAppMgrInterface::controlledShutDownArgs args;
		args.stage = SHUTDOWN_REQUEST;
		dbMgrBundle << args;
		dbAppMgr_.pChannelOwner()->send();
		sent = true;
	}

	if (sent)
	{
		this->intInterface().processUntilChannelsEmpty();
	}
}


/**
 *	This method updates counters and 
 *  sends a failure message back to the client.
 */
void LoginApp::handleFailure( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID, int status,
		const char * pDescription /* = NULL */,
		LogOnParamsPtr pParams /* = NULL */ )
{
	if (status == LogOnStatus::LOGIN_REJECTED_RATE_LIMITED)
	{
		loginStats_.incRateLimited();
	}
	else
	{
		loginStats_.incFails();
	}
	++gNumLoginFailures;
	
	// Reset replies counter each interval (0.5 sec).
	if (repliedFailsCounterResetTime_ <= timestamp())
	{
		repliedFailsCounterResetTime_ = timestamp()
											+ (stampsPerSecond() / 2);
		numFailRepliesLeft_ = Config::maxRepliesOnFailPerSecond() / 2;
	}

	if (numFailRepliesLeft_ > 0)
	{
		--numFailRepliesLeft_;

		NETWORK_INFO_MSG( "LoginApp::handleFailure: "
					"LogOn for %s failed, LogOnStatus: %d, description '%s'.\n",
				addr.c_str(), status, pDescription ? pDescription : "none" );

		Mercury::Bundle * pBundle = NULL;
		std::auto_ptr< Mercury::Bundle > pTempBundle;

		if (pChannel)
		{
			pBundle = &(pChannel->bundle());
		}
		else
		{
			pTempBundle.reset( extInterface_.createOffChannelBundle() );
			pBundle = pTempBundle.get();
		}

		// Replies to failed login attempts are not reliable as that would be a
		// DOS'ing vulnerability
		pBundle->startReply( replyID, Mercury::RELIABLE_NO );
		*pBundle << (int8)status;
		if (pDescription != NULL)
		{
			*pBundle << pDescription;
		}


		if (pBundle->numDataUnits() > WARN_BUNDLE_PACKETS)
		{
			NETWORK_WARNING_MSG( "LoginApp::handleFailure: "
				"Sent LogOnStatus %d with size larger than %d packets.\n",
				status, WARN_BUNDLE_PACKETS );
		}

		if (pChannel)
		{
			pChannel->send( pBundle );
		}
		else
		{
			Mercury::UDPBundle & udpBundle =
				static_cast< Mercury::UDPBundle & >( *pBundle );
			extInterface_.send( addr, udpBundle );
		}
	}

	// Erase the mapping for this attempt if appropriate
	if (pParams)
	{
		loginRequests_.erase( addr );
	}
}


/**
 *	This method marks the address as banned and sends a failure message back to the client.
 */
void LoginApp::handleBanIP( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		LogOnParamsPtr pParams, ::time_t banDuration )
{
	uint64 banEndTimestamp = timestamp() + banDuration * stampsPerSecond();
	ipAddressBanMap_[addr.ip] = banEndTimestamp;

	this->handleFailure( addr, pChannel, replyID,
		LogOnStatus::LOGIN_REJECTED_IP_ADDRESS_BAN,
		"Logins from this IP address currently are not permitted",
		pParams );
}


/**
 *	This method is the one that actually receives the login requests.
 */
void LoginApp::login( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	Mercury::Channel * pChannel = header.pChannel.get();
	uint64 currTimestamp = timestamp();

	if (Config::rateLimitEnabled() &&
		(currTimestamp >
			lastRateLimitCheckTime_ + Config::rateLimitDurationInStamps()) )
	{
		// reset number of allowed logins per time block if we're rate limiting
		numAllowedLoginsLeft_ = Config::loginRateLimit();
		lastRateLimitCheckTime_ = currTimestamp;
	}

	if (!Config::allowLogin())
	{
		if (Config::verboseLoginFailures())
		{
			WARNING_MSG( "LoginApp::login: Dropping"
					" login attempt from %s because "
					"config/allowLogin is false.\n",
				source.c_str() );
		}

		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_REJECTED_LOGINS_NOT_ALLOWED );
		data.finish();
		return;
	}

	IPAddressBanMap::iterator it = ipAddressBanMap_.find(source.ip);
	if (ipAddressBanMap_.end() != it)
	{
		if (currTimestamp > it->second)
		{
			// IP address ban is over
			ipAddressBanMap_.erase(it);
		}
		else
		{
			if (Config::verboseLoginFailures())
			{
				WARNING_MSG( "LoginApp::login: "
						"Dropping login attempt from %s because this"
						" IP address is currently banned.\n",
					source.c_str() );
			}
			loginStats_.incFailedByIPAddressBan();
			this->handleFailure( source, pChannel, header.replyID,
				LogOnStatus::LOGIN_REJECTED_IP_ADDRESS_BAN );
			data.finish();
			return;
		}
	}

	// Do periodic cleanup of IP address bans.
	if (currTimestamp > nextIPAddressBanMapCleanupTime_)
	{
		nextIPAddressBanMapCleanupTime_ = currTimestamp + Config::ipBanListCleanupInterval() * stampsPerSecond();
		for (IPAddressBanMap::iterator it = ipAddressBanMap_.begin(); ipAddressBanMap_.end() != it;)
		{
			if (currTimestamp > it->second )
			{
				ipAddressBanMap_.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}
	if (!source.ip)
	{
		// spoofed address trying to login as web client!
		if (Config::verboseLoginFailures())
		{
			NETWORK_ERROR_MSG( "LoginApp::login: Spoofed empty address\n" );
		}
		data.retrieve( data.remainingLength() );
		loginStats_.incFails();
		return;
	}

	bool isReattempt = (loginRequests_.count( source ) != 0);
	INFO_MSG( "LoginApp::login: %s from %s\n",
		isReattempt ? "Re-attempt" : "Attempt", source.c_str() );

	++gNumLoginAttempts;

	ClientServerProtocolVersion serverProtocol =
		ClientServerProtocolVersion::currentVersion();
	ClientServerProtocolVersion clientProtocol;
	data >> clientProtocol;

	if (data.error())
	{
		if (Config::verboseLoginFailures())
		{
			ERROR_MSG( "LoginApp::login: "
					"Not enough data on stream (%d bytes total)\n",
				header.length );
		}

		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_MALFORMED_REQUEST );

		return;
	}

	if (!serverProtocol.supports( clientProtocol ))
	{
		if (Config::verboseLoginFailures())
		{
			INFO_MSG( "LoginApp::login: "
					"User at %s tried to log on with protocol version %s. "
					"Expected %s.\n",
				source.c_str(),
				clientProtocol.c_str(),
				serverProtocol.c_str() );
		}

		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_BAD_PROTOCOL_VERSION );
		data.finish();

		return;
	}

	// First check whether this is a repeat attempt from a recent pending
	// login before attempting to decrypt and log in.
	if (this->handleResentPendingAttempt( source, header.replyID ))
	{
		// ignore this one, it's in progress
		loginStats_.incPending();
		data.finish();
		return;
	}

	bool isRateLimited = Config::rateLimitEnabled() &&
			(numAllowedLoginsLeft_ == 0);
	if (isRateLimited)
	{
		if (Config::verboseLoginFailures())
		{
			NOTICE_MSG( "LoginApp::login: "
					"Login from %s not allowed due to rate limiting\n",
				source.c_str() );
		}

		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_REJECTED_RATE_LIMITED );
		data.finish();
		return;
	}

	if (!this->isDBReady())
	{
		if (Config::verboseLoginFailures())
		{
			INFO_MSG( "LoginApp::login: "
				"Attempted login when database not yet ready.\n" );
		}
		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_REJECTED_DB_NOT_READY );
		data.finish();
		return;
	}

	if (systemOverloaded_ != 0)
	{
		if (systemOverloadedTime_ + stampsPerSecond() < currTimestamp)
		{
			systemOverloaded_ = 0;
		}
		else
		{
			if (Config::verboseLoginFailures())
			{
				INFO_MSG( "LoginApp::login: "
					"Attempted login when system"
					" overloaded or not yet ready.\n" );
			}
			this->handleFailure( source, pChannel, header.replyID,
				systemOverloaded_ );
			data.finish();
			return;
		}
	}

	if (this->processForLoginChallenge( source, pChannel, header.replyID,
			data ))
	{
		return;
	}

	// Read off login parameters
	LogOnParamsPtr pParams = new LogOnParams();

	// Save the message so we can have multiple attempts to read it
	int dataLength = data.remainingLength();
	if (dataLength > Config::maxLoginMessageSize())
	{
		if (Config::verboseLoginFailures())
		{
			ERROR_MSG( "LoginApp::login: "
				"Attempted login with too long credentials.\n" );
		}
		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_MALFORMED_REQUEST );
		data.finish();
		return;
	}
	const void * pDataData = data.retrieve( dataLength );

	StreamEncoder * pEncoder = pLogOnParamsEncoder_.get();

	do
	{
		MemoryIStream attempt = MemoryIStream( pDataData, dataLength );

		if (pParams->readFromStream( attempt, pEncoder ))
		{

			if ((pParams->username().length() > Config::maxUsernameLength()) ||
				(pParams->password().length() > Config::maxPasswordLength())
				)
			{
				// username or password are too long
				if (Config::verboseLoginFailures())
				{
					ERROR_MSG( "LoginApp::login: "
						"Attempted login with too long credentials.\n" );
				}

				this->handleFailure( source, pChannel, header.replyID,
					LogOnStatus::LOGIN_MALFORMED_REQUEST );
				data.finish();
				return;
			}

			// We are successful, move on
			break;
		}

		if (pEncoder && Config::allowUnencryptedLogins())
		{
			// If we tried using encryption, have another go without it
			pEncoder = NULL;
			continue;
		}

		// Nothing left to try, bail out
		if (Config::verboseLoginFailures())
		{
			ERROR_MSG( "LoginApp::login: "
					"Could not destream login parameters. Possibly caused by "
					"mis-matching LoginApp keypair, or different "
					"client/server res trees." );
		}
		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LOGIN_MALFORMED_REQUEST );
		return;
		// Does not reach here
	}
	while (false);

	// First check whether this is a repeat attempt from a recent
	// resolved login before attempting to log in.
	if (this->handleResentCachedAttempt( source, pParams, header.replyID ))
	{
		// ignore this one, we've seen it recently
		return;
	}

	if (Config::rateLimitEnabled())
	{
		// We've done the hard work of decrypting the logon parameters now, so
		// we count this as a login with regards to rate-limiting.
		--numAllowedLoginsLeft_;
	}

	// Check that it has encryption key if we disallow unencrypted logins
	if (pParams->encryptionKey().empty() && !Config::allowUnencryptedLogins())
	{
		if (Config::verboseLoginFailures())
		{
			ERROR_MSG( "LoginApp::login: "
					"No encryption key supplied, and server is not allowing "
					"unencrypted logins." );
		}
		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LogOnStatus::LOGIN_MALFORMED_REQUEST );
		return;
	}
	
	if (Config::passwordlessLoginsOnly() && !pParams->password().empty())
	{
		if (Config::verboseLoginFailures())
		{
			WARNING_MSG( "LoginApp::login: "
					"Login attempt from %s is using password"
					" while passwordlessLoginsOnly option is active\n",
				source.c_str() );
		}
		loginStats_.incAttemptsWithPassword();
		this->handleFailure( source, pChannel, header.replyID,
			LogOnStatus::LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD );
		return;
	}

	INFO_MSG( "LoginApp::login: Logging in user '%s' (%s)\n",
		pParams->username().c_str(),
		source.c_str() );

	// Remember that this attempt is now in progress and discard further
	// attempts from that address for some time after it completes.
	ClientLoginRequest & loginRequest = loginRequests_[ source ];
	loginRequest.reset();
	loginRequest.pChannel( pChannel );
	loginRequest.pParams( pParams );

	DatabaseReplyHandler * pDBHandler =
		new DatabaseReplyHandler( *this, source, pChannel,
			header.replyID, pParams );

	Mercury::Bundle	& dbBundle = this->dbAppAlpha().bundle();
	dbBundle.startRequest( DBAppInterface::logOn, pDBHandler );

	dbBundle << source << *pParams;

	this->dbAppAlpha().send();
}


/**
 *	This method determines whether a login challenge should be sent, or a login
 *	response should be received.
 *
 *	@return true if no further processing is required, false otherwise.
 */
bool LoginApp::processForLoginChallenge( const Mercury::Address & source,
		Mercury::Channel * pChannel,
		Mercury::ReplyID replyID,
		BinaryIStream & data )
{
	ClientLoginRequests::iterator iter = loginRequests_.find( source );

	if (iter != loginRequests_.end())
	{
		ClientLoginRequest & request = iter->second;
		if (request.didFailChallenge())
		{
			this->handleFailure( source, pChannel, replyID,
				LogOnStatus::LOGIN_REJECTED_CHALLENGE_ERROR,
				"Failed login challenge" );
			data.finish();
			return true;
		}

		if (request.pLoginChallenge())
		{
			// Still waiting on a response, this is probably just a duplicate.
			// Resend the same challenge.
			this->sendChallengeReply( source, pChannel, replyID,
				request.challengeType(), request.pLoginChallenge() );
			data.finish();
			return true;
		}
		else
		{
			// We've received the reply already.
			return false;
		}
	}

	const BW::string challengeType = Config::challengeType();

	if (challengeType.empty())
	{
		// No challenge configured.
		return false;
	}

	LoginChallengePtr pChallenge =
		challengeFactories_.createChallenge( challengeType );

	if (!pChallenge)
	{
		ERROR_MSG( "LoginApp::login: "
				"Configured login challenge factory (\"%s\") failed to create "
				"a challenge instance\n",
			challengeType.c_str() );

		this->handleFailure( source, pChannel, replyID,
			LogOnStatus::LOGIN_REJECTED_CHALLENGE_ERROR,
			"Failed to instantiate login challenge." );

		data.finish();
		return true;
	}

	ClientLoginRequest & loginRequest = loginRequests_[ source ];
	loginRequest.pChannel( pChannel );
	loginRequest.setLoginChallenge( challengeType, pChallenge );

	this->sendChallengeReply( source, pChannel, replyID, challengeType,
		pChallenge );

	data.finish();
	return true;
}


/*
 *	Override from TimerHandler.
 */
void LoginApp::handleTimeout( TimerHandle handle, void * arg ) /* override */
{
	switch (uintptr(arg))
	{
		case TIMEOUT_TICK:
		{
			this->advanceTime();
			break;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: ProbeHandler
// -----------------------------------------------------------------------------

/**
 *	This method handles the probe message.
 */
void LoginApp::probe( const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & /*data*/ )
{
	if (Config::logProbes())
	{
		INFO_MSG( "LoginApp::probe: Got probe from %s\n", source.c_str() );
	}

	if (!Config::allowProbe() || header.length != 0) return;

	Mercury::UDPBundle bundle;

	bundle.startReply( header.replyID, Mercury::RELIABLE_NO );

	char buf[256];
	gethostname( buf, sizeof(buf) ); buf[sizeof(buf)-1]=0;
	bundle << PROBE_KEY_HOST_NAME << buf;

#ifndef _WIN32
	struct passwd *pwent = getpwuid( getUserId() );
	const char * username = pwent ? (pwent->pw_name ? pwent->pw_name : "") : "";
	bundle << PROBE_KEY_OWNER_NAME << username;

	if (!pwent)
	{
		ERROR_MSG( "LoginApp::probe: "
			"Process uid %d doesn't exist on this system!\n", getUserId() );
	}

#else
	DWORD bsz = sizeof( buf );
	if (!GetUserName( buf, &bsz ))
	{
		buf[ 0 ]=0;
	}
	bundle << PROBE_KEY_OWNER_NAME << buf;
#endif

	bw_snprintf( buf, sizeof(buf), "%d", gNumLogins );
	bundle << PROBE_KEY_USERS_COUNT << buf;

	extInterface_.send( source, bundle );
}


/**
 *	This method handles the client sending up the response for a previously
 *	issued challenge.
 */
void LoginApp::challengeResponse( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	ClientLoginRequests::iterator iter = loginRequests_.find( source );

	if (iter == loginRequests_.end())
	{
		// Probably timed out.
		*(header.pBreakLoop) = true;
		data.finish();
		return;
	}

	ClientLoginRequest & request = iter->second;

	if (!request.pLoginChallenge())
	{
		// Challenge response already verified, this is a re-send, let 
		// login message handler re-send cached result.
		data.finish();
		return;
	}

	TimeStamp start( BW::timestamp() );

	float calculationDuration;
	data >> calculationDuration;

	// Read and verify the response.
	if (!iter->second.pLoginChallenge()->readResponseFromStream( data ))
	{
		NOTICE_MSG( "LoginApp::login: Client %s failed login challenge "
				"(took %.03fms to verify)\n",
			source.c_str(),
			start.ageInSeconds() * 1000.0 );

		// Let LoginApp::login send back failure.
		request.didFailChallenge( true );
		request.clearChallenge();
		data.finish();
		return;
	}

	TRACE_MSG( "LoginApp::login: Client %s passed login challenge "
			"(took %.03fms for client to calculate, took %.03fms to verify)\n",
		source.c_str(),
		calculationDuration * 1000.f,
		start.ageInSeconds() * 1000.0 );

	request.clearChallenge();

	calculationDuration = std::min( (float)MAX_SANE_CALCULATION_SECONDS,
									calculationDuration );

	if (calculationDuration < (float)MAX_SANE_CALCULATION_SECONDS)
	{
		loginStats_.challengeCalculationTimeSample( calculationDuration );
	}

	loginStats_.challengeVerificationTimeSample(
		float( start.ageInSeconds() ) );
}


/**
 *	This method sends a reply to a client indicating that logging in has been
 *	successful. It also caches this information so that it can be resent if
 *	necessary.
 */
void LoginApp::sendAndCacheSuccess( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		const LoginReplyRecord & replyRecord,
		const BW::string & serverMsg, LogOnParamsPtr pParams )
{
	ClientLoginRequest & request = loginRequests_[ addr ];
	request.setData( replyRecord, serverMsg );

	MF_ASSERT_DEV( *pParams == *request.pParams() );

	this->sendSuccess( addr, pChannel, replyID, request );

	// Do not let the map get too big. Just check every so often to get rid of
	// old caches.

	if (loginRequests_.size() > 100)
	{
		ClientLoginRequests::iterator iter = loginRequests_.begin();

		while (iter != loginRequests_.end())
		{
			ClientLoginRequests::iterator prevIter = iter;
			++iter;

			if (prevIter->second.isTooOld())
			{
				loginRequests_.erase( prevIter );
			}
		}
	}
}


/**
 *	This method sends a reply to a client indicating that logging in has been
 *	successful.
 */
void LoginApp::sendSuccess( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		const ClientLoginRequest & request )
{
	MemoryOStream data;

	data << (int8)LogOnStatus::LOGGED_ON;
	
	const BW::string & encryptionKey = request.pParams()->encryptionKey();

	if (!encryptionKey.empty())
	{
		// We have to encrypt the reply record because it contains the session
		// key
		Mercury::EncryptionFilterPtr pFilter =
			Mercury::EncryptionFilter::create( 
				Mercury::SymmetricBlockCipher::create( encryptionKey ) );
		MemoryOStream clearText;
		request.writeSuccessResultToStream( clearText );
		pFilter->encryptStream( clearText, data );
	}
	else
	{
		request.writeSuccessResultToStream( data );
	}

	loginStats_.incSuccesses();
	++gNumLogins;

	this->sendRawReply( addr, pChannel, replyID, data );
}


/**
 *	This method sends a reply back to the client with a challenge.
 */
void LoginApp::sendChallengeReply( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		const BW::string & challengeType,
		LoginChallengePtr pChallenge )
{
	MemoryOStream data;

	data << (uint8) LogOnStatus::LOGIN_CHALLENGE_ISSUED << challengeType;

	if (!pChallenge->writeChallengeToStream( data ))
	{
		ERROR_MSG( "LoginApp::sendChallengeReply: "
				"Failed to stream challenge data (\"%s\") to bundle.\n",
			challengeType.c_str() );

		this->handleFailure( addr, pChannel, replyID,
			LogOnStatus::LOGIN_REJECTED_CHALLENGE_ERROR,
			"Failed to send challenge data." );

		return;
	}

	this->sendRawReply( addr, pChannel, replyID, data );
}


/**
 *	This method sends the given payload back to the client as a login reply.
 */
void LoginApp::sendRawReply( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		BinaryIStream & payload )
{
	Mercury::Bundle * pBundle = NULL;
	Mercury::UDPBundle offChannelBundle;

	if (pChannel)
	{
		pBundle = &(pChannel->bundle());
	}
	else
	{
		pBundle = &offChannelBundle;
	}

	pBundle->startReply( replyID, Mercury::RELIABLE_NO );

	pBundle->transfer( payload, payload.remainingLength() );

	if (pBundle->numDataUnits() > WARN_BUNDLE_PACKETS)
	{
		NETWORK_WARNING_MSG( "LoginApp::sendRawReply: "
				"Sent reply with size larger than %d > %d packets.\n", 
			pBundle->numDataUnits(), WARN_BUNDLE_PACKETS );
	}

	if (pChannel)
	{
		pChannel->send( pBundle );
	}
	else
	{
		this->extInterface().send( addr, offChannelBundle );
	}
}


/**
 *	This method checks whether there is a login in progress from this
 *	address.
 */
bool LoginApp::handleResentPendingAttempt( const Mercury::Address & addr,
		Mercury::ReplyID replyID )
{
	ClientLoginRequests::iterator iter = loginRequests_.find( addr );

	if (iter == loginRequests_.end())
	{
		return false;
	}

	ClientLoginRequest & request = iter->second;

	if (request.hasPendingChallenge())
	{
		// Re-send the challenge.
		this->sendChallengeReply( addr, request.pChannel(), replyID,
			request.challengeType(), request.pLoginChallenge() );

		return false;
	}

	if (!request.isPendingAuthentication())
	{
		return false;
	}

	DEBUG_MSG( "LoginApp::handleResentPendingAttempt: "
			"Ignoring repeat attempt from %s "
			"while another attempt is in progress (for '%s')\n",
		addr.c_str(),
		request.pParams()->username().c_str() );

	return true;
}


/**
 *	This method checks whether there is a cached login attempt from this
 *	address. If there is, it is assumed that the previous reply was dropped and
 *	this one is resent.
 */
bool LoginApp::handleResentCachedAttempt( const Mercury::Address & addr,
		LogOnParamsPtr pParams, Mercury::ReplyID replyID )
{
	ClientLoginRequests::iterator iter = loginRequests_.find( addr );

	if (iter != loginRequests_.end())
	{
		ClientLoginRequest & request = iter->second;
		if (!request.isTooOld() && *request.pParams() == *pParams)
		{
			DEBUG_MSG( "%s retransmitting successful login to %s\n",
					   addr.c_str(),
					   request.pParams()->username().c_str() );
			this->sendSuccess( addr, request.pChannel(), replyID, request );

			return true;
		}
	}

	return false;
}


/**
 *  Handles incoming shutdown requests.  This is basically another way of
 *  triggering a controlled system shutdown instead of sending a SIGUSR1.
 */
void LoginApp::controlledShutDown( const Mercury::Address &source,
	Mercury::UnpackedMessageHeader &header,
	BinaryIStream &data )
{
	INFO_MSG( "LoginApp::controlledShutDown: "
		"Got shutdown command from %s\n", source.c_str() );

	this->controlledShutDown();
}


void LoginApp::controlledShutDown()
{
	mainDispatcher_.breakProcessing();
}


void LoginApp::clearIPAddressBans()
{
	INFO_MSG( "Clearing the list of IP address bans of size %lu\n",
				ipAddressBanMap_.size() );
	ipAddressBanMap_.clear();
}


/*
 *	Override from ServerApp.
 */
void LoginApp::onSignalled( int sigNum ) /* override */
{
	this->ServerApp::onSignalled( sigNum );

	if (sigNum == SIGUSR1)
	{
		this->controlledShutDown();
	}
}


/**
 *	This method is run if challengeType option is modified by the watcher.
 */
void LoginApp::onChallengeTypeModified( const BW::string & oldValue,
		const BW::string & newValue )
{
	INFO_MSG( "LoginApp::onChallengeTypeModified: "
		"challengeType has been updated from '%s' to '%s'\n",
			oldValue.c_str(), newValue.c_str() );

	loginRequests_.clear();
}

/**
 *	This method returns challengeType configuration option value.
 */
BW::string LoginApp::challengeType() const
{
	return Config::challengeType();
}

/**
 *	This method update challengeType configuration option value and invoke
 *	onChallengeTypeModified() handler if the value is modified
 */
void LoginApp::challengeType( BW::string value )
{
	if (!value.empty() && !challengeFactories_.getFactory( value ))
	{
		ERROR_MSG( "LoginApp::challengeType: "
				"No such challenge type: \"%s\"\n",
				value.c_str() );
		return;
	}

	BW::string oldValue = Config::challengeType();
	Config::challengeType.set( value );

	if (oldValue != value)
	{
		this->onChallengeTypeModified( oldValue, value );
	}
}

// -----------------------------------------------------------------------------
// Section: LoginStats
// -----------------------------------------------------------------------------


// Make the EMA bias equivalent to having the most recent 5 samples represent
// 86% of the total weight. This is purely arbitrary, and may be adjusted to
// increase or decrease the sensitivity of the login statistics as reported in
// the 'averages' watcher directory.
static const uint WEIGHTING_NUM_SAMPLES = 5;

/**
 * The EMA bias for the login statistics.
 */
const float LoginApp::LoginStats::COUNT_BIAS = 2.f /
	(WEIGHTING_NUM_SAMPLES + 1);

/**
 *	The EMA bias for the challenge calculation and verification time averages.
 */
const float LoginApp::LoginStats::TIME_BIAS = 
	EMA::calculateBiasFromNumSamples( 100 );


/**
 *	Constructor.
 */
LoginApp::LoginStats::LoginStats():
			fails_( COUNT_BIAS ),
			rateLimited_( COUNT_BIAS ),
			pending_( COUNT_BIAS ),
			successes_( COUNT_BIAS ),
			all_( COUNT_BIAS ),
			failedByIPAddressBan_( COUNT_BIAS ),
			attemptsWithPassword_( COUNT_BIAS ),
			calculationTime_( TIME_BIAS ),
			verificationTime_( TIME_BIAS )
{}

BW_END_NAMESPACE

// loginapp.cpp
