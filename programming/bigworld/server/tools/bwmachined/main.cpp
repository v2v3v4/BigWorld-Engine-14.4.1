#include "common_machine_guard.hpp"
#include "linux_machine_guard.hpp"

#include "bwmachined.hpp"
#include "server/bwservice.hpp"
#include "cstdmf/bwversion.hpp"

#include "cstdmf/bw_string.hpp"
#include <sys/utsname.h>

BW_USE_NAMESPACE

static char usage[] =
	"Usage: %s [args]\n"
	"-f/--foreground   Run machined in the foreground (i.e. not as a daemon)\n"
	"-v/--version      Display version information for BWMachined\n"
	"-p/--pid          Set path for this process's PID file.\n";

/**
 * This value is the target hard-limit of maxium file descriptor for
 * processes started by bwmachined.
 * Processes still need to raise their own current limit (soft-limit)
 * to take advantage of the extra file descriptors
 */
static const unsigned long desiredMaximumFileDescriptorHardLimit = 16384L;

int BIGWORLD_MAIN_NO_RESMGR( int argc, char * argv[] )
{
	bool daemon = true;
	BW::string pidPath = "";
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp( argv[i], "-f" ) || !strcmp( argv[i], "--foreground" ))
		{
			daemon = false;
		}
		
		else if ((strcmp( argv[i], "-p" ) == 0) ||
				(strcmp( argv[i], "--pid" ) == 0))
		{
			++i;
			if (i >= argc)
			{
				fprintf( stderr, 
					"Invalid command line - no argument for -p/--pid\n" );
				return EXIT_FAILURE;
			}
			else
			{
				// Set PID file to this path
				pidPath = argv[i];
			}
		}

		else if ((strcmp( argv[i], "-v" ) == 0) ||
				(strcmp( argv[i], "--version" ) == 0))
		{
			const BW::string & bwversion = BWVersion::versionString();
			printf( "BWMachined (BigWorld %s %s. %s %s)\nProtocol version %d\n",
				bwversion.c_str(), MF_CONFIG, __TIME__, __DATE__,
				BWMACHINED_VERSION );
			return EXIT_SUCCESS;
		}

		else if (strcmp( argv[i], "--help" ) == 0)
		{
			printf( usage, argv[0] );
			return EXIT_SUCCESS;
		}

		else
		{
			fprintf( stderr, "Invalid argument: '%s'\n", argv[i] );
			return EXIT_FAILURE;
		}
	}

	// Open syslog to allow us to log messages
	openlog( argv[0], 0, LOG_DAEMON );

	// Attempt to create the machined instance prior to becoming a daemon
	// to allow better error reporting in the init.d script
	BWMachined machined;
	
	if (daemon && !pidPath.empty())
	{
		machined.setPidPath( pidPath );
	}

	// Turn ourselves into a daemon if required
	initProcessState( daemon );
	srand( (int)timestamp() );

	rlimit rlimitData;
	rlimitData.rlim_cur = RLIM_INFINITY;
	rlimitData.rlim_max = RLIM_INFINITY;

	if (bw_prlimit( /*pid*/ 0, RLIMIT_CORE, &rlimitData, NULL ) == -1)
	{
		syslog( LOG_ERR, "Unable to set core file privileges: %s\n",
					strerror( errno ) );
	}

	// Ensure kernel settings for socket buffer sizes are sufficient
	checkSocketBufferSizes();

	// Ensure child processes can increase file descriptor
	// limits if required
	if (!raiseFileDescriptorHardLimit( desiredMaximumFileDescriptorHardLimit ))
	{
		if (getuid() == 0)
		{
			return EXIT_FAILURE;
		}

		printf( "Failed to raise file descriptor limit as non-root user: %s\n",
			strerror( errno ) );
	}

	if (BWMachined::pInstance())
	{
		return machined.run();
	}
	else
	{
		return EXIT_FAILURE;
	}
}

// main.cpp
