#include "server_platform_linux.hpp"

// TODO: this include is only temporary until structures
//       are transitioned into more appropriate locations.
#include "common_machine_guard.hpp"

#include "server/glob.hpp"
#include "server/server_info.hpp"

#include "cstdmf/build_config.hpp"
#include "cstdmf/bw_platform_info.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"

#include <libgen.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>


namespace
{


/*
 *	This function is a convenience to test the existence of a directory without
 *	emitting any error messages.
 */
bool directoryExists( const BW::string & directoryPath )
{
	struct stat statBuf;

	// TODO: this only checks for existence rather than explicitly being a
	// directory.
	return (stat( directoryPath.c_str(), &statBuf ) == 0);
}

};


BW_BEGIN_NAMESPACE


/**
 *	Visitor class for binary paths.
 */
class BinaryPathVisitor
{
public:
	/** Destructor.*/
	virtual ~BinaryPathVisitor() {}

	/**
	 *	This method is called for each binary path to visit, as well as chdir()
	 *	being called for the path just before.
	 *
	 *	The original current working directory will be restored when the
	 *	entire visit loop completes or terminates.
	 *
	 *	@param path A binary path.
	 */
	virtual void onBinaryPath( const BW::string & path ) = 0;

protected:
	/** Constructor. */
	BinaryPathVisitor() {}
};



/**
 *	Constructor.
 */
ServerPlatformLinux::ServerPlatformLinux() :
	ServerPlatform(),
	hasExtendedStats_( false )
{
	this->initConfigSuffixes();
	this->initKernelVersion();

	isInitialised_ = this->initArchitecture();
}


/**
 *	This method initialises the suffixes of the binary paths to avoid costly
 *	string manipulations when a request to start a binary is encounterd.
 *
 *	Due to the large number of permutations of path suffixes now due to
 *	changing directory structures, architecture support, and even more
 *	directory structure changes, along with the need for backwards
 *	compatability support in BWMachineD, we pre-generate all the path
 *	suffixes based for each supported BW_CONFIG type to help avoid lots
 *	of string manipulation during runtime.
 */
