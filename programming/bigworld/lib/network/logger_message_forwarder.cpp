#include "pch.hpp"
#include "cstdmf/config.hpp"

#if ENABLE_WATCHERS
#include "network/channel.hpp"
#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/logger_endpoint.hpp"
#include "network/logger_message_forwarder.hpp"
#include "network/machine_guard.hpp"
#include "network/portmap.hpp"

#include "cstdmf/debug_filter.hpp"
#include "cstdmf/log_meta_data.hpp"
#include "cstdmf/watcher.hpp"


#include <cstring>
#include <time.h>

#include "network/bsd_snprintf.h"

DECLARE_DEBUG_COMPONENT2( "Network", 0 );

BW_BEGIN_NAMESPACE

/// Logger Message Forwarder Singleton
BW_SINGLETON_STORAGE( LoggerMessageForwarder )

namespace MessageLogger
{
/**
 *  Returns whether loggers with the given version should open an additional
 *  UDP connection. Applies to < BW2.3.
 *
 *  @param major the logger's major version
 *  @param major the logger's minor version
 *
 *  @return whether it should open a UDP connection
 */
bool versionShouldOpenUDP( uint16 major, uint16 minor )
{
	return major < 2 ||
		(major == 2 && minor < MESSAGE_LOGGER_VERSION_WITHOUT_EXTRA_UDP);
}


/**
 *  Returns whether loggers with the given version support the sending of
 *  metadata. Applies to BW >= 2.9.
 *
 *  @param major the logger's major version
 *  @param major the logger's minor version
 *
 *  @return whether it supports metadata
 */
bool versionSupportsMetaData( uint16 major, uint16 minor )
{
	return major > 2 ||
		(major == 2 && minor >= MESSAGE_LOGGER_VERSION_STRING_METADATA);
}


/**
 *  Returns whether loggers with the given version support the sending of
 *  metadata as a series of arguments. Applies to >= BW14.4 (aka BW2.10).
 *
 *  @param major the logger's major version
 *  @param major the logger's minor version
 *
 *  @return whether it supports metadata as arguments
 */
bool versionSupportsMetaDataArgs( uint16 major, uint16 minor )
{
	return major > 2 ||
		(major == 2 && minor > MESSAGE_LOGGER_VERSION_STRING_METADATA);
}

} // namespace MessageLogger

namespace // anonymous
{

const uint8 MESSAGE_LOGGER_VERSION_STRING_ID = 7;

const ServerAppInstanceID UNASSIGNED_APP_INSTANCE_ID = 0;

} // end anonymous namespace 


/**
 *
 */
void LoggerMessageHeader::write( MemoryOStream & stream ) const
{
	stream << componentPriority_ << messagePriority_ <<
		messageSource_ << category_;
	return;
}


/**
 *
 */
void LoggerMessageHeader::read( MemoryIStream & stream )
{
	stream >> componentPriority_ >> messagePriority_ >>
		messageSource_ >> category_;
	return;
}



/**
 * * NOTE *
 * Any time you change the serialised format of this class, you must also update
 * LoggerComponentMessage in bigworld/tools/server/pycommon/messages.py to
 * match, otherwise server tools will not be able to talk to the logger.
 */
void LoggerComponentMessage::write( BinaryOStream &os ) const
{
	os << version_ << loggerID_ << uid_ << pid_ << componentName_;
}


/**
 *
 */
void LoggerComponentMessage::read( BinaryIStream &is )
{
	is >> version_;

	if (version_ < MESSAGE_LOGGER_VERSION_STRING_ID)
	{
		uint8 loggerIDNum;
		is >> loggerIDNum;

		char buf[4];
		bw_snprintf( buf, sizeof( buf ), "%d", (int)loggerIDNum );
		loggerID_.assign( buf );

		version_ = MESSAGE_LOGGER_VERSION;
	}
	else
	{
		is >> loggerID_;
	}

	is >> uid_ >> pid_ >> componentName_;
}



// ----------------------------------------------------------------------------
// Section: FindLoggerHandler
// ----------------------------------------------------------------------------

/**
 *	This class is used in LoggerMessageForwarder::findLoggerInterfaces to
 *	receive responses from MessageLoggers on the network and add them to the
 *	LoggerMessageForwarder class.
 */
