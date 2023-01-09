#include "bwmachined.hpp"

#include "common_machine_guard.hpp"
#include "incoming_packet.hpp"
#include "server_platform_linux.hpp"


#include "network/file_stream.hpp"
#include "network/portmap.hpp"

#include "cstdmf/memory_stream.hpp"


#include "cstdmf/build_config.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/bw_platform_info.hpp"

#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


BW_BEGIN_NAMESPACE

#if !defined( CODE_INLINE )
#include "bwmachined.ipp"
#endif

// This is the file that state gets written to in between bwmachined restarts.
const char* BWMachined::STATE_FILE = "/var/run/bwmachined.state";

/// BWMachined Singleton.
BW_SINGLETON_STORAGE( BWMachined )

namespace
{
	bool g_serverRunning = true;
} // anonymous namespace

/**
 *  If we are shutdown with `service bwmachined2 stop`, drop the process table
 *  to a file so we can restart bwmachined without losing track of all our
 *  processes.
 */
static void sigterm( int sig )
{
	if (BWMachined::pInstance())
	{
		BWMachined::pInstance()->save();
	}

	g_serverRunning = false;
}


/**
 *	Constructor.
 */
BWMachined::BWMachined() :
	pidPath_(),
	packetTimeoutHandler_(),
	broadcastAddr_( 0 ),
	ep_(),
	epBroadcast_(),
	epLocal_(),
	cluster_( *this ),
	tags_(),
	timingMethod_( "gettime" ),
	pServerPlatform_( new ServerPlatformLinux ),
	systemInfo_(),
	procs_(),
	birthListeners_( *pServerPlatform_ ),
	deathListeners_( *pServerPlatform_ ),
	users_(),
	callbacks_(),
	pServerInfo_( new ServerInfo ),
	maxPacketDelayMillisec_( 100 )
{
	syslog( LOG_INFO, "--- BWMachined start ---" );

	if (!pServerPlatform_->isInitialised())
	{
		syslog( LOG_CRIT, "Failed to initialise for server platform." );
		exit( EXIT_FAILURE );
	}

	users_.flush();

	// Bind the 3 endpoints we listen on
	ep_.socket( SOCK_DGRAM );
	epLocal_.socket( SOCK_DGRAM );
	epBroadcast_.socket( SOCK_DGRAM );

	if (!this->readConfigFile())
	{
		syslog( LOG_CRIT, "Invalid configuration file" );
		exit( EXIT_FAILURE );
	}

	this->initNetworkInterfaces();

	// Do all the once-off writing to the SystemInfo struct, things that don't
	// change, like hostname, number of CPUs etc.
	const BW::vector<float> & speeds = pServerInfo_->cpuSpeeds();
	if (speeds.size() == 0)
	{
		syslog( LOG_CRIT, "Unable to obtain any valid processor speed info" );
		exit( EXIT_FAILURE );
	}

	SystemInfo &si = systemInfo_;

	si.nCpus = speeds.size();
	si.cpuSpeed = (int)speeds[0];
	for (uint j=0; j < si.nCpus; j++)
		si.cpu.push_back( MaxStat() );

	si.m.cpuSpeed_ = si.cpuSpeed;
	si.m.setNCpus( si.nCpus );
	si.m.hostname_ = pServerInfo_->serverName();
	si.m.version_ = BWMACHINED_VERSION;
	si.m.outgoing( true );

	si.hpm.cpuSpeed_ = si.cpuSpeed;
	si.hpm.setNCpus( si.nCpus );
	si.hpm.hostname_ = pServerInfo_->serverName();
	si.hpm.version_ = BWMACHINED_VERSION;
	si.hpm.outgoing( true );

	// Force the first update so we know all the stats are in a readable state
	this->updateSystemInfo();

	// Try to re-read any existing bwmachined state
	this->load();

	// Register SIGTERM handler
	signal( SIGTERM, sigterm );
}


/**
 * Destructor.
 */
BWMachined::~BWMachined()
{
	// Delete PID file if path has been set
	if (!pidPath_.empty())
	{
		int pid;
		FILE *pidFile = fopen( pidPath_.c_str(), "r" );
		if (pidFile != NULL)
		{
			if (fscanf( pidFile, "%d", &pid ) == 1)
			{
				fclose( pidFile );
				if (pid == mf_getpid())
				{
					if (unlink( pidPath_.c_str() ))
					{
						syslog( LOG_ERR, "BWMachined::~BWMachined(): "
							"Unable to remove PID file '%s': %s\n",
							pidPath_.c_str(), strerror( errno ) );
					}
				}
			}
			else
			{
				fclose( pidFile );
				syslog( LOG_ERR, "BWMachined::~BWMachined(): "
					"Unable to read PID from file '%s'\n", pidPath_.c_str() );
			}
		}
		else
		{
			syslog( LOG_ERR, "BWMachined::~BWMachined(): "
				"PID file missing from '%s'\n", pidPath_.c_str() );
		}
	}

	bw_safe_delete( pServerInfo_ );
	pServerInfo_ = NULL;
}


/**
 * Discover the broadcast interface to use and init all the endpoints.
 */
void BWMachined::initNetworkInterfaces()
{
	// Determine which network interface will be sending broadcast messages
	if (broadcastAddr_ == 0 && !this->findBroadcastInterface())
	{
		syslog( LOG_CRIT, "Failed to determine default broadcast interface. "
					"Make sure UDP ports %d and %d are not firewalled and that "
					"your broadcast route is set correctly. e.g. "
					"/sbin/ip route add broadcast 255.255.255.255 dev eth0",
			PORT_MACHINED, PORT_BROADCAST_DISCOVERY );
		exit( EXIT_FAILURE );
	}

	if (!ep_.good() ||
		 ep_.bind( htons( PORT_MACHINED ), broadcastAddr_ ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to '%s'. %s.",
							inet_ntoa((struct in_addr &)broadcastAddr_),
							strerror(errno) );
		exit( EXIT_FAILURE );
	}
	ep_.setbroadcast( true );

	if (!epLocal_.good() ||
		 epLocal_.bind( htons( PORT_MACHINED ), LOCALHOST ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to (lo). %s.",
							strerror(errno) );
		exit( EXIT_FAILURE );
	}

	if (!epBroadcast_.good() ||
		 epBroadcast_.bind( htons( PORT_MACHINED ), BROADCAST ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to '%s'. %s.",
							inet_ntoa((struct in_addr &)BROADCAST),
							strerror(errno) );
		exit( EXIT_FAILURE );
	}

	cluster_.ownAddr_ = broadcastAddr_;
}


/**
 * This method sends a broadcast message out to the network and listens for it
 * on all our interfaces to determine what our default broadcast interface is.
 */
