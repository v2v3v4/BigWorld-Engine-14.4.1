#ifndef LOGGER_MESSAGE_FORWARDER_HPP
#define LOGGER_MESSAGE_FORWARDER_HPP

#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS

#if defined(__INTEL_COMPILER)
// force safe inheritance member pointer declaration for Intel compiler
#pragma pointers_to_members( pointer-declaration, full_generality )
#endif

#include "network/endpoint.hpp"
#include "network/forwarding_string_handler.hpp"
#include "network/interfaces.hpp"
#include "network/watcher_nub.hpp"

#include "cstdmf/debug_message_callbacks.hpp"
#include "cstdmf/debug_message_source.hpp"
#include "cstdmf/log_meta_data.hpp"
#include "cstdmf/singleton.hpp"

#include <memory>
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
	class NetworkInterface;
} // end namespace Mercury


/* MESSAGE_LOGGER_VERSION changes:
 * This version refers to the network protocol.
 *  3: Prior to this version, the action of deleting connections did not work
 *	   over TCP, so an additional UDP socket was required to allow the
 *     component to be deleted from MessageLogger.
 *
 *  7: l and z size modification flags were changed to always send 8 bytes as
 *     opposed to host specific sizeof( long ). This meant that it was
 *     possible log message from previous versions to be lost.
 *     The logger ID has now been changed from an uint8 to a string.
 *
 *	8: LoggerMessageHeader changed format with 2 new fields
 *	   - DebugMessageSource messageSource_
 *	   - BW::string category_
 *
 *	9: Log Metadata added as optional extra (char *) data to the end of the
 *	   stream. If there is any extra bytes at the end that have not yet been
 *	   destreamed they will be treated as log metadata.
 *
 *	10: Log Metadata has been modified to be a stream of key pair values
 *	    instead of a (char *).
 */

#define MESSAGE_LOGGER_VERSION_WITHOUT_EXTRA_UDP  3

#define MESSAGE_LOGGER_VERSION_WITHOUT_METADATA 8

#define MESSAGE_LOGGER_VERSION_STRING_METADATA 	9

#define MESSAGE_LOGGER_VERSION 	10



namespace MessageLogger
{

//namespace NetworkFormat
//{

typedef uint8 NetworkVersion;

typedef uint8 NetworkMessagePriority;

//} // namespace NetworkFormat

// Utility functions to determine the capability of the given version.
bool versionShouldOpenUDP( uint16 major, uint16 minor );
bool versionSupportsMetaData( uint16 major, uint16 minor );
bool versionSupportsMetaDataArgs( uint16 major, uint16 minor );

} // namespace MessageLogger

#define MESSAGE_LOGGER_NAME 	"message_logger"

/*
 *	This has to match the struct pack format for LoggerComponentMessage in
 *	bigworld/tools/server/pycommon/messages.py
 */
typedef BW::string LoggerID; 	// This has to match the struct pack format for
								// LoggerComponentMessage in
								// bigworld/tools/server/pycommon/messages.py

enum
{
	MESSAGE_LOGGER_MSG = WATCHER_MSG_EXTENSION_START, // 107
	MESSAGE_LOGGER_REGISTER,
	MESSAGE_LOGGER_PROCESS_BIRTH,
	MESSAGE_LOGGER_PROCESS_DEATH,
	MESSAGE_LOGGER_APP_ID
};

const int LOGGER_MSG_SIZE = 2048;

// TODO: pragma pack should be un-nesc here as we don't actually stream the
//       struct at all, only its contents
#pragma pack( push, 1 )
/**
 *  The header section that appears at the start of each message sent to
 *  MessageLogger.
 */
struct LoggerMessageHeader
{
	// TODO: move componentPriority_ into LoggerComponentMessage
	MessageLogger::NetworkMessagePriority	componentPriority_;
	MessageLogger::NetworkMessagePriority	messagePriority_;
	DebugMessageSource	messageSource_;
	BW::string	category_;

	void write( MemoryOStream & stream ) const;
	void read( MemoryIStream & stream );
};
#pragma pack( pop )

#pragma pack( push, 1 )
/**
 * The message that is sent to MessageLogger to register with it.
 */
class LoggerComponentMessage
{
public:
	virtual ~LoggerComponentMessage() {};

	MessageLogger::NetworkVersion	version_;
	LoggerID				loggerID_;
	uint16					uid_;
	uint32					pid_;
	BW::string				componentName_;

	void write( BinaryOStream &os ) const;
	void read( BinaryIStream &is );
};
#pragma pack( pop )

class LoggerEndpoint;

// This class is defined in the .cpp
class FindLoggerHandler;


/**
 *	This class is used to forward log messages to any attached loggers.
 */