class FindLoggerHandler : public MachineGuardMessage::ReplyHandler
{
public:
	FindLoggerHandler( LoggerMessageForwarder & messageForwarder,
		Mercury::Address * pQueryAddress = NULL );

	virtual bool onProcessStatsMessage(
		ProcessStatsMessage & psm, uint32 ipAddress );

private:
	LoggerMessageForwarder & messageForwarder_;
	Mercury::Address * pQueryAddress_;
};


/**
 *	Constructor.
 */
FindLoggerHandler::FindLoggerHandler(
		LoggerMessageForwarder & messageForwarder,
		Mercury::Address * pQueryAddress /* = NULL */ ) :
	messageForwarder_( messageForwarder ),
	pQueryAddress_( pQueryAddress )
{
}


/*
 *	Override from MachineGuardMessage::ReplyHandler.
 */
bool FindLoggerHandler::onProcessStatsMessage(
	ProcessStatsMessage & psm, uint32 ipAddress )
{
	Mercury::Address address( ipAddress, psm.port_ );
	bool canAdd = pQueryAddress_ == NULL || address == *pQueryAddress_;

	if (psm.pid_ != 0 && canAdd)
	{
		messageForwarder_.addLogger( address, psm.majorVersion_,
									psm.minorVersion_ );
	}

	return true;
}


// ----------------------------------------------------------------------------
// Section: LoggerMessageForwarder
// ----------------------------------------------------------------------------

/**
 *	Constructor.
 */
LoggerMessageForwarder::LoggerMessageForwarder(
		const BW::string & appName,
		Endpoint & endpoint,
		Mercury::EventDispatcher & dispatcher,
		LoggerID loggerID,
		bool isEnabled,
		bool shouldFilterSpam,
		bool shouldSummariseSpam,
		unsigned spamFilterThreshold ) :
	appID_( UNASSIGNED_APP_INSTANCE_ID ),
	appName_( appName ),
	endpoint_( endpoint ),
	dispatcher_( dispatcher ),
	loggerID_( loggerID ),
	isEnabled_( isEnabled ),
	hasBeenEnabled_( isEnabled ),
	shouldFilterSpam_( shouldFilterSpam ),
	shouldSummariseSpam_( shouldSummariseSpam ),
	spamTimerHandle_(),
	spamFilterThreshold_( spamFilterThreshold ),
	spamHandler_( "* Suppressed %d in last 1s: %s" ),
	handlerCache_(),
	recentlyUsedHandlers_(),
	loggersReadWriteLock_(),
	loggers_()
{
	if (isEnabled_)
	{
		TRACE_MSG( "LoggerMessageForwarder::LoggerMessageForwarder: "
			"Finding loggers ...\n" );

		// find all loggers on the network.
		this->findLoggerInterfaces();

		TRACE_MSG( "LoggerMessageForwarder::LoggerMessageForwarder: "
			"Total # loggers on network: %" PRIzu "\n", loggers_.size() );
	}

	DebugFilter::instance().addMessageCallback( this );

	this->init();
}


/**
 *	Destructor
 */
LoggerMessageForwarder::~LoggerMessageForwarder()
{
	// Stop spam suppression timer
	spamTimerHandle_.cancel();

	DebugFilter::instance().deleteMessageCallback( this );

	// Cleanup all the Forwarding String Handlers
	HandlerCache::iterator cacheIter = handlerCache_.begin();

	while (cacheIter != handlerCache_.end())
	{
		bw_safe_delete( cacheIter->second );
		cacheIter++;
	}

	handlerCache_.clear();

	// Remove any loggers that were created in the constructor doing
	// findLoggerInterfaces()
	while (!loggers_.empty())
	{
		LoggerEndpoint * pLogger = loggers_.back();
		if (pLogger != NULL)
		{
			DEBUG_MSG( "LoggerMessageForwarder::~LoggerMessageForwarder: "
				"Removed %s. # loggers = %" PRIzu "\n",
				pLogger->addr().c_str(), (loggers_.size() - 1) );
			bw_safe_delete( pLogger );
		}
		loggers_.pop_back();
	}
}


/**
 *	This method finds all loggers that are currently running on the network.
 */
