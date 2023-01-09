#include "format_strings.hpp"


BW_BEGIN_NAMESPACE

static const char * FORMAT_STRINGS_FILE = "strings";


/**
 *	Destructor.
 */
FormatStringsMLDB::~FormatStringsMLDB()
{
}


/*
 *	Override from FileHandler.
 */
bool FormatStringsMLDB::init( const char *logLocation, const char *mode )
{
	const char *formatStringsPath = this->join( logLocation,
		FORMAT_STRINGS_FILE );
	return BinaryFileHandler::init( formatStringsPath, mode );
}


/*
 *	Override from FileHandler.
 */
bool FormatStringsMLDB::read()
{
	long len = pFile_->length();
	pFile_->seek( 0 );

	while (pFile_->tell() < len)
	{
		LogStringInterpolator *pHandler = new LogStringInterpolator();
		pHandler->read( *pFile_ );

		// This could happen if we're reading and the logger was halfway
		// through dumping a fmt when we calculated 'len'.
		if (pFile_->error())
		{
			WARNING_MSG( "FormatStringsMLDB::read: "
				"Error encountered while reading strings file '%s': %s\n",
				filename_.c_str(), pFile_->strerror() );
			return false;
		}

		this->addFormatStringToMap( pHandler->formatString(), pHandler );
	}

	return true;
}


/**
 * This method is invoked from FileHandler::refresh() to clear our current
 * self knowledge prior to re-reading the format string file.
 */
void FormatStringsMLDB::flush()
{
	this->clear();
}


void FormatStringsMLDB::clear()
{
	offsetMap_.clear();
	FormatStrings::clear();
}

bool FormatStringsMLDB::canAppendToDB()
{
	// If we aren't in append mode then we can't modify the file further.
	return (mode_ == "a+");
}


/**
 * If we're in write mode and the fmt string passed in does not currently exist
 * in the mapping, it will be added to the mapping and written to disk.
 */
bool FormatStringsMLDB::writeFormatStringToDB( LogStringInterpolator *pHandler )
{
	return pHandler->write( *pFile_ );
}


/**
 * This method returns the string handler that should be used to parse
 * the log entry provided.
 *
 * @returns A pointer to a LogStringInterpolator on success, NULL on error.
 */
const LogStringInterpolator* FormatStringsMLDB::getHandlerForLogEntry(
	const LogEntry &entry )
{
	OffsetMap::iterator it = offsetMap_.find( entry.stringOffset() );

	if (it == offsetMap_.end())
	{
		return NULL;
	}

	return it->second;
}


/**
 * This method add the new format string to the map.
 */
void FormatStringsMLDB::addFormatStringToMap( const BW::string & fmt,
		LogStringInterpolator *pHandler )
{
	offsetMap_[ pHandler->fileOffset() ] = pHandler;

	FormatStrings::addFormatStringToMap( fmt, pHandler );
}


BW_END_NAMESPACE

// format_strings.cpp
