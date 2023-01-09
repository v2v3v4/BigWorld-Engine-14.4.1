#include "linux_machine_guard.hpp"

#include "bwmachined.hpp"

#include "server/server_info.hpp"
#include "network/basictypes.hpp"
#include "network/network_utils.hpp"

#include "cstdmf/build_config.hpp"
#include "cstdmf/bw_platform_info.hpp"

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <linux/sysctl.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


BW_BEGIN_NAMESPACE

bool getProcessTimes( FILE *f, unsigned long int *utimePtr,
	unsigned long int *stimePtr, unsigned long int *vsizePtr,
	unsigned long int *starttimePtr, int *cpuPtr );

const char * machinedConfFile = "/etc/bwmachined.conf";
const char * bigworldConfFile = "/etc/bigworld.conf";

// Anonymous namespace
namespace
{

typedef BW::map< int, std::pair< PidMessageWithDestination *, BW::string > > PendingProcessMap;
static PendingProcessMap s_pendingProcesses;

}


/**
 *	This function handles the SIGCHLD signal caused by the termination of
 *	one of the child processes this bwmachined instance fork()ed.
 */
void sigChildHandler( int )
{
	// Clean up any child zombies.
	waitpid( -1, NULL, WNOHANG );
}


/**
 *
 */
void initProcessState( bool isDaemon )
{
	if (isDaemon)
	{
		daemon( 0, 0 );
	}
	else
	{
		printf( "Not running as daemon\n" );
	}

	// Handle sigchld so we know when our children stop.
	signal( SIGCHLD, sigChildHandler );
}


/**
 *
 */
void ProcessInfo::init( const ProcessMessage &pm )
{
}


/**
 *
 */
void cleanupProcessState()
{
}


/**
 *	This method gives the machine guard a chance to add Fds to the main loop's
 *	select call
 */
int getInterestingFds( fd_set *readfds, fd_set *writefds, fd_set *exceptfds )
{
	int maxFd = -1;
	for (PendingProcessMap::const_iterator it = s_pendingProcesses.begin();
		it != s_pendingProcesses.end(); ++it)
	{
		FD_SET( it->first, readfds );
		maxFd = std::max( maxFd, it->first );
	}
	return maxFd;
}


/**
 *	This method gives the machine guard a chance to handle any Fds which select
 *	has reported as ready
 */
void handleInterestingFds( fd_set *readfds, fd_set *writefds, fd_set *exceptfds )
{
	PendingProcessMap::iterator it = s_pendingProcesses.begin();

	while (it != s_pendingProcesses.end())
	{
		int fd = it->first;

		if (!FD_ISSET( fd, readfds ))
		{
			++it;
			continue;
		}

		PidMessageWithDestination * pPmwd = it->second.first;
		const BW::string binaryName = it->second.second;
		s_pendingProcesses.erase( it++ );

		error_t childError;
		int readlen = read( fd, &childError, sizeof( childError ) );
		close( fd );

		if (readlen == 0)
		{
			// Pipe closed by exec
			pPmwd->running_ = true;
		}
		else if (readlen == -1)
		{
			// An error occurred? We don't know if the child succeeded or not...
			syslog( LOG_ERR, "Error checking child status: %s, assuming exec "
				"for %s failed",
				strerror( errno ), binaryName.c_str() );
			pPmwd->pid_ = 0;
			pPmwd->running_ = false;
		}
		else
		{
			// We got an error code back from the child
			syslog( LOG_ERR, "Error starting child process %s: "
					"%s, check syslog for details",
				binaryName.c_str(), strerror( childError ) );
			pPmwd->pid_ = 0;
			pPmwd->running_ = false;
		}
		pPmwd->sendToTarget();
		bw_safe_delete( pPmwd );
	}
}


/**
 * This method ensures that the buffers for read and write sockets will be
 * sufficient.
 */
void checkSocketBufferSizes()
{
	int readBufSize  = getMaxBufferSize( /*isReadBuffer:*/ true );
	int writeBufSize = getMaxBufferSize( /*isReadBuffer:*/ false );

	const char* errorMsg = 
		"%s socket buffer has insufficient size. Expecting %d, actual %d.\n";

	if ((readBufSize != -1) && (readBufSize < MIN_RCV_SKT_BUF_SIZE))
	{
		syslog( LOG_ERR, errorMsg, "Read", MIN_RCV_SKT_BUF_SIZE, readBufSize );
	}

	if ((writeBufSize != -1) && (writeBufSize < MIN_SND_SKT_BUF_SIZE))
	{
		syslog( LOG_ERR, errorMsg, "Write",
			MIN_SND_SKT_BUF_SIZE, writeBufSize );
	}
}


