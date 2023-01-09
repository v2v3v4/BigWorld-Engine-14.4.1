#include "pch.hpp"

#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

TEST( UTF8 )
{
	//const char* russian = "Привет";
	const char russian[] = {	
		(char)0xd0, (char)0x9f, (char)0xd1, (char)0x80, (char)0xd0, (char)0xb8, 
		(char)0xd0, (char)0xb2, (char)0xd0, (char)0xb5, (char)0xd1, (char)0x82, 
		(char)0x00 
	};

	//const char* japanese = "こんにちは、お元気ですか？";
	const char japanese[] = {
		(char)0xe3, (char)0x81, (char)0x93, (char)0xe3, (char)0x82, (char)0x93, 
		(char)0xe3, (char)0x81, (char)0xab, (char)0xe3, (char)0x81, (char)0xa1, 
		(char)0xe3, (char)0x81, (char)0xaf, (char)0xe3, (char)0x80, (char)0x81, 
		(char)0xe3, (char)0x81, (char)0x8a, (char)0xe5, (char)0x85, (char)0x83, 
		(char)0xe6, (char)0xb0, (char)0x97, (char)0xe3, (char)0x81, (char)0xa7, 
		(char)0xe3, (char)0x81, (char)0x99, (char)0xe3, (char)0x81, (char)0x8b, 
		(char)0xef, (char)0xbc, (char)0x9f, (char)0x00
	};

	const char invalidUtf8[] = { 'a', (char)0xFE, 'b', '\0'};
	char * convertedUtf8 = "a0xFEb";

	CHECK_EQUAL( 1u, utf8ParseLeadByte( '\0' ) );
	CHECK_EQUAL( 0u, utf8ParseLeadByte( (char)0xFF ) );
	CHECK( !utf8IsContinuationByte( '\0' ) );
	CHECK( !utf8IsContinuationByte( (char)0xFF ) );

	// check all positive 7-bit values
	for (int c = 0; c < 128; ++c)
	{
		CHECK_EQUAL( 1u, utf8ParseLeadByte( (char)c ) );
	}

	CHECK_EQUAL( 2u, utf8ParseLeadByte( russian[0] ) );
	CHECK_EQUAL( 3u, utf8ParseLeadByte( japanese[0] ) );
	
	CHECK( !utf8IsContinuationByte(japanese[0]) );
	CHECK( utf8IsContinuationByte(japanese[1]) );
	CHECK_EQUAL( 0u, utf8ParseLeadByte( japanese[1] ) );

	CHECK_EQUAL( 6u, utf8StringLength(russian) );
	CHECK_EQUAL( 13u, utf8StringLength(japanese) );

	CHECK( isValidUtf8Str( japanese ) );
	CHECK( isValidUtf8Str( russian ) );

	CHECK_EQUAL( (const char *)NULL, findNextNonUtf8Char( japanese ));
	CHECK_EQUAL( (const char *)NULL, findNextNonUtf8Char( russian ));
	CHECK_EQUAL( invalidUtf8 + 1, findNextNonUtf8Char( invalidUtf8 ) );

	BW::string utf8Str;
	toValidUtf8Str( invalidUtf8, utf8Str );
	CHECK_EQUAL( 0, strcmp( convertedUtf8, utf8Str.c_str() ) );
}

TEST( HexCharConv )
{
	// check valid chars
	CHECK_EQUAL( 0, bw_hextodec( '0' ) );
	CHECK_EQUAL( 1, bw_hextodec( '1' ) );
	CHECK_EQUAL( 2, bw_hextodec( '2' ) );
	CHECK_EQUAL( 3, bw_hextodec( '3' ) );
	CHECK_EQUAL( 4, bw_hextodec( '4' ) );
	CHECK_EQUAL( 5, bw_hextodec( '5' ) );
	CHECK_EQUAL( 6, bw_hextodec( '6' ) );
	CHECK_EQUAL( 7, bw_hextodec( '7' ) );
	CHECK_EQUAL( 8, bw_hextodec( '8' ) );
	CHECK_EQUAL( 9, bw_hextodec( '9' ) );

	CHECK_EQUAL( 10, bw_hextodec( 'a' ) );
	CHECK_EQUAL( 11, bw_hextodec( 'b' ) );
	CHECK_EQUAL( 12, bw_hextodec( 'c' ) );
	CHECK_EQUAL( 13, bw_hextodec( 'd' ) );
	CHECK_EQUAL( 14, bw_hextodec( 'e' ) );
	CHECK_EQUAL( 15, bw_hextodec( 'f' ) );

	CHECK_EQUAL( 10, bw_hextodec( 'A' ) );
	CHECK_EQUAL( 11, bw_hextodec( 'B' ) );
	CHECK_EQUAL( 12, bw_hextodec( 'C' ) );
	CHECK_EQUAL( 13, bw_hextodec( 'D' ) );
	CHECK_EQUAL( 14, bw_hextodec( 'E' ) );
	CHECK_EQUAL( 15, bw_hextodec( 'F' ) );

	// check invalid chars
	for (char c = 0; c < '0'; ++c)
	{
		CHECK_EQUAL( -1, bw_hextodec( c ) );
	}

	for (char c = '9'+1; c < 'A'; ++c)
	{
		CHECK_EQUAL( -1, bw_hextodec( c ) );
	}

	for (char c = 'F'+1; c < 'a'; ++c)
	{
		CHECK_EQUAL( -1, bw_hextodec( c ) );
	}

	for (int c = 'f'+1; c < 128; ++c)
	{
		CHECK_EQUAL( -1, bw_hextodec( (char)c ) );
	}

	for (char c = -128; c < 0; ++c)
	{
		CHECK_EQUAL( -1, bw_hextodec( c ) );
	}
}

