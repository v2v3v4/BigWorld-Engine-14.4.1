#ifndef SERVER_PLATFORM_HPP
#define SERVER_PLATFORM_HPP

#include "process_binary_version.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_set.hpp"

#include "network/machine_guard.hpp"


BW_BEGIN_NAMESPACE


class ServerInfo;
class SystemInfo;


/**
 *	This abstract class provides the basis for any host platform implementation.
 *
 *	Platforms (eg: Linux, Windows) will specialise this class to provide
 *	the expected functionality required by bwmachined.
 */
class ServerPlatform
{
public:
	typedef BW::set< BW::string > ProcessSet;

	ServerPlatform();

	bool isInitialised() const;


	/**
	 *	This method tests whether the requested binary directory exists and is
	 *	suitable for starting server processes from.
	 *
	 *	@param bwRoot The base path to the BigWorld directory structure.
	 *	@param bwConfig The type of binaries to discover (Hybrid / Debug)
	 *	@param binaryDir The valid discovered binary directory to start
	 *	                 processes from.
	 *
	 *	@return true on discovering a valid directory, false otherwise.
	 */
	virtual bool findUserBinaryDirForConfig( const BW::string & bwRoot,
		const BW::string & bwConfig, BW::string & binaryDir ) = 0;

	/**
	 *	This method checks whether a given PID is running.
	 *
	 * 	@param pid The process ID to check the status of.
	 *
	 *	@return true if the process is running, false otherwise.
	 */
	virtual bool isProcessRunning( uint16 pid ) const = 0;

	/**
	 *	This method performs an explicit update of core system information
	 *	such as CPU and memory state.
	 *
	 *	@param systemInfo The structure to place the updated information into.
	 *	@param pServerInfo A pointer to the ServerInfo structure to use when
	 *	                   performing the update.
	 *
	 *	return true when the update was successful, false otherwise.
	 */
	virtual bool updateSystemInfo( SystemInfo & systemInfo,
		ServerInfo * pServerInfo ) = 0;

	/**
	 *	This method fills a list of CoreDumps with core file summaries for any
	 *	core files and assertions found under bwRoot.
	 *
	 *	@param uid The UID of the user the core dump check relates to.
	 *	@param bwRoot The root directory to search for the core files in.
	 *	@param coreDumps The UserMessage::CoreDumps structure to fill with
	 *                   core file summary information.
	 */
	virtual bool checkCoreDumps( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot, UserMessage::CoreDumps & coreDumps ) = 0;


	bool determineVersion( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		BW::string & versionString );

protected:

	void initVersionsProcesses();
	
	
	/**
	 *	This method checks if the binaries in the given set exist under
	 *	the given BW_ROOT path.
	 *
	 *	@param uid		The user ID we are checking for (used in log output).
	 *	@param bwRoot 	The BW_ROOT to check for binaries under.
	 *	@return 		true if there is a binary for each process name in the
	 *					given set.
	 */
	virtual bool checkBinariesExist( MachineGuardMessage::UserId uid, 
		const BW::string & bwRoot,
		const ProcessSet & processes ) = 0; 

	bool isInitialised_;

	/// The versions' processes. That is, the set of processes for each named
	/// version.
	typedef BW::map< ProcessBinaryVersion, ProcessSet > VersionsProcesses;
	VersionsProcesses versionsProcesses_;
};

BW_END_NAMESPACE

#endif // SERVER_PLATFORM_HPP
