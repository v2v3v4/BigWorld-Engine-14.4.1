#ifndef ML_METADATA_HPP
#define ML_METADATA_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/memory_stream.hpp"

#include "jsoncpp/include/json/json.h"

BW_BEGIN_NAMESPACE

namespace MLMetadata
{

/**
 * Common interface to store interim metadata.
 */
class LogMetadataStream
{
public:
	virtual LogMetadataStream & add( const BW::string & key, int value ) = 0;
	virtual LogMetadataStream & add( const BW::string & key,
									long long int value ) = 0;
	virtual LogMetadataStream & add( const BW::string & key,
									unsigned int value ) = 0;
	virtual LogMetadataStream & add( const BW::string & key, float value ) = 0;
	virtual LogMetadataStream & add( const BW::string & key, double value ) = 0;
	virtual LogMetadataStream & add( const BW::string & key,
									const BW::string & value ) = 0;
	virtual bool setJSON( const BW::string & json ) = 0;
	virtual BW::string getJSON() = 0;
};

/**
 * MLDB-specific interface to store interim metadata.
 */
class MLDBMetadataStream : public LogMetadataStream
{
public:
	MLDBMetadataStream();
	LogMetadataStream & add( const BW::string & key, int value );
	LogMetadataStream & add( const BW::string & key, long long int value );
	LogMetadataStream & add( const BW::string & key, unsigned int value );
	LogMetadataStream & add( const BW::string & key, float value );
	LogMetadataStream & add( const BW::string & key, double value );
	LogMetadataStream & add( const BW::string & key, const BW::string & value );
	bool setJSON( const BW::string & json );
	BW::string getJSON();

private:
	bool jsonAdded_;
	Json::Value root_;
};

/**
 * Destreams key/value pairs and pushes them to a database-appropriate object
 * that marshals the metadata ready for writing.
 */
class LogMetadataBuilder
{
public:
	void process( MemoryIStream & input, LogMetadataStream & output );

private:
	void cleanString( BW::string & toClean );
};

};

BW_END_NAMESPACE

#endif // ML_METADATA_HPP
