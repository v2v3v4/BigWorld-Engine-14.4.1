#include "pch.hpp"

#include "cstdmf/object_pool.hpp"

/**
 *	Brief tests of the object pool implementation
 *  to ensure it compiles and performs correctly.
 */

BW_BEGIN_NAMESPACE

TEST( HandleTable_Basic)
{
	typedef HandleTable<> PoolType;

	// Test handles
	PoolType::Handle handle;
	CHECK( !handle.isValid() );

	PoolType pool;

	// Test initial pool
	CHECK( !pool.isValid( handle ) );

	// Create an object
	PoolType::Handle handleA = pool.create();
	PoolType::Handle handleB = pool.create();
	PoolType::Handle handleC = pool.create();

	// Make sure all are valid
	CHECK( handleA.isValid() );
	CHECK( handleB.isValid() );
	CHECK( handleC.isValid() );

	// Make sure that all are unique
	CHECK( handleA != handleC );
	CHECK( handleB != handleC );

	// Validate handle copies and equality comparison
	PoolType::Handle tmp = handleA;
	CHECK( handleA == tmp );

	// Remove one of the objects
	pool.release( handleB );

	CHECK( !pool.isValid( handleB ) );

	// Now allocate again
	PoolType::Handle handleD = pool.create( );

	CHECK( handleA != handleD );
	CHECK( handleB != handleD );
	CHECK( handleC != handleD );
}

TEST( PackedObjectPool_Basic)
{
	typedef PackedObjectPool<uint32> PoolType;

	// Test handles
	PoolType::Handle handle;
	CHECK( !handle.isValid() );

	PoolType pool;

	// Test initial pool
	CHECK( !pool.isValid( handle ) );

	// Create an object
	PoolType::Handle handleA = pool.create();
	PoolType::Handle handleB = pool.create();
	PoolType::Handle handleC = pool.create();

	CHECK( handleA != handleC );
	CHECK( handleB != handleC );

	CHECK_EQUAL( 3, pool.size() );

	// Assign the objects
	pool.lookup(handleA) = 123;
	pool.lookup(handleB) = 456;
	pool.lookup(handleC) = 789;

	CHECK_EQUAL( 123, pool.lookup(handleA) );
	CHECK_EQUAL( 456, pool.lookup(handleB) );
	CHECK_EQUAL( 789, pool.lookup(handleC) );

	// Remove one of the objects
	pool.release(handleB);
	CHECK_EQUAL( 2, pool.size() );

	CHECK( !pool.isValid(handleB) );

	// Now allocate again
	PoolType::Handle handleD = pool.create();

	CHECK( handleA != handleD );
	CHECK( handleB != handleD );
	CHECK( handleC != handleD );

	// Now assign again
	pool.lookup(handleD) = 007;

	CHECK_EQUAL( 007, pool.lookup(handleD) );

	CHECK_EQUAL( 123, pool.lookup(handleA) );
	CHECK_EQUAL( 789, pool.lookup(handleC) );

	CHECK_EQUAL( 3, pool.size() );

	// Test iteration
	// TODO: Test const iterators and attempting to assign a value through the iterators.
	uint32 count = 0;
	for (PoolType::iterator iter = pool.begin(); iter != pool.end(); ++iter)
	{
		PoolType::Object& obj = *iter;

		switch (count)
		{
		case 0:
			CHECK_EQUAL( 123, obj ); break;
		case 1:
			CHECK_EQUAL( 789, obj ); break;
		case 2:
			CHECK_EQUAL( 007, obj ); break;
		default:
			// No test
			break;
		}

		count++;
	}

	PoolType::Handle handleE = pool.create();
	PoolType::Handle handleF = pool.create();
	PoolType::Handle handleG = pool.create();

	CHECK( handleA != handleE );
	CHECK( handleB != handleE );
	CHECK( handleC != handleE );
	CHECK( handleD != handleE );

	CHECK( handleA != handleF );
	CHECK( handleB != handleF );
	CHECK( handleC != handleF );
	CHECK( handleD != handleF );
	CHECK( handleE != handleF );

	CHECK( handleA != handleG );
	CHECK( handleB != handleG );
	CHECK( handleC != handleG );
	CHECK( handleD != handleG );
	CHECK( handleE != handleG );
	CHECK( handleF != handleG );
}

BW_END_NAMESPACE

// test_object_pool.cpp
