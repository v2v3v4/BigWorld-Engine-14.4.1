#include "pch.hpp"

#include "cstdmf/bit_reader.hpp"
#include "cstdmf/bit_writer.hpp"
#include "cstdmf/memory_stream.hpp"


BW_BEGIN_NAMESPACE

TEST( BitReader_bitsRequired )
{	
	CHECK_EQUAL( 0, BitReader::bitsRequired( 0 ) );
	CHECK_EQUAL( 0, BitReader::bitsRequired( 1 ) );
	CHECK_EQUAL( 1, BitReader::bitsRequired( 2 ) );
	CHECK_EQUAL( 2, BitReader::bitsRequired( 3 ) );
	CHECK_EQUAL( 2, BitReader::bitsRequired( 4 ) );
	CHECK_EQUAL( 3, BitReader::bitsRequired( 5 ) );
	CHECK_EQUAL( 3, BitReader::bitsRequired( 8 ) );
	CHECK_EQUAL( 4, BitReader::bitsRequired( 9 ) );
	CHECK_EQUAL( 4, BitReader::bitsRequired( 16 ) );

	for (int i = 2; i < 32; ++i)
	{
		CHECK_EQUAL( i, BitReader::bitsRequired( 1 << i ) );
		CHECK_EQUAL( i + 1, BitReader::bitsRequired( (1 << i) + 1 ) );
	}
}

TEST( BitReader_readBits )
{
	BitWriter writer;

	for (int i = 1; i < 32; ++i)
	{
		writer.add( i, i );
	}

	MemoryOStream stream;
	stream.addBlob( writer.bytes(), writer.usedBytes() );

	BitReader reader( stream );

	for (int i = 1; i < 32; ++i)
	{
		CHECK_EQUAL( i, reader.get( i ) );
	}
}

BW_END_NAMESPACE

// test_bit_reader.cpp
