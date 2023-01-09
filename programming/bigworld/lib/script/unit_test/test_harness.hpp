#ifndef SCRIPT_TEST_HARNESS_HPP
#define SCRIPT_TEST_HARNESS_HPP

#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

class ScriptUnitTestHarness : public BaseResMgrUnitTestHarness
{
public:
	ScriptUnitTestHarness() : BaseResMgrUnitTestHarness( "script" ) 
	{
		PyImportPaths importPaths;
		importPaths.addNonResPath( "." );
		importPaths.addResPath( "." );
		
		if (!Script::init( importPaths ))
		{
			BWUnitTest::unitTestError( "Could not initialise Script module" );
		}
	}
};

BW_END_NAMESPACE

#endif // SCRIPT_TEST_HARNESS_HPP