void ServerPlatformLinux::initConfigSuffixes()
{
	const BW::string newDirectoryPrefix( "/game/bin/server/" );
	const BW::string oldDirectoryPrefix( "/bigworld/bin/" );
	const BW::string serverSuffix( "/server" );
	const BW::string arch64Suffix( "64" );
	const BW::string bwConfigAsLowerHybrid( "hybrid" );
	const BW::string bwConfigAsLowerDebug( "debug" );

	const BW::string & platformBuildStr = BW::PlatformInfo::buildStr();
	const BW::string & platformStr = BW::PlatformInfo::str();

	// Hybrid
	// /game/bin/server/BW::PlatformInfo::buildStr()/server
	preparedHybridSuffix_.push_back( 
		newDirectoryPrefix + platformBuildStr + serverSuffix );

	// /game/bin/server/BW::PlatformInfo::str()/server
	preparedHybridSuffix_.push_back(
		newDirectoryPrefix + platformStr + serverSuffix );

	// /game/bin/server/Hybrid64
	BW::string newDirOldHybrid( newDirectoryPrefix +
		BW_CONFIG_HYBRID_OLD + arch64Suffix );

	preparedHybridSuffix_.push_back( newDirOldHybrid );
	preparedOldHybridSuffix_.push_back( newDirOldHybrid );

	// /game/bin/server/hybrid64
	BW::string newDirOldLowcaseHybrid( newDirectoryPrefix +
		bwConfigAsLowerHybrid + arch64Suffix );
	preparedHybridSuffix_.push_back( newDirOldLowcaseHybrid );
	preparedOldHybridSuffix_.push_back( newDirOldLowcaseHybrid );

	// Now repeat the above for the old directory layout

	// /bigworld/bin/BW::PlatformInfo::buildStr()/server
	preparedHybridSuffix_.push_back(
		oldDirectoryPrefix + platformBuildStr + serverSuffix );

	// /bigworld/bin/BW::PlatformInfo::str()/server
	preparedHybridSuffix_.push_back(
		oldDirectoryPrefix + platformStr + serverSuffix );

	// /bigworld/bin/Hybrid64
	BW::string oldDirOldHybrid( oldDirectoryPrefix +
		BW_CONFIG_HYBRID_OLD + arch64Suffix );

	preparedHybridSuffix_.push_back( oldDirOldHybrid );
	preparedOldHybridSuffix_.push_back( oldDirOldHybrid );

	// /bigworld/bin/hybrid64
	BW::string oldDirOldLowcaseHybrid( oldDirectoryPrefix +
		bwConfigAsLowerHybrid + arch64Suffix );
	preparedHybridSuffix_.push_back( oldDirOldLowcaseHybrid );
	preparedOldHybridSuffix_.push_back( oldDirOldLowcaseHybrid );

	/*
	 * Debug
	 */
	
	// /game/bin/server/BW::PlatformInfo::buildStr()_debug/server
	preparedDebugSuffix_.push_back(
		newDirectoryPrefix + platformBuildStr + BW::string( "_debug" ) +
		serverSuffix );

	// /game/bin/server/BW::PlatformInfo::str()_debug/server
	preparedDebugSuffix_.push_back(
		newDirectoryPrefix + platformStr + BW::string( "_debug" ) +
		serverSuffix );

	// /game/bin/server/Debug64
	BW::string newDirOldDebug( newDirectoryPrefix +
		BW_CONFIG_DEBUG_OLD + arch64Suffix );
	preparedDebugSuffix_.push_back( newDirOldDebug );
	preparedOldDebugSuffix_.push_back( newDirOldDebug );

	// /game/bin/server/debug64
	BW::string newDirOldLowcaseDebug( newDirectoryPrefix +
		bwConfigAsLowerDebug + arch64Suffix );
	preparedDebugSuffix_.push_back( newDirOldLowcaseDebug );
	preparedOldDebugSuffix_.push_back( newDirOldLowcaseDebug );

	// Now repeat the above for the old directory layout

	// /bigworld/bin/BW::PlatformInfo::buildStr()_debug/server
	preparedDebugSuffix_.push_back(
		oldDirectoryPrefix + platformBuildStr + BW::string( "_debug" ) +
		serverSuffix );

	// /bigworld/bin/BW::PlatformInfo::str()_debug/server
	preparedDebugSuffix_.push_back(
		oldDirectoryPrefix + platformStr + BW::string( "_debug" ) +
		serverSuffix );

	// /bigworld/bin/Debug64
	BW::string oldDirOldDebug( oldDirectoryPrefix +
		BW_CONFIG_DEBUG_OLD + arch64Suffix );
	preparedDebugSuffix_.push_back( oldDirOldDebug );
	preparedOldDebugSuffix_.push_back( oldDirOldDebug );

	// /bigworld/bin/debug64
	BW::string oldDirOldLowcaseDebug( oldDirectoryPrefix +
		bwConfigAsLowerDebug + arch64Suffix );
	preparedDebugSuffix_.push_back( oldDirOldLowcaseDebug );
	preparedOldDebugSuffix_.push_back( oldDirOldLowcaseDebug );
}


/**
 * Determine whether we are running on a 32bit or 64bit machine.
 */
bool ServerPlatformLinux::initArchitecture()
{
	struct utsname sysinfo;

	if (uname( &sysinfo ) == -1)
	{
		syslog( LOG_ERR, "Unable to discover system architecture: %s. "
				"Continuing with 64 bit architecture support. ",
			strerror( errno ) );
		return true;
	}

	if (strcmp( sysinfo.machine, "x86_64") != 0)
	{
		syslog( LOG_CRIT, "Host architecture does not appear to be 64 bit."
			" Only 64 bit Linux platforms are supported." );
		return false;
	}

	return true;
}


