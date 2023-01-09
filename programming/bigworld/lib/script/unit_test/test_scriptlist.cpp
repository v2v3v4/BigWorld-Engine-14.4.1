#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptList_general )
{
	ScriptList pList = ScriptList::create( 4 );
	CHECK( pList );
	CHECK( pList.size() == 4 );
	CHECK( pList.setItem( 0, ScriptString::create( "test1" ) ) );
	CHECK( pList.setItem( 1, ScriptString::create( "test2" ) ) );
	CHECK( pList.setItem( 2, ScriptString::create( "test3" ) ) );
	CHECK( pList.setItem( 3, ScriptString::create( "test4" ) ) );
	CHECK( pList.size() == 4 );
	
	CHECK( pList.append( ScriptString::create( "test5" ) ) );
	CHECK( pList.size() == 5 );
	
	ScriptString pStr = ScriptString::create( pList.getItem( 0 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test1" ) == 0 );
	
	pStr = ScriptString::create( pList.getItem( 1 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test2" ) == 0 );
	
	pStr = ScriptString::create( pList.getItem( 2 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test3" ) == 0 );
	
	pStr = ScriptString::create( pList.getItem( 3 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test4" ) == 0 );
	
	
	pStr = ScriptString::create( pList.getItem( 4 ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test5" ) == 0 );
	
	CHECK( pList.size() == 5 );
}

} // namespace BW
