#include "pch.hpp"

#include "TestResultBWOut.hpp"
#include "CppUnitLite2/src/Failure.h"
#include <iostream>
#include <iomanip>

#include "unit_test.hpp"


BW_BEGIN_NAMESPACE

TestResultBWOut::TestResultBWOut( const BW::string & name, 
								 bool useXML /*= false*/ ) :
	TestResult(),
	name_( name ),
	usingXML_( useXML )
{
}


void TestResultBWOut::StartTests ()
{
	TestResult::StartTests();
	if (usingXML_)
	{
		this->StartTestsXML();
	}
	else
	{
		this->StartTestsTXT();
	}
}


void TestResultBWOut::AddFailure (const Failure & failure) 
{
    TestResult::AddFailure(failure);
	if (usingXML_)
	{
		this->AddFailureXML( failure );
	}
	else
	{
		this->AddFailureTXT( failure );
	}
}


void TestResultBWOut::EndTests () 
{
    TestResult::EndTests();
	if (usingXML_)
	{
		this->EndTestsXML();
	}
	else
	{
		this->EndTestsTXT();
	}
}


void TestResultBWOut::StartTestsXML()
{
	BWUnitTest::unitTestInfo( "<TestRun>\n" );
	BWUnitTest::unitTestInfo( "\t<FailedTests>\n" );
}


void TestResultBWOut::AddFailureXML( const Failure & failure ) 
{
	BWUnitTest::unitTestInfo( "\t\t<FailedTest>\n" );
	BWUnitTest::unitTestInfo( "\t\t\t<Name>%s</Name>\n", failure.TestName() );
	BWUnitTest::unitTestInfo( "\t\t\t<Location>\n" );
	BWUnitTest::unitTestInfo( "\t\t\t\t<File>%s</File>\n", failure.FileName() );
	BWUnitTest::unitTestInfo( "\t\t\t\t<Line>%i</Line>\n", failure.LineNumber() );
	BWUnitTest::unitTestInfo( "\t\t\t</Location>\n" );
	BWUnitTest::unitTestInfo( "\t\t\t<Message>%s</Message>\n", failure.Condition() );
	BWUnitTest::unitTestInfo( "\t\t</FailedTest>\n" );
}


void TestResultBWOut::EndTestsXML() 
{
	BWUnitTest::unitTestInfo( "\t</FailedTests>\n" );
	BWUnitTest::unitTestInfo( "\t<Statistics>\n" );
	BWUnitTest::unitTestInfo( "\t\t<Tests> %i </Tests>\n", m_testCount );
	BWUnitTest::unitTestInfo( "\t\t<Failures> %i </Failures>\n", m_failureCount );
	BWUnitTest::unitTestInfo( "\t\t<Time> %.3f </Time>\n", m_secondsElapsed );
	BWUnitTest::unitTestInfo( "\t</Statistics>\n" );
	BWUnitTest::unitTestInfo( "</TestRun>\n" );
}


void TestResultBWOut::StartTestsTXT()
{
	BWUnitTest::unitTestInfo( "====================\n" );
	BWUnitTest::unitTestInfo( "Running Tests on: %s\n", name_.c_str() );
}


void TestResultBWOut::AddFailureTXT( const Failure & failure ) 
{
	BWUnitTest::unitTestInfo( "Failed Test: %s\n", failure.TestName() );
	BWUnitTest::unitTestInfo( "File: %s\n", failure.FileName() );
	BWUnitTest::unitTestInfo( "Line: %i\n", failure.LineNumber() );
	BWUnitTest::unitTestInfo( "Condition: %s\n", failure.Condition() );
	BWUnitTest::unitTestInfo( "--------------------\n");
}


void TestResultBWOut::EndTestsTXT() 
{
	BWUnitTest::unitTestInfo( "Tests run: %i\n", m_testCount );
	BWUnitTest::unitTestInfo( "Failures: %i\n", m_failureCount );
	BWUnitTest::unitTestInfo( "Time: %.3f\n", m_secondsElapsed );
}

BW_END_NAMESPACE

// TestResultBWOut.cpp