/**
 *	This method checks the local host's kernel version.
 *
 *	The kernel version can determine the structure of key data read from the
 *	/proc filesystem which allows bwmachined to provide more detailed
 *	system information.
 */
void ServerPlatformLinux::initKernelVersion()
{
	// Determine which kernel version is running. This is important
	// as before 2.6 /proc/stat didn't have as much information.

	hasExtendedStats_ = false;

	FILE *fp;
	const char *kernel_version_file = "/proc/sys/kernel/osrelease";
	if ((fp = fopen( kernel_version_file, "r" )) == NULL)
	{
		syslog( LOG_ERR, "Couldn't read %s: %s", kernel_version_file,
				strerror( errno ) );
		hasExtendedStats_ = false;
	}

	char line[ 512 ];
	uint major, minor;

	fgets( line, sizeof( line ), fp );
	if (sscanf( line, "%d.%d", &major, &minor) != 2)
	{
		syslog( LOG_ERR, "Invalid line in %s: '%s'",
				kernel_version_file, line );
	}
	else
	{
		if ((major > 2) || (minor > 4))
		{
			hasExtendedStats_ = true;
		}

		if (hasExtendedStats_)
		{
			syslog( LOG_INFO, "Kernel version %d.%d detected: "
				"Using extended stats from /proc/stats\n", major, minor );
		}
	}

	fclose( fp );
}

/**
 *
 */
bool ServerPlatformLinux::configDirectoryExists( const BW::string & bwRoot,
	const StringList & pregeneratedConfigPaths, BW::string & binaryDir )
{
	BW::string potentialPath;

	StringList::const_iterator iter;

	iter = pregeneratedConfigPaths.begin();
	while (iter != pregeneratedConfigPaths.end())
	{
		potentialPath = bwRoot + (*iter);

		if (directoryExists( potentialPath ))
		{
			binaryDir = potentialPath;
			return true;
		}

		++iter;
	}

	return false;
}


/*
 *	Override from ServerPlatform.
 *
 *	Find whether a directory containing server binaries exists.
 *
 *	Given the BW_ROOT (base directory of a BigWorld server),
 *	BW_CONFIG (hybrid/debug), system architecture (32/64),
 *	test combinations of layouts based on BigWorld's directory
 *	restructuring to try and find an appropriate directory that
 *	contains server binaries.
 *
 *	The history of binary locations has been (from newest to oldest)
 *	bigworld/bin/<platform>_[config]/server
 *	 - <platform> = el (for centos, rhel), fedora, unknown
 *	 - [config] = hybrid, debug
 *	bigworld/bin/<platform>_[config]/server
 *	 - <platform> = centos, rhel, fedora, unknown
 *	 - [config] = hybrid, debug
 *	bigworld/bin/[config][arch]
 *	 - [config] = Hybrid, Debug
 *	 - [arch] = 64, (empty for 32)
 *
 *	@returns true if a valid directory was discovered, false otherwise.
 */
bool ServerPlatformLinux::findUserBinaryDirForConfig( const BW::string & bwRoot,
		const BW::string & bwConfig, BW::string & binaryDir ) /* override */
{
	/*
	 * This section is all that will be executed everytime the function
	 * is invoked.
	 */
	// In Hybrid mode, populate the options
	if (bwConfig == BW_CONFIG_HYBRID)
	{
		return this->configDirectoryExists( bwRoot,
			preparedHybridSuffix_, binaryDir );
	}
	else if (bwConfig == BW_CONFIG_DEBUG)
	{
		return this->configDirectoryExists( bwRoot,
			preparedDebugSuffix_, binaryDir );
	}
	else if (bwConfig == BW_CONFIG_HYBRID_OLD)
	{
		return this->configDirectoryExists( bwRoot,
			preparedOldHybridSuffix_, binaryDir );
	}
	else if (bwConfig == BW_CONFIG_DEBUG_OLD)
	{
		return this->configDirectoryExists( bwRoot,
			preparedOldDebugSuffix_, binaryDir );
	}

	return false;
}