void LoggerMessageForwarder::findLoggerInterfaces(
	Mercury::Address * addr /* = NULL */ )
{
	// Construct mgm message asking for LoggerInterfaces.
	ProcessStatsMessage psm;
	psm.param_ = psm.PARAM_USE_CATEGORY | psm.PARAM_USE_NAME;
	psm.category_ = psm.WATCHER_NUB;
	psm.name_ = MESSAGE_LOGGER_NAME;

	// TODO: This is maybe not the most appropriate?
	FindLoggerHandler handler( *this, addr );
	Mercury::Reason reason = psm.sendAndRecv( endpoint_, BROADCAST, &handler );

	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "LoggerMessageForwarder::findLoggerInterfaces: "
			"MGM::sendAndRecv() failed (%s)\n",
			Mercury::reasonToString( (Mercury::Reason&)reason ) );
	}
}


/**
 *	This method initialises the LoggerMessageForwarder.
 */
void LoggerMessageForwarder::init()
{
	MF_WATCH( "logger/add", *this,
		&LoggerMessageForwarder::watcherHack,
		&LoggerMessageForwarder::watcherAddLogger,
		"Used by MessageLogger to add itself as a logging destination" );

	MF_WATCH( "logger/addTCP", *this,
		&LoggerMessageForwarder::watcherHack,
		&LoggerMessageForwarder::watcherAddLoggerTCP,
		"Used by MessageLogger to add itself as a logging destination"
		" (from 2.5)" );

	MF_WATCH( "logger/addTCPWithMetaData", *this,
		&LoggerMessageForwarder::watcherHack,
		&LoggerMessageForwarder::watcherAddLoggerTCPWithMetaDataV2_9,
		"Used by MessageLogger to add itself as a logging destination"
		" (from 2.9)" );

	MF_WATCH( "logger/addTCPWithMetaDataStream", *this,
		&LoggerMessageForwarder::watcherHack,
		&LoggerMessageForwarder::watcherAddLoggerTCPWithMetaDataV14_4,
		"Used by MessageLogger to add itself as a logging destination"
		" (from 14.4)" );

	MF_WATCH( "logger/size", *this, &LoggerMessageForwarder::size,
		"The number of loggers this process is sending to" );

	MF_WATCH( "logger/enabled", *this,
		&LoggerMessageForwarder::watcherGetEnabled,
		&LoggerMessageForwarder::watcherSetEnabled,
		"Whether or not to forward messages to attached logs" );

	MF_WATCH( "logger/filterThreshold", DebugFilter::instance(),
		MF_ACCESSORS( DebugMessagePriority, DebugFilter, filterThreshold ),
		"Controls the level at which messages are sent to connected loggers.\n"
		"A higher value reduces the volume of messages sent." );

	MF_WATCH( "logger/shouldFilterSpam", shouldFilterSpam_,
		Watcher::WT_READ_WRITE,
		"True if spam detection is enabled, false otherwise." );

	MF_WATCH( "logger/spamThreshold", spamFilterThreshold_,
		Watcher::WT_READ_WRITE,
		"The maximum number of a particular message that will be sent to the "
		"logs each second." );

	MF_WATCH( "logger/addSuppressionPattern", *this,
		&LoggerMessageForwarder::suppressionWatcherHack,
		&LoggerMessageForwarder::addSuppressionPattern,
		"Adds a new spam suppression pattern to this logger" );

	MF_WATCH( "logger/delSuppressionPattern", *this,
		&LoggerMessageForwarder::suppressionWatcherHack,
		&LoggerMessageForwarder::delSuppressionPattern,
		"Removes a spam suppression pattern from this logger" );

	MF_WATCH( "logger/shouldSummariseSpam", shouldSummariseSpam_,
		Watcher::WT_READ_WRITE,
		"True if spam detection should output summaries of ignored messages." );

	MF_WATCH( "config/hasDevelopmentAssertions", DebugFilter::instance(),
		MF_ACCESSORS( bool, DebugFilter, hasDevelopmentAssertions ),
		"If true, the process will be stopped when a development-time "
		"assertion fails" );

	// NOTE: There is no read lock around loggers_ as only the
	// main thread will remove loggers, and only the main thread
	// will use watchers.
	WatcherPtr pNewWatcher =
		new SequenceWatcher<Loggers>( loggers_ );
	pNewWatcher->addChild( "*", new BaseDereferenceWatcher(
		LoggerEndpoint::pWatcher()) );

	Watcher::rootWatcher().addChild( "logger/loggers", pNewWatcher );

	// Register a timer for doing spam suppression
	spamTimerHandle_ = dispatcher_.addTimer( 1000000 /* 1s */, this, NULL,
		"LoggerSpamTimer" );
}


