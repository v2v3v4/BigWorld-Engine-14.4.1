#include "query_result.hpp"

#include "log_entry.hpp"
#include "logging_component.hpp"
#include "user_log.hpp"

#include "cstdmf/debug_message_source.hpp"


BW_BEGIN_NAMESPACE

char QueryResult::s_linebuf_[ 2048 ];


QueryResult::QueryResult() :
	host_(),
	username_( NULL ),
	component_( NULL )
{ }


QueryResult::QueryResult( const LogEntry & entry, LogCommonMLDB * pBWLog,
	const UserLog * pUserLog, const LoggingComponent * pComponent,
	const BW::string & message, const BW::string & metadata ) :

	time_( entry.time().asDouble() ),
	host_( pBWLog->getHostByAddr( pComponent->getAddress().ip ) ),
	pid_( pComponent->getPID() ),
	appInstanceID_( pComponent->getAppInstanceID() ),
	username_( pUserLog->getUsername().c_str() ),
	component_( pBWLog->getComponentNameByAppTypeID(
					pComponent->getAppTypeID() ) ),
	categoryName_( pBWLog->getCategoryNameByID(
					entry.categoryID() ) ),
	messageSource_( entry.messageSource() ),
	severity_( entry.messagePriority() ),
	message_( message ),
	metadata_( metadata ),
	stringOffset_( entry.stringOffset() ),
	maxHostnameLen_( pUserLog->maxHostnameLen() )
{ }


/*
 * The following functions used to be macros in the body of the following
 * methods but are implemented as functions to avoid macro ugliness.
 */
static int buf_remain( const char * cursor, const char * end )
{
	return end - cursor - 1;
}


static bool truncate( char * end, int * & pLen )
{
	const char truncmsg[] = "<== message truncated!\n";
	strcpy( end - strlen( truncmsg ) - 1, truncmsg );

	if (pLen != NULL)
	{
		*pLen = sizeof( QueryResult::s_linebuf_ ) - 1;
	}

	return false;
}


static bool seek_cursor( char * & cursor, char * end, bool & previous,
	int * & pLen, int n = 0 )
{
	if (n)
	{
		if (cursor + n >= end)
		{
			return truncate( end, pLen );
		}

		cursor += n;
	}
	else
	{
		while (cursor < end && *cursor)
		{
			cursor++;
		}

		if (cursor == end)
		{
			return truncate( end, pLen );
		}
	}

	previous = true;

	return true;
}


static void pad_prior( char * & cursor, char * end, bool & previous,
	int * & pLen )
{
	if (previous)
	{
		*cursor = ' ';
		seek_cursor( cursor, end, previous, pLen, 1 );
	}
}


/**
 * Format this log result according to the supplied display flags (see
 * DisplayFlags in constants.hpp).
 *
 * Uses a static buffer, so you must copy the result of this call away if you
 * want to preserve it. Writes the length of the formatted string to pLen if
 * it is non-NULL.
 */