/*
 *	Override from ServerPlatform.
 */
bool ServerPlatformLinux::isProcessRunning( uint16 pid ) const /* override */
{
	char buf[32];
	struct stat statinfo;
	int ret;

	snprintf( buf, sizeof( buf ) - 1, "/proc/%d", pid );

	if ((ret = stat( buf, &statinfo )) == -1 && errno != ENOENT)
	{
		syslog( LOG_ERR, "isProcessRunning(): stat(%s) failed (%s)",
			buf, strerror( errno ) );
	}

	return (ret == 0);
}


/*
 *	Override from ServerPlatform.
 */
bool ServerPlatformLinux::updateSystemInfo( SystemInfo & systemInfo,
		ServerInfo * pServerInfo ) /* override */
{
	char line[ 512 ];
	FILE * fp;

	// CPU updates
	uint32	cpu;
	// These sizes are determined in Linux source code (fs/proc/stat.c)
	uint64	jiffyUser, jiffyNice, jiffySyst, jiffyIdle;
	uint64	jiffyIOwait, jiffyIRQ, jiffySoftIRQ;

	if ((fp = fopen( "/proc/stat", "r" )) == NULL)
	{
		syslog( LOG_ERR, "Couldn't read /proc/stat: %s", strerror( errno ) );
		return false;
	}

	// Line CPU load summary line
	fgets( line, sizeof( line ), fp );

	// Read each CPU load individually
	uint64 totalWork, totalIdle;
	uint64 systemIOwait = 0;
	uint64 systemTotalWork = 0;
	for (uint i=0; i < systemInfo.nCpus; i++)
	{
		fgets( line, sizeof( line ), fp );

		// If we can read in extended stats (as of kernel 2.6) do it
		if (hasExtendedStats_)
		{
			if (sscanf( line, "cpu%u %" PRIu64 " %" PRIu64 " %" PRIu64
							" %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "",
					&cpu, &jiffyUser, &jiffyNice, &jiffySyst, &jiffyIdle,
					&jiffyIOwait, &jiffyIRQ, &jiffySoftIRQ) != 8)
			{
				syslog( LOG_ERR, "Invalid line in /proc/stat: '%s'", line );
				break;
			}
		}
		else
		{
			// For kernel 2.4 support old style proc information
			if (sscanf( line, "cpu%u %" PRIu64 " %" PRIu64 " %" PRIu64
					" %" PRIu64,
				&cpu, &jiffyUser, &jiffyNice, &jiffySyst, &jiffyIdle) != 5)
			{
				syslog( LOG_ERR, "Invalid line in /proc/stat: '%s'", line );
				break;
			}
		}

		if (cpu != i)
		{
			syslog( LOG_CRIT, "Line %d of /proc/stat was cpu%u, not cpu%d",
				i, cpu, i );
		}

		// val = total of all the time spent performing work
		// max = total work time + total idle time
		totalWork = jiffyUser + jiffyNice + jiffySyst;
		totalIdle = jiffyIdle;

		if (hasExtendedStats_)
		{
			totalIdle += jiffyIOwait + jiffyIRQ + jiffySoftIRQ;
		}

		systemInfo.cpu[i].val.update( totalWork );
		systemInfo.cpu[i].max.update( totalWork + totalIdle );

		systemIOwait += jiffyIOwait;
		systemTotalWork += totalWork + totalIdle;
	}
	systemInfo.iowait.val.update( systemIOwait );
	systemInfo.iowait.max.update( systemTotalWork );

	fclose( fp );

	// Memory updates
	pServerInfo->updateMem();
	systemInfo.mem.max.update( pServerInfo->memTotal() );
	systemInfo.mem.val.update( pServerInfo->memUsed() );

	// IP-level packet statistics
	if ((fp = fopen( "/proc/net/snmp", "r" )) == NULL)
	{
		syslog( LOG_ERR, "Couldn't read /proc/net/snmp: %s",
			strerror( errno ) );
		return false;
	}

	// skip the IP header line
	// format string sizes are determined in Linux source (net/ipv4/proc.c)
	fgets( line, sizeof( line ), fp );
	if (fscanf( fp,"%*s %*d %*d %*d %*d %*d %*d %*d "
					"%" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
		&systemInfo.packDropIn.next(), &systemInfo.packTotIn.next(),
		&systemInfo.packTotOut.next(), &systemInfo.packDropOut.next() ) != 4)
	{
		syslog( LOG_ERR, "Failed to read packet loss information from "
				"/proc/net/snmp" );
	}
	fclose( fp );

	// Interface level packet and bit counts
	if ((fp = fopen( "/proc/net/dev", "r" )) == NULL)
	{
		syslog( LOG_ERR, "Couldn't open /proc/net/dev: %s", strerror( errno ) );
		return false;
	}

	// Skip header lines
	fgets( line, sizeof( line ), fp );
	fgets( line, sizeof( line ), fp );

	for (unsigned int i=0; fgets( line, sizeof( line ), fp ) != NULL; )
	{
		// If we've already got a struct for this interface, re-use it,
		// otherwise make a new one
		if (i >= systemInfo.ifInfo.size())
			systemInfo.ifInfo.push_back( InterfaceInfo() );
		struct InterfaceInfo &ifInfo = systemInfo.ifInfo[i];
		char ifname[32];

		// Drop info about the loopback interface
		sscanf( line, " %[^:]", ifname );
		if (strstr( ifname, "lo" ) == NULL)
		{
			ifInfo.name = ifname;
			// format string sizes are determined in Linux source
			// (net/core/dev.c)
			sscanf( line,
				" %*[^:]:%" PRIu64 " %" PRIu64 " %*d %*d %*d %*d %*d %*d "
				"%" PRIu64 " %" PRIu64,
				&ifInfo.bitsTotIn.next(),
				&ifInfo.packTotIn.next(),
				&ifInfo.bitsTotOut.next(),
				&ifInfo.packTotOut.next() );

			// Turn byte counts into bit counts
			ifInfo.bitsTotIn.cur() *= 8;
			ifInfo.bitsTotOut.cur() *= 8;
			i++;
		}
	}

	fclose( fp );

	return true;
}


