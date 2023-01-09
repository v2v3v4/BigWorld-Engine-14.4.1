#include "active_files.hpp"

#include "user_log.hpp"

#include "cstdmf/debug.hpp"

#include <unistd.h>
#include <string.h>
#include <errno.h>


BW_BEGIN_NAMESPACE

bool ActiveFiles::init( const BW::string &logPath,
	UserLogs *pUserLogs )
{
	logPath_ = logPath;
	pUserLogs_ = pUserLogs;

	return true;
}


/**
 * This method does nothing as this file is never read.
 *
 * @returns This method always returns true.
 */
bool ActiveFiles::read()
{
	return true;
}


/**
 * This method does nothing as this file is never read.
 *
 * @returns This method always returns true.
 */
bool ActiveFiles::handleLine( const char *line )
{
	return true;
}


/**
 * This method removes the old file referenced by this instance while updating
 * and storing all current entries, args and metadata files for the current
 * UserLogs.
 *
 * @returns true on success, false on error.
 */
bool ActiveFiles::update()
{
	MF_ASSERT( pUserLogs_ != NULL );

	const char *filePath = TextFileHandler::join( logPath_.c_str(),
								"active_files" );
	// Open file and clobber
	if (!TextFileHandler::init( filePath, "w" ))
	{
		ERROR_MSG( "ActiveFiles::update: Couldn't open %s for writing: %s\n",
			this->filename(), strerror( errno ) );
		return false;
	}

	char buf[ 512 ];

	// Dump current entries, args and metadata files
	UserLogs::const_iterator it = pUserLogs_->begin();
	while (it != pUserLogs_->end())
	{
		// Skip userlogs with no active segment
		if (!it->second->hasActiveSegments())
		{
			++it;
			continue;
		}

		BW::string username = it->second->getUsername();
		BW::string segmentSuffix = it->second->activeSegmentSuffix();

		bw_snprintf( buf, sizeof( buf ), "%s/entries.%s",
			username.c_str(), segmentSuffix.c_str() );

		if (!this->writeLine( buf ))
		{
			ERROR_MSG( "ActiveFiles::update: Couldn't write '%s': %s\n",
				buf, strerror( errno ) );
			this->close();
			return false;
		}

		snprintf( buf, sizeof( buf ), "%s/args.%s",
			username.c_str(), segmentSuffix.c_str() );

		if (!this->writeLine( buf ))
		{
			ERROR_MSG( "ActiveFiles::update: Couldn't write '%s': %s\n",
				buf, strerror( errno ) );
			this->close();
			return false;
		}

		bw_snprintf( buf, sizeof( buf ), "%s/metadata.%s",
			username.c_str(), segmentSuffix.c_str() );

		if (!this->writeLine( buf ))
		{
			ERROR_MSG( "ActiveFiles::update: Couldn't write '%s': %s\n",
				buf, strerror( errno ) );
			this->close();
			return false;
		}

		++it;
	}

	this->close();
	return true;
}


/**
 * Removes the file the class owns.
 *
 * @returns true on success, false on error.
 */
bool ActiveFiles::deleteFile()
{
	if (unlink( this->filename() ) && errno != ENOENT)
	{
		ERROR_MSG( "ActiveFiles::deleteFile: "
			"Failed to remove 'active_files': %s\n", strerror( errno ) );
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// active_files.cpp