/**
 *	This method registers the ID of the current component with Message Logger.
 *
 *	@param appID The component ID of the current app to register.
 */
void LoggerMessageForwarder::registerAppID( ServerAppInstanceID appID )
{
	appID_ = appID;

	ReadWriteLock::ReadGuard readGuard( loggersReadWriteLock_ );

	for (Loggers::iterator it = loggers_.begin();
		it != loggers_.end();
		++it)
	{
		this->sendAppID( *it );
	}
}


/**
 *  This method adds a string to this object's suppression prefix list.
 */
void LoggerMessageForwarder::addSuppressionPattern( BW::string prefix )
{
	SuppressionPatterns::iterator iter = std::find(
		suppressionPatterns_.begin(), suppressionPatterns_.end(), prefix );

	if (iter == suppressionPatterns_.end())
	{
		suppressionPatterns_.push_back( prefix );
		this->updateSuppressionPatterns();
	}
	else
	{
		WARNING_MSG( "LoggerMessageForwarder::addSuppressionPattern: "
			"Not re-adding pattern '%s'\n",
			prefix.c_str() );
	}
}


/**
 *  This method deletes a string from the object's suppression prefix list.
 */
void LoggerMessageForwarder::delSuppressionPattern( BW::string prefix )
{
	SuppressionPatterns::iterator iter = std::find(
		suppressionPatterns_.begin(), suppressionPatterns_.end(), prefix );

	if (iter != suppressionPatterns_.end())
	{
		suppressionPatterns_.erase( iter );
		this->updateSuppressionPatterns();
	}
	else
	{
		ERROR_MSG( "LoggerMessageForwarder::delSuppressionPattern: "
			"Tried to erase unknown suppression pattern '%s'\n",
			prefix.c_str() );
	}
}


/**
 *  This method updates FormatStringHandler::isSuppressible_ for all existing
 *  handlers based on the current list of suppression patterns.  Typically, this
 *  is only called from addSuppressionPattern() immediately after app startup,
 *  so the list is empty or small.
 */
void LoggerMessageForwarder::updateSuppressionPatterns()
{
	ReadWriteLock::ReadGuard readGuard( handlerCacheLock_ );
	for (HandlerCache::iterator iter = handlerCache_.begin();
		 iter != handlerCache_.end(); ++iter)
	{
		ForwardingStringHandler & handler = *iter->second;
		handler.isSuppressible( this->isSuppressible( handler.fmt() ) );
	}
}


/**
 *	This method adds a logger that we should forward to.
 */
