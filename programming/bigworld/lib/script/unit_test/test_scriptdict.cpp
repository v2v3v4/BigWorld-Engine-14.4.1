#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptDict_general )
{
	ScriptDict pDict = ScriptDict::create();
	CHECK( pDict );
	CHECK( pDict.size() == 0 );
	CHECK( pDict.setItem( "item1", 
			ScriptString::create( "value1" ),
			ScriptErrorPrint() ) );
	CHECK( pDict.size() == 1 );
	CHECK( pDict.setItem( "item2",
			ScriptString::create( "value2" ),
			ScriptErrorPrint() ) );
	CHECK( pDict.size() == 2 );
	CHECK( pDict.setItem( "item3",
			ScriptString::create( "value3" ),
			ScriptErrorPrint() ) );
	CHECK( pDict.size() == 3 );
	
	ScriptString pStr = ScriptString::create( 
		pDict.getItem( "item1", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "value1" ) == 0 );
	
	pStr = ScriptString::create( 
		pDict.getItem( "item2", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "value2" ) == 0 );
	
	pStr = ScriptString::create( 
		pDict.getItem( "item3", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "value3" ) == 0 );
	
	CHECK( pDict.size() == 3 );
	
	ScriptDict pOtherDict = ScriptDict::create();
	CHECK( pOtherDict );
	CHECK( pOtherDict.setItem( "item2", 
		ScriptString::create( "newvalue2" ), ScriptErrorPrint() ) );
	CHECK( pOtherDict.setItem( "item4", 
		ScriptString::create( "newvalue4" ), ScriptErrorPrint() ) );
		
	CHECK( pDict.update( pOtherDict, ScriptErrorPrint() ) );
		
	pStr = ScriptString::create( 
		pDict.getItem( "item1", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "value1" ) == 0 );
	
	pStr = ScriptString::create( 
		pDict.getItem( "item2", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "newvalue2" ) == 0 );
	
	pStr = ScriptString::create( 
		pDict.getItem( "item3", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "value3" ) == 0 );
	
	pStr = ScriptString::create( 
		pDict.getItem( "item4", ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "newvalue4" ) == 0 );
	
	CHECK( pDict.size() == 4 );
}

} // namespace BW
