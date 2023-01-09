#include "pch.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE


TEST( BWSNPrintf_test )
{
	char buf[10];

	const char TEST_9[] = "012345678";
	const char TEST_10[] = "0123456789";

	// Test with 9 characters + NULL terminator = 10 characters
	int res = bw_snprintf( buf, sizeof(buf), "%s", TEST_9 );
	CHECK_EQUAL( 9, res );
	CHECK_EQUAL( '\0', buf[9] );
	CHECK_EQUAL( BW::string( TEST_9 ), BW::string( buf ) );

	res = bw_snprintf( buf, sizeof(buf), "%s", TEST_10 );
	CHECK_EQUAL( 10, res );
	CHECK_EQUAL( '\0', buf[9] );
	// The 10th character of TEST_10 will have been truncated.
	CHECK_EQUAL( BW::string( TEST_9 ), BW::string( buf ) );
}

TEST( BWSNPrintf_test_zerosize )
{
	const char TEST_STR[] = "01234567";
	
	char buf[10] = {0};
	memset( buf, 0xFF, ARRAY_SIZE( buf ));

	int res = bw_snprintf( buf, 0, "%s", TEST_STR );

	// Make sure it still outputs how much space is needed
	CHECK_EQUAL( static_cast<int>(ARRAY_SIZE(TEST_STR) - 1), res );
	// Make sure it hasn't written to any characters in the buffer
	for (size_t i = 0; i < ARRAY_SIZE(buf); ++i)
	{
		CHECK_EQUAL( 0xFFu, (uint8)buf[i] );
	}
}

BW_END_NAMESPACE

// test_bw_snprintf.cpp
