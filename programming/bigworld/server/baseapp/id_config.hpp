#ifndef ID_CONFIG_HPP
#define ID_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This contains configuration options for ids.
 */
class IDConfig
{
public:
	static ServerAppOption< int > criticallyLowSize;
	static ServerAppOption< int > lowSize;
	static ServerAppOption< int > desiredSize;
	static ServerAppOption< int > highSize;

private:
	IDConfig();
};

BW_END_NAMESPACE

#endif // ID_CONFIG_HPP
