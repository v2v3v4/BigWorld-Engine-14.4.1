#ifndef ML_METADATA_MONGODB_HPP
#define ML_METADATA_MONGODB_HPP

#include "../metadata.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "mongo/client/dbclient.h"

BW_BEGIN_NAMESPACE

namespace MLMetadata
{

class MongoDBMetadataStream : public MLMetadata::LogMetadataStream
{
public:
	MongoDBMetadataStream();
	MLMetadata::LogMetadataStream & add( const BW::string & key, int value );
	MLMetadata::LogMetadataStream & add( const BW::string & key,
										long long int value );
	MLMetadata::LogMetadataStream & add( const BW::string & key,
										unsigned int value );
	MLMetadata::LogMetadataStream & add( const BW::string & key,
										float value );
	MLMetadata::LogMetadataStream & add( const BW::string & key,
										double value );
	MLMetadata::LogMetadataStream & add( const BW::string & key,
										const BW::string & value );
	bool setJSON( const BW::string & json );
	BW::string getJSON();
	mongo::BSONObj getBSON();

private:
	// Need two BSONObjBuilders because there's no way to clear it if a JSON
	// string is added midway.
	mongo::BSONObjBuilder binaryOutput_;
	mongo::BSONObjBuilder binaryOutputJSON_;

	bool jsonAdded_;
};

}

BW_END_NAMESPACE


#endif // ML_METADATA_MONGODB_HPP
