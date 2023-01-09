#include "pch.hpp"

#include "cstdmf/md5.hpp"

#include <string.h>

/**
 *	Brief tests of the MD5 implementation against some known
 *	test data.
 */

BW_BEGIN_NAMESPACE

// Test vectors from:
// "Independent implementation of MD5 (RFC 1321)" by ghost@aladdin.com

TEST( MD5_emptyString )
{
	MD5 hash;

	MD5::Digest initialDigest = hash;
	CHECK_EQUAL( false, initialDigest.isEmpty() );
	CHECK_EQUAL( "D41D8CD98F00B204E9800998ECF8427E", initialDigest.quote() );
}


TEST( MD5_a )
{
	MD5 hash;

	hash.append( "a", static_cast<int>(strlen( "a" )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "0CC175B9C0F1B6A831C399E269772661", digest.quote() );
}


TEST( MD5_abc )
{
	MD5 hash;

	hash.append( "abc", static_cast<int>(strlen( "abc" )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "900150983CD24FB0D6963F7D28E17F72", digest.quote() );
}


TEST( MD5_message_digest )
{
	MD5 hash;

	hash.append( "message digest",
		static_cast<int>(strlen( "message digest" )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "F96B697D7CB7938D525A2F31AAF161D0", digest.quote() );
}


TEST( MD5_abcdefghijklmnopqrstuvwxyz )
{
	MD5 hash;

	hash.append( "abcdefghijklmnopqrstuvwxyz",
		static_cast<int>(strlen( "abcdefghijklmnopqrstuvwxyz" )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "C3FCD3D76192E4007DFB496CCA67E13B", digest.quote() );
}


TEST( MD5_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 )
{
	MD5 hash;

	const char TEST_STR[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	hash.append( TEST_STR, static_cast<int>(strlen( TEST_STR )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "D174AB98D277D9F5A5611C2C9F419D9F", digest.quote() );
}


TEST( MD5_12345678901234567890123456789012345678901234567890123456789012345678901234567890 )
{
	MD5 hash;

	const char TEST_STR[] =
		"123456789012345678901234567890"
		"123456789012345678901234567890"
		"12345678901234567890";
	hash.append( TEST_STR, static_cast<int>(strlen( TEST_STR )) );
	MD5::Digest digest = hash;

	CHECK_EQUAL( false, digest.isEmpty() );
	CHECK_EQUAL( "57EDF4A22BE3C955AC49DA2E2107B67A", digest.quote() );
}


BW_END_NAMESPACE

// test_md5.cpp
