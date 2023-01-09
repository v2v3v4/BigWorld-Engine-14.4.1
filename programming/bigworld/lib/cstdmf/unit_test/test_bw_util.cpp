#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/bw_util.hpp"

#include <stdio.h>
#include <algorithm>


BW_BEGIN_NAMESPACE

TEST( FormatPath )
{
	BW::string validPath( "/home/tester/mf/" );
	BW::string workingPathWithout( "/home/tester/mf" );
	BW::string workingPathWith( "/home/tester/mf/" );

	CHECK_EQUAL( validPath, BWUtil::formatPath( workingPathWith ) );
	CHECK_EQUAL( validPath, BWUtil::formatPath( workingPathWithout ) );
}


TEST( SanitisePath )
{
	BW::string validPath( "/home/tester/mf" );

#ifdef _WIN32
	BW::string windowsPath( "\\home\\tester\\mf" );
	BWUtil::sanitisePath( windowsPath );
	CHECK_EQUAL( validPath, windowsPath );
#else
	BW::string linuxPath( "/home/tester/mf" );
	BWUtil::sanitisePath( linuxPath );
	CHECK_EQUAL( validPath, linuxPath );
#endif
}

TEST( NormalisePath )
{
	CHECK( BWUtil::normalisePath( "" ) == "." );
	CHECK( BWUtil::normalisePath( "." ) == "." );
	CHECK( BWUtil::normalisePath( ".." ) == ".." );

	CHECK( BWUtil::normalisePath( "..\\.." ) == "../.." );
	CHECK( BWUtil::normalisePath( "..\\." ) == ".." );
	CHECK( BWUtil::normalisePath( ".\\.." ) == ".." );
	CHECK( BWUtil::normalisePath( "./.." ) == ".." );
	CHECK( BWUtil::normalisePath( "../." ) == ".." );

	CHECK( BWUtil::normalisePath( "../a" ) == "../a" );
	CHECK( BWUtil::normalisePath( "../../a" ) == "../../a" );
	CHECK( BWUtil::normalisePath( "../a/../b" ) == "../b" );
	CHECK( BWUtil::normalisePath( "../a/../b/../" ) == ".." );

	CHECK( BWUtil::normalisePath( "a\\b\\c" ) == "a/b/c" );
	CHECK( BWUtil::normalisePath( "a/b/c" ) == "a/b/c" );
	CHECK( BWUtil::normalisePath( "/a/b/c" ) == "/a/b/c" );
	CHECK( BWUtil::normalisePath( "a\\\\c" ) == "a/c" );
	CHECK( BWUtil::normalisePath( "c:\\a\\b\\c" ) == "c:/a/b/c" );
	CHECK( BWUtil::normalisePath( "c:\\a\\b\\c\\..\\..\\d" ) == "c:/a/d" );
	CHECK( BWUtil::normalisePath( "c:\\a\\b\\c\\..\\..\\d\\.." ) == "c:/a" );
	CHECK( BWUtil::normalisePath( "c:\\a\\b\\c\\..\\..\\d\\..\\.." ) == "c:/" );

	CHECK( BWUtil::normalisePath( "a/./b/../c" ) == "a/c" );
	CHECK( BWUtil::normalisePath( "a/../b/./c" ) == "b/c" );
	CHECK( BWUtil::normalisePath( "a/../b/../c" ) == "c" );
	CHECK( BWUtil::normalisePath( "a/../b/../c/.." ) == "." );
	CHECK( BWUtil::normalisePath( "../a/../b/../c/.." ) == ".." );
	CHECK( BWUtil::normalisePath( "../a/../b/../c/.." ) == ".." );
}

TEST( IsAbsolutePath )
{
	CHECK( !BWUtil::isAbsolutePath( "" ) );
	CHECK( !BWUtil::isAbsolutePath( "." ) );
	CHECK( !BWUtil::isAbsolutePath( "a/b/c" ) );

	CHECK( BWUtil::isAbsolutePath( "/." ) );
	CHECK( BWUtil::isAbsolutePath( "/.." ) );
	CHECK( BWUtil::isAbsolutePath( "/a/b/c" ) );
}


// Currently commented out as the implementation for windows and linux
// differ in behaviour
#if 0
TEST( FilePath )
{
	BW::string longPathToFile( "/home/tester/mf/bigworld/bin/Hybrid/cellapp" );
	BW::string longPathToDirSlash( "/home/tester/mf/bigworld/bin/Hybrid/" );
	BW::string longPathToDir( "/home/tester/mf/bigworld/bin/Hybrid" );
	BW::string shortPathToFile( "/testFile" );
	BW::string shortPathToDir( "/" );

	BWUnitTest::unitTestInfo( "res1: %s\n", BWUtil::getFilePath( longPathToFile ).c_str() );
	BWUnitTest::unitTestInfo( "res2: %s\n", BWUtil::getFilePath( longPathToDirSlash ).c_str() );
	BWUnitTest::unitTestInfo( "res3: %s\n", BWUtil::getFilePath( longPathToDir ).c_str() );
	BWUnitTest::unitTestInfo( "res4: %s\n", BWUtil::getFilePath( shortPathToFile ).c_str() );
	BWUnitTest::unitTestInfo( "res5: %s\n", BWUtil::getFilePath( shortPathToDir ).c_str() );

	BWUnitTest::unitTestInfo( "path dir: %s\n--\n", BWUtil::executableDirectory().c_str() );
	BWUnitTest::unitTestInfo( "path    : %s\n", BWUtil::executablePath().c_str() );
}
#endif


TEST( SanitiseNonPrintableChars )
{
	using namespace BW;
	CHECK_EQUAL( sanitiseNonPrintableChars( string() ), string() );
	CHECK_EQUAL( sanitiseNonPrintableChars( string( "\x0", 1 ) ), "\\x00" );
	CHECK_EQUAL( sanitiseNonPrintableChars( "\t" ), "\\x09" );
	CHECK_EQUAL( sanitiseNonPrintableChars( "\x7F" ), "\\x7F" );
	CHECK_EQUAL( sanitiseNonPrintableChars( "\xFF" ), "\\xFF" );
	CHECK_EQUAL( sanitiseNonPrintableChars(
		string( "\x0""foobar\x0", sizeof( "\x0""foobar\x0" ) - 1 ) ),
		"\\x00foobar\\x00" );
}

BW_END_NAMESPACE

// test_bw_util.cpp
