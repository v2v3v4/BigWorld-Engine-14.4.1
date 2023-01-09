#include "pch.hpp"

#include "cstdmf/bw_string_ref.hpp"

namespace BW {

TEST( StringRef_empty )
{
	const StringRef empty;
	CHECK( empty.empty() );
	CHECK_EQUAL( 0u, empty.size() );
	CHECK_EQUAL( 0u, empty.length() );
	CHECK( empty.data() != NULL );
	CHECK( empty == string() );
}

TEST( StringRef_compare )
{
	char LIT_STR[] = "string_literal";
	char LIT_SIZE = ARRAY_SIZE(LIT_STR);
	string bwstr( LIT_STR );

	StringRef cstr_a( LIT_STR );
	StringRef cstr_b( LIT_STR );
	CHECK_EQUAL( 0, cstr_a.compare( cstr_b ) );
	CHECK( cstr_a == cstr_b );
	CHECK( cstr_a == StringRef( LIT_STR ) );

	StringRef buf_a( LIT_STR, LIT_SIZE - 1 );
	StringRef buf_b( LIT_STR, LIT_SIZE - 1 );
	CHECK_EQUAL( 0, buf_a.compare( buf_b ) );
	CHECK_EQUAL( 0, buf_a.compare( cstr_b ) );
	CHECK( buf_a == buf_b );
	CHECK( buf_a == cstr_b );

	StringRef str_a( bwstr );
	StringRef str_b( bwstr );
	CHECK_EQUAL( 0, str_a.compare( str_b ) );
	CHECK( str_a == str_b );
	CHECK( str_a == std::string( LIT_STR ) );

	StringRef abc( "abc" );
	StringRef def( "def" );

	CHECK( abc != def );
	CHECK( abc <= def );
	CHECK( abc <= abc );
	CHECK( def >= def );
	CHECK( def >= abc );
	CHECK( abc < def );
	CHECK( def > abc );

	StringRef ab( "ab" );
	CHECK( abc > ab );
	CHECK( ab < abc );
}

TEST( StringRef_compare_lower )
{
	StringRef abc( "abc" );
	StringRef ABC( "ABC" );
	StringRef def( "def" );
	StringRef DEF( "DEF" );

	CHECK_EQUAL( 0, abc.compare_lower( ABC ) );
	CHECK_EQUAL( 0, ABC.compare_lower( abc ) );

	CHECK( abc.equals_lower( ABC ) );
	CHECK( ABC.equals_lower( abc ) );
	CHECK( !abc.equals_lower( def ) );

	CHECK( abc.compare_lower(DEF) < 0 );
	CHECK( ABC.compare_lower(def) < 0 );
	CHECK( def.compare_lower(ABC) > 0 );
	CHECK( DEF.compare_lower(abc) > 0 );

	CHECK( abc.substr( 0, 2 ).compare_lower( ABC ) < 0 );
	CHECK( abc.compare_lower( ABC.substr( 0, 2 ) ) > 0 );
}

TEST( StringRef_substr )
{
	StringRef a("0123456789abcdefghij");
	
	StringRef sub1 = a.substr(10);
	CHECK_EQUAL( 10u, sub1.length() );
	CHECK_EQUAL( 0, memcmp( "abcdefghij", sub1.data(), 10 ) );

	StringRef sub2 = a.substr(5, 3);
	CHECK_EQUAL( 3u, sub2.length() );
	CHECK_EQUAL( 0, memcmp( "567", sub2.data(), 3 ) );

	StringRef sub3 = a.substr(12, 100);
	CHECK_EQUAL( 8u, sub3.length() );
	CHECK_EQUAL( 0, memcmp( "cdefghij", sub3.data(), 8 ) );

	StringRef sub4 = a.substr(a.size()-3, 50);
	CHECK_EQUAL( 3u, sub4.length() );
	CHECK_EQUAL( 0, memcmp( "hij", sub4.data(), 3 ) );
}

TEST( StringRef_find )
{
	const StringRef s("This is a string");

	// search from beginning of string
	CHECK_EQUAL( 2u, s.find( StringRef( "is" ) ) );

	// search from position 5
	CHECK_EQUAL( 5u, s.find( StringRef( "is" ), 5) );

	// find a single character
	CHECK_EQUAL( 8u, s.find( 'a' ) );

	// find a single character
	CHECK_EQUAL( StringRef::npos, s.find( 'q' ) );

	// check longer substring
	CHECK_EQUAL( StringRef::npos, StringRef( "ab" ).find( "abc" ) );

	CHECK_EQUAL( 0, WStringRef( L"ABC" ).compare_lower( L"abc" ) );
}

TEST( StringRef_rfind )
{
	const StringRef s("This is a string");

	// search backwards from beginning of string
	CHECK_EQUAL( 5u, s.rfind( StringRef( "is" ) ) );

	// search backwards from position 4
	CHECK_EQUAL( 2u, s.rfind( StringRef( "is" ), 4 ) );

	// find a single character
	CHECK_EQUAL( 10u, s.rfind( 's' ) );

	// find a single character
	CHECK_EQUAL( StringRef::npos, s.rfind( 'q' ) );

	// check substring at start
	CHECK_EQUAL( 0u, StringRef( "abcdef" ).rfind( "ab" ) );

	// check substring at end
	CHECK_EQUAL( 4u, StringRef( "abcdef" ).rfind( "ef" ) );
}

TEST( StringRef_find_first_of )
{
	// the test string
	const StringRef str = StringRef( "Hello World!" );

	// strings and chars to search for
	const StringRef search_str = StringRef( "o" );
	const char* search_cstr = "Good Bye!";

	CHECK_EQUAL( 4u, str.find_first_of( search_str ) );
	CHECK_EQUAL( 7u, str.find_first_of( search_str, 5 ) );
	CHECK_EQUAL( 1u, str.find_first_of( search_cstr ) );
	CHECK_EQUAL( 4u, str.find_first_of( search_cstr, 0, 4 ) );
	// 'x' is not in "Hello World', thus it will return std::string::npos
	CHECK_EQUAL( StringRef::npos, str.find_first_of( 'x' ) );
}

TEST( StringRef_find_first_not_of )
{
	// the test string
	const StringRef str = StringRef( "Hello World!" );

	// strings and chars to search for
	const StringRef search_str = StringRef( "lo " );
	const char* search_cstr = "Helo!";

	CHECK_EQUAL( 0u, str.find_first_not_of( search_str ) );
	CHECK_EQUAL( 6u, str.find_first_not_of( search_str, 2 ) );
	CHECK_EQUAL( 5u, str.find_first_not_of( search_cstr ) );
	CHECK_EQUAL( 4u, str.find_first_not_of( search_cstr, 0, 3 ) );

	CHECK_EQUAL( 1u, str.find_first_not_of( 'H' ) );

	CHECK_EQUAL( StringRef::npos, StringRef("aaaaa").find_first_not_of( 'a' ) );

	CHECK_EQUAL( StringRef::npos, StringRef( "ab" ).find_first_not_of( "ab" ) );

	CHECK_EQUAL( StringRef::npos, StringRef( "ab" ).find_first_not_of( "ba" ) );
}

TEST ( StringRef_find_last_of )
{
	// the test string
	const StringRef str = StringRef( "Hello World!" );

	// strings and chars to search for
	const StringRef search_str = StringRef( "o" );
	const char* search_cstr = "Good Bye!";

	CHECK_EQUAL( 7u, str.find_last_of( search_str ) );
	CHECK_EQUAL( 4u, str.find_last_of( search_str, 5 ) );
	CHECK_EQUAL( 11u, str.find_last_of( search_cstr ) );
	CHECK_EQUAL( 10u, str.find_last_of( search_cstr, StringRef::npos, 4 ) );
	// 'x' is not in "Hello World', thus it will return std::string::npos
	CHECK_EQUAL( StringRef::npos, str.find_last_of( 'x' ) );

	CHECK_EQUAL( 5u, StringRef("abcdef").find_last_of( "ef" ) );
}

TEST( StringRef_find_last_not_of )
{
	CHECK_EQUAL( 5u, StringRef( "abcdef" ).find_last_not_of( 'a' ) );
	CHECK_EQUAL( 5u, StringRef( "abcdef" ).find_last_not_of( "ab" ) );
	CHECK_EQUAL( 4u, StringRef( "abcdef" ).find_last_not_of( 'f' ) );
	CHECK_EQUAL( 3u, StringRef( "abcdef" ).find_last_not_of( "ef" ) );
	CHECK_EQUAL( 0u, StringRef( "abcdef" ).find_last_not_of( "bcdef" ) );
	CHECK_EQUAL( StringRef::npos, StringRef( "abcdef" ).find_last_not_of( "abcdef" ) );
	CHECK_EQUAL( StringRef::npos, StringRef( "abcdef" ).find_last_not_of( "abcde", 4 ) );
}

TEST( StringRef_operator_plus )
{
	const char * cstr = "cstr";
	BW::string str = "str";
	BW::StringRef ref("ref");
	BW::string result1 = str + ref;
	CHECK_EQUAL( "cstrref", cstr + ref );
	CHECK_EQUAL( "refcstr", ref + cstr );
	CHECK_EQUAL( "strref", str + ref );
	CHECK_EQUAL( "refref", ref + ref );
}

} // namespace BW