/**
 *
 */
bool updateProcessStats( ProcessInfo &pi )
{
	// try to open the file
	char pinfoFilename[ 64 ];
	bw_snprintf( pinfoFilename, sizeof( pinfoFilename ),
		"/proc/%d/stat", (int)pi.m.pid_ );

	FILE *pinfo;
	if ((pinfo = fopen( pinfoFilename, "r" )) == NULL)
	{
		if (errno != ENOENT)
		{
			syslog( LOG_ERR, "Couldn't open %s: %s",
				pinfoFilename, strerror( errno ) );
		}

		return false;
	}

	// Retrieve process stats
	unsigned long int utime, stime, vsize, starttime;
	int cpu;
	if (!getProcessTimes( pinfo, &utime, &stime, &vsize, &starttime, &cpu ))
	{
		syslog( LOG_ERR, "Failed to update process stats for '%s': %s",
				pinfoFilename, strerror( errno ) );
		fclose( pinfo );
		return false;
	}

	if ((pi.starttime) && (pi.starttime != starttime))
	{
		syslog( LOG_ERR, "updateProcessStats: Process %d starttime differs from "
				"last known starttime (old %lu, curr %lu).",
				(int)pi.m.pid_, pi.starttime, starttime );
		fclose( pinfo );
		return false;
	}

	pi.cpu.update( utime + stime );
	pi.mem.update( vsize );
	pi.affinity = cpu;
	pi.starttime = starttime;

	fclose( pinfo );
	return true;
}


/**
 *
 */
bool getProcessTimes( FILE *f, unsigned long int *utimePtr,
	unsigned long int *stimePtr, unsigned long int *vsizePtr,
	unsigned long int *starttimePtr, int *cpuPtr )
{
	if (fseek( f, SEEK_SET, 0 ) != 0)
	{
		return false;
	}

	// This hideous list of variables and the associated format string
	// originates from  'man 5 proc' under the section '/proc/[number]/stat'
	// also cross-reference in Linux source (fs/proc/array.c)
	int	pid;
	char name[ 255 ];
	char state;
	int	ppid, pgrp, session, tty, tpgid;
	unsigned long int flags, minflt, cminflt, majflt, cmajflt, utime, stime;
	long int cutime, cstime, priority, nice, num_threads, itrealvalue;
	unsigned long int starttime, vsize;
	long int rss;
	unsigned long int rlim, startcode, endcode, startstack, kstkesp, kstkeip;
	unsigned long int signal, blocked, sigignore, sigcatch;
	unsigned long int wchan, nswap, cnswap;
	int exit_signal, processor;
	unsigned long rt_priority = 0, policy = 0;

	int returnVal = fscanf( f, "%d %s %c %d %d %d %d %d " // pid, ...
		"%lu %lu %lu %lu %lu %lu %lu " // flags, ....
		"%ld %ld %ld %ld %ld %ld " // cutime, ...
		"%lu %lu %ld %lu %lu %lu %lu %lu %lu " // starttime, ...
		"%lu %lu %lu %lu %lu %lu %lu " // signal, ...
		"%d %d %lu %lu", // exit_signal, ...

		&pid, name, &state,
		&ppid, &pgrp, &session, &tty, &tpgid,
		&flags, &minflt, &cminflt, &majflt, &cmajflt, &utime, &stime,
		&cutime, &cstime, &priority, &nice, &num_threads, &itrealvalue,
		&starttime, &vsize, &rss,
		&rlim, &startcode, &endcode, &startstack, &kstkesp, &kstkeip,
		&signal, &blocked, &sigignore, &sigcatch, &wchan, &nswap, &cnswap,
		&exit_signal, &processor, &rt_priority, &policy );

	static const int minResultsExpected = 39; // 2.6 added rt_priority...
	if ((returnVal < minResultsExpected) || (ferror( f )))
	{
		return false;
	}

	if ( utimePtr ) *utimePtr = utime;
	if ( stimePtr ) *stimePtr = stime;
	if ( vsizePtr ) *vsizePtr = vsize;
	if ( starttimePtr ) *starttimePtr = starttime;
	if ( cpuPtr ) *cpuPtr = processor;

	return true;
}


