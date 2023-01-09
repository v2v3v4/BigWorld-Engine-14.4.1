#include "user_segment.hpp"

#include "logging_component.hpp"
#include "log_entry.hpp"

#include <stdio.h>
#include <dirent.h>
#include <string.h>


BW_BEGIN_NAMESPACE

UserSegment::UserSegment( const BW::string userLogPath, const char *suffix ) :
	pEntries_( NULL ),
	pArgs_( NULL ),
	numEntries_( 0 ),
	argsSize_( 0 ),
	isGood_( true ),
	userLogPath_( userLogPath ),
	pMetadataMLDB_( NULL )
{
	// Generate a suffix if none provided
	if (suffix == NULL)
	{
		time_t now = time( NULL );
		struct tm *pTime = localtime( &now );
		MF_ASSERT( pTime != NULL);
		this->buildSuffixFrom( *pTime, suffix_ );
	}
	else
	{
		suffix_ = suffix;
	}

	return;
}


UserSegment::~UserSegment()
{
	if (pEntries_)
	{
		delete pEntries_;
	}

	if (pArgs_)
	{
		delete pArgs_;
	}

	if (pMetadataMLDB_)
	{
		delete pMetadataMLDB_;
		pMetadataMLDB_ = NULL;
	}
}



bool UserSegment::updateEntryBounds()
{
	numEntries_ = pEntries_->length() / sizeof( LogEntry );
	argsSize_ = pArgs_->length();

	LogEntry entry;

	bool isOK =  true;

	// Work out current start and end times
	if (numEntries_ > 0)
	{
		isOK &= this->readEntry( 0, entry );
		start_ = entry.time();
		isOK &= this->readEntry( numEntries_ - 1, entry );
		end_ = entry.time();
	}

	return isOK;
}


/**
 * Retrieves a specific LogEntry from the UserSegment files.
 */
bool UserSegment::readEntry( int entryNum, LogEntry & entry )
{
	pEntries_->seek( entryNum * sizeof( LogEntry ) );
	*pEntries_ >> entry;

	if (pEntries_->error())
	{
		ERROR_MSG( "UserSegment::readEntry: "
			"Failed to read entry %d (of %d) from '%s': %s\n",
			entryNum, numEntries_, suffix_.c_str(), pEntries_->strerror() );
		return false;
	}

	return true;
}


/**
 *	This method creates a string to be used as a filename suffix using
 *	the provided time structure to format the date from.
 *
 *	@param pTime	The time structure to create the string with.
 *	@param newSuffix	The newly created suffix string.
 *
 *	@returns true if the suffix was successfully created, false otherwise.
 */
bool UserSegment::buildSuffixFrom( struct tm & pTime,
	BW::string & newSuffix ) const
{
	char tmpBuff[ 1024 ];

	if (strftime( tmpBuff, sizeof( tmpBuff ) - 1, "%Y-%m-%d-%T", &pTime ) == 0)
	{
		ERROR_MSG( "UserSegment::buildSuffixFrom: "
			"Unable to format provided time to build suffix with.\n" );
		return false;
	}

	newSuffix = tmpBuff;

	return true;
}

BW_END_NAMESPACE

// user_segment.cpp
