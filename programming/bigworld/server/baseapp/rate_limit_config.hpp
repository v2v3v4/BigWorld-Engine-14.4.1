#ifndef RATE_LIMIT_CONFIG_HPP
#define RATE_LIMIT_CONFIG_HPP

#include "server/server_app_option.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This contains configuration options for RateLimitMessageFilter.
 */
class RateLimitConfig
{
public:
	static ServerAppOption< uint > warnMessagesPerSecond;
	static ServerAppOption< uint > maxMessagesPerSecond;
	static ServerAppOption< uint > warnBytesPerSecond;
	static ServerAppOption< uint > maxBytesPerSecond;

	static ServerAppOption< uint > warnMessagesPerTick;
	static ServerAppOption< uint > maxMessagesPerTick;
	static ServerAppOption< uint > warnBytesPerTick;
	static ServerAppOption< uint > maxBytesPerTick;

	static ServerAppOption< uint > warnMessagesBuffered;
	static ServerAppOption< uint > maxMessagesBuffered;
	static ServerAppOption< uint > warnBytesBuffered;
	static ServerAppOption< uint > maxBytesBuffered;

	static bool postInit();

private:
	RateLimitConfig();
};

BW_END_NAMESPACE

#endif // RATE_LIMIT_CONFIG_HPP