void LoggerMessageForwarder::addLogger( const Mercury::Address & addr,
							uint16 majorVersion, uint16 minorVersion )
{
	if (addr.isNone())
	{
		WARNING_MSG( "LoggerMessageForwarder::addLogger: "
			"Received invalid logger address 0.0.0.0.\n" );
		return;
	}

	LoggerEndpoint * pLoggerEndpoint = NULL;
	{
		ReadWriteLock::ReadGuard readGuard( loggersReadWriteLock_ );
		Loggers::iterator iter =
			std::find( loggers_.begin(), loggers_.end(), addr );

		if (iter != loggers_.end())
		{
			WARNING_MSG( "LoggerMessageForwarder::addLogger: "
					"Re-adding %s\n", addr.c_str() );
			pLoggerEndpoint = *iter;
		}
	}

	if (pLoggerEndpoint == NULL)
	{
		pLoggerEndpoint = 
			new LoggerEndpoint( dispatcher_, this, addr );

		if (!pLoggerEndpoint->init( majorVersion, minorVersion ))
		{
			ERROR_MSG( "LoggerMessageForwarder::addLogger: "
				"Failed to add logger %s\n", addr.c_str() );
			bw_safe_delete( pLoggerEndpoint );
			return;
		}

		{
			// Should be owner of lock as this is coming from watcher
			ReadWriteLock::WriteGuard writeGuard( 
				loggersReadWriteLock_ );
			loggers_.push_back( pLoggerEndpoint );
		}

	}

	// tell the logger about us.
	MemoryOStream os;
	os << (int)MESSAGE_LOGGER_REGISTER;

	LoggerComponentMessage lrm;
	if (MessageLogger::versionSupportsMetaDataArgs(
			majorVersion, minorVersion ))
	{
		lrm.version_ = MESSAGE_LOGGER_VERSION;
	}
	else if (MessageLogger::versionSupportsMetaData(
			majorVersion, minorVersion ))
	{
		lrm.version_ = MESSAGE_LOGGER_VERSION_STRING_METADATA;
	}
	else
	{
		lrm.version_ = MESSAGE_LOGGER_VERSION_WITHOUT_METADATA;
	}
	lrm.loggerID_ = loggerID_;
	lrm.pid_ = mf_getpid();
	lrm.uid_ = ::getUserId();
	lrm.componentName_ = appName_;
	lrm.write( os );

	pLoggerEndpoint->send( os );

	INFO_MSG( "LoggerMessageForwarder::addLogger: "
				"Added %s. # loggers = %" PRIzu "\n",
			addr.c_str(), loggers_.size() );

	// This must be after the INFO_MSG above, otherwise the logger won't know
	// enough about this app to set the app ID.
	if (appID_ > UNASSIGNED_APP_INSTANCE_ID)
	{
		this->sendAppID( pLoggerEndpoint );
	}
}


/**
 *	This method removes a logger that we have been forwarding to.
 */
void LoggerMessageForwarder::delLogger( const Mercury::Address & addr )
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	LoggerEndpoint * logger  = NULL;
	{
		ReadWriteLock::WriteGuard writeGuard( loggersReadWriteLock_ );
		Loggers::iterator iter =
			std::find( loggers_.begin(), loggers_.end(), addr );

		if (iter == loggers_.end())
		{
			return;
		}

		logger = (*iter);

		loggers_.erase( iter );
	}

	{
		ReadWriteLock::ReadGuard readGuard( loggersReadWriteLock_ );
		INFO_MSG( "LoggerMessageForwarder::delLogger: "
			"Removed %s. # loggers = %" PRIzu "\n",
				logger->addr().c_str(), loggers_.size() );
	}
	bw_safe_delete( logger );
}


/**
 *	This method sends a newly known process AppID to a log receiving process.
 */
void LoggerMessageForwarder::sendAppID( LoggerEndpoint * ep )
{
	MF_ASSERT( appID_ != 0 );

	MemoryOStream os;
	os << (int)MESSAGE_LOGGER_APP_ID << appID_;

	if (!ep->send( os ))
	{
		ERROR_MSG( "LoggerMessageForwarder::sendAppID: "
			"Failed to send app ID to %s\n", ep->addr().c_str() );
	}
}


/**
 *	This method is a work-around for implementing a write-only watcher.
 */
Mercury::Address LoggerMessageForwarder::watcherHack() const
{
	return Mercury::Address( 0, 0 );
}



/**
 *	This method returns a ForwardingStringHandler to use for the provided
 *	format string.
 *
 *	@return  NULL if the format string is an invalid one.
 */
ForwardingStringHandler * LoggerMessageForwarder::findForwardingStringHandler(
	const char * formatString )
{
	{
		ReadWriteLock::ReadGuard readGuard( handlerCacheLock_ );
		HandlerCache::iterator it = handlerCache_.find( formatString );

		if (it != handlerCache_.end())
		{
			return it->second;
		}
	}

	{
		ForwardingStringHandler * pHandler =
			new ForwardingStringHandler( formatString,
				this->isSuppressible( formatString ) );

		if (!pHandler->isOk())
		{
			ERROR_MSG( "LoggerMessageForwarder::findForwardingStringHandler:"
				"Failed to create ForwardingStringHandler. This might be "
				"because the format string is invalid: %s\n", formatString );

			delete pHandler;

			// Explicitly assign a NULL handler to the cache to avoid
			// re-creation of a known bad format string handler.
			pHandler = NULL;
		}

		// We do two lookups as read locks are not upgradable to write locks
		ReadWriteLock::WriteGuard writeGuard( handlerCacheLock_ );
		handlerCache_[ formatString ] = pHandler;
		return pHandler;
	}
}