TEST( WideHexCharConv )
{
	// check valid chars
	CHECK_EQUAL( 0, bw_hextodec( L'0' ) );
	CHECK_EQUAL( 1, bw_hextodec( L'1' ) );
	CHECK_EQUAL( 2, bw_hextodec( L'2' ) );
	CHECK_EQUAL( 3, bw_hextodec( L'3' ) );
	CHECK_EQUAL( 4, bw_hextodec( L'4' ) );
	CHECK_EQUAL( 5, bw_hextodec( L'5' ) );
	CHECK_EQUAL( 6, bw_hextodec( L'6' ) );
	CHECK_EQUAL( 7, bw_hextodec( L'7' ) );
	CHECK_EQUAL( 8, bw_hextodec( L'8' ) );
	CHECK_EQUAL( 9, bw_hextodec( L'9' ) );

	CHECK_EQUAL( 10, bw_hextodec( L'a' ) );
	CHECK_EQUAL( 11, bw_hextodec( L'b' ) );
	CHECK_EQUAL( 12, bw_hextodec( L'c' ) );
	CHECK_EQUAL( 13, bw_hextodec( L'd' ) );
	CHECK_EQUAL( 14, bw_hextodec( L'e' ) );
	CHECK_EQUAL( 15, bw_hextodec( L'f' ) );

	CHECK_EQUAL( 10, bw_hextodec( L'A' ) );
	CHECK_EQUAL( 11, bw_hextodec( L'B' ) );
	CHECK_EQUAL( 12, bw_hextodec( L'C' ) );
	CHECK_EQUAL( 13, bw_hextodec( L'D' ) );
	CHECK_EQUAL( 14, bw_hextodec( L'E' ) );
	CHECK_EQUAL( 15, bw_hextodec( L'F' ) );

	// check invalid chars
	for (wchar_t c = 0; c < L'0'; ++c)
	{
		CHECK_EQUAL( -1, bw_whextodec( c ) );
	}

	for (wchar_t c = L'9'+1; c < L'A'; ++c)
	{
		CHECK_EQUAL( -1, bw_whextodec( c ) );
	}

	for (wchar_t c = L'F'+1; c < L'a'; ++c)
	{
		CHECK_EQUAL( -1, bw_whextodec( c ) );
	}

	for (wchar_t c = L'f'+1; c < 0xFFFF; ++c) // wchar_t is unsigned.
	{
		CHECK_EQUAL( -1, bw_whextodec( c ) );
	}
}


TEST( bw_str_copy )
{
	const char ABCD[] = "abcd";
	char out2[2] = {0};
	char out4[4] = {0};
	char out8[8] = {0};

	char * result = NULL;

	// should truncate
	result = bw_str_copy( out2, ARRAY_SIZE( out2 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, "a" ) );

	// should truncate
	result = bw_str_copy( out4, ARRAY_SIZE( out4 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, "abc" ) );

	// should fit
	result = bw_str_copy( out8, ARRAY_SIZE( out8 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, ABCD ) );
}

TEST( bw_str_append )
{
	const char ABCD[] = "abcd";
	const char EFGH[] = "efgh";
	char out4[4] = {0};
	char out8[8] = {0};
	char out16[16] = {0};
	char * result;

	// should truncate
	result = bw_str_append( out4, ARRAY_SIZE( out4 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, "abc" ) );

	// should truncate
	result = bw_str_append( out8, ARRAY_SIZE( out8 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, "abcd" ) );
	result = bw_str_append( out8, ARRAY_SIZE( out8 ), EFGH, ARRAY_SIZE( EFGH ) );
	CHECK_EQUAL( 0, strcmp( result, "abcdefg" ) );

	// everything should fit
	result = bw_str_append( out16, ARRAY_SIZE( out16 ), ABCD, ARRAY_SIZE( ABCD ) );
	CHECK_EQUAL( 0, strcmp( result, ABCD) );
	result = bw_str_append( out16, ARRAY_SIZE( out16 ), EFGH, ARRAY_SIZE( EFGH ) );
	CHECK_EQUAL( 0, strcmp( result, "abcdefgh" ) );
}

TEST( bw_str_substr )
{
	char a[] = "0123456789abcdefghij";
	char out8[8] = {0};
	char out32[32] = {0};
	char * result;

	out8[0] = 'z';
	result = bw_str_substr( out8, ARRAY_SIZE( out8 ), a, bw_str_len(a), 1 );
	CHECK_EQUAL( 0, strcmp( "", result )  );

	result = bw_str_substr( out32, ARRAY_SIZE( out32 ), a, 10, ARRAY_SIZE( a ) );
	CHECK_EQUAL( 0, strcmp( "abcdefghij", result ) );

	result = bw_str_substr( out32, ARRAY_SIZE( out32 ), a, 5, 3 );
	CHECK_EQUAL( 0, strcmp( "567", result ) );

	result = bw_str_substr( out32, ARRAY_SIZE( out32 ), a, 12, 100 );
	CHECK_EQUAL( 0, strcmp( "cdefghij", result ) );

	result = bw_str_substr( out32, ARRAY_SIZE( out32 ), a, bw_str_len( a ) - 3, 50);
	CHECK_EQUAL( 0, strcmp( "hij", result ) );
}

BW_END_NAMESPACE

// test_string_utils.cpp
