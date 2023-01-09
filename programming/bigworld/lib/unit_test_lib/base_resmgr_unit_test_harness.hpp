#ifndef BASE_RESMGR_TEST_HARNESS_HPP
#define BASE_RESMGR_TEST_HARNESS_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class BaseResMgrUnitTestHarness
{
public:
	// Should be set in main.cpp
	static int 		s_cmdArgC;
	static char ** 	s_cmdArgV; 

public:
	BaseResMgrUnitTestHarness( const char* testName );
	virtual ~BaseResMgrUnitTestHarness();

	bool isOK() const { return isOK_; }

private:
	bool isOK_;
};

BW_END_NAMESPACE

#endif // BASE_RESMGR_TEST_HARNESS_HPP
