#include "pch.hpp"

#include "test_harness.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "pyscript/script.hpp"
#include "script/script_object.hpp"


namespace BW
{

TEST_F( ScriptUnitTestHarness, ScriptMapping_general )
{
	ScriptDict pDict = ScriptDict::create();
	CHECK( pDict );
	CHECK( pDict.size() == 0 );
	CHECK( pDict.setItem( "item1", ScriptInt::create( 1 ),
		ScriptErrorPrint() ) );
	CHECK( pDict.size() == 1 );
	CHECK( pDict.setItem( "item2", ScriptInt::create( 2 ),
		ScriptErrorPrint() ) );
	CHECK( pDict.size() == 2 );
	CHECK( pDict.setItem( "item3", ScriptInt::create( 3 ),
		ScriptErrorPrint() ) );
	CHECK( pDict.size() == 3 );
	
	ScriptMapping pMapping = ScriptMapping::create( pDict );
	CHECK( pMapping );
	
	ScriptList pValues = pMapping.values(ScriptErrorPrint());
	ScriptIter pIter = pValues.getIter(ScriptErrorPrint());
	
	int sum = 0, idx = 0;
	// Items are not in order so lets just sum
	while (ScriptInt pInt = ScriptInt::create( pIter.next() ))
	{
		sum += pInt.asLong();
		++idx;
	}
	
	CHECK( idx == 3 );
	CHECK( sum == (1+2+3) ) ;
	
	ScriptInt pInt = ScriptInt::create( 
		pMapping.getItem( "item1", ScriptErrorPrint() ) );
	CHECK( pInt );
	CHECK( pInt.asLong() == 1 );
	
	pInt = ScriptInt::create( 
		pMapping.getItem( "item2", ScriptErrorPrint() ) );
	CHECK( pInt );
	CHECK( pInt.asLong() == 2 );
	
	pInt = ScriptInt::create( 
		pMapping.getItem( "item3", ScriptErrorPrint() ) );
	CHECK( pInt );
	CHECK( pInt.asLong() == 3 );
}

} // namespace BW