/**
 *	This method is called for each log message. It forwards a message to each
 *	attached logger.
 */
bool LoggerMessageForwarder::handleMessage(
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData & metaData,
	const char * pFormatString, va_list argPtr )
{
	if (loggers_.empty() || !isEnabled_)
	{
		return false;
	}

#ifdef _WIN32
	// Unable to handle logging without UID variable set
	if (::getUserId() == 0)
	{
		return false;
	}
#endif

	// Find/create the handler object for this format string
	ForwardingStringHandler * pHandler =
		this->findForwardingStringHandler( pFormatString );

	if (pHandler == NULL)
	{
		return false;
	}

	// This must be done before the call to isSpamming() for this logic to be
	// the exact opposite of that in handleTimeout()
	pHandler->addRecentCall();

	// If this isn't considered to be spam, parse and send.
	if (!shouldFilterSpam_ || !this->isSpamming( pHandler ))
	{
		this->parseAndSend( pHandler,
			messagePriority, pCategory,
			messageSource, &metaData, argPtr );
	}

	// If this is the first time this handler has been used this second, put
	// it in the used handlers collection.
	if (shouldFilterSpam_ &&
		pHandler->isSuppressible() && pHandler->numRecentCalls() == 1)
	{
		SimpleMutexHolder smh( recentlyUsedCacheHandlersMutex_ );
		recentlyUsedHandlers_.push_back( pHandler );
	}

	return false;
}


/**
 *  This method is called each second to summarise info about the log messages
 *  that have been spamming in the last second.
 */
void LoggerMessageForwarder::handleTimeout( TimerHandle handle, void * arg )
{
	// Send a message about each handler that exceeded its quota, and reset all
	// call counts.

	if (shouldFilterSpam_)
	{
		SimpleMutexHolder smh( recentlyUsedCacheHandlersMutex_ );
		RecentlyUsedHandlers::iterator iter = recentlyUsedHandlers_.begin();
		for ( ; iter != recentlyUsedHandlers_.end(); ++iter)
		{
			ForwardingStringHandler * pHandler = *iter;

			if (shouldSummariseSpam_ && this->isSpamming( pHandler ))
			{
				this->parseAndSend( &spamHandler_,
					MESSAGE_PRIORITY_DEBUG, /* pCategory */ NULL,
					/* messageSource */ MESSAGE_SOURCE_CPP,
					/* pMetaData */ NULL,
					pHandler->numRecentCalls() - spamFilterThreshold_,
					pHandler->fmt().c_str() );
			}

			pHandler->clearRecentCalls();
		}

		recentlyUsedHandlers_.clear();
	}
}


/**
 *  This method returns true if the given format string should be suppressed if
 *  it exceeds the spam suppression threshold.  This is used to set the
 *  isSuppressible_ member of ForwardingStringHandler.  It is not used when
 *  deciding whether or not to suppress a particular log message from being
 *  sent.
 */
bool LoggerMessageForwarder::isSuppressible(
	const BW::string & formatString ) const
{
	SuppressionPatterns::const_iterator iter = suppressionPatterns_.begin();

	while (iter != suppressionPatterns_.end())
	{
		const BW::string & patt = *iter;

		// If there is an empty pattern in the suppression list, then
		// everything is a candidate for suppression.
		if (patt.empty())
		{
			return true;
		}

		// This basically means formatString.startswith( patt )
		if ((formatString.size() >= patt.size()) &&
			(std::mismatch(
				patt.begin(), patt.end(), formatString.begin() ).first ==
			patt.end()))
		{
			return true;
		}

		++iter;
	}

	return false;
}


/**
 *  This method assembles the stream for a log message and sends it to all known
 *  loggers.
 */
