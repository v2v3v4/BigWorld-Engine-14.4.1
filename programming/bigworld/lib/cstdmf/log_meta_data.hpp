#ifndef BW_LOG_META_DATA_HPP
#define BW_LOG_META_DATA_HPP

#include "bw_namespace.hpp"
#include "bw_string.hpp"
#include "cstdmf_dll.hpp"

#if !defined( _WIN32 )
#include <unistd.h>
#endif

BW_BEGIN_NAMESPACE

class MemoryOStream;


/**
 *	This class is a collection of metadata key/value pairs.
 *
 *	The primary purpose of this class is to stream key/value pairs into a
 *	format that can be transmitted to MessageLogger.
 */
class LogMetaData
{
public:

	/**
	 *	This enumeration describes the set of known metadata value types that
	 *	are able to be serialised.
	 */
	typedef char MessageLoggerValueType;
	static const MessageLoggerValueType MLVT_JSON_STRING;
	static const MessageLoggerValueType MLVT_INT;
	static const MessageLoggerValueType MLVT_UNSIGNED_INT;
	static const MessageLoggerValueType MLVT_LONG_LONG_INT;
	static const MessageLoggerValueType MLVT_STRING;
	static const MessageLoggerValueType MLVT_FLOAT;
	static const MessageLoggerValueType MLVT_DOUBLE;


	CSTDMF_DLL LogMetaData();
	CSTDMF_DLL ~LogMetaData();

	/**
	 *	This templatised method adds a key/value pair to the metadata object.
	 *
	 *	A NULL (or empty) key will cause the addition to be ignored.
	 *	A NULL value (eg: a ptr to a NULL) will be added as an empty string.
	 *
	 *	@param pKey A pointer to the key to add.
	 *	@param value The value to add as metadata.
	 */
	template< class VALUE_TYPE >
	void add( const char * pKey, const VALUE_TYPE & value )
	{
		if ((pKey == NULL) || (pKey[0] == '\0'))
		{
			// Ideally this error should be logged, however we are most likely
			// already within a logging stack. Still possible, just need to
			// test behaviour.
			return;
		}

		this->streamKey( pKey );
		this->streamValue( value );
	}

	void addJSON( const char * pJSON );
	CSTDMF_DLL const char * getRawJSON() const;
	CSTDMF_DLL const MemoryOStream * pData() const;

	static const char * key_stackTrace;
	static const char * key_jsonString;
private:
	void streamKey( const char * pKey );

	void streamValue( int value );
	void streamValue( unsigned int value );
	void streamValue( long long value );
	void streamValue( const char * pValue );
	void streamValue( float value );
	void streamValue( double value );

	// pData is a ptr to allow a forward declaration of MemoryOStream.
	// This is being forced by debug.hpp and circular dependencies in
	// binary_stream.hpp trying to use MF_ASSERT
	MemoryOStream * pData_;

	const char * pRawJSON_;
};

BW_END_NAMESPACE

#endif // BW_LOG_META_DATA_HPP
