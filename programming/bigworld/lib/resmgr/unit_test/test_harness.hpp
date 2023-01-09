#ifndef RESMGR_TEST_HARNESS_HPP
#define RESMGR_TEST_HARNESS_HPP

#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"


BW_BEGIN_NAMESPACE

class ResMgrUnitTestHarness : public BaseResMgrUnitTestHarness
{
public:
	ResMgrUnitTestHarness() : BaseResMgrUnitTestHarness( "resmgr" ) {}
};

BW_END_NAMESPACE

#endif // RESMGR_TEST_HARNESS_HPP
