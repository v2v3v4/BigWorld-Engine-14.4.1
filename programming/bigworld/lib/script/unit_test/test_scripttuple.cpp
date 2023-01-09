#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptTuple_general )
{
	ScriptTuple pTuple = ScriptTuple::create( 4 );
	CHECK( pTuple );
	CHECK( pTuple.size() == 4 );
	CHECK( pTuple.setItem( 0, ScriptString::create( "test1" ) ) );
	CHECK( pTuple.setItem( 1, ScriptString::create( "test2" ) ) );
	CHECK( pTuple.setItem( 2, ScriptString::create( "test3" ) ) );
	CHECK( pTuple.setItem( 3, ScriptString::create( "test4" ) ) );
	CHECK( pTuple.size() == 4 );
	
	ScriptString pStr = ScriptString::create( pTuple.getItem( 0 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test1" ) == 0 );
	
	pStr = ScriptString::create( pTuple.getItem( 1 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test2" ) == 0 );
	
	pStr = ScriptString::create( pTuple.getItem( 2 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test3" ) == 0 );
	
	pStr = ScriptString::create( pTuple.getItem( 3 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test4" ) == 0 );
	
	CHECK( pTuple.size() == 4 );
}

} // namespace BW