/**
 *	This method puts a key/value pair into the shell environment to make it
 *	available to a process that is about to be executed.
 */
char * putEnvAlloc(const char * name, const char * value)
{
	int len = strlen( name ) + strlen( value ) + 2;
	char * str = new char[ len ];
	strcpy( str, name );
	strcat( str, "=" );
	strcat( str, value );
	putenv( str );
	return str;
}


/**
 *  Start a new process on this machine, using the provided configuration.
 *
 *	The machined object needs to be passed in so that we can close down all its
 *	sockets so the child processes don't hang on to them.
 *
 *	If this function returns true, pPmwd->pid_ and pPmwd->running_ must have
 *	been updated.
 * 
 *	If this function returns false, the machine guard code is responsible for
 *	sending and deallocating the PidMessageWithDestination.
 */
bool startProcess( const char * bwBinaryDir,
	const char * bwResPath,
	const char * config,
	const char * binaryName,
	MachineGuardMessage::UserId uid,
	uint16 gid,
	const char * home,
	int argc,
	const char ** argv,
	BWMachined &machined,
	PidMessageWithDestination * pPmwd )
{
	pid_t childpid;
	// This is { read, write }
	int statusPipe[ 2 ];

	if (pipe( statusPipe ) == -1)
	{
		syslog( LOG_ERR,
			"Failed to create status pipe: %s, aborting exec for %s",
			strerror( errno ), binaryName );
		pPmwd->pid_ = 0;
		pPmwd->running_ = false;
		return true;
	}

	if ((childpid = fork()) == 0)
	{
		close( statusPipe[ 0 ] );

		if (fcntl( statusPipe[ 1 ], F_SETFD, FD_CLOEXEC ) == -1)
		{
			syslog( LOG_ERR,
				"Failed to set status pipe to close-on-exec: %s, aborting "
				"exec for %s",
				strerror( errno ), binaryName );
			write( statusPipe[ 1 ], &errno, sizeof( errno ) );
			exit( EXIT_FAILURE );
		}

		if (setgid( gid ) == -1)
		{
			syslog( LOG_ERR,
				"Failed to setgid() to %d for user %d, group will be root\n",
				gid, uid );
		}

		if (setuid( uid ) == -1)
		{
			syslog( LOG_ERR, "Failed to setuid to %d, aborting exec for %s\n",
				uid, binaryName );
			write( statusPipe[ 1 ], &errno, sizeof( errno ) );
			exit( EXIT_FAILURE );
		}

		// Note: the binary directory is checked after performing setUID() in
		// order to handle a potential relative directory ~/

		// figure out the right bin dir
		char path[ 512 ];
		strcpy( path, bwBinaryDir );
		strcat( path, "/" );

		// change to it
		chdir( path );

		// now add the exe name
		strncat( path, binaryName, 32 );
		argv[0] = path;

		// Assemble the --res commandline switch
		argv[ argc++ ] = "--res";
		argv[ argc++ ] = bwResPath;

		// Close parent sockets
		machined.closeEndpoints();

		// Close sockets tracking other processes
		for (PendingProcessMap::const_iterator it = s_pendingProcesses.begin();
			it != s_pendingProcesses.end(); ++it)
		{
			close( it->first );
		}

		// Insert env variable for timing settings
		putEnvAlloc( "BW_TIMING_METHOD", machined.timingMethod() );

		// Insert env variable for home directory
		putEnvAlloc( "HOME", home );

		// Now a quick check to see if the file exists before we run it.
		// We will still attempt to execute on failure but this allows
		// better user feedback.
		struct stat statbuffer;

		if (stat( path, &statbuffer ) != 0)
		{
			syslog( LOG_ERR, "Unable to stat '%s': %s",
				path, strerror( errno ) );
		}

		if (!S_ISREG( statbuffer.st_mode ))
		{
			syslog( LOG_ERR, "'%s' is not a regular file.", path );
		}

		syslog( LOG_INFO, "UID %d execing '%s'", uid, path );

		argv[ argc ] = NULL;
		// __kyl__ (21/4/2008) This is a rather worrying const cast. If execv()
		// modifies any of the arguments, we're screwed since we're pointing
		// to constant strings.
		int result = execv( path, const_cast< char * const * >( argv ) );

		if (result == -1)
		{
			syslog( LOG_ERR, "Failed to exec '%s': %s\n", 
				path, strerror( errno ) );
		}

		write( statusPipe[ 1 ], &errno, sizeof( errno ) );
		exit( EXIT_FAILURE );
	}
	else if (childpid == -1)
	{
		close( statusPipe[ 1 ] );
		close( statusPipe[ 0 ] );
		syslog( LOG_ERR,
			"Failed to fork: %s, aborting exec for %s",
			strerror( errno ), binaryName );
		pPmwd->pid_ = 0;
		pPmwd->running_ = false;
		return true;
	}
	else
	{
		close( statusPipe[ 1 ] );
		s_pendingProcesses[ statusPipe[ 0 ] ] =
			std::make_pair( pPmwd, BW::string( binaryName ) );
		pPmwd->pid_ = (uint16)childpid;
		return false;
	}
}


