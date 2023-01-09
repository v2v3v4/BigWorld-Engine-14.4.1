#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptArgs_none )
{
	ScriptArgs noArgs = ScriptArgs::none();
	CHECK( noArgs );
	// TODO: Assumption args = tuple
	ScriptTuple pTuple = ScriptTuple::create(noArgs);
	CHECK( pTuple );
	CHECK( pTuple.size() == 0 );
}

bool checkArgs( ScriptArgs pArgs, int count )
{
	ScriptIter it = pArgs.getIter( ScriptErrorPrint() );
	
	int idx = 0;
	while( ScriptInt pArg = ScriptInt::create( it.next() ) )
	{
		if ( ++idx != pArg.asLong() )
		{
			return false;
		}
	}
	
	return idx == count;
}

TEST_F( ScriptUnitTestHarness, ScriptArgs_create )
{
	// The aim of this test is to check ordering of args
	CHECK( checkArgs( ScriptArgs::create( 1 ), 1 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2 ), 2 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3 ), 3 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3, 4 ), 4 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3, 4, 5 ), 5 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3, 4, 5, 6 ), 6 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3, 4, 5, 6, 7 ), 7 ) );
	CHECK( checkArgs( ScriptArgs::create( 1, 2, 3, 4, 5, 6, 7, 8 ), 8 ) );
}

TEST_F( ScriptUnitTestHarness, ScriptArgs_createTypeMixup )
{
	// The aim of this test is to validate template args are not repeated or
	// in the wrong place by passing different types to each
	ScriptArgs::create( 1 );
	ScriptArgs::create( 1, 2.f );
	ScriptArgs::create( 1, 2.f, 3.0 );
	ScriptArgs::create( 1, 2.f, 3.0, 4u );
	ScriptArgs::create( 1, 2.f, 3.0, 4u, '5' );
	ScriptArgs::create( 1, 2.f, 3.0, 4u, '5', "6" );
	ScriptArgs::create( 1, 2.f, 3.0, 4u, '5', "6", ScriptInt::create( 7 ) );
	ScriptArgs::create( 1, 2.f, 3.0, 4u, '5', "6", ScriptInt::create( 7 ), 
		ScriptModule::getOrCreate( "module_arg_8", ScriptErrorPrint() ) );
}

TEST_F( ScriptUnitTestHarness, ScriptArgs_createForSingleScriptObjectIsATrap )
{
	// The aim of this test is to verify Script::create isn't overloading in 
	// an suprising way
	ScriptObject testScriptObject = ScriptString::create( "Test" );

	// Check ScriptArgs::create( ScriptObject() ) method, does the desired
	// behaviour of creating a ScriptArgs object, instead of doing the same
	// behaviour as other ScriptObject::create() methods
	ScriptArgs oneScriptObjectBad = ScriptArgs::create( testScriptObject );
	CHECK( oneScriptObjectBad );
	CHECK( ScriptArgs::check( oneScriptObjectBad ) );
	CHECK( ScriptTuple::check( oneScriptObjectBad ) );
	ScriptTuple oneScriptObjectBadTuple =
		ScriptTuple::create( oneScriptObjectBad );
	CHECK_EQUAL( 1u, oneScriptObjectBadTuple.size() );
	CHECK_EQUAL( testScriptObject, oneScriptObjectBadTuple.getItem( 0 ) );

	// Explicitly call the template static method to ensure it works as expected
	ScriptArgs oneScriptObjectGood = ScriptArgs::create< ScriptObject >(
		testScriptObject );
	CHECK( ScriptArgs::check( oneScriptObjectGood ) );
	CHECK( ScriptTuple::check( oneScriptObjectGood ) );
	ScriptTuple oneScriptObjectTuple =
		ScriptTuple::create( oneScriptObjectGood );
	CHECK_EQUAL( 1u, oneScriptObjectTuple.size() );
	CHECK_EQUAL( testScriptObject, oneScriptObjectTuple.getItem( 0 ) );

	ScriptArgs twoScriptObjects =
		ScriptArgs::create( testScriptObject, testScriptObject );
	CHECK( ScriptArgs::check( twoScriptObjects ) );
	CHECK( ScriptTuple::check( twoScriptObjects ) );
	ScriptTuple twoScriptObjectsTuple = ScriptTuple::create( twoScriptObjects );
	CHECK_EQUAL( 2u, twoScriptObjectsTuple.size() );
	CHECK_EQUAL( testScriptObject, twoScriptObjectsTuple.getItem( 0 ) );
	CHECK_EQUAL( testScriptObject, twoScriptObjectsTuple.getItem( 1 ) );
}

} // namespace BW
