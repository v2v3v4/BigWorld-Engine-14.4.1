#ifndef CELL_APP_LOAD_CONFIG_HPP
#define CELL_APP_LOAD_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class stores the configuration options for CellAppMgr.
 */
class CellAppLoadConfig
{
public:
	static ServerAppOption< float > lowerBound;
	static ServerAppOption< float > safetyBound;
	static ServerAppOption< float > safetyRatio;
	static ServerAppOption< float > warningLevel;

	static bool postInit();
};

BW_END_NAMESPACE

#endif // CELL_APP_LOAD_CONFIG_HPP
// cell_app_load_config.hpp
