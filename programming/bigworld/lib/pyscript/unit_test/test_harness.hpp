#ifndef PYSCRIPT_TEST_HARNESS_HPP
#define PYSCRIPT_TEST_HARNESS_HPP

#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"
#include "unit_test_lib/unit_test.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

class PyScriptUnitTestHarness : public BaseResMgrUnitTestHarness
{
public:
	PyScriptUnitTestHarness() : BaseResMgrUnitTestHarness( "pyscript" ) 
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

#endif // PYSCRIPT_TEST_HARNESS_HPP
