#include "manager_app_config.hpp"

// These need to be defined before including server_app_option_macros.hpp
#define BW_CONFIG_CLASS ManagerAppConfig
#define BW_CONFIG_PREFIX ""
#include "server_app_option_macros.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ManagerAppConfig
// -----------------------------------------------------------------------------

BW_OPTION( bool, shutDownServerOnBadState, true );
BW_OPTION( bool, shutDownServerOnBaseAppDeath, false );
BW_OPTION( bool, shutDownServerOnCellAppDeath, false );
BW_OPTION( bool, shutDownServerOnServiceAppDeath, false );
BW_OPTION( bool, shutDownServerOnServiceDeath, false );
BW_OPTION( bool, isBadStateWithNoServiceApps, false );

BW_END_NAMESPACE

//  manager_app_config.cpp