bool BWMachined::findBroadcastInterface()
{

	BW::map< u_int32_t, BW::string > interfaces;
	Endpoint epListen;
	struct timeval tv;
	fd_set fds;
	char streamBuf[ MGMPacket::MAX_SIZE ];

	// Initialise the endpoint
	epListen.socket( SOCK_DGRAM );
	if (!epListen.good() ||
		 epListen.bind( htons( PORT_BROADCAST_DISCOVERY ) ) == -1)
	{
		syslog( LOG_CRIT, "Couldn't bind broadcast listener socket to port %d",
				PORT_BROADCAST_DISCOVERY );
		return false;
	}
	epListen.setbroadcast( true );

	// Perform a discovery of all network interfaces on this host
	if (!epListen.getInterfaces( interfaces ))
	{
		syslog( LOG_CRIT, "Failed to discover network interfaces" );
		return false;
	}

	QueryInterfaceMessage qim;
	bool ok = qim.sendto( epListen, htons( PORT_BROADCAST_DISCOVERY ),
						  BROADCAST, MGMPacket::PACKET_STAGGER_REPLIES );
	if (!ok)
	{
		syslog( LOG_CRIT, "Failed to send broadcast discovery message." );
		return false;
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// Listen for the message we just sent to come back to ourselves.
	while (1)
	{
		FD_ZERO( &fds );
		FD_SET( epListen.fileno(), &fds );
		int selgot = select( (epListen.fileno())+1, &fds, NULL, NULL, &tv );
		if (selgot == 0)
		{
			syslog( LOG_CRIT,
				"Timed out before receiving any broadcast discovery responses. "
					"Check that firewall rules are not preventing the sending "
					"and receipt of UDP broadcast messages on port %d.", 
				PORT_BROADCAST_DISCOVERY );
			return false;
		}
		else if (selgot == -1)
		{
			syslog( LOG_CRIT, "Broadcast discovery select error. %s.",
					strerror(errno) );
			return false;
		}
		else
		{
			sockaddr_in	sin;

			// Read packet into buffer
			int len = epListen.recvfrom( &streamBuf, sizeof( streamBuf ), sin );
			if (len == -1)
			{
				syslog( LOG_ERR, "Broadcast discovery recvfrom error. %s.",
						strerror( errno ) );
				continue;
			}

			syslog( LOG_INFO, "Broadcast discovery receipt from %s.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr) );

			// check messages received against the list of our interfaces
			BW::map< u_int32_t, BW::string >::iterator iter;

			iter = interfaces.find( (u_int32_t &)sin.sin_addr.s_addr );
			if (iter != interfaces.end())
			{
				syslog( LOG_INFO,
					"Confirmed %s (%s) as default broadcast route interface.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr),
					iter->second.c_str() );
				broadcastAddr_ = sin.sin_addr.s_addr;
				break;
			}

			syslog( LOG_ERR,
					"Broadcast discovery %s not a valid interface.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr) );
		}
	}

	return true;
}


/**
 *  Writes this machine's process table to /var/run/bwmachined.state so it can
 *  be restarted under the same state.
 */
void BWMachined::save()
{
	FileStream out( STATE_FILE, "w" );

	if (out.error())
	{
		syslog( LOG_ERR, "Couldn't write process state to %s: %s",
			STATE_FILE, out.strerror() );
		unlink( STATE_FILE );
		return;
	}

	for (unsigned i=0; i < procs_.size(); i++)
	{
		MemoryOStream processBuffer;

		ProcessInfo &pi = procs_[ i ];
		processBuffer << pi.cpu << pi.mem << pi.affinity << pi.starttime;
		pi.m.write( processBuffer );

		// Add the process information to the file.
		out.appendString( (const char *)processBuffer.data(),
							processBuffer.size() );
	}

	if (out.error())
	{
		syslog( LOG_ERR, "Failed to write process table to %s: %s",
			STATE_FILE, out.strerror() );
		unlink( STATE_FILE );
		return;
	}

	if (procs_.size() > 0)
	{
		syslog( LOG_INFO, "Wrote %" PRIzu " entries to %s prior to shutdown",
			procs_.size(), STATE_FILE );
	}
}


/**
 *  Reads a process table from /var/run/bwmachined.state.
 */
void BWMachined::load()
{
	FileStream in( STATE_FILE, "r" );
	struct stat statinfo;

	// If the file doesn't exist, just bail silently
	if (in.error())
		return;

	// If the file is too old, blow it away.
	// 10 minutes should be long enough to restart machined for most people.
	if (in.stat( &statinfo ) == 0)
	{
		time_t age = time( NULL ) - statinfo.st_mtime;
		if (age > 10 * 60)
		{
			syslog( LOG_INFO, "Ignoring out-of-date %s (%d seconds old)",
				STATE_FILE, (int)age );
			unlink( STATE_FILE );
			return;
		}
	}
	else
	{
		syslog( LOG_ERR, "Couldn't stat %s: %s",
			STATE_FILE, in.strerror() );
		return;
	}

	int len = in.length();
	uint totalEntries = 0;
	while (in.tell() < len)
	{
		// Read the process record into a temporary buffer because
		// ProcessMessage::readExtra will assume it is only reading
		// a single message and consume all the data.
		BW::string processBufferStr;
		in >> processBufferStr;

		MemoryIStream processBuffer(
			processBufferStr.data(), processBufferStr.size() );

		ProcessInfo pi;
		processBuffer >> pi.cpu >> pi.mem >> pi.affinity >> pi.starttime;
		pi.m.read( processBuffer );

		if (processBuffer.error() || in.error())
		{
			syslog( LOG_ERR, "Process table %u in %s corrupt",
						totalEntries, STATE_FILE );
			unlink( STATE_FILE );
			return;
		}

		if (!validateProcessInfo( pi ))
		{
			syslog( LOG_ERR, "Failed to restore %s (pid:%d uid:%d).",
				pi.m.name_.c_str(), pi.m.pid_, pi.m.uid_ );
		}
		else
		{
			procs_.push_back( pi );
			syslog( LOG_INFO, "Restored %s (pid:%d uid:%d)",
				pi.m.name_.c_str(), pi.m.pid_, pi.m.uid_ );
		}
		++totalEntries;
	}

	if (len > 0)
	{
		syslog( LOG_INFO, "Restored %" PRIzu " of %u entries from %s",
			procs_.size(), totalEntries, STATE_FILE );
	}

	unlink( STATE_FILE );
}


/**
 *  Close all of the open sockets owned by this BWMachined object.  This is
 *  called from inside startProcess() to ensure that child processes don't hold
 *  onto these sockets.
 */
void BWMachined::closeEndpoints()
{
	ep_.close();
	epBroadcast_.close();
	epLocal_.close();
}


/*
 * Returns the value of the specified BWMachined configuration option.  
 * Configuration option is either from bwmachined.conf or bigworld.conf.
 */
const char * BWMachined::findOption( 
	const char * optionName, const char * oldOptionName )
{
	TagsMap::iterator tagIter = tags_.find( "bwmachined" );

	if (tagIter != tags_.end())
	{
		Tags::iterator valueIter = tagIter->second.begin();
		while (valueIter != tagIter->second.end())
		{
			const char * line = valueIter->c_str();

			if (strstr( line, optionName ) == line)
			{
				const char * pValue = strchr( line, '=' );

				if (pValue)
				{
					++pValue;

					while (*pValue && isblank( *pValue ))
					{
						++pValue;
					}

					return pValue;
				}
			}

			++valueIter;
		}

	}


	// For backward compatibility.
	if (oldOptionName)
	{
		tagIter = tags_.find( oldOptionName );

		if ((tagIter != tags_.end()) &&
			!tagIter->second.empty())
		{
			return tagIter->second.front().c_str();
		}
	}

	return NULL;
}


/** 
 * Set PID path read in from command line arguments.
 */
void BWMachined::setPidPath( BW::string pidPath )
{
	pidPath_ = pidPath;
}


/* Read the tags from specified configuration file.
 * No need to close input file.
 */
bool BWMachined::readConfigFile( FILE * file )
{

	char buf[ 512 ];
	BW::string currTag;
	bool isOkay = true;

	while (isOkay && (fgets( buf, sizeof( buf ), file ) != NULL))
	{
		if ((buf[0] == '#') || (buf[0] == 0) || (buf[0] == '\n'))
		{
			continue;
		}

		int len = strlen( buf );

		if (buf[0] == '[')
		{
			if ((buf[ len - 1 ] == '\n') &&
				(buf[ len - 2 ] == ']'))
			{
				buf[ len - 2 ] = '\0';
				currTag = buf + 1;

				// We want to have it added even if it is empty.
				tags_[ currTag ];
			}
			else
			{
				isOkay = false;
				syslog( LOG_ERR, "Invalid tag '%s'\n", buf );
			}
		}
		else if (!currTag.empty())
		{
			if (buf[ len - 1 ] == '\n')
			{
				buf[ len - 1 ] = '\0';

				tags_[ currTag ].push_back( BW::string( buf ) );
			}
			else
			{
				syslog( LOG_ERR, "Max tag length is %" PRIzu "\n",
						sizeof( buf ) - 1 );
				isOkay = false;
			}
		}
	}


	if (!isOkay)
	{
		return false;
	}

	return true;
}


/**
 * This method reads the tags from both bwmachined.conf and bigworld.conf.
 */
bool BWMachined::readConfigFile()
{
	tags_.clear();

	// Read configuration files and updates tags_.
	FILE * file = fopen( machinedConfFile, "r" );

	if (file == NULL)
	{
		syslog( LOG_WARNING, "Global config file %s doesn't exist",
			machinedConfFile );
		return true;
	}

	bool hasReadMachinedConfFile = this->readConfigFile( file );
	fclose( file );

	file = fopen( bigworldConfFile, "r" );
	bool hasReadBWConfFile = false;

	if (file == NULL)
	{
		syslog( LOG_WARNING, "Global config file %s doesn't exist",
			bigworldConfFile );
	}
	else
	{
		hasReadBWConfFile = this->readConfigFile( file );
		fclose( file );
	}

	// Invalid configuration file.
	if (!hasReadMachinedConfFile && !hasReadBWConfFile)
	{
		return false;
	}


	// Extract the timing method configuration option.
	const char * optionValue =
		this->findOption( "timing_method", "TimingMethod" );

	if (optionValue)
	{
		timingMethod_ = optionValue;
		syslog( LOG_INFO, "Using %s timing", timingMethod_.c_str() );
	}	


	// Extract architecture configuration option.
	optionValue = this->findOption( "architecture", "architecture" );

	if ((optionValue != NULL) && (optionValue[0] != '\0'))
	{
		syslog( LOG_INFO, "The 'architecture' configuration option has been "
			"deprecated. Ignoring." );
	}


	// Handles internal interface configuration option.
	optionValue = this->findOption( "internal_interface", "InternalInterface" );

	if (optionValue)
	{
		BW::string internalIfaceStr = optionValue;

		typedef BW::map< u_int32_t, BW::string > Interfaces;
		Interfaces interfaces;

		if (!ep_.getInterfaces( interfaces ) || interfaces.empty())
		{
			syslog( LOG_ERR, "Could not get interfaces" );
			return false;
		}

		// Use this as the broadcast interface address if it's a valid dotted
		// decimal address string.
		if (inet_aton( internalIfaceStr.c_str(),
					(in_addr *)(&broadcastAddr_) ))
		{
			Interfaces::iterator iIface = interfaces.find( broadcastAddr_ );

			if (iIface == interfaces.end())
			{
				syslog( LOG_ERR, "Could not find configured internal interface "
						"'%s' in interfaces for this machine, falling back to "
						"auto-discovery",
					internalIfaceStr.c_str() );
				broadcastAddr_ = 0;
			}
			else
			{
				syslog( LOG_INFO, "Using configured internal interface "
						"identified by address %s (%s)\n",
					inet_ntoa( (in_addr&)(broadcastAddr_)),
					iIface->second.c_str() );
			}
		}
		else
		{
			// Otherwise, assume this is the name of the interface, try and
			// find it in the map of interfaces.
			Interfaces::iterator iIface = interfaces.begin();
			bool found = false;

			while (iIface != interfaces.end())
			{
				if (iIface->second == internalIfaceStr)
				{
					found = true;
					broadcastAddr_ = iIface->first;
					syslog( LOG_INFO,
						"Using configured internal interface %s (%s)",
							inet_ntoa( (in_addr&)broadcastAddr_ ),
							iIface->second.c_str() );
					break;
				}
				++iIface;
			}

			if (!found)
			{
				syslog( LOG_ERR, "Could not find configured internal "
						"interface named '%s', falling back to auto-discovery",
					internalIfaceStr.c_str() );
			}
		}
	}

	// 
	optionValue = this->findOption( "max_packet_delay", "MaxPacketDelay" );

	if (optionValue)
	{
		int tmpMaxPacketDelay;
		if (sscanf( optionValue, "%d", &tmpMaxPacketDelay ) != 1)
		{
			syslog( LOG_ERR, "Failed to parse MaxPacketDelay value. "
				"Using default value %dms", maxPacketDelayMillisec_ );
		}
		else
		{
			syslog( LOG_INFO, "Setting maximum packet delay to %dms",
				tmpMaxPacketDelay );
			maxPacketDelayMillisec_ = tmpMaxPacketDelay;
		}
	}

	return true;
}


/**
 *
 */
int BWMachined::run()
{
	// Generate a timestamp to set the base timestamp offset, since the
	// subsequent calls to addTimer() will cause the offset to get stuck high
	this->timeStamp();

	cluster_.birthHandler_.addTimer();
	cluster_.floodTriggerHandler_.addTimer();

	// The UpdateHandler is the thing that updates machine and process
	// statistics, checks for dead listeners etc, basically all the housekeeping
	// we want every so often.
	UpdateHandler updateHandler( *this );
	callbacks_.add( this->timeStamp() + UPDATE_INTERVAL,
		UPDATE_INTERVAL, &updateHandler, NULL, "UpdateHandler" );

	// listen for requests
	syslog( LOG_INFO, "Listening for requests" );
	// Obtain the highest FD for the call to select()
	int maxfd = std::max( ep_.fileno(),
					std::max( epBroadcast_.fileno(), epLocal_.fileno() ) );

	struct timeval tv;
	fd_set fds;

	int exitCode = EXIT_SUCCESS;

	// If PID path is blank after examining arguments and config file, don't 
	// attempt to create a PID file.
	if (!pidPath_.empty())
	{
		FILE * pidFile = fopen( pidPath_.c_str(), "w" );

		if (pidFile == NULL)
		{
			syslog( LOG_INFO, "Cannot access PID file %s, error: %s", 
					pidPath_.c_str(), strerror( errno ) );
			exitCode = EXIT_FAILURE;
			return exitCode;
		}

		if (fprintf( pidFile, "%d", mf_getpid() ) < 0)
		{
			syslog( LOG_INFO, "Cannot write to PID file %s, error: %s", 
					pidPath_.c_str(), strerror( errno ) );
			exitCode = EXIT_FAILURE;
			fclose( pidFile );
			return exitCode;
		}
		fclose( pidFile );
	}

	// process packets until we time out
	while (g_serverRunning)
	{
		// Process scheduled callbacks
		TimeQueue64::TimeStamp tickTime = this->timeStamp();
		callbacks_.process( tickTime );

		// We'll select() until the next timer is due to go off
		TimeQueue64::TimeStamp ttn = callbacks_.nextExp( tickTime );
		timeStampToTV( ttn, tv );

		// Note a Win32 improvement could be done here.  Issues an
		// overlapped RecvFrom and then do a WaitForMultipleObjects on the
		// Socket as well as the process handles

		FD_ZERO( &fds );
		FD_SET( ep_.fileno(), &fds );
		FD_SET( epBroadcast_.fileno(), &fds );
		FD_SET( epLocal_.fileno(), &fds );

		int osmaxfd = getInterestingFds( &fds, NULL, NULL );

		int selgot = select( std::max( maxfd+1, osmaxfd+1 ),
			&fds, NULL, NULL, &tv );

		if (selgot == 0) 
		{
			continue;
		}
		else if (selgot == -1)
		{
			if (errno != EINTR)
			{
				syslog( LOG_CRIT, "select got an error: %s\n",
					strerror( errno ) );
				exitCode = EXIT_FAILURE;
				break;
			}

			syslog( LOG_ERR, "select returned %d - %s\n",
				errno, strerror( errno ) );
			continue;
		}

		// Read from any of the endpoints that have data
		if (FD_ISSET( ep_.fileno(), &fds ))
		{
			this->readPacket( ep_, tickTime );
		}

		if (FD_ISSET( epLocal_.fileno(), &fds ))
		{
			this->readPacket( epLocal_, tickTime );
		}

		if (FD_ISSET( epBroadcast_.fileno(), &fds ))
		{
			this->readPacket( epBroadcast_, tickTime );
		}

		handleInterestingFds( &fds, NULL, NULL );
	}

	callbacks_.clear();

	return exitCode;
}


/**
 *
 */
void BWMachined::readPacket( Endpoint & ep, TimeQueue64::TimeStamp & tickTime )
{
	// ok, let's read the request then
	static char streamBuf[ MGMPacket::MAX_SIZE ];
	sockaddr_in	sin;

	// Read packet into buffer
	int len = ep.recvfrom( &streamBuf, sizeof( streamBuf ), sin );

	if (len == -1)
	{
		syslog( LOG_ERR, "recvfrom got an error: %s\n", strerror( errno ) );
		return;
	}

	// Destream the packet
	MemoryIStream is( streamBuf, len );
	MGMPacket *pPacket = new MGMPacket( is );

	if (is.error())
	{
		syslog( LOG_ERR, "Dropping packet with bogus message" );
		bw_safe_delete( pPacket );
		return;
	}

	// Schedule broadcast packets for later
	if (pPacket->shouldStaggerReply())
	{
		callbacks_.add( tickTime + (rand() % maxPacketDelayMillisec_),
						0, &packetTimeoutHandler_,
						new IncomingPacket( *this, pPacket, sin ),
						"IncomingPacket" );
		return;
	}

	this->handlePacket( ep, sin, *pPacket );
	bw_safe_delete( pPacket );
}


/**
 *
 */
void BWMachined::updateSystemInfo()
{
	static bool firstTime = true;
	SystemInfo &si = systemInfo_;

	// Do the platform specific stuff
	if (!pServerPlatform_->updateSystemInfo( si, pServerInfo_ ))
	{
		return;
	}

	// Do platform specific updates again if this is the first execution of this
	// function, since all the 'old' values won't have been set yet and we'll
	// get errors unless we update twice.
	if (firstTime)
	{
		firstTime = false;

		if (!pServerPlatform_->updateSystemInfo( si, pServerInfo_ ))
		{
			return;
		}
	}

	// Write new CPU and mem stats into the MGM
	for (uint i=0; i < si.nCpus; i++)
	{
		uint64 cpuDiff = std::max( si.cpu[i].max.delta(), (uint64)1 );
		uint8 cpuLoad = (uint8)(si.cpu[i].val.delta()*0xff / cpuDiff);

		// val.delta() is effectively the % of CPU usage as the jiffy time on
		// x86 should result in a total diff of 100.
		si.m.cpuLoads_[i] = cpuLoad;

		// High Precision statistics
		si.hpm.cpuLoads_[i] = cpuLoad;
	}

	// If extended stats are being read from /proc/stat we can use the
	// IO wait time as well.
	uint64 ioWaitDiff = std::max( si.iowait.max.delta(), (uint64)1 );
	si.hpm.ioWaitTime_ = (uint8)((si.iowait.val.delta() * 0xff) / ioWaitDiff);

	si.m.mem_   = (uint8)(si.mem.val.cur()*0xff / si.mem.max.cur());
	si.hpm.mem_ = (uint8)(si.mem.val.cur()*0xff / si.mem.max.cur());

	// Write network stats into the MGM
	if (si.m.ifStats_.empty())
	{
		si.m.setNInterfaces( si.ifInfo.size() );
	}

	if (si.hpm.ifStats_.empty())
	{
		si.hpm.setNInterfaces( si.ifInfo.size() );
	}


	// Calculate the interface stats
	for (int i=0; i < si.m.nInterfaces_; i++)
	{

		// Low Precision interface stats
		si.m.ifStats_[i].name_ = si.ifInfo[i].name;

		si.m.ifStats_[i].bitsIn_ = std::min(
			si.ifInfo[i].bitsTotIn.delta() / BIT_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].bitsOut_ = std::min(
			si.ifInfo[i].bitsTotOut.delta() / BIT_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].packIn_ = std::min(
			si.ifInfo[i].packTotIn.delta() / PACK_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].packOut_ = std::min(
			si.ifInfo[i].packTotOut.delta() / PACK_INCREMENT, (uint64)0xff );

		// High Precision interface stats
		si.hpm.ifStats_[ i ].name_    = si.ifInfo[ i ].name;
		si.hpm.ifStats_[ i ].bitsIn_  = si.ifInfo[ i ].bitsTotIn.delta();
		si.hpm.ifStats_[ i ].bitsOut_ = si.ifInfo[ i ].bitsTotOut.delta();
		si.hpm.ifStats_[ i ].packIn_  = si.ifInfo[ i ].packTotIn.delta();
		si.hpm.ifStats_[ i ].packOut_ = si.ifInfo[ i ].packTotOut.delta();
	}

	// System-wide IP level packet loss stats
	si.m.inDiscards_ = (uint8)std::min( si.packDropIn.cur(), (uint64)0xff );
	si.m.outDiscards_ = (uint8)std::min( si.packDropOut.cur(), (uint64)0xff );

	si.hpm.inDiscards_ = (uint8)std::min( si.packDropIn.cur(), (uint64)0xff );
	si.hpm.outDiscards_ = (uint8)std::min( si.packDropOut.cur(), (uint64)0xff );
}


/**
 *
 */
void BWMachined::update()
{
	// Update the machine's stats
	this->updateSystemInfo();

	// Check on all our processes
	for (unsigned int i=0; i < procs_.size(); i++)
	{
		ProcessInfo &pi = procs_[ i ];
		if (!updateProcessStats( pi ) && errno == ENOENT)
		{
			syslog( LOG_ERR, "%s (uid:%d) died without deregistering!\n",
				pi.m.c_str(), pi.m.uid_ );
			removeRegisteredProc( i-- );
		}
	}

	// Check on all our listeners too, since they may have gone away
	birthListeners_.checkListeners();
	deathListeners_.checkListeners();

	// Cleanup zombie children that didn't send us a SIGCHLD for some reason
	waitpid( -1, NULL, WNOHANG );
}


/**
 * Inform other machined's on the network about the birth or death of a process
 * on this machine so that they can in turn inform their registered listeners
 * about it.
 */
bool BWMachined::broadcastToListeners( ProcessMessage &pm, int type )
{
	uint8 oldparam = pm.param_;
	pm.param_ = type | pm.PARAM_IS_MSGTYPE;

	bool ok = pm.sendto( ep_, htons( PORT_MACHINED ), BROADCAST,
		MGMPacket::PACKET_STAGGER_REPLIES );

	pm.param_ = oldparam;
	return ok;
}


/**
 *
 */
void BWMachined::removeRegisteredProc( unsigned index )
{
	if (index >= procs_.size())
	{
		syslog( LOG_ERR, "Can't remove reg proc at index %d/%" PRIzu "",
			index, procs_.size() );
		return;
	}

	ProcessInfo &pinfo = procs_[ index ];
	ProcessMessage pm;
	pm << pinfo.m;
	this->broadcastToListeners( pm, pm.NOTIFY_DEATH );

	procs_.erase( procs_.begin() + index );
}


/**
 *
 */
void BWMachined::sendSignal (const SignalMessage & sm)
{
	for (uint i=0; i < procs_.size(); i++)
	{
		ProcessInfo &pm = procs_[i];

		if (pm.m.matches( sm ))
		{
			kill( pm.m.pid_, sm.signal_ );

			syslog( LOG_INFO, "sendSignal: signal = %d pid = %d uid = %d",
				sm.signal_, pm.m.pid_, pm.m.uid_ );
		}
	}
}


/**
 * This method invokes the handler for each message in the packet.
 */
void BWMachined::handlePacket( Endpoint & ep, sockaddr_in & sin,
	MGMPacket & packet )
{
	MGMPacket replies;

	for (unsigned i=0; i < packet.messages_.size(); i++)
	{
		if (!this->handleMessage( ep, sin, *packet.messages_[i], replies ))
		{
			return;
		}
	}

	if (replies.messages_.size() > 0)
	{
		MemoryOStream os;

		if (replies.write( os ))
		{
			ep.sendto( os.data(), os.size(), sin );
		}
		else
		{
			// The most likely cause of this will be due to an over sized
			// MGMPacket.
			syslog( LOG_ERR, "handlePacket: Unable to generate reponse to "
				"message from %s.", ep.c_str() );
		}

	}
}


/**
 *
 */
bool BWMachined::handleMessage( Endpoint & ep, sockaddr_in & sin,
	MachineGuardMessage & mgm, MGMPacket & replies )
{
	// If the message we received is not known, don't attempt to process
	// any further. It is possible the data received is a valid structure
	// that has had its tail corrupted.
	if (mgm.flags_ & mgm.MESSAGE_NOT_UNDERSTOOD)
	{
		syslog( LOG_ERR, "Received unknown message: %s", mgm.c_str() );
		mgm.outgoing( true );
		replies.append( mgm );
		return true;
	}

	switch (mgm.message_)
	{

	case MachineGuardMessage::LISTENER_MESSAGE:
	{
		ListenerMessage &lm = static_cast< ListenerMessage& >( mgm );

		if (lm.param_ == (lm.ADD_BIRTH_LISTENER | lm.PARAM_IS_MSGTYPE))
		{
			birthListeners_.add( lm, sin.sin_addr.s_addr );
		}
		else if (lm.param_ == (lm.ADD_DEATH_LISTENER | lm.PARAM_IS_MSGTYPE))
		{
			deathListeners_.add( lm, sin.sin_addr.s_addr );
		}
		else
		{
			syslog( LOG_ERR, "Unrecognised param field for ListenerMessage: %x",
				lm.param_ );
			return false;
		}

		// Ack to sender
		lm.outgoing( true );
		replies.append( lm );

		return true;
	}

	// Note: this is identical (virtually) to the case below, any changes
	// should be mirrored there too.
	case MachineGuardMessage::WHOLE_MACHINE_MESSAGE:
	{
		// This should be machines replying to a broadcast keepalive poll
		if (mgm.outgoing())
		{
			uint32 inaddr = sin.sin_addr.s_addr;

			if (cluster_.pFloodReplyHandler_ &&
				mgm.seq() == cluster_.pFloodReplyHandler_->seq())
			{
				cluster_.pFloodReplyHandler_->markReceived( inaddr );
			}
			else
			{
				syslog( LOG_ERR, "Unsolicited WMM from %s:%d",
					inet_ntoa( (in_addr&)inaddr ), ntohs( sin.sin_port ) );
			}
		}

		// These are regular requests that could be coming from tools or other
		// machineds
		else
		{
			systemInfo_.m.copySeq( mgm );
			replies.append( systemInfo_.m );
		}
		return true;
	}

	// Note: this is identical (virtually) to the case above, any changes
	// should be mirrored there too.
	case MachineGuardMessage::HIGH_PRECISION_MACHINE_MESSAGE:
	{
		// This should be machines replying to a broadcast keepalive poll
		if (mgm.outgoing())
		{
			uint32 inaddr = sin.sin_addr.s_addr;

			if (cluster_.pFloodReplyHandler_ &&
				mgm.seq() == cluster_.pFloodReplyHandler_->seq())
			{
				cluster_.pFloodReplyHandler_->markReceived( inaddr );
			}
			else
			{
				syslog( LOG_ERR, "Unsolicited HPMM from %s:%d",
					inet_ntoa( (in_addr&)inaddr ), ntohs( sin.sin_port ) );
			}
		}

		// These are regular requests that could be coming from tools or other
		// machineds
		else
		{
			systemInfo_.hpm.copySeq( mgm );
			replies.append( systemInfo_.hpm );
		}
		return true;
	}

	case MachineGuardMessage::MACHINE_PLATFORM_MESSAGE:
	{
		MachinePlatformMessage &mpm = static_cast< MachinePlatformMessage& >( mgm );
		mpm.platformInfo_ = PlatformInfo::str();
		mpm.outgoing( true );
		replies.append( mpm );
		return true;
	}

	case MachineGuardMessage::PROCESS_MESSAGE:
	{
		ProcessMessage &pm = static_cast< ProcessMessage& >( mgm );

		// Explicit instances of this class shouldn't be using param filters
		if (!(pm.param_ & pm.PARAM_IS_MSGTYPE))
		{
			syslog( LOG_ERR, "PROCESS_MESSAGE tried to use param filters! (%x)",
				pm.param_ );
			return false;
		}

		int msgtype = pm.param_ & ~pm.PARAM_IS_MSGTYPE;

		// Don't allow other machines to register/deregister their processes
		// on this machine.
		if ((msgtype == pm.REGISTER || msgtype == pm.DEREGISTER) &&
			(uint32&)sin.sin_addr != cluster_.ownAddr_ &&
			(uint32&)sin.sin_addr != LOCALHOST)
		{
			syslog( LOG_ERR, "%s tried to register a process here!",
				inet_ntoa( sin.sin_addr ) );
			return false;
		}

		switch (msgtype)
		{
		case ProcessMessage::REGISTER:
		{
			// Make sure this new proc doesn't clash with an existing one
			unsigned int i = 0;
			while (i < procs_.size())
			{
				ProcessMessage &psm = procs_[ i ].m;

				if ((pm.pid_ == psm.pid_) &&
					(pm.category_ == psm.category_) &&
					(pm.name_ == psm.name_))
				{
					break;
				}

				// can't have two things on one port
				if (pm.port_ == psm.port_)
				{
					syslog( LOG_ERR, "%d registered on port (%d) "
						"that belonged to %d",
						pm.pid_, pm.port_, psm.pid_ );
					removeRegisteredProc( i );
				}
				else
				{
					++i;
				}
			}

			// This indicates re-registration.  This is bad.
			if (i < procs_.size())
			{
				syslog( LOG_ERR, "Received re-registration for %s\n",
					pm.c_str() );

			}
			else
			{
				// Make new entry for the new process
				procs_.push_back( ProcessInfo() );
			}

			// Write the registration info into the ProcessStatsMessage and
			// mark it as outgoing for future sends
			ProcessInfo &pi = procs_[i];
			pi.m << pm;
			pi.m.outgoing( true );

			// Update it twice to ensure fields are in a readable state
			for (int j=0; j < 2; j++)
			{
				updateProcessStats( pi );
			}

			// platform-specific initialisation
			pi.init( pm );

			// Tell listeners about it
	 		broadcastToListeners( pm, pm.NOTIFY_BIRTH );

			// and confirm the registration to the sender
			pm.outgoing( true );
			replies.append( pm );

			syslog( LOG_INFO, "Added %s for uid:%d (debug: at index %d)\n",
				pm.c_str(), pm.uid_, i );
			return true;
		}

		case ProcessMessage::DEREGISTER:
		{
			unsigned int i;
			for (i=0; i < procs_.size(); i++)
				if (pm.pid_ == procs_[i].m.pid_)
					break;

			if (i >= procs_.size())
				syslog( LOG_ERR, "Couldn't find pid %d to deregister it\n",
					pm.pid_ );
			else
				removeRegisteredProc( i );

			// confirm the deregistration to the sender
			pm.outgoing( true );
			replies.append( pm );

			syslog( LOG_INFO, "Deregistered %s for uid:%d\n",
				pm.c_str(), pm.uid_ );
			return true;
		}

		case ProcessMessage::NOTIFY_BIRTH:
		{
			birthListeners_.handleNotify( this->endpoint(), pm, sin.sin_addr );
			return true;
		}

		case ProcessMessage::NOTIFY_DEATH:
		{
			deathListeners_.handleNotify( this->endpoint(), pm, sin.sin_addr );
			return true;
		}

		default:
			syslog( LOG_ERR, "Unrecognised ProcessMessage type: %d", msgtype );
			return false;
		}
	}

	case MachineGuardMessage::PROCESS_STATS_MESSAGE:
	{
		ProcessStatsMessage &query = static_cast< ProcessStatsMessage& >( mgm );

		// Find the processes this matches
		bool found = false;
		for (BW::vector< ProcessInfo >::iterator it = procs_.begin();
			 it != procs_.end(); ++it)
		{
			ProcessInfo &pi = *it;
			if (pi.m.matches( query ))
			{
				// Update load and mem stats on the MGM for this process
				SystemInfo &si = systemInfo_;
				uint64 cpuDiff = std::max(
					si.cpu[ pi.affinity % si.nCpus ].max.delta(),
					(uint64)1 );

				if (pi.affinity >= (int)si.nCpus)
					syslog( LOG_ERR, "ProcessInfo (%s) has invalid affinity %d",
						pi.m.c_str(), pi.affinity );

				ProcessStatsMessage &reply = pi.m;
				reply.cpu_ = (uint8)(pi.cpu.delta()*0xff / cpuDiff);
				reply.mem_ = (uint8)(pi.mem.cur()*0xff / si.mem.max.cur());

				// Add reply to the stream
				reply.copySeq( query );
				replies.append( reply );
				found = true;
			}
		}

		// If nothing found, send back a message with pid == 0 so that recv
		// loops can terminate on client side
		if (!found)
		{
			query.pid_ = 0;
			query.outgoing( true );
			replies.append( query );
		}

		return true;
	}

	case MachineGuardMessage::CREATE_MESSAGE:
	case MachineGuardMessage::CREATE_WITH_ARGS_MESSAGE:
	{
		return this->handleCreateMessage( ep, sin, mgm, replies );
	}

	case MachineGuardMessage::SIGNAL_MESSAGE:
	{
		syslog( LOG_INFO, "Signal message from %s:%d",
					inet_ntoa( (in_addr&)sin.sin_addr.s_addr ),
					ntohs( sin.sin_port ) );

		SignalMessage &sm = static_cast< SignalMessage& >( mgm );
		sendSignal( sm );
		return true;
	}

	case MachineGuardMessage::TAGS_MESSAGE:
	{
		TagsMessage &tm = static_cast< TagsMessage& >( mgm );

		// Queries send through a vector with a single tag in it
		if (tm.tags_.size() != 1)
		{
			syslog( LOG_ERR,
				"Tags queries must pass only one tag (%" PRIzu " passed)",
				tm.tags_.size() );
			return false;
		}
		BW::string query = tm.tags_[0];
		tm.tags_.clear();

		// An empty query indicates we're asking for the list of tag categories
		if (query == "")
		{
			for (TagsMap::iterator it = tags_.begin(); it != tags_.end(); ++it)
				tm.tags_.push_back( it->first );
			tm.exists_ = true;
		}

		// Otherwise we want a listing of a particular category
		else
		{
			TagsMap::iterator it = tags_.find( query );

			if (it != tags_.end())
			{
				Tags &tags = it->second;
				tm.tags_.resize( tags.size() );
				std::copy( tags.begin(), tags.end(), tm.tags_.begin() );
				tm.exists_ = true;
			}
			else
				tm.exists_ = false;
		}

		// Send the results back
		tm.outgoing( true );
		replies.append( tm );
		return true;
	}

	case MachineGuardMessage::USER_MESSAGE:
	{
		UserMessage &um = static_cast< UserMessage& >( mgm );
		BW::vector< UserMessage* > matches;

#		define FAIL 							\
		{ 										\
			users_.notfound_.copySeq( um );		\
			replies.append( users_.notfound_ );	\
			return true;						\
		}

		// If we're using PARAM_USE_UID, do a hash lookup
		if (um.param_ & um.PARAM_USE_UID)
		{
			UserMessage *pMatch = users_.fetch( um.uid_ );
			if (pMatch != NULL)
				matches.push_back( pMatch );
		}

		// Otherwise do linear search through user mapping
		else
		{
			for (UserMap::Map::iterator it = users_.map_.begin();
				 it != users_.map_.end(); ++it)
			{
				if (it->second.matches( um ))
					matches.push_back( &it->second );
			}
		}

		// If no matches
		if (matches.empty())
		{
			// If it was a specific search, forcefully make a new entry for this
			// guy
			if (um.param_ & (um.PARAM_USE_UID | um.PARAM_USE_NAME))
			{
				struct passwd *ent = um.param_ & um.PARAM_USE_UID ?
					getpwuid( um.uid_ ) : getpwnam( um.username_.c_str() );

				if (ent == NULL)
					FAIL;

				matches.push_back( users_.add( ent ) );
			}
			else
				FAIL;
		}

		for (unsigned i=0; i < matches.size(); i++)
		{
			matches[i]->copySeq( um );

			// Refresh environment if requested
			if (um.param_ & um.PARAM_REFRESH_ENV)
			{
				users_.getEnv( *matches[i] );
			}

			// We need to copy the param from the incoming message into the
			// outgoing one so it knows whether it needs to bother writing
			// coredump info or not.
			matches[i]->param_ = um.param_;
			if (um.param_ & um.PARAM_CHECK_COREDUMPS)
			{
				pServerPlatform_->checkCoreDumps( um.uid_,
						matches[i]->mfroot_, (*matches[ i ]).coredumps_ );
			}

			if (um.param_ & um.PARAM_GET_VERSION)
			{
				pServerPlatform_->determineVersion( um.uid_, 
					matches[i]->mfroot_, matches[i]->versionString_ );
			}

			replies.append( *matches[i] );
		}

		return true;
#		undef FAIL
	}

	case MachineGuardMessage::PID_MESSAGE:
	{
		PidMessage &pm = static_cast< PidMessage& >( mgm );
		pm.running_ = pServerPlatform_->isProcessRunning( pm.pid_ );
		pm.outgoing( true );
		replies.append( pm );
		return true;
	}

	case MachineGuardMessage::RESET_MESSAGE:
	{
		ResetMessage &rm = static_cast< ResetMessage& >( mgm );
		this->readConfigFile();
		users_.flush();
		syslog( LOG_INFO, "Flushing tags and user mapping at %s's request",
			inet_ntoa( sin.sin_addr ) );
		rm.outgoing( true );
		replies.append( rm );
		return true;
	}

	case MachineGuardMessage::MACHINED_ANNOUNCE_MESSAGE:
	{
		MachinedAnnounceMessage &mam =
			static_cast< MachinedAnnounceMessage& >( mgm );

		if (mam.type_ == mam.ANNOUNCE_BIRTH)
		{
			// If you're getting these you're newborn.
			if (mam.outgoing())
			{
				cluster_.birthHandler_.markReceived( sin.sin_addr.s_addr,
					mam.count_ );
				return true;
			}

			// When machines initially announce their own birth, we tell them
			// how many machines are on the cluster
			else
			{
				uint32 inaddr = sin.sin_addr.s_addr;
				if (cluster_.machines_.count( inaddr ) == 0)
				{
					cluster_.machines_.insert( inaddr );
					cluster_.chooseBuddy();
				}

				mam.outgoing( true );
				mam.count_ = cluster_.machines_.size();
				replies.append( mam );
				return true;
			}
		}

		// This is a machined telling us that a machine has gone offline
		else if (mam.type_ == mam.ANNOUNCE_DEATH)
		{
			uint32 deadaddr = (uint32)mam.addr_;

			if (deadaddr != cluster_.ownAddr_)
			{
				char addrstr[32];
				strcpy( addrstr, inet_ntoa( sin.sin_addr ) );
				syslog( LOG_INFO, "%s says %s is gone",
					addrstr, inet_ntoa( (in_addr&)deadaddr ) );

				cluster_.machines_.erase( deadaddr );
				cluster_.chooseBuddy();
			}
			else
			{
				cluster_.birthHandler_.addTimer();
				syslog( LOG_INFO,
					"Reports of my death have been greatly exaggerated!" );
			}
			return true;
		}

		// This is confirming that a particular host exists
		else if (mam.type_ == mam.ANNOUNCE_EXISTS)
		{
			if (cluster_.machines_.count( mam.addr_ ) == 0)
			{
				syslog( LOG_INFO, "Apparently %s is running machined",
					inet_ntoa( (in_addr&)mam.addr_ ) );
				cluster_.machines_.insert( mam.addr_ );
				cluster_.chooseBuddy();
			}
			return true;
		}

		else
		{
			syslog( LOG_ERR, "Received unknown MAM type %d", mam.type_ );
			return false;
		}
	}

	// should limit these messages to only anything coming from the local
	// interface
	case MachineGuardMessage::QUERY_INTERFACE_MESSAGE:
	{
		QueryInterfaceMessage &qim = static_cast< QueryInterfaceMessage& >(mgm);

		if (qim.address_ == QueryInterfaceMessage::INTERNAL)
		{
			qim.address_ = broadcastAddr_;
		}
		else
		{
			syslog( LOG_ERR, "Received QIF request for unknown type %d\n",
						qim.address_ );
		}

		// Now the query has been filled in, respond
		qim.outgoing( true );
		replies.append( qim );

		return true;
	}

	default:
		syslog( LOG_ERR, "Unknown message (%d) not marked as "
			"MESSAGE_NOT_UNDERSTOOD!!!", mgm.message_ );
		return false;

	}

	return false;
}


/**
 *
 */
bool BWMachined::handleCreateMessage( Endpoint &ep, sockaddr_in &sin,
	MachineGuardMessage &mgm, MGMPacket &replies )
{
	CreateMessage &cm = static_cast< CreateMessage& >( mgm );
	syslog( LOG_INFO, "Got message: %s", cm.c_str() );

	// We tell the client what the pid of the started process is
	PidMessageWithDestination *pPmwd = new PidMessageWithDestination();
	pPmwd->copySeq( cm );
	pPmwd->outgoing( true );
	pPmwd->target( ep, sin.sin_port, sin.sin_addr.s_addr );

	// Extract the user message for this guy and refresh his config info
	UserMessage *pUm = users_.fetch( cm.uid_ );

	// If the user is not mapped yet, add the mapping now
	if (pUm == NULL)
	{
		struct passwd *ent = getpwuid( cm.uid_ );
		if (ent == NULL)
		{
			syslog( LOG_ERR, "UID %d doesn't exist on this system, "
				"not starting %s", cm.uid_, cm.name_.c_str() );
			pPmwd->running_ = 0;
			replies.append( *pPmwd, true );
			return true;
		}

		pUm = users_.add( ent );
	}

	if (!users_.getEnv( *pUm ))
	{
		syslog( LOG_ERR, "Couldn't get env for user %s, not starting %s",
			pUm->username_.c_str(), cm.c_str() );
		pPmwd->running_ = 0;
		replies.append( *pPmwd, true );
		return true;
	}

	const char *pConfig = cm.config_.c_str();

	if ((bw_stricmp( BW_CONFIG_HYBRID.c_str(), pConfig ) != 0) &&
		(bw_stricmp( BW_CONFIG_DEBUG.c_str(),  pConfig ) != 0))
	{
		syslog( LOG_ERR, "Rejected process start request for user %s for "
			"invalid configuration '%s'", pUm->username_.c_str(), pConfig );
		pPmwd->running_ = 0;
		replies.append( *pPmwd, true );
		return true;
	}

	// Make sure there are no ..'s in the exe path so that we are definitely
	// executing something from within bigworld/bin/Hybrid
	if (cm.name_.find( ".." ) != BW::string::npos ||
		cm.config_.find( ".." ) != BW::string::npos)
	{
		syslog( LOG_ERR,
			"Illegal '..' in process name or config, not starting %s/%s",
			cm.name_.c_str(), cm.config_.c_str() );

		pPmwd->running_ = 0;
		replies.append( *pPmwd, true );
		return true;
	}

	// Note: This check for the _helpers directory is by no means extensive.
	//       It could effectively be worked around by running a command such
	//       as   commands/./_helpers/snapshot_helper  however as we are
	//       already running in a trusted environment, we assume no
	//       malicious attempts will occur. This is just a courtesy error
	//       message for behaviour that is not recommended.

	// Don't run any programs in the commands/_helpers directory as we have
	// setuid root processes there.
	if (cm.name_.find( "commands/_helpers" ) != BW::string::npos)
	{
		syslog( LOG_ERR,
			"Denied request to run _helper process '%s'",
			cm.name_.c_str() );
		pPmwd->running_ = 0;
		replies.append( *pPmwd, true );
		return true;
	}

	// magic +2 args is for --res <blah> in startProcess()
	static const unsigned int NUM_SPARE_ARGS_FOR_STARTPROCESS = 2;

	if (mgm.message_ == MachineGuardMessage::CREATE_MESSAGE)
	{
		// figure out what arguments we want to pass it
		unsigned int argc = 0;
		static const unsigned int MAX_ARGC = 10;

		const char *argv[ MAX_ARGC + NUM_SPARE_ARGS_FOR_STARTPROCESS ];

		// platform-specific code should fill in argv[0] once path is
		// decided
		argv[ argc++ ] = NULL;

		// first arg as -machined signifies we're starting things up
		argv[ argc++ ] = "-machined";

		// see if we are recovering
		if (cm.recover_)
		{
			argv[ argc++ ] = "-recover";
		}

		char forwardArg[32];

		if (cm.fwdIp_ != 0)
		{
			bw_snprintf( forwardArg, sizeof( forwardArg ), "%s:%d",
				inet_ntoa( (in_addr&)cm.fwdIp_ ),
				ntohs( cm.fwdPort_ ) );
			argv[ argc++ ] = "-forward";
			argv[ argc++ ] = forwardArg;
		}

		// make sure the code above doesn't get too unruly
		if (argc >= MAX_ARGC)
		{
			syslog( LOG_ERR, "Not starting process %s for uid %d: "
				"too many args (%d)", cm.name_.c_str(), cm.uid_, argc );
			pPmwd->running_ = 0;
			replies.append( *pPmwd, true );
			return true;
		}

		BW::string binaryDir;

		if (!pServerPlatform_->findUserBinaryDirForConfig( pUm->mfroot_,
			cm.config_, binaryDir ))
		{
			syslog( LOG_ERR, "Not starting process %s for uid %d: "
				"unable to determine valid binary location",
					cm.name_.c_str(), cm.uid_  );
			pPmwd->running_ = 0;
			replies.append( *pPmwd, true );
			return true;
		}

		if (startProcess( binaryDir.c_str(),
			pUm->bwrespath_.c_str(), binaryDir.c_str(),
			cm.name_.c_str(), pUm->uid_, pUm->gid_, pUm->home_.c_str(),
			argc, argv, *this, pPmwd ))
		{
			replies.append( *pPmwd, true );
		}
	}
	else	// mgm.message_ == MachineGuardMessage::CREATE_WITH_ARGS_MESSAGE
	{
		CreateWithArgsMessage &cwam =
			static_cast< CreateWithArgsMessage& >( cm );

		unsigned int argc = 0;
		// +2 args for process name and -recover
		const char **argv = new const char*[cwam.args_.size() +
								NUM_SPARE_ARGS_FOR_STARTPROCESS + 2];

		// platform-specific code should fill in argv[0] once path is
		// decided
		argv[ argc++ ] = NULL;

		// Append args from message
		for (CreateWithArgsMessage::Args::const_iterator i =
				cwam.args_.begin(); i != cwam.args_.end(); ++i)
		{
			argv[ argc++ ] = i->c_str();
		}

		// see if we are recovering
		if (cm.recover_)
		{
			argv[ argc++ ] = "-recover";
		}

		BW::string binaryDir;

		if (!pServerPlatform_->findUserBinaryDirForConfig( pUm->mfroot_,
			cm.config_, binaryDir ))
		{
			syslog( LOG_ERR, "Not starting process %s for uid %d: "
				"unable to determine valid binary location",
					cm.name_.c_str(), cm.uid_  );
			pPmwd->running_ = 0;
			replies.append( *pPmwd, true );
			return true;
		}

		if (startProcess( binaryDir.c_str(),
			pUm->bwrespath_.c_str(), binaryDir.c_str(),
			cm.name_.c_str(), pUm->uid_, pUm->gid_, pUm->home_.c_str(),
			argc, argv, *this, pPmwd ))
		{
			replies.append( *pPmwd, true );
		}

		bw_safe_delete_array( argv );
	}

	return true;

}


// -----------------------------------------------------------------------------
// Section: Broadcast handling stuff
// -----------------------------------------------------------------------------

/**
 *
 */
void BWMachined::PacketTimeoutHandler::handleTimeout(
	TimerHandle handle, void * pUser )
{
	IncomingPacket *pIncomingPacket = static_cast< IncomingPacket * >( pUser );
	pIncomingPacket->handle();
}


/**
 *
 */
void BWMachined::PacketTimeoutHandler::onRelease( TimerHandle handle,
		void *pUser )
{
	IncomingPacket *pIncomingPacket = static_cast< IncomingPacket * >( pUser );
	bw_safe_delete( pIncomingPacket );
}


// -----------------------------------------------------------------------------
// Section: Callback handling stuff
// -----------------------------------------------------------------------------

/**
 * This method retrieves a timestamp that can be used as a 'now' value for
 * calls to TimeQueue's methods.
 */
TimeQueue64::TimeStamp BWMachined::timeStamp()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	return ((TimeQueue64::TimeStamp)tv.tv_sec)*1000 + tv.tv_usec/1000;
}


/**
 *
 */
int BWMachined::maxPacketDelay() const
{
	return maxPacketDelayMillisec_;
}


/**
 *
 */
void BWMachined::UpdateHandler::handleTimeout( TimerHandle handle, void *pUser )
{
	machined_.update();
}

BW_END_NAMESPACE

// bwmachined.cpp
