#include "metadata.hpp"

#include "cstdmf/debug.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


BW_BEGIN_NAMESPACE

static const char * METADATA_FILE = "metadata";


/**
 * Constructor.
 */
MetadataMLDB::MetadataMLDB()
{}


/**
 * Destructor.
 */
MetadataMLDB::~MetadataMLDB()
{
	this->flush();
}


bool MetadataMLDB::init( const char *pUserLogDir, const char *pSuffix,
	const char *pMode )
{
	if (pUserLogDir == NULL || pSuffix == NULL)
	{
		return false;
	}

	char buf[ 1024 ];

	bw_snprintf( buf, sizeof( buf ), "%s.%s", METADATA_FILE, pSuffix );

	const char *pMetadataFullFilePath = this->join( pUserLogDir, buf );
	return BinaryFileHandler::init( pMetadataFullFilePath, pMode );
}


/**
 * This method write new metadata to MLDB.
 */
bool MetadataMLDB::writeMetadataToMLDB( const BW::string & metadata )
{
	// Write the metadata to disk
	*pFile_ << metadata;

	if (!pFile_->commit())
	{
		CRITICAL_MSG( "MetadataMLDB::writeMetadataToMLDB: "
			"Couldn't write metadata for %s\n", metadata.c_str() );
		return false;
	}

	return true;
}


/**
 *
 */
bool MetadataMLDB::readFromOffset( MessageLogger::MetaDataOffset offset,
	BW::string & result )
{
	if (!pFile_)
	{
		ERROR_MSG( "MetadataMLDB::readFromOffset: No file stream exists\n" );
		return false;
	}

	if (pFile_->seek( offset ) == -1)
	{
		ERROR_MSG( "MetadataMLDB::readFromOffset: "
				"Unable to seek to meta data location: %s\n",
			pFile_->strerror() );
		return false;
	}

	// Read the metadata from disk
	(*pFile_) >> result;

	return true;
}

BW_END_NAMESPACE

// metadata.cpp
