#ifndef METADATA_MLDB_HPP
#define METADATA_MLDB_HPP

#include "../binary_file_handler.hpp"
#include "../types.hpp"

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE


/**
 * This class handles the key/value metadata logs
 */
class MetadataMLDB : public BinaryFileHandler
{
public:
	MetadataMLDB();
	~MetadataMLDB();

	bool init( const char *pUserLogDir, const char *pSuffix,
		const char *pMode );

	bool writeMetadataToMLDB( const BW::string & metadata );

	bool readFromOffset( MessageLogger::MetaDataOffset offset,
		BW::string & result );

private:

	/*
	 * This method is not required or for this class as it doesn't perform
	 * a bulk read operation. readFromOffset() provides the read interface
	 * that is utilised by the dependant reading code.
	 */
	bool read() { return true; }

};

BW_END_NAMESPACE

#endif // METADATA_MLDB_HPP
