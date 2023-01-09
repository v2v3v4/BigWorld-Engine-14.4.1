#include "cell_app_load_config.hpp"

#define BW_CONFIG_CLASS CellAppLoadConfig
#define BW_CONFIG_PREFIX "cellAppMgr/"
#define BW_COMMON_PREFIX "cellAppLoad/"
#include "server/server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: General options
// -----------------------------------------------------------------------------

BW_OPTION( float, lowerBound, 0.02f );
BW_OPTION( float, safetyBound, 0.85f );
BW_OPTION( float, safetyRatio, 1.1f );
BW_OPTION( float, warningLevel, 0.75f );

bool CellAppLoadConfig::postInit()
{
#define CONFIG_CHECK( LOWER, UPPER )										\
	if (LOWER() > UPPER())													\
	{																		\
		WARNING_MSG( "CellAppLoadConfig: cellAppMgr/cellAppLoad/" #LOWER	\
			"should be less than cellAppMgr/cellAppLoad/" #UPPER "\n" );					\
	}
	CONFIG_CHECK( lowerBound, safetyBound )
	CONFIG_CHECK( lowerBound, warningLevel )

	return true;
}

BW_END_NAMESPACE

// cell_app_load_config.cpp
