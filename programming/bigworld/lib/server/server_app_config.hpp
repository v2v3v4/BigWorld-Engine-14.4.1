#ifndef SERVER_APP_CONFIG_HPP
#define SERVER_APP_CONFIG_HPP

#include "server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class contains the configuration options for a ServerApp.
 */
class ServerAppConfig
{
public:
	static ServerAppOption< int > updateHertz;

	static ServerAppOption< BW::string > personality;
	static ServerAppOption< BW::string > serverMode;
	static ServerAppOption< bool > isProduction;

	static ServerAppOption< float > timeSyncPeriod;
	static ServerAppOption< int > timeSyncPeriodInTicks;

	static ServerAppOption< bool > useDefaultSpace;

	static ServerAppOption< long > maxOpenFileDescriptors;

	static ServerAppOptionGetSet< float > channelTimeoutPeriod;

	static ServerAppOptionGetSet< bool > allowInteractiveDebugging;

	static ServerAppOption< uint > maxSharedDataValueSize;

	static ServerAppOption< float > maxMgrRegisterStagger;

	static ServerAppOption< int > numStartupRetries;

#if ENABLE_PROFILER
	static ServerAppOption< bool > hasHitchDetection;
	static ServerAppOption< float > hitchDetectionThreshold;

	static ServerAppOption< BW::string > profilerJsonDumpDirectory;

	static ServerAppOption< int > profilerMaxThreads;
#endif

	static bool postInit();
	static bool init( bool (*postInitFn)() );

	// Helper functions
	static int secondsToTicks( float seconds, int lowerBound );
	static float expectedTickPeriod() { return 1.f/updateHertz(); }

private:
	ServerAppConfig();
};

BW_END_NAMESPACE

#endif // SERVER_APP_CONFIG_HPP
