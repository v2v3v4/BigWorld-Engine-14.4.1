#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

// import math module, call cos function and check output result
TEST_F( ScriptUnitTestHarness, ScriptModule_ImportMathModule )
{
	ScriptModule pMath = ScriptModule::import( "math", ScriptErrorPrint() );
	CHECK( pMath );

	ScriptObject pCos = pMath.getAttribute( "cos", ScriptErrorPrint() );
	CHECK( pCos );

	ScriptObject res = pCos.callFunction( ScriptArgs::create( 0.0f ), 
		ScriptErrorPrint( "Python_ImportMathModule" ) );
	CHECK( ScriptFloat::check( res ) );

	float fRes = 0.f;
	res.convertTo( fRes, ScriptErrorClear() );
	CHECK_CLOSE( 1.0f, fRes, 0.001f );
}

// create a module and add attributes to it, then see if they are still there
TEST_F( ScriptUnitTestHarness, ScriptModule_CreateAModule )
{
	{
		ScriptModule module = ScriptModule::getOrCreate( "test_module",
			ScriptErrorPrint() );
		CHECK( module );

		CHECK( module.setAttribute( "test",
			ScriptString::create( "test" ),
			ScriptErrorPrint() ) );
		
	}
	
	{
		ScriptModule module = ScriptModule::getOrCreate( "test_module",
			ScriptErrorPrint() );
		CHECK( module );
		
		ScriptString str = ScriptString::create( 
			module.getAttribute( "test", ScriptErrorPrint() ) );
			
		CHECK( str );
		
		CHECK( strcmp( str.c_str(), "test" ) == 0 );
	}
}

// import a module then reload it
TEST_F( ScriptUnitTestHarness, ScriptModule_Reload )
{
	ScriptModule module = ScriptModule::import( "test_script",
		ScriptErrorPrint() );
	CHECK( module );
		
	CHECK( ScriptModule::reload( module, ScriptErrorPrint() ) );
}


TEST_F( ScriptUnitTestHarness, ScriptModule_AddModule )
{
	ScriptModule module = ScriptModule::getOrCreate( "test_module",
		ScriptErrorPrint() );
	CHECK( module );
	CHECK( module.addObject( "test_obj",
			ScriptString::create( "test" ),
			ScriptErrorPrint() ) );

	ScriptString testStr = ScriptString::create( 
			module.getAttribute( "test_obj", ScriptErrorPrint() ) );
	CHECK( testStr );

	CHECK( strcmp( testStr.c_str(), "test" ) == 0 );
}

TEST_F( ScriptUnitTestHarness, ScriptModule_AddIntConstant )
{
	const long testConst = 7;
	ScriptModule module = ScriptModule::getOrCreate( "test_module",
		ScriptErrorPrint() );
	CHECK( module );
	CHECK( module.addIntConstant( "test_const",
		testConst, ScriptErrorPrint() ) );

	ScriptInt testConstObj = ScriptInt::create(
		module.getAttribute( "test_const", ScriptErrorPrint() ) );
	CHECK( testConstObj );
	CHECK( testConstObj.asLong() == testConst );
}

} // namespace BW 