class LoggerMessageForwarder :
	public DebugMessageCallback,
	public TimerHandler,
	public Singleton< LoggerMessageForwarder >
{
	friend class FindLoggerHandler;
	friend class LoggerEndpoint;

public:
	LoggerMessageForwarder(
		const BW::string & appName,
		Endpoint & endpoint,
		Mercury::EventDispatcher & dispatcher,
		LoggerID loggerID = BW::string(),
		bool isEnabled = true,
		bool shouldFilterSpam = true,
		bool shouldSummariseSpam = true,
		unsigned spamFilterThreshold = 10 );

	virtual ~LoggerMessageForwarder();

	void registerAppID( ServerAppInstanceID appID );

	void addSuppressionPattern( BW::string prefix );
	void delSuppressionPattern( BW::string prefix );

	virtual bool handleMessage(
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData & metaData,
		const char * pFormatString, va_list argPtr );

	virtual void handleTimeout( TimerHandle handle, void * arg );

private:
	void findLoggerInterfaces( Mercury::Address * addr = NULL );
	void init();

	Mercury::Address watcherHack() const;
	BW::string suppressionWatcherHack() const { return BW::string(); }

	void addLogger( const Mercury::Address & addr, uint16 majorVersion,
					uint16 minorVersion );
	void delLogger( const Mercury::Address & addr );

	void sendAppID( LoggerEndpoint * ep );

	void watcherAddLogger( Mercury::Address addr )
	{
		findLoggerInterfaces( &addr );
	}

	void watcherAddLoggerTCP( Mercury::Address addr );

	void watcherAddLoggerTCPWithMetaDataV2_9( Mercury::Address addr );

	void watcherAddLoggerTCPWithMetaDataV14_4( Mercury::Address addr );

	bool watcherGetEnabled() const
	{
		return isEnabled_;
	}

	void watcherSetEnabled( bool enabled );

	ForwardingStringHandler * findForwardingStringHandler(
		const char * formatString );

	void parseAndSend( ForwardingStringHandler * pHandler,
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData * pMetaData,
		va_list argPtr );

	void parseAndSend( ForwardingStringHandler * pHandler,
		DebugMessagePriority messagePriority, const char * pCategory,
		DebugMessageSource messageSource, const LogMetaData * pMetaData, ... );


	int size() const	{ return int( loggers_.size() ); }

	bool isSuppressible( const BW::string & format ) const;

	bool isSpamming( ForwardingStringHandler * pHandler ) const
	{
		return shouldFilterSpam_ && 
			pHandler->isSuppressible() &&
				(spamFilterThreshold_ == 0 ||
				(pHandler->numRecentCalls() > spamFilterThreshold_));
	}

	void updateSuppressionPatterns();


	ServerAppInstanceID appID_;

	BW::string appName_;

	/// This is the socket that is actually used for sending log messages.
	Endpoint & endpoint_;

	/// The dispatcher we register a timer with for managing spam suppression.
	Mercury::EventDispatcher & dispatcher_;

	// The ID used by the process when registering with MessageLoggers.
	// If this ID does not match a MessageLogger's filter, the process will
	// not log to that MessageLogger.
	LoggerID loggerID_;

	bool isEnabled_;
	bool hasBeenEnabled_;

	/// A list of the format string prefixes that we will suppress.
	typedef BW::vector< BW::string > SuppressionPatterns;
	SuppressionPatterns suppressionPatterns_;
	
	// True if log spam filtering is enabled
	bool shouldFilterSpam_;

	/// True if log spam summaries are dispalyed
	bool shouldSummariseSpam_;
	
	/// The timer handle for managing spam suppression.
	TimerHandle spamTimerHandle_;

	/// The maximum number of times a particular format string can be emitted
	/// each second.
	unsigned spamFilterThreshold_;

	/// The forwarding string handler that is used for sending spam summaries.
	ForwardingStringHandler spamHandler_;

	/// The collection of format string handlers that we have already seen.
	ReadWriteLock handlerCacheLock_;
	typedef BW::map< BW::string, ForwardingStringHandler * > HandlerCache;
	HandlerCache handlerCache_;

	/// A collection of all the handlers that have been used since the last time
	/// handleTimeout() was called.
	SimpleMutex recentlyUsedCacheHandlersMutex_;
	typedef BW::vector< ForwardingStringHandler* > RecentlyUsedHandlers;
	RecentlyUsedHandlers recentlyUsedHandlers_;

	ReadWriteLock loggersReadWriteLock_;
	typedef BW::vector< LoggerEndpoint * > Loggers;
	Loggers loggers_;

};


#if defined( __INTEL_COMPILER )
#pragma pointers_to_members( pointer-declaration, best_case )
#endif

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

#endif // LOGGER_MESSAGE_FORWARDER_HPP
