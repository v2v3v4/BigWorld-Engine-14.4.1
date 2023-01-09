#include "pch.hpp"

#include "log_meta_data.hpp"

#include "memory_stream.hpp"

BW_BEGIN_NAMESPACE

/*static*/ const char * LogMetaData::key_stackTrace = "__bw_stack";
/*static*/ const char * LogMetaData::key_jsonString = "__bw_json";

/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_JSON_STRING	= 'j';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_INT			= 'i';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_UNSIGNED_INT	= 'u';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_LONG_LONG_INT	= 'l';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_STRING			= 's';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_FLOAT			= 'f';
/*static*/ const LogMetaData::MessageLoggerValueType LogMetaData::MLVT_DOUBLE			= 'd';


/**
 *	Default Constructor.
 */
LogMetaData::LogMetaData() :
	pData_( NULL ), pRawJSON_( NULL )
{ }


/**
 *	Destructor.
 */
LogMetaData::~LogMetaData()
{
	bw_safe_delete( pData_ );
}


/**
 *	This method streams the provided string as a special JSON string.
 *
 *	This method is only intended to be utilised as a support to the Python
 *	API layer which currently passes JSON through directly.
 *
 *	@param pJSON A pointer to the JSON string to stream. Must remain allocated
 *  for the lifetime of this LogMetaData object.
 */
void LogMetaData::addJSON( const char * pJSON )
{
	// TODO: This could possibly become an MF_ASSERT_DEV?
	if (!pJSON)
	{
		return;
	}

	// TODO: if an empty string is sent through, it seems to break the ML
	//       receiving side. Need to do a GDB here to track down what is
	//       causing this. for now just don't send it.
	if (strlen( pJSON ) == 0)
	{
		return;
	}

	// Store the raw string value in case an older message logger wants it
	if (pRawJSON_)
	{
		WARNING_MSG( "LogMetaData::addJSON: raw JSON has already been added, "
				"ignoring additional attempts. Existing value is: %s\n",
				pRawJSON_ );
		return;
	}
	// Not taking a copy because the Python call currently guarantees that it
	// will be allocated for as long as we need it.
	pRawJSON_ = pJSON;

	if (pData_ == NULL)
	{
		pData_ = new MemoryOStream;
	}


	(*pData_) <<
		// The key
		MLVT_STRING << LogMetaData::key_jsonString <<
		// The value
		MLVT_JSON_STRING << pJSON;
}

/**
 * This method returns a pointer to the raw JSON string passed in via addJSON.
 * Only returns a value if one was passed in, and does not translate
 * entries that were added via streamValue.
 *
 * @return A pointer to the string.
 */
const char * LogMetaData::getRawJSON() const
{
	return pRawJSON_;
}

/**
 *	This method applies the specified key to the metadata stream.
 *
 *	@param pKey The key to stream into the metadata.
 */
void LogMetaData::streamKey( const char * pKey )
{
	if (pData_ == NULL)
	{
		pData_ = new MemoryOStream;
	}

	(*pData_) << MLVT_STRING << pKey;
}


/**
 *	This method streams the provided integer to the metadata stream.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( int value )
{
	(*pData_) << MLVT_INT << value;
}


/**
 *	This method streams the provided long long to the metadata stream.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( long long value )
{
	(*pData_) << MLVT_LONG_LONG_INT << value;
}


/**
 *	This method streams the provided unsigned integer to the metadata stream.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( unsigned int value )
{
	(*pData_) << MLVT_UNSIGNED_INT << value;
}


/**
 *	This method streams the provided string to the metadata stream.
 *
 *	If a NULL pointer is provided, this method will stream an empty string
 *	in its place.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( const char * pValue )
{
	if (pValue)
	{
		(*pData_) << MLVT_STRING << pValue;
	}
	else
	{
		// TODO: potentially should consider this an MF_ASSERT_DEV case
		//       however, we are already within a message log stack at
		//       this point. do we want to start a new stack process?
		(*pData_) << MLVT_STRING << "";
	}
}


/**
 *	This method streams the provided float to the metadata stream.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( float value )
{
	(*pData_) << MLVT_FLOAT << value;
}


/**
 *	This method streams the provided double to the metadata stream.
 *
 *	@param value The value to stream into the metadata.
 */
void LogMetaData::streamValue( double value )
{
	(*pData_) << MLVT_DOUBLE << value;
}


/**
 *	This method returns a pointer to the MemoryOStream containing the metadata.
 *
 *	@return A pointer to the metadata stream.
 */
const MemoryOStream * LogMetaData::pData() const
{
	return pData_;
}

BW_END_NAMESPACE

// log_meta_data.cpp
