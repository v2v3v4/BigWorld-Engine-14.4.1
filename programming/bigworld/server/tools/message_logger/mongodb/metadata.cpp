#include "metadata.hpp"

BW_BEGIN_NAMESPACE

namespace MLMetadata
{

/**
 * Create the MongoDB metadata object
 */
MongoDBMetadataStream::MongoDBMetadataStream() : jsonAdded_( false )
{
}

/**
 * Add a key and accompanying int value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
									const BW::string & key, int value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value;
	}
	return *this;
}

/**
 * Add a key and accompanying long long int value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
								const BW::string & key, long long int value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value;
	}
	return *this;
}

/**
 * Add a key and accompanying unsigned int value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
									const BW::string & key, unsigned int value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value;
	}
	return *this;
}

/**
 * Add a key and accompanying float value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
									const BW::string & key, float value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value;
	}
	return *this;
}

/**
 * Add a key and accompanying double value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
									const BW::string & key, double value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value;
	}
	return *this;
}

/**
 * Add a key and accompanying string value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
MLMetadata::LogMetadataStream & MongoDBMetadataStream::add(
							const BW::string & key, const BW::string & value )
{
	if (!jsonAdded_)
	{
		binaryOutput_ << key.c_str() << value.c_str();
	}
	return *this;
}

/**
 * Write a fully-formed JSON object to the metadata. Overwrites any existing
 * key/value pairs and prevents future additions.
 *
 * @param json a fully formed JSON string to save as metadata
 * @return whether the JSON was valid, and therefore whether the update
 * succeeded. Further updates are allowed on failure.
 */
bool MongoDBMetadataStream::setJSON( const BW::string & json )
{
	if (!jsonAdded_)
	{
		try
		{
			binaryOutputJSON_.appendElements(
										mongo::fromjson( json.c_str() ) );
			jsonAdded_ = true;
		}
		catch (mongo::MsgAssertionException)
		{
			// Invalid JSON provided
			return false;
		}
	}
	return jsonAdded_;
}

/**
 * Return the JSON representation of this metadata object
 *
 * @return the JSON metadata string
 */
BW::string MongoDBMetadataStream::getJSON()
{
	std::string temp = this->getBSON().jsonString();
	return BW::string( temp.begin(), temp.end() );
}

/**
 * Return the BSON representation of this metadata object, ready to store in
 * MongoDB.
 *
 * @return the BSON metadata object
 */
mongo::BSONObj MongoDBMetadataStream::getBSON()
{
	if (jsonAdded_)
	{
		return binaryOutputJSON_.obj();
	}
	else
	{
		return binaryOutput_.obj();
	}
}

} // namespace MLMetadata

BW_END_NAMESPACE