/**
 *
 */
bool validateProcessInfo( const ProcessInfo &processInfo )
{
	// try to open the file
	char pinfoFilename[ 64 ];
	bw_snprintf( pinfoFilename, sizeof( pinfoFilename ),
		"/proc/%d/stat", (int)processInfo.m.pid_ );

	FILE *fp;
	if ((fp = fopen( pinfoFilename, "r" )) == NULL)
	{
		if (errno != ENOENT)
		{
			syslog( LOG_ERR, "Couldn't open %s: %s",
				pinfoFilename, strerror( errno ) );
		}

		return false;
	}

	unsigned long int starttime;

	bool status = getProcessTimes( fp, NULL, NULL, NULL, &starttime, NULL );
	fclose( fp );

	if ((status) &&
		(processInfo.starttime) && (starttime != processInfo.starttime))
	{
		syslog( LOG_ERR, "validateProcessInfo: Process %d starttime differs "
					"from last known starttime (old %lu, curr %lu).",
					(int)processInfo.m.pid_, processInfo.starttime, starttime );
		return false;
	}

	return status;
}


int bw_prlimit( pid_t pid, bw_rlimit_resource resource, const struct rlimit *new_limit,
	struct rlimit *old_limit )
{
#if defined( BW_USE_PRLIMIT )
	return prlimit( pid, resource, new_limit, old_limit );
#else
	if (old_limit != NULL)
	{
		if (getrlimit( resource, old_limit ) == -1)
		{
			return -1;
		}

	}

	return (new_limit == NULL) ? 0 : setrlimit( resource, new_limit );
#endif
}


/**
 * This method attempts to ensure we can use file descriptors at least this
 * high.
 *
 * The limit also propagates to our child processes, where it's more
 * interesting.
 */
bool raiseFileDescriptorHardLimit( unsigned long desiredLimit )
{
	// On Linux, RLIMIT_NOFILE is limited to /proc/sys/fs/nr_open
	const char *kernel_nr_open_file = "/proc/sys/fs/nr_open";
	FILE *fp = fopen( kernel_nr_open_file, "r" );

	if (fp != NULL)
	{
		unsigned long nr_open;
		char line[ 512 ];
		fgets( line, sizeof( line ), fp );

		int numLinesRead = sscanf( line, "%lu", &nr_open );
		fclose( fp );

		if (numLinesRead != 1)
		{
			syslog( LOG_ERR, "Invalid line in %s: '%s'",
				kernel_nr_open_file, line );
			return false;
		}

		// Cap the requested limit to the system defined limit
		if (desiredLimit > nr_open)
		{
			syslog( LOG_WARNING, "Requested file descriptor limit "
				"of %lu but %s is currently %lu. Setting "
				"file descriptor limit to %lu",
				desiredLimit, kernel_nr_open_file, nr_open, nr_open );
			desiredLimit = nr_open;
		}
	}

	rlimit nofile;

	if (bw_prlimit( /*pid*/ 0, RLIMIT_NOFILE, NULL, &nofile ) == -1)
	{
		syslog( LOG_ERR, "Unable to get file descriptor limits: %s",
					strerror( errno ) );
		return false;
	}

	if (nofile.rlim_max < desiredLimit)
	{
		nofile.rlim_max = desiredLimit;

		if (bw_prlimit( /*pid*/ 0, RLIMIT_NOFILE, &nofile, NULL ) == -1)
		{
			syslog( LOG_ERR, "Unable to set file descriptor limits to %lu: %s",
						desiredLimit, strerror( errno ) );
			return false;
		}
	}

	return true;
}

BW_END_NAMESPACE

// linux_machine_guard.cpp