namespace // (anonymous)
{

/**
 *	This class is used to visit binary paths to look for core dumps.
 */
class CoreDumpBinaryPathVisitor : public BinaryPathVisitor
{
public:
	/**
	 *	Constructor
	 *
	 *	@param coreDumps 	This will be filled by core dump data.
	 */
	CoreDumpBinaryPathVisitor( UserMessage::CoreDumps & coreDumps ) :
		coreDumps_( coreDumps )
	{}

	// Override from BinaryPathVisitor.
	void onBinaryPath( const BW::string & path ) /* override */;

private:
	UserMessage::CoreDumps & coreDumps_;
};


} // end namespace (anonymous)


/*
 *	Override from ServerPlatform.
 */
bool ServerPlatformLinux::checkCoreDumps( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		UserMessage::CoreDumps & coreDumps ) /* override */
{
	if (bwRoot.empty())
	{
		syslog( LOG_ERR, "Unable to check core dumps for uid:%d, "
				"bwRoot is empty\n",
			uid );
		return false;
	}

	// The provided core dumps should be empty, but ensure it is before
	// filling it with our contents.
	coreDumps.clear();

	CoreDumpBinaryPathVisitor visitor( coreDumps );

	return this->visitBinaryPaths( uid, bwRoot, visitor, "checking cores" );
}


/*
 *	This method is overridden from BinaryPathVisitor.
 */
