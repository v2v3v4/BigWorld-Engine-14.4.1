#include "pch.hpp"

#include "unit_test.hpp"

#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug_filter.hpp"

#define USE_CPP_UNIT_LITE
#ifdef USE_CPP_UNIT_LITE

#include "CppUnitLite2/src/CppUnitLite2.h"
#include "CppUnitLite2/src/TestResultStdErr.h"
#include "TestResultBWOut.hpp"
#else

// CppUnit related
#include "cppunit/CompilerOutputter.h"
#include "cppunit/TestResult.h"
#include "cppunit/TestResultCollector.h"
#include "cppunit/TestRunner.h"
#include "cppunit/TestFailure.h"
#include "cppunit/Exception.h"
#include "cppunit/TextTestProgressListener.h"
#include "cppunit/extensions/TestFactoryRegistry.h"

#endif


#include <cstring>
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

namespace BWUnitTest
{

#ifdef USE_CPP_UNIT_LITE

int runTest( const BW::string & testName, int argc, char* argv[] )
{
	CStdMf::checkUnattended();

	DebugFilter::shouldWriteToConsole( false );

	for (int i = 1; i < argc; ++i)
	{
		if ((strcmp( "-v", argv[i] ) == 0) ||
				(strcmp( "--verbose", argv[i] ) == 0))
		{
			DebugFilter::shouldWriteToConsole( true );
			break;
		}
	}

#ifdef MF_SERVER
	// Output using CppUnitLite2's standard error outputter
	TestResultStdErr result;
#else
	bool useXML = false;

	for (int i = 0; i < argc; ++i)
	{
		if (strcmp( argv[ i ], "--xml" ) == 0 ||
			strcmp( argv[ i ], "-x" ) == 0)
		{
			useXML = true;
			break;
		}
	}

	// Output using BigWorld's outputter 
	TestResultBWOut result( testName, useXML );
#endif // MF_SERVER

	TestRegistry::Instance().Run( result );
	TestRegistry::Destroy();
	return result.FailureCount();	
}

#else

class BWTestProgressListener: public CppUnit::TestListener
{
public:
	BWTestProgressListener();
	virtual ~BWTestProgressListener() {}
public: // from CppUnit::TestListener
	virtual void startTest( CppUnit::Test * test );
	virtual void addFailure( const CppUnit::TestFailure & failure );
	virtual void endTest( CppUnit::Test * test );

private:
	bool currentTestFailed_;

};

BWTestProgressListener::BWTestProgressListener():
		currentTestFailed_ ( false )
{

}


void BWTestProgressListener::startTest( CppUnit::Test * test )
{
	BWUnitTest::unitTestInfo( "%s:", test->getName().c_str() );
	fflush( stdout );
	currentTestFailed_ = false;
}

void BWTestProgressListener::addFailure( const CppUnit::TestFailure & failure )
{
	currentTestFailed_ = true;

}

void BWTestProgressListener::endTest( CppUnit::Test * test )
{
	const BW::string & testName = test->getName();
	const int MAX_TESTNAME_LENGTH = 50;
	BWUnitTest::unitTestInfo( "%*s\n",
		MAX_TESTNAME_LENGTH - testName.size(),
		currentTestFailed_ ? "FAILED" : "OK" );
}

int runTest( const BW::string & testName, int argc, char* argv[] )
{
	DebugFilter::shouldWriteToConsole( false );

	for (int i = 1; i < argc; ++i)
	{
		if ((strcmp( "-v", argv[i] ) == 0) ||
			(strcmp( "--verbose", argv[i] ) == 0))
		{
			DebugFilter::shouldWriteToConsole( true );
		}
	}

	// Create the event manager and test controller
	CppUnit::TestResult controller;

	// Add a listener that collects test result
	CppUnit::TestResultCollector result;
	controller.addListener( &result );

#if 0 // original CppUnit progress listener
	// Add a listener that print dots as test run.
	CppUnit::TextTestProgressListener progress;
#else
	BWTestProgressListener progress;
#endif

	controller.addListener( &progress );

	CppUnit::TestRunner runner;
	runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );

	BWUnitTest::unitTestInfo("Running %s:\n", testName.c_str() );
	runner.run( controller );
	BWUnitTest::unitTestError("\n");

	// Print test in a compiler compatible format.
	CppUnit::CompilerOutputter outputter( &result, std::cout );
	outputter.write();

	return result.testFailures();
}

#endif // USE_CPP_UNIT_LITE

// wrapper for printf to also print out to dbgview/VS in unit tests.
int unitTestError( const char *_Format, ... )
{
	va_list vaArgs;

	va_start( vaArgs, _Format );
	int len = vfprintf( stderr, _Format, vaArgs );
#ifndef MF_SERVER
	// create the formated string to send to VS/dbgview
	// NOTE: if the output is longer than 1024 chars we drop it.
	char fStr[1024];
	if ( _vsnprintf( fStr, 1024, _Format, vaArgs ) > -1 )
	{
		OutputDebugStringA( fStr );
	}
#endif
	va_end( vaArgs );

	return len;
}

int unitTestInfo( const char *_Format, ... )
{
	va_list vaArgs;

	va_start( vaArgs, _Format );
	int len = vprintf( _Format, vaArgs );
#ifndef MF_SERVER
	// create the formated string to send to VS/dbgview
	// NOTE: if the output is longer than 1024 chars we drop it.
	char fStr[1024];
	if ( _vsnprintf( fStr, 1024, _Format, vaArgs ) > -1 )
	{
		OutputDebugStringA( fStr );
	}
#endif
	va_end( vaArgs );

	return len;
}

} // namespace BWUnitTest

BW_END_NAMESPACE

// unit_test.cpp