void LoggerMessageForwarder::parseAndSend(
	ForwardingStringHandler * pHandler,
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData * pMetaData,
	va_list argPtr )
{
	MemoryOStream os;
	LoggerMessageHeader msgHeader;

	msgHeader.messagePriority_ = messagePriority;
	msgHeader.messageSource_ = messageSource;
	msgHeader.category_ = (pCategory) ? pCategory : BW::string();

	os << (int)MESSAGE_LOGGER_MSG;
	msgHeader.write( os );
	os << pHandler->fmt();

	pHandler->parseArgs( argPtr, os );

	// Two output streams: One with a raw JSON metadata string, one with a
	// stream of metadata arguments. Pointers because there's no need for them
	// unless there's metadata in the message.
	std::auto_ptr<MemoryOStream> pOsWithMetaDataV2_9;
	std::auto_ptr<MemoryOStream> pOsWithMetaDataV14_4;

	// pMetaData may be nonexistent (NULL) from sources that have no use for
	// it. Only write it if we have a non-NULL pointer.
	//
	// NOTE: This only works because metadata is the last value on the stream
	// and MessageLogger treats all remaining/unprocessed data as metadata.
	bool anyMetadata = pMetaData && pMetaData->pData();
	if (anyMetadata)
	{
		const int metaDataReserveSize = 128;
		pOsWithMetaDataV2_9.reset( new MemoryOStream( os.remainingLength() +
													metaDataReserveSize ) );
		pOsWithMetaDataV2_9->addBlob( os.retrieve( 0 ), os.remainingLength() );
		pOsWithMetaDataV14_4.reset( new MemoryOStream( os.remainingLength() +
													metaDataReserveSize ) );
		pOsWithMetaDataV14_4->addBlob( os.retrieve( 0 ), os.remainingLength() );

		if (pMetaData->getRawJSON())
		{
			*pOsWithMetaDataV2_9 << pMetaData->getRawJSON();
		}

		MemoryOStream * pMetaDataStream = const_cast< MemoryOStream * >(
			pMetaData->pData() );

		if (pMetaDataStream->size())
		{
			pOsWithMetaDataV14_4->addBlob( pMetaDataStream->retrieve( 0 ),
				pMetaDataStream->remainingLength() );
		}
	}

	{
		ReadWriteLock::ReadGuard readGuard( loggersReadWriteLock_ );
		for (Loggers::iterator iter = loggers_.begin();
			 iter != loggers_.end(); ++iter)
		{
			if (anyMetadata && (*iter)->shouldSendMetaData())
			{
				if ((*iter)->shouldStreamMetaDataArgs())
				{
					(*iter)->send( *pOsWithMetaDataV14_4 );
				}
				else
				{
					(*iter)->send( *pOsWithMetaDataV2_9 );
				}
			}
			else
			{
				(*iter)->send( os );
			}
		}
	}
}


/**
 *  This method is the same as above, except that the var args aren't already
 *  packaged up.  This allows us to send log messages without actually going via
 *  the *_MSG macros.  This is used to send the spam summaries.
 */
void LoggerMessageForwarder::parseAndSend(
	ForwardingStringHandler * pHandler,
	DebugMessagePriority messagePriority, const char * pCategory,
	DebugMessageSource messageSource, const LogMetaData * pMetaData,
	... )
{
	va_list argPtr;
	va_start( argPtr, pMetaData );

	this->parseAndSend( pHandler,
		messagePriority, pCategory, messageSource, pMetaData, argPtr );

	va_end( argPtr );
}


void LoggerMessageForwarder::watcherAddLoggerTCP( Mercury::Address addr )
{
	this->addLogger( addr, 2, MESSAGE_LOGGER_VERSION_WITHOUT_METADATA );
}

void LoggerMessageForwarder::watcherAddLoggerTCPWithMetaDataV2_9(
											Mercury::Address addr )
{
	this->addLogger( addr, 2, MESSAGE_LOGGER_VERSION_STRING_METADATA  );
}

void LoggerMessageForwarder::watcherAddLoggerTCPWithMetaDataV14_4(
											Mercury::Address addr )
{
	// Version 2.10 == 14.4. Don't want to call with (14, 4) here yet in case it
	// causes larger widespread problems.
	this->addLogger( addr, 2, MESSAGE_LOGGER_VERSION );
}

void LoggerMessageForwarder::watcherSetEnabled( bool enabled )
{
	isEnabled_ = enabled;

	if (isEnabled_ && !hasBeenEnabled_)
	{
		// find all loggers on the network.
		this->findLoggerInterfaces();

		hasBeenEnabled_ = true;
	}
}

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

// logger_message_forwarder.cpp