void CoreDumpBinaryPathVisitor::onBinaryPath( 
		const BW::string & binaryPath ) /* override */
{
	Glob coreGlobMatches;

	// It is nescessary to glob both * and */server due to the directory
	// layouts changing over the years. The main targets to hit are
	// game/bin/server/*/server       - centos5/el7
	// bigworld/bin/server/*/server   - centos5/el7
	// game/bin/*                     - Hybrid64/hybrid64
	// bigworld/bin/*                 - Hybrid64/hybrid64
	const bool foundMatches =
		(0 == coreGlobMatches.match( "*/server/core.*" )) ||
		(0 == coreGlobMatches.match( "*/core.*", GLOB_APPEND ));

	if (!foundMatches)
	{
		return;
	}

	// Limiting the reporting of core dumps to only 10 to avoid
	// exceeding the MGMPacket::MAX_SIZE.
	static const size_t MAX_CORES = 10;

	char coreFilePathCopy[ PATH_MAX ];

	size_t numReplied = 0;
	
	while ((numReplied < coreGlobMatches.size()) && 
			(coreDumps_.size() < MAX_CORES))
	{
		const char * coreFilePath = coreGlobMatches[ numReplied ];
		UserMessage::CoreDump cd;
		cd.filename_ = coreFilePath;

		// Extract subdirectory that the coredump is in
		strcpy( coreFilePathCopy, coreFilePath );

		// Use info from assertion if it's there.  Magic 6 is length of the
		// string '/core.'.
		const char * corePrefix = "/core.";

		char assertFilePath[ PATH_MAX ];

		bw_snprintf( assertFilePath, sizeof( assertFilePath ),
			"%s/assert.%s.log", dirname( coreFilePathCopy ),
			strstr( coreFilePath, corePrefix ) + strlen( corePrefix ) );

		FILE * fp = fopen( assertFilePath, "r" );

		if (fp != NULL)
		{
			char c;

			while ((c = fgetc( fp )) != EOF)
			{
				cd.assert_.push_back( c );
			}

			fclose( fp );
		}
		/*
		 * We're attempting to open an assertion file even though it may
		 * not exist. The alternative would be to stat the file first and
		 * incur the extra syscall.
		else
		{
			syslog( LOG_ERR, "Unable to open assertion file %s: %s",
				assertFilePath, strerror( errno ) );
		}
		*/

		// Get timestamp for coredump
		struct stat statinfo;

		if (stat( coreFilePath, &statinfo ) == 0)
		{
			cd.time_ = statinfo.st_ctime;
		}
		else
		{
			syslog( LOG_ERR, "Couldn't stat() %s: %s",
				coreFilePath, strerror( errno ) );
		}

		coreDumps_.push_back( cd );
		++numReplied;
	}

	int syslogStatusPriority = LOG_INFO;

	if (numReplied < coreGlobMatches.size())
	{
		syslogStatusPriority = LOG_ERR;
	}

	syslog( syslogStatusPriority,
			"Found %" PRIzu " coredumps in %s, %" PRIzu " reported.",
			coreGlobMatches.size(), binaryPath.c_str(), numReplied );
}


/**
 *	This method visits all possible existing binary paths.
 *
 *	@param uid 				The UID we are searching under (used for log
 *							output).
 *	@param bwRoot 			The BW_ROOT to visit binary paths for.
 *	@param visitor			The visitor object.
 *	@param purposeString 	The purpose for visiting. Used for log output.
 *
 *	@return true on success, false otherwise.
 */