const char *QueryResult::format( unsigned flags, int * pLen )
{
	char *cursor = s_linebuf_;
	char *end = s_linebuf_ + sizeof( s_linebuf_ );
	bool previous = false;

	memset( s_linebuf_, ' ', sizeof( s_linebuf_ ) );
	s_linebuf_[sizeof( s_linebuf_ ) - 1] = '\0';

	// If this is a pad line, just chuck in -- like grep does
	if (host_.empty())
	{
		bw_snprintf( s_linebuf_, sizeof( s_linebuf_ ), "--\n" );
		if (pLen)
		{
			*pLen = 3;
		}
		return s_linebuf_;
	}

	if (flags & (SHOW_DATE | SHOW_TIME))
	{
		struct tm breakdown;
		LogTime time( time_ );
		// This assignment allows compilation on 32 bit systems.
		const time_t seconds = time.secs_;
		localtime_r( &seconds, &breakdown );

		if (flags & SHOW_DATE)
		{
			strftime( cursor, buf_remain( cursor, end ), "%a %d %b %Y ",
				&breakdown );
			seek_cursor( cursor, end, previous, pLen );
		}

		if (flags & SHOW_TIME)
		{
			strftime( cursor, buf_remain( cursor, end ), "%T", &breakdown );

			if (!seek_cursor( cursor, end, previous, pLen ))
			{
				return s_linebuf_;
			}

			snprintf( cursor, buf_remain( cursor, end ), ".%03d ",
				time.msecs_ );

			if (!seek_cursor( cursor, end, previous, pLen, 5 ))
			{
				return s_linebuf_;
			}
		}
	}

	if (flags & SHOW_HOST)
	{
		pad_prior( cursor, end, previous, pLen );

		bw_snprintf( cursor, buf_remain( cursor, end ), "%-.*s",
			maxHostnameLen_, host_.c_str() );

		int hostLength = host_.length();
		if (hostLength < maxHostnameLen_)
		{
			// overwrite inserted null char with space
			cursor[ hostLength ] = ' ';
		}

		if (!seek_cursor( cursor, end, previous, pLen, maxHostnameLen_ ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_USER)
	{
		pad_prior( cursor, end, previous, pLen );
		snprintf( cursor, buf_remain( cursor, end ), "%-10s", username_ );

		if (!seek_cursor( cursor, end, previous, pLen, 10 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_PID)
	{
		pad_prior( cursor, end, previous, pLen );
		snprintf( cursor, buf_remain( cursor, end ), "%-5d", pid_ );

		if (!seek_cursor( cursor, end, previous, pLen, 5 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_PROCS)
	{
		pad_prior( cursor, end, previous, pLen );
		snprintf( cursor, buf_remain( cursor, end ), "%-10s", component_ );

		if (!seek_cursor( cursor, end, previous, pLen, 10 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_APPID)
	{
		pad_prior( cursor, end, previous, pLen );

		if (appInstanceID_)
		{
			snprintf( cursor, buf_remain( cursor, end ),
				"%-3d", appInstanceID_ );
		}
		else
		{
			snprintf( cursor, buf_remain( cursor, end ), "   " );
		}

		if (!seek_cursor( cursor, end, previous, pLen, 3 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_MESSAGE_SOURCE_TYPE)
	{
		pad_prior( cursor, end, previous, pLen );
		snprintf( cursor, buf_remain( cursor, end ), "%-6s",
			messageSourceAsCString( messageSource_ ) );

		if (!seek_cursor( cursor, end, previous, pLen, 6 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_SEVERITY)
	{
		pad_prior( cursor, end, previous, pLen );
		snprintf( cursor, buf_remain( cursor, end ), "%-8s",
			messagePrefix( (DebugMessagePriority)severity_ ) );

		if (!seek_cursor( cursor, end, previous, pLen, 8 ))
		{
			return s_linebuf_;
		}
	}

	if (flags & SHOW_CATEGORY)
	{
		pad_prior( cursor, end, previous, pLen );

		size_t categoryNameLength = 0;

		if (categoryName_ != NULL)
		{
			categoryNameLength = strlen( categoryName_ );
		}

		if (categoryNameLength > 0)
		{
			int sizeWritten = snprintf( cursor, buf_remain( cursor, end ),
			   	"[%s]", categoryName_ );

			if (!seek_cursor( cursor, end, previous, pLen, sizeWritten ))
			{
				return s_linebuf_;
			}
		}
		else
		{
			previous = false;
		}
	}

	if (flags & SHOW_MESSAGE)
	{
		pad_prior( cursor, end, previous, pLen );
		int msgcol = (int)(cursor - s_linebuf_);
		const char *c = message_.c_str();

		while (*c)
		{
			*cursor = *c++;

			if (!seek_cursor( cursor, end, previous, pLen, 1 ))
			{
				return s_linebuf_;
			}

			// If the character just read was a newline and we're not done, fill
			// out the left hand side so the message columns line up
			if (cursor[-1] == '\n' && *c)
			{
				char *padpoint = cursor;

				if (!seek_cursor( cursor, end, previous, pLen, msgcol ))
				{
					return s_linebuf_;
				}

				memset( padpoint, ' ', msgcol );
			}
		}
	}

	if (cursor[-1] != '\n')
	{
		*cursor = '\n';

		if (!seek_cursor( cursor, end, previous, pLen, 1 ))
		{
			return s_linebuf_;
		}
		*cursor = '\0';
	}
	else
	{
		*cursor = '\0';
	}

	if (pLen != NULL)
	{
		*pLen = (int)(cursor - s_linebuf_);
	}

	return s_linebuf_;
}


/**
 *	This method returns the Metadata associated with the query result.
 */
const BW::string & QueryResult::metadata() const
{
	return metadata_;
}


/**
 *
 */
const BW::string & QueryResult::getMessage() const
{
	return message_;
}


/**
 *
 */
MessageLogger::FormatStringOffsetId QueryResult::getStringOffset() const
{
	return stringOffset_;
}


/**
 *
 */
double QueryResult::getTime() const
{
	return time_;
}


/**
 *
 */
const char * QueryResult::getHost() const
{
	return host_.c_str();
}


/**
 *
 */
int QueryResult::getPID() const
{
	return pid_;
}


/**
 *	This method returns the components instance ID.
 */
MessageLogger::AppInstanceID QueryResult::getAppInstanceID() const
{
	return appInstanceID_;
}


/**
 *
 */
const char * QueryResult::getUsername() const
{
	return username_;
}


/**
 *
 */
const char * QueryResult::getComponent() const
{
	return component_;
}


/**
 *
 */
int QueryResult::getSeverity() const
{
	return severity_;
}

BW_END_NAMESPACE

// query_result.cpp
