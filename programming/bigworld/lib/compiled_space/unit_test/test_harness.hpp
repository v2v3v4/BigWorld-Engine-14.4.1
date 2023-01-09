#ifndef COMPILED_SPACE_TEST_HARNESS_HPP
#define COMPILED_SPACE_TEST_HARNESS_HPP

#include "pch.hpp"

#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"


BW_BEGIN_NAMESPACE

class CompiledSpaceUnitTestHarness : public BaseResMgrUnitTestHarness
{
public:
	CompiledSpaceUnitTestHarness() : BaseResMgrUnitTestHarness( "compiled_space" ) {}
};

BW_END_NAMESPACE

#endif // COMPILED_SPACE_TEST_HARNESS_HPP
