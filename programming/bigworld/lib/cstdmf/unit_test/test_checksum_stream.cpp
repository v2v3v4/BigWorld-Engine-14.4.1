#include "pch.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/checksum_stream.hpp"

#include <string.h>


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{

const char BLOB_DATA[] = "Here is a blob";
const int32 testInt = 0xdeadbeef;
const BW::string testString( "some data" );

} // end namespace (anonymous)


TEST( XORChecksumScheme )
{
	MemoryOStream base;

	ChecksumSchemePtr pScheme = XORChecksumScheme::create();

	uint32 checksumWritten = 0;

	{
		ChecksumOStream writer( base, pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		// Actually not required if writer is destructed, but test here
		// anyway.

		MemoryOStream checksum;
		writer.finalise( &checksum );

		checksumWritten = *reinterpret_cast< const uint32 * >(
			checksum.retrieve( sizeof( uint32 ) ) );
	}

	base.rewind();
	pScheme = XORChecksumScheme::create();

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		MemoryOStream checksum;
		CHECK( reader.verify( &checksum ) );

		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );

		CHECK_EQUAL(
			checksumWritten,
			*reinterpret_cast< const uint32 * >(
				checksum.retrieve( sizeof( uint32 ) ) ) );
	}
}


TEST( MD5SumScheme )
{
	MemoryOStream base;

	ChecksumSchemePtr pScheme = MD5SumScheme::create();

	{
		ChecksumOStream writer( base, pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		int sizeBeforeFinalising = writer.size();

		// When writer is destructed, finalise() is called automatically, test
		// here anyway so we can check sizes.
		writer.finalise();

		size_t checksumSize = writer.size() - sizeBeforeFinalising;

		CHECK_EQUAL( pScheme->streamSize(), checksumSize );
	}

	base.rewind();
	pScheme = MD5SumScheme::create();

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		CHECK( reader.verify() );

		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}
}


TEST( MD5Sum_WithBufferedChecksumOStream )
{
	MemoryOStream base;

	ChecksumSchemePtr pScheme = MD5SumScheme::create();

	{
		// Force the ChecksumOStream to use buffering.

		ChecksumOStream writer( static_cast< BinaryOStream & >( base ),
			pScheme );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		int sizeBeforeFinalising = writer.size();

		// When writer is destructed, finalise() is called automatically, test
		// here anyway so we can check sizes.
		writer.finalise();

		size_t checksumSize = writer.size() - sizeBeforeFinalising;

		CHECK_EQUAL( pScheme->streamSize(), checksumSize );
	}

	base.rewind();
	pScheme = MD5SumScheme::create();

	{
		ChecksumIStream reader( base, pScheme );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		CHECK( reader.verify() );

		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}
}


TEST( NULLScheme )
{
	MemoryOStream base;

	{
		ChecksumOStream writer( base, /* pChecksumScheme */ NULL );
		BinaryOStream & stream = writer;
		stream << testInt << testString;

		stream.addBlob( BLOB_DATA, sizeof( BLOB_DATA ) );

		MemoryOStream checksum;
		writer.finalise( &checksum );

		CHECK_EQUAL( 0, checksum.remainingLength() );
	}

	base.rewind();

	{
		ChecksumIStream reader( base, /*pChecksumScheme = */ NULL );

		CHECK( !reader.error() );

		int32 testIntActual = 0;
		BW::string testStringActual;
		reader >> testIntActual >> testStringActual;

		CHECK_EQUAL( testIntActual, testInt );
		CHECK_EQUAL( testStringActual, testString );
		CHECK_EQUAL( reader.remainingLength(), int( sizeof( BLOB_DATA ) ) );

		const void * blobData = reader.retrieve( reader.remainingLength() );
		CHECK_EQUAL( memcmp( blobData, BLOB_DATA, sizeof( BLOB_DATA ) ), 0 );

		MemoryOStream checksum;
		CHECK( reader.verify( &checksum ) );
		CHECK_EQUAL( 0, checksum.remainingLength() );

		CHECK( reader.remainingLength() == 0 );
		CHECK( !reader.error() );

		CHECK( base.remainingLength() == 0 );
		CHECK( !base.error() );
	}
}


BW_END_NAMESPACE

// test_checksum_stream.cpp
