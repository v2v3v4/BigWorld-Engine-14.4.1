#ifndef ENTITYDEF_TEST_HARNESS_HPP
#define ENTITYDEF_TEST_HARNESS_HPP

#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"
#include "unit_test_lib/unit_test.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

class EntityDefUnitTestHarness : public BaseResMgrUnitTestHarness
{
public:
	EntityDefUnitTestHarness() : BaseResMgrUnitTestHarness( "entitydef" ) 
	{
		PyImportPaths importPaths;
		importPaths.addNonResPath( "." );
		importPaths.addResPath( "." );
		
		if (!Script::init( importPaths ))
		{
			BWUnitTest::unitTestError( "Could not initialise Script module\n" );
		}
	}
};

BW_END_NAMESPACE

#endif // ENTITYDEF_TEST_HARNESS_HPP
