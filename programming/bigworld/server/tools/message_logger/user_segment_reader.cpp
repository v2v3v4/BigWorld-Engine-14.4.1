#include <Python.h>

#include "user_segment_reader.hpp"

#include "log_entry.hpp"
#include "log_string_interpolator.hpp"
#include "py_query_result.hpp"

#include <dirent.h>


BW_BEGIN_NAMESPACE

UserSegmentReader::UserSegmentReader( const BW::string userLogPath,
		const char *suffix ) :
	UserSegment( userLogPath, suffix ),
	isSegmentOK_( true )
{ }


// Candidate for cleanup.
// This method is almost identical to UserSegmentWriter. is it worth
// implementing in the base class.
bool UserSegmentReader::init()
{
	char buf[ 1024 ];
	static const char *mode = "r";

	bw_snprintf( buf, sizeof( buf ), "%s/entries.%s",
		userLogPath_.c_str(), suffix_.c_str() );

	pEntries_ = new FileStream( buf, mode );
	if (!pEntries_->good())
	{
		ERROR_MSG( "UserSegmentReader::init: "
			"Couldn't open entries file %s for reading: %s\n",
			buf, pEntries_->strerror() );
		isGood_ = false;
		return false;
	}

	// Generate args filename
	bw_snprintf( buf, sizeof( buf ), "%s/args.%s",
		userLogPath_.c_str(), suffix_.c_str() );
	pArgs_ = new FileStream( buf, mode );
	if (!pArgs_->good())
	{
		ERROR_MSG( "UserSegmentReader::init: "
			"Couldn't open args file %s for reading: %s\n",
			buf, pArgs_->strerror() );
		isGood_ = false;
		return false;
	}

	// check if entry size is 0
	if (pEntries_->length() == 0)
	{
		isSegmentOK_ = false;
		isGood_ = false;
	}

	if (isSegmentOK_)
	{
		isSegmentOK_ = this->updateEntryBounds();
	}

	pMetadataMLDB_ = new MetadataMLDB();
	if (!pMetadataMLDB_->init( userLogPath_.c_str(), suffix_.c_str(), mode ))
	{
		ERROR_MSG( "UserSegmentReader::init: "
			"Unable to initialise metadata.\n" );
		return false;
	}

	return isSegmentOK_;
}


int UserSegmentReader::filter( const struct dirent *ent )
{
	return !strncmp( "entries.", ent->d_name, 8 );
}


/**
 * Returns true if this segment's underlying files have been modified since
 * the last call to calculateLengths().
 */
bool UserSegmentReader::isDirty() const
{
	return int( numEntries_ * sizeof( LogEntry ) ) < pEntries_->length();
}


/**
 * Returns the entry number of the smallest time >= time (when direction == 1)
 * or greatest time <= time (when direction == -1) or -1 if none found.
 */
int UserSegmentReader::findEntryNumber( LogTime &time,
	SearchDirection direction )
{
	// Early return if time is outside the range of this segment (which happens
	// often when searching from the beginning or to the end of a log).
	if (direction == QUERY_FORWARDS && time <= start_)
	{
		return 0;
	}

	if (direction == QUERY_BACKWARDS && time >= end_)
	{
		return numEntries_ - 1;
	}

	// Now do binary search
	int left = 0;
	int right = numEntries_ - 1;
	int mid;

	LogTime midtime;

	while (1)
	{
		mid = direction == 1 ? (left+right)/2 : (left+right+1)/2;
		pEntries_->seek( mid * sizeof( LogEntry ) );
		*pEntries_ >> midtime;

		if (left >= right)
		{
			break;
		}

		if (direction > 0)
		{
			if (time <= midtime)
			{
				right = mid;
			}
			else
			{
				left = mid+1;
			}
		}
		else
		{
			if (time < midtime)
			{
				right = mid-1;
			}
			else
			{
				left = mid;
			}
		}
	}

	if ((direction == QUERY_FORWARDS && time <= midtime) ||
		(direction == QUERY_BACKWARDS && midtime <= time))
	{
		return mid;
	}

	return -1;
}


// Candidate for cleanup. functionality is duplicated in UserSegment::readEntry
bool UserSegmentReader::seek( int n )
{
	return pEntries_->seek( n * sizeof( LogEntry ) );
}


const LogTime & UserSegmentReader::getStartLogTime() const
{
	return start_;
}


const LogTime & UserSegmentReader::getEndLogTime() const
{
	return end_;
}


// Candidate for cleanup. This method is awful, and a symptom of the bad
// design of the querying structure of bwlog.
bool UserSegmentReader::interpolateMessage( const LogEntry &entry,
	const LogStringInterpolator *pHandler, BW::string &result )
{
	pArgs_->seek( entry.argsOffset() );
	return const_cast< LogStringInterpolator * >( pHandler )->streamToString(
															*pArgs_, result );
}


/**
 *	This method retrieves the meta data from the log entry.
 *
 *	This is then utilised by PyUserLog() to construct a PyQueryResult with
 *	the result.
 */
bool UserSegmentReader::metadata( const LogEntry & entry, BW::string & result )
{
	uint16 expectedLength = entry.metadataLength();

	if (expectedLength)
	{
		bool isOK = pMetadataMLDB_->readFromOffset(
				entry.metadataOffset(), result );

		return (isOK && (result.length() == expectedLength));
	}

	return true;
}


FileStream * UserSegmentReader::getArgStream()
{
	return pArgs_;
}


int UserSegmentReader::getEntriesLength() const
{
	return pEntries_->length();
}


int UserSegmentReader::getArgsLength() const
{
	return pArgs_->length();
}


int UserSegmentReader::getMetadataLength() const
{
	return pMetadataMLDB_->length();
}


BW_END_NAMESPACE

// user_segment_reader.cpp
