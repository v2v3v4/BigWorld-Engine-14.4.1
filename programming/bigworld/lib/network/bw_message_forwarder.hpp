#ifndef BW_MESSAGE_FORWARDER_HPP
#define BW_MESSAGE_FORWARDER_HPP

#include "cstdmf/config.hpp"

#if defined( MF_SERVER ) && (ENABLE_WATCHERS != 0)

#include "event_dispatcher.hpp"
#include "logger_message_forwarder.hpp"
#include "network_interface.hpp"
#include "watcher_nub.hpp"

#include <memory>

BW_BEGIN_NAMESPACE

class BWMessageForwarder
{
public:
	BWMessageForwarder( const char * componentName,
			const char * configPath, bool isForwarding,
			Mercury::EventDispatcher & dispatcher,
			Mercury::NetworkInterface & networkInterface );

	WatcherNub & watcherNub() 	{ return watcherNub_; }

private:
	WatcherNub watcherNub_;
	std::auto_ptr< LoggerMessageForwarder > pForwarder_;
};


#define BW_MESSAGE_FORWARDER3( NAME, CONFIG_PATH, ENABLED,					\
												DISPATCHER, INTERFACE )		\
	BWMessageForwarder BW_forwarder__( NAME, CONFIG_PATH,					\
			ENABLED, DISPATCHER, INTERFACE );								\
																			\
	if (!BW_forwarder__.watcherNub().isOkay())								\
	{																		\
		ERROR_MSG( "Init watcher fail: %s\n",								\
			BW_forwarder__.watcherNub().errorMsg().c_str() );				\
		INTERFACE.prepareForShutdown();										\
		return 0;															\
	}																		\
																			\
	if (BWConfig::isBad())													\
	{																		\
		return 0;															\
	}																		\
	(void)0


/*
 *	This method is used to add the ability to forward messages to loggers.
 */
#define BW_MESSAGE_FORWARDER2( NAME, CONFIG_PATH, ENABLED,					\
												DISPATCHER, INTERFACE )		\
	BW_MESSAGE_FORWARDER3( #NAME, #CONFIG_PATH,								\
			ENABLED, DISPATCHER, INTERFACE )

#define BW_MESSAGE_FORWARDER( NAME, CONFIG_PATH, DISPATCHER, INTERFACE )	\
	BW_MESSAGE_FORWARDER2( NAME, CONFIG_PATH, true, DISPATCHER, INTERFACE )

BW_END_NAMESPACE

#endif // MF_SERVER && ENABLE_WATCHERS


#endif // BW_MESSAGE_FORWARDER_HPP
