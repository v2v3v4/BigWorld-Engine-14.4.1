#include "blob_or_null.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

int BlobOrNull_token = 0;

/**
 *	This function is used to serialise a potentially NULL blob into a stream.
 */
BinaryOStream & operator<<( BinaryOStream & stream, const BlobOrNull & blob )
{
	if (blob.pData() && blob.length())
	{
		stream.appendString( blob.pData(), blob.length() );
	}
	else	// NULL value or just empty string
	{
		stream.appendString( "", 0 );
		stream << uint8(!blob.isNull());
	}

	return stream;
}

/**
 *	This function is used to deserialise a potentially NULL blob from a stream.
 */
BinaryIStream & operator>>( BinaryIStream & stream, BlobOrNull & blob )
{
	int length = stream.readStringLength();

	if (length > 0)
	{
		blob = BlobOrNull( (char*) stream.retrieve( length ), length );
	}
	else
	{
		uint8 isNotNull;
		stream >> isNotNull;

		blob = isNotNull ? BlobOrNull( "", 0 ) : BlobOrNull();
	}

	return stream;
}

BW_END_NAMESPACE

// blob_or_null.cpp
