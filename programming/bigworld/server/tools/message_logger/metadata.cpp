#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "metadata.hpp"

#include "cstdmf/log_meta_data.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

namespace MLMetadata
{

namespace
{
/**
 * Check whether the most recent operation on the memory stream was successful.
 * Generates a message if there was an error.
 *
 * @param stream a stream that has performed a recent operation
 * @return true if there was an error, false if not
 */
bool hasError( MemoryIStream & stream )
{
	if (stream.error())
	{
		ERROR_MSG( "MLMetadata::hasError: Unexpected error while"
		            " reading message metadata\n");
		return true;
	}
	return false;
}

} // anonymous namespace

/**
 * Create the MLDB metadata object
 */
MLDBMetadataStream::MLDBMetadataStream() : jsonAdded_( false )
{
}


/**
 * Add a key and accompanying int value to the metadata object
 *
 * @param key metadata key
 * @param value metadata value
 * @return this stream, suitable for further chained operations
 */
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key, int value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value;
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
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key,
											long long int value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value;
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
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key,
											unsigned int value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value;
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
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key,
											float value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value;
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
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key,
											double value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value;
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
LogMetadataStream & MLDBMetadataStream::add( const BW::string & key,
											const BW::string & value )
{
	if (!jsonAdded_)
	{
		root_[ key.c_str() ] = value.c_str();
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
bool MLDBMetadataStream::setJSON( const BW::string & json )
{
	// Replace everything with the input.
	if (!jsonAdded_)
	{
		Json::Reader reader;
		root_.clear();
		// If the JSON is invalid, parse will return false; if it's valid,
		// want to block future updates.
		jsonAdded_ = reader.parse( json.c_str(), root_ );
	}
	return jsonAdded_;
}


/**
 * Return the JSON representation of this metadata object, ready to store in
 * MLDB.
 *
 * @return the JSON metadata string
 */
BW::string MLDBMetadataStream::getJSON()
{
	Json::FastWriter writer;
	std::string result = writer.write( root_ );
	return BW::string( result.begin(), result.end() );
}


/**
 * Fix any UTF8 encoding issues within the string
 *
 * @param toClean the string to clean in-place.
 */
void LogMetadataBuilder::cleanString( BW::string & toClean )
{
	if (!isValidUtf8Str( toClean.c_str() ))
	{
		BW::string utf8Str;
		toValidUtf8Str( toClean.c_str(), utf8Str );
		toClean = utf8Str;
	}
}


/**
 * Process the input stream to extract the metadata components. Writes the
 * results to the output stream, which should contain a complete metadata
 * structure when done.
 *
 * @param inputStream the stream to read the data from
 * @param outputStream the stream to write the metadata to
 */
void LogMetadataBuilder::process( MemoryIStream & inputStream,
								LogMetadataStream & outputStream)
{
	LogMetaData::MessageLoggerValueType type;
	BW::string key;

	while (inputStream.remainingLength())
	{
		// Read the key off
		inputStream >> type;
		if (hasError( inputStream ))
		{
			return;
		}

		if (type != LogMetaData::MLVT_STRING)
		{
			ERROR_MSG( "LogMetadataBuilder::process: "
				"Key expected as string (read %d instead). "
					"Terminating metadata processing.\n", type );
			return;
		}

		inputStream >> key;
		inputStream >> type;
		if (hasError( inputStream ))
		{
			return;
		}
		cleanString( key );

		if (type == LogMetaData::MLVT_JSON_STRING)
		{
			if (strcmp( key.c_str(), LogMetaData::key_jsonString ) != 0)
			{
				ERROR_MSG( "LogMetadataBuilder::process: "
					"JSON string provided for key (%s).\n", key.c_str() );
			}

			BW::string buffer;
			inputStream >> buffer;
			if (hasError( inputStream ))
			{
				return;
			}
			cleanString( buffer );
			if (outputStream.setJSON( buffer ))
			{
				// A valid JSON argument overrides all others, so nothing left
				// to process.
				return;
			}
		}
		else if (type == LogMetaData::MLVT_INT)
		{
			int iv; inputStream >> iv;
			if (hasError( inputStream ))
			{
				return;
			}
			outputStream.add( key, iv );
		}
		else if (type == LogMetaData::MLVT_LONG_LONG_INT)
		{
			long long int liv; inputStream >> liv;
			if (hasError( inputStream ))
			{
				return;
			}
			outputStream.add( key, liv );
		}
		else if (type == LogMetaData::MLVT_UNSIGNED_INT)
		{
			unsigned int uiv; inputStream >> uiv;
			if (hasError( inputStream ))
			{
				return;
			}
			outputStream.add( key, uiv );

		}
		else if (type == LogMetaData::MLVT_STRING)
		{
			BW::string buffer;
			inputStream >> buffer;
			if (hasError( inputStream ))
			{
				return;
			}
			cleanString( buffer );
			outputStream.add( key, buffer );
		}
		else if (type == LogMetaData::MLVT_FLOAT)
		{
			float fv; inputStream >> fv;
			if (hasError( inputStream ))
			{
				return;
			}
			outputStream.add( key, fv );
		}
		else if (type == LogMetaData::MLVT_DOUBLE)
		{
			double dv; inputStream >> dv;
			if (hasError( inputStream ))
			{
				return;
			}
			outputStream.add( key, dv );
		}
		else
		{
			ERROR_MSG( "Unknown metadata type: %d\n", type );
			BW::string empty;
			outputStream.add( key, empty );
		}
	}
}

} // namespace MLMetadata

BW_END_NAMESPACE

// metadata.cpp
