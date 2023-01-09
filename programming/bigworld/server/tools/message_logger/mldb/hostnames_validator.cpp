#include "hostnames_validator.hpp"
#include "hostnames.hpp"
#include "../constants.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_util.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>


BW_BEGIN_NAMESPACE


/**
 * Constructor.
 */
HostnamesValidatorMLDB::HostnamesValidatorMLDB() :
	tempDirPath_()
{ }


/**
 * Destructor.
 */
HostnamesValidatorMLDB::~HostnamesValidatorMLDB()
{
	this->flush();

	if (this->isOpened())
	{
		this->close();
		// Regardless of the results of the non-critical close() it is best to
		// still attempt to bw_unlink the file and remove the temp dir.
	}

	if (filename_.length() > 0)
	{
		if (bw_unlink( filename_.c_str() ) == -1)
		{
			ERROR_MSG( "HostnamesValidatorMLDB::~HostnamesValidatorMLDB: "
				"Unable to remove temporary validation hostnames file '%s': "
				"%s\n", filename_.c_str(), strerror( errno ) );
			// This shouldn't be considered a failure case as the temporary
			// file won't hinder any operational behaviour.
		}
	}

	if (tempDirPath_.length() > 0)
	{
		if (rmdir( tempDirPath_.c_str() ) == -1)
		{
			ERROR_MSG( "HostnamesValidatorMLDB::~HostnamesValidatorMLDB: "
				"Unable to remove temporary directory '%s': %s\n",
				tempDirPath_.c_str(), strerror( errno ) );
			// This shouldn't be considered a failure case as the temporary
			// directory won't hinder any operational behaviour.
		}
	}
}


/**
 * This method generates a hostnames filename within a temporary path.
 *
 * @returns true on success, false on error.
 */
bool HostnamesValidatorMLDB::init( const char * logLocation )
{
	if (logLocation == NULL)
	{
		return false;
	}

	// Store the path so we can rmdir it when done.
	tempDirPath_ = this->getTempPath( logLocation );

	if (tempDirPath_.length() == 0)
	{
		return false;
	}

	const char *hostnamesFullFilePath = this->join( tempDirPath_.c_str(),
		HostnamesMLDB::getHostnamesFilename() );
	return TextFileHandler::init( hostnamesFullFilePath, "w+" );
}


/**
 * This method is not required or implemented for this class, as the class is
 * not designed to read temporary files, only to write to them. It is simply
 * here as a stubbed implementation of the pure virtual function in
 * TextFileHandler.
 */
bool HostnamesValidatorMLDB::handleLine( const char * /*line*/ )
{
	return true;
}


/**
 *
 */
void HostnamesValidatorMLDB::flush()
{
	this->clear();
}

/**
 * This method is used to write the entire validationList_ out to db.
 *
 * @returns true on success, false on error.
 */
bool HostnamesValidatorMLDB::writeHostnamesToDB() const
{
	HostnameStatusMap::const_iterator iter = validationList_.begin();

	while (iter != validationList_.end())
	{
		if (!this->writeHostEntryToDB( iter->first,
			iter->second->getHostname().c_str() ))
		{
			return false;
		}
		++iter;
	}

	return true;
}


/**
 * This method writes an address/hostname map combination to file.
 *
 * @returns true on success, false on failure
 */
bool HostnamesValidatorMLDB::writeHostEntryToDB( MessageLogger::IPAddress addr,
	const char * pHostname ) const
{
	// Write the mapping to disk
	const char *ipstr = inet_ntoa( (in_addr&)addr );
	char line[ 2048 ];
	bw_snprintf( line, sizeof( line ), "%s %s", ipstr, pHostname );

	if (!this->writeLine( line ))
	{
		CRITICAL_MSG( "HostnamesValidatorMLDB::writeHostEntryToDB: "
			"Couldn't write hostname mapping for %s\n", line );
		return false;
	}

	return true;
}


/**
 * This static method gets a temporary hostnames file path.
 *
 * @param  pBasePath	The base path within which to create the temporary
 * 						directory.
 *
 * @returns A new hostnames filename in a uniquely-named temporary path, or an
 * 			empty string on error.
 */
BW::string HostnamesValidatorMLDB::getTempPath( const char * pBasePath ) const
{
	// This variable is used by mkdtemp() as the template name for the
	// temporary directory creation. The last 6 characters must be XXXXXX. The
	// variable will be modified by the call to mkdtemp(), therefore we need to
	// create a non-const pointer.
	char *pTempDirname = strdup( FileHandler::join( pBasePath,
		"/validate_hostnames-XXXXXX" ) );
	if (pTempDirname == NULL)
	{
		ERROR_MSG( "HostnamesValidatorMLDB::getTempPath: Insufficient memory "
			"available for strdup." );
		return BW::string();
	}

	if (mkdtemp( pTempDirname ) == NULL)
	{
		ERROR_MSG( "HostnamesValidatorMLDB::getTempPath: "
			"Failed to create temporary directory: %s", strerror( errno ) );
		free( pTempDirname );
		return BW::string();
	}

	// Convert the modified char* to String form so we can free it and return
	// the string.
	BW::string tempDirString = pTempDirname;
	free( pTempDirname );

	return tempDirString;
}


BW_END_NAMESPACE

// hostnames_validator.cpp