bool ServerPlatformLinux::visitBinaryPaths(
		MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		BinaryPathVisitor & visitor,
		const char * purposeString /* = NULL */ )
{
	if (!purposeString)
	{
		purposeString = "visiting binary paths";
	}

	// Save the current working directory as we chdir to reduce the path
	// size.
	char cwd[ PATH_MAX ];

	if (getcwd( cwd, sizeof( cwd ) ) == NULL)
	{
		syslog( LOG_ERR, "Failed to get CWD while %s for uid:%d: %s",
			purposeString,
			uid, strerror( errno ) );
		return false;
	}

	StringList potentialBinaryPaths;

	potentialBinaryPaths.push_back( bwRoot + "/game/bin/server" );
	potentialBinaryPaths.push_back( bwRoot + "/bigworld/bin" );

	// This vector contains pairs of <potential bin path, errno>
	typedef BW::vector< std::pair< BW::string, int > > PathErrorList;
	PathErrorList pathErrors;

	StringList::const_iterator iter = potentialBinaryPaths.begin();

	while (iter != potentialBinaryPaths.end())
	{
		const BW::string & potentialPath = *iter;
		if (chdir( potentialPath.c_str() ) == -1)
		{
			pathErrors.push_back( std::make_pair( potentialPath, errno ) );
		}
		else
		{
			visitor.onBinaryPath( potentialPath );
		}

		++iter;
	}

	// Only log errors if all paths failed.
	if (potentialBinaryPaths.size() == pathErrors.size())
	{
		syslog( LOG_ERR,
			"Failed to chdir to bin paths to %s for uid:%d. "
				"Paths attempted:",
			purposeString, uid );

		PathErrorList::const_iterator badPathIter = pathErrors.begin();

		while (badPathIter != pathErrors.end())
		{
			syslog( LOG_ERR, "%s (%s);",
				badPathIter->first.c_str(), strerror( badPathIter->second ) );

			++badPathIter;
		}
	}

	// Now return to the original directory
	chdir( cwd );

	return true;
}


namespace // (anonymous)
{


/**
 *	This class is a visitor that tries to find a set of process binaries in any
 *	of the binary paths visited.
 */
class ProcessBinarySetMatcher : public BinaryPathVisitor
{
public:

	/**
	 *	Cosntructor.
	 *
	 *	@param processSet 	The process binary name set to check.
	 */
	ProcessBinarySetMatcher( const ServerPlatform::ProcessSet & processSet ) :
			BinaryPathVisitor(),
			processSet_( processSet ),
			hasMatch_( false )
	{}

	// Override from BinaryPathVisitor.
	void onBinaryPath( const BW::string & path ) /* override */;

	/**
	 *	This method returns the version string in the first version file found.
	 */
	bool hasMatch() const { return hasMatch_; }

private:
	const ServerPlatform::ProcessSet & processSet_;
	bool hasMatch_;
};


/*
 *	Override from BinaryPathVisitor.
 */
void ProcessBinarySetMatcher::onBinaryPath(
		const BW::string & path ) /* override */
{
	if (hasMatch_)
	{
		// We already found our process set in an earlier binary path. No need
		// to check this time.
		return;
	}

	static const char * GLOB_PREFIXES[] = {
		"/*/server/",
		"/*/"
	};

	for (size_t i = 0; i < ARRAY_SIZE( GLOB_PREFIXES ); ++i)
	{
		Glob processSetGlob;

		bool haveMissingProcess = false;

		ServerPlatform::ProcessSet::const_iterator iter =
			processSet_.begin();
		while (iter != processSet_.end())
		{
			if (0 != processSetGlob.match( 
					(path + BW::string( GLOB_PREFIXES[i] ) + *iter).c_str() ))
			{
				// No match or some other error.
				// Give up on this prefix.
				haveMissingProcess = true;
				break;
			}

			++iter;
		}

		if (!haveMissingProcess)
		{
			// We have a complete set, we can finish now without checking
			// the other binary paths.

			hasMatch_ = true;
		}
	}
}


} // end namespace (anonymous)


/*
 *	Override from ServerPlatform.
 */
bool ServerPlatformLinux::checkBinariesExist(
		MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		const ProcessSet & processSet ) /* override */
{
	ProcessBinarySetMatcher visitor( processSet );

	if (!this->visitBinaryPaths( uid, bwRoot, visitor, 
			"checking binary existence" ))
	{
		// Failed to visit binary paths - no match then.
		return false;
	}

	return visitor.hasMatch();
}


BW_END_NAMESPACE

// server_platform_linux.cpp

