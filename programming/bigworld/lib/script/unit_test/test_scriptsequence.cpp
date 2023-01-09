#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptSequence_general )
{
	// We are working with a list as its a sequence
	ScriptList pList = ScriptList::create( 4 );
	// We can not use a list as a sequence until all values are set,
	// so we set them all to none
	pList.setItem( 0, ScriptObject::none() );
	pList.setItem( 1, ScriptObject::none() );
	pList.setItem( 2, ScriptObject::none() );
	pList.setItem( 3, ScriptObject::none() );

	ScriptSequence pSequence = ScriptSequence::create( pList );
	CHECK( pSequence );
	CHECK( pSequence.size() == 4 );
	CHECK( pSequence.setItem( 0,
		ScriptString::create( "test1" ), ScriptErrorPrint() ) );
	CHECK( pSequence.setItem( 1,
		ScriptString::create( "test2" ), ScriptErrorPrint() ) );
	CHECK( pSequence.setItem( 2,
		ScriptString::create( "test3" ), ScriptErrorPrint() ) );
	CHECK( pSequence.setItem( 3,
		ScriptString::create( "test4" ), ScriptErrorPrint() ) );
	CHECK( pSequence.size() == 4 );
	
	ScriptString pStr = ScriptString::create( 
		pSequence.getItem( 0, ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test1" ) == 0 );
	
	pStr = ScriptString::create( pSequence.getItem( 1, ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test2" ) == 0 );
	
	pStr = ScriptString::create( pSequence.getItem( 2, ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test3" ) == 0 );
	
	pStr = ScriptString::create( pSequence.getItem( 3, ScriptErrorPrint() ) );
	CHECK( pStr );
	CHECK( strcmp( pStr.c_str(), "test4" ) == 0 );
	
	CHECK( pSequence.size() == 4 );
}

} // namespace BW
