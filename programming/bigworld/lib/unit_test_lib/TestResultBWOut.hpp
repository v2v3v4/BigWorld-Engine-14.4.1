#ifndef TESTRESULTBWOUT_H
#define TESTRESULTBWOUT_H

#include "cstdmf/bw_namespace.hpp"

#include "CppUnitLite2/src/TestResult.h"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class TestResultBWOut : public TestResult
{
public:
	TestResultBWOut( const BW::string & name, bool useXML = false );

	virtual void StartTests ();
    virtual void AddFailure (const Failure & failure);
    virtual void EndTests ();

	void StartTestsXML();
	void AddFailureXML( const Failure & failure );
	void EndTestsXML();

	void StartTestsTXT();
	void AddFailureTXT( const Failure & failure );
	void EndTestsTXT();

private:
	BW::string name_;
	bool usingXML_;
};

BW_END_NAMESPACE

#endif // TESTRESULTBWOUT_H

