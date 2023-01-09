#include "log_storage.hpp"


BW_BEGIN_NAMESPACE


namespace // anonymous
{

const MessageLogger::NetworkVersion OLDEST_VERSION_WITHOUT_SOURCE_ENUM = 7;

} // end anonymous namespace


/**
 * Constructor.
 */
LogStorage::LogStorage( Logger & logger ) :
	writeToStdout_( false ),
	logger_( logger )
{}


LogStorage::~LogStorage()
{}


void LogStorage::writeToStdout( bool status )
{
	writeToStdout_ = status;
}


namespace
{

/**
 * Handles receiving responses from BWMachined when querying for usernames.
 */
class UsernameHandler : public MachineGuardMessage::ReplyHandler
{
public:
	virtual ~UsernameHandler() {}

	/**
	 * This method handles receipt of a UserMessage.
	 *
	 * @returns Always returns false to avoid further message handling.
	 */
	virtual bool onUserMessage( UserMessage & userMessage,
		MessageLogger::IPAddress /*addr*/ )
	{
		if (userMessage.uid_ != UserMessage::UID_NOT_FOUND)
		{
			username_ = userMessage.username_;
		}

		return false;
	}
	BW::string username_;
};

} // anonymous namespace


/**
 * This method adds an incoming message to the logs.
 *
 * @returns enum type AddLogMessageResult
 */
LogStorage::AddLogMessageResult LogStorage::addLogMessage(
		const LoggerComponentMessage & componentMessage,
		const Mercury::Address & address, MemoryIStream & inputStream )
{
	// Stream off the header and the format string
	LoggerMessageHeader header;
	BW::string format;
	int len = inputStream.remainingLength();

	// If a message has been passed through to this point, it has already
	// been through the OLDEST_SUPPORTED_MESSAGE_LOGGER_VERSION check in
	// logger.cpp, so for the time being read off the old 2 element header
	// and fill in the DebugMessageSource and category.
	if (componentMessage.version_ <= OLDEST_VERSION_WITHOUT_SOURCE_ENUM)
	{
		header.messageSource_ = MESSAGE_SOURCE_CPP;
		inputStream >> header.componentPriority_ >> header.messagePriority_;
		if (header.messagePriority_ == DEPRECATED_PRIORITY_SCRIPT)
		{
			header.messagePriority_ = MESSAGE_PRIORITY_INFO;
			header.messageSource_ = MESSAGE_SOURCE_SCRIPT;
		}
	}
	else
	{
		header.read( inputStream );
	}

	if (!logger_.shouldLogPriority( header.messagePriority_ ))
	{
		return LOG_ADDITION_IGNORED;
	}

	inputStream >> format;

	if (inputStream.error())
	{
		ERROR_MSG( "LogStorage::addLogMessage: Log message from %s was too "
			"short (only %d bytes)\n", address.c_str(), len );
		return LOG_ADDITION_FAILED;
	}

	// Get the format string handler
	LogStringInterpolator *pHandler = this->getFormatStrings()->resolve( format );
	if (pHandler == NULL)
	{
		ERROR_MSG( "LogStorage::addLogMessage: Couldn't add format string to "
			"mapping. '%s'\n", format.c_str() );
		return LOG_ADDITION_FAILED;
	}

	// Cache the IP address -> Hostname mapping if neccessary
	BW::string hostname;
	GetHostResult result = this->getHostnames()->getHostByAddr(
								address.ip, hostname );

	if (result == BW_GET_HOST_ERROR)
	{
		ERROR_MSG( "LogStorage::addLogMessage: Error resolving host '%s'\n",
			address.c_str() );
		return LOG_ADDITION_FAILED;
	}
	else if ((result == BW_GET_HOST_ADDED) && this->getHostnamesValidator())
	{
		// If a validator exists, ensure any newly added hosts are propagated
		// through to it as VALIDATED
		this->getHostnamesValidator()->addValidateHostname(
				address.ip, hostname, BW_HOSTNAME_STATUS_VALIDATED );
	}

	MessageLogger::CategoryID categoryID =
		this->getCategories()->resolveOrCreateNameToID( header.category_ );

	return this->writeLogToDB( componentMessage, address, inputStream, header,
		pHandler, categoryID );
}


/**
 * Queries BWMachined in an attempt to resolve the username associated to uid.
 *
 * @param uid     The UID to query BWmachined to resolve.
 * @param addr    The address of the machine to initially query (where the
 *                 log initially came from.
 * @param result  Username that has been resolved from uid.
 *
 * @returns Mercury::REASON_SUCCESS on success, other on failure.
 */
Mercury::Reason LogStorage::resolveUID( uint16 uid,
	MessageLogger::IPAddress ipAddress, BW::string & result )
{
	// Hard coded a case for UID 0 which will appear when windows
	// users haven't specified a UID. This should default to the root
	// user.
	if (uid == 0)
	{
		result = "root";
		return Mercury::REASON_SUCCESS;
	}

	int reason;
	MessageLogger::IPAddress queryIpAddress = ipAddress;
	UsernameHandler handler;
	UserMessage um;
	um.uid_ = uid;
	um.param_ = um.PARAM_USE_UID;

	reason = um.sendAndRecv( 0, queryIpAddress, &handler );
	if (reason != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "LogStorage::resolveUID: UserMessage query to %s for "
						"UID %hu failed: %s\n",
					inet_ntoa( (in_addr&)queryIpAddress ),
					uid, Mercury::reasonToString( (Mercury::Reason&)reason ) );

		INFO_MSG( "LogStorage::resolveUID: Retrying UID query for %hu as "
					"broadcast.\n",	uid );

		queryIpAddress = BROADCAST;
		reason = um.sendAndRecv( 0, queryIpAddress, &handler );
	}

	// If we still haven't been able to resolve after trying a broadcast, fail
	if (reason != Mercury::REASON_SUCCESS)
	{
		return (Mercury::Reason&)reason;
	}

	// If we couldn't resolve the username, just use his UID as his username
	if (handler.username_.empty())
	{
		WARNING_MSG( "LogStorage::resolveUID: "
			"Couldn't resolve UID %hu, using UID as username\n", uid );

		char buf[ 128 ];
		bw_snprintf( buf, sizeof( buf ), "%hu", uid );
		result = buf;
	}
	else
	{
		// Now we are sure the resolving was successful, update the result
		// string
		result = handler.username_;
	}

	return (Mercury::Reason&)reason;
}

BW_END_NAMESPACE

// log_storage.cpp
