#include "reviver_config.hpp"

#define BW_CONFIG_CLASS ReviverConfig
#define BW_CONFIG_PREFIX "reviver/"
#include "server/server_app_option_macros.hpp"

#include "server/reviver_common.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General options
// -----------------------------------------------------------------------------

BW_OPTION_RO( float, reattachPeriod, 10.f );
BW_OPTION( float, pingPeriod, REVIVER_DEFAULT_PING_PERIOD );
BW_OPTION( float, subjectTimeout, REVIVER_DEFAULT_SUBJECT_TIMEOUT );
BW_OPTION( bool, shutDownOnRevive, true );
BW_OPTION( float, timeout, 3.0 );
BW_OPTION( int, timeoutInPings, 0 );


/**
 *
 */
bool ReviverConfig::postInit()
{
	bool result = ServerAppConfig::postInit();

	if (result)
	{
		if (ReviverConfig::timeoutInPings() == 0)
		{
			timeoutInPings.set( int( timeout() / pingPeriod() + 0.5f ) );

			if (timeoutInPings() < 1)
			{
				ERROR_MSG( "ReviverConfig::postInit: reviver/timeout is too "
							"small. timeout = %.2f. pingPeriod = %.2f\n",
						timeout(), pingPeriod() );
				result = false;
			}
		}
		else
		{
			INFO_MSG( "ReviverConfig::postInit: "
				"The reviver/timeoutInPings option is deprecated. Use "
				"reviver/timeout instead.\n" );
		}

		if (pingPeriod() > subjectTimeout())
		{
			CRITICAL_MSG( "ReviverConfig::postInit: "
				"The revier/subjectTimeout must be larger than "
				"reviver/pingPeriod." );
		}
	}

	return result;
}

BW_END_NAMESPACE

// reviver_config.cpp
