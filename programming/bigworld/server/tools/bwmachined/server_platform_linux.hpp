#ifndef SERVER_PLATFORM_LINUX_HPP
#define SERVER_PLATFORM_LINUX_HPP

#include "server_platform.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE


class BinaryPathVisitor;

/**
 *	This class implements platform operations for Linux.
 */
class ServerPlatformLinux : public ServerPlatform
{
public:
	ServerPlatformLinux();

	// Overrides from ServerPlatform

	bool findUserBinaryDirForConfig( const BW::string & bwRoot,
		const BW::string & bwConfig, BW::string & binaryDir ) /* override */;

	bool isProcessRunning( uint16 pid ) const /* override */;

	bool updateSystemInfo( SystemInfo & systemInfo,
		ServerInfo * pServerInfo ) /* override */;

	bool checkCoreDumps( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		UserMessage::CoreDumps & coreDumps ) /* override */;

protected:
	bool checkBinariesExist( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		const ProcessSet & set ) /* override */;

private:
	typedef BW::vector< BW::string > StringList;

	void initConfigSuffixes();
	void initKernelVersion();
	bool initArchitecture();

	bool configDirectoryExists( const BW::string & bwRoot,
		const StringList & pregeneratedConfigPaths, BW::string & binaryDir );

	bool visitBinaryPaths( MachineGuardMessage::UserId uid,
		const BW::string & bwRoot,
		BinaryPathVisitor & visitor,
		const char * purposeString = NULL );

	StringList preparedHybridSuffix_;
	StringList preparedDebugSuffix_;
	StringList preparedOldHybridSuffix_;
	StringList preparedOldDebugSuffix_;

	// The word size of the host architecture (32 / 64)
	uint8 hostArchitecture_;
	BW::string architecture_;

	bool hasExtendedStats_;
};

BW_END_NAMESPACE

#endif // SERVER_PLATFORM_LINUX_HPP
