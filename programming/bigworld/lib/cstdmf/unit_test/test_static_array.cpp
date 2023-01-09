#include "pch.hpp"

#include "cstdmf/static_array.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

TEST( StaticArray_testSize )
{	
	StaticArray< uint32, 1 >			unitArray( 1 );
	StaticArray< uint8, 1000 * 1000 >	bigArray( 1000 * 1000 );

	CHECK_EQUAL( 1U,			unitArray.size() );
	CHECK_EQUAL( 1000U * 1000U,	bigArray.size() );
}

TEST( StaticArray_testIndex )
{
	StaticArray< float, 10 >	floatArray(10);

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		floatArray[ i ] = (float)i;
	}

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		CHECK( isEqual( (float)i, floatArray[ i ] ) );
	}
}

TEST( StaticArray_testAssign )
{
	StaticArray< int16, 100 >	testArray(100);

	// Fill with stuff
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		testArray[ i ] = static_cast<int16>(i + 123);
	}

	// Assign 0
	testArray.assign( 0);
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 0, testArray[ i ] );
	}

	// Assign 321
	testArray.assign( 321 );
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 321, testArray[ i ] );
	}	
}

TEST( DynamicEmbeddedArray_testSize )
{
	DynamicEmbeddedArray< uint32, 1 >			unitArray(1);
	DynamicEmbeddedArray< uint8, 1000 * 1000 >	bigArray(1000 * 1000);

	CHECK_EQUAL( 1U,			unitArray.size() );
	CHECK_EQUAL( 1000U * 1000U,	bigArray.size() );
}

TEST( DynamicEmbeddedArray_testIndex )
{
	DynamicEmbeddedArray< size_t, 10 >	floatArray(10);

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		floatArray[ i ] = i;
	}

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		CHECK_EQUAL( i, floatArray[ i ] );
	}
}


TEST( DynamicEmbeddedArray_testAssign )
{
	DynamicEmbeddedArray< int16, 100 >	testArray(100);

	// Fill with stuff
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		testArray[ i ] = static_cast<int16>(i + 123);
	}

	// Assign 0
	testArray.assign( 0);
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 0, testArray[ i ] );
	}

	// Assign 321
	testArray.assign( 321 );
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 321, testArray[ i ] );
	}	
}

TEST( DynamicEmbeddedArray_testGrow )
{
	DynamicEmbeddedArray< size_t, 16 >	testArray(0);

	size_t expectedSize = sizeof(size_t)*testArray.capacity() + 2 * sizeof(size_t) + sizeof(size_t*);
	CHECK_EQUAL(sizeof(testArray), expectedSize );

	for ( size_t i = 0; i < 32; i++ )
	{
		testArray.push_back( i );
	}

	CHECK_EQUAL( 32U, testArray.size());
	CHECK_EQUAL( 36U, testArray.capacity());

	for ( size_t i = 0; i < 32; i++ )
	{
		MF_ASSERT(testArray[i] == i);
	}

	testArray.clear();
	CHECK_EQUAL( testArray.size(), 0U );
	CHECK_EQUAL( testArray.capacity(), 36U );
}

TEST( DynamicEmbeddedArray_testRuntimeWarning )
{
	BW_GUARD;
	DynamicEmbeddedArrayWithWarning< size_t, 4 >	testArray5(0);

	size_t expectedSize2 = sizeof(size_t)*testArray5.capacity() + 2 * sizeof(size_t) + sizeof(size_t*);
	CHECK_EQUAL(sizeof(testArray5), expectedSize2 );

	for ( size_t i = 0; i < 5; i++ )
	{
		testArray5.push_back( i );
	}
	// please check the log, ExpandableStaticArrayWithWarning will print a warning
	// saying that heap allocations happened

	CHECK_EQUAL( 5U, testArray5.size());
	CHECK_EQUAL( 6U, testArray5.capacity());
}

struct PosStruct
{
	int x_;
	int y_;

	inline void set( int x, int y)
	{
		x_ = x;
		y_ = y;
	}
};

TEST( DynamicEmbeddedArray_testEmtpyPushback )
{
	DynamicEmbeddedArray< PosStruct, 4 >	testArray5(0);

	size_t expectedSize2 = sizeof(PosStruct)*testArray5.capacity() + 2 * sizeof(size_t) + sizeof(size_t*);
	CHECK_EQUAL(sizeof(testArray5), expectedSize2 );

	for ( int i = 0; i < 8; i++ )
	{
		testArray5.push_back().set( i, i * 10 );
	}

	for ( size_t i = 0; i < 8; i++ )
	{
		CHECK_EQUAL( (int)i, testArray5[i].x_ );
		CHECK_EQUAL( (int)(i * 10), testArray5[i].y_ );
	}

	CHECK_EQUAL( 8U, testArray5.size());
	CHECK_EQUAL( 9U, testArray5.capacity());
}

TEST( DynamicEmbeddedArray_testEmbedded )
{
	DynamicEmbeddedArray< size_t, 8 >	testArray2(0);

	for ( size_t i = 0; i < 4; i++ )
	{
		testArray2.push_back( i );
	}
	CHECK_EQUAL( testArray2.size(), 4U );
	CHECK_EQUAL( testArray2.capacity(), 8U );
}

TEST( DynamicEmbeddedArray_testEraseSwapLast )
{
	DynamicEmbeddedArray< int, 8 >	testArray3(4);

	testArray3[0] = 100;
	testArray3[1] = 101;
	testArray3[2] = 102;
	testArray3[3] = 103;
	// test erase_swap_last
	CHECK_EQUAL( testArray3.size(), 4U);
	CHECK_EQUAL( testArray3.capacity(), 8U);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 3U);
	CHECK_EQUAL( testArray3[0], 103);
	CHECK_EQUAL( testArray3[1], 101);
	CHECK_EQUAL( testArray3[2], 102);
	testArray3.erase_swap_last(1);
	CHECK_EQUAL( testArray3.size(), 2U);
	CHECK_EQUAL( testArray3[0], 103);
	CHECK_EQUAL( testArray3[1], 102);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 1U);
	CHECK_EQUAL( testArray3[0], 102);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 0U);
}

TEST( DynamicEmbeddedArray_testFindAndAddUnique )
{
	DynamicEmbeddedArray< int, 8 >	ar(0);

	ar.add_unique( 10 );
	ar.add_unique( 18 );
	ar.add_unique( 1 );
	ar.add_unique( -1 );
	ar.add_unique( 8 );
	ar.add_unique( 18 );

	CHECK_EQUAL( ar.size(), 5U );
	ar.erase_by_value( 18 );

	CHECK_EQUAL( ar.size(), 4U );

	ar.erase_by_value( -2 );
	ar.erase_by_value( 18 );

	CHECK_EQUAL( ar.size(), 4U );

	ar.add_unique( 8 );
	ar.add_unique( 18 );

	CHECK_EQUAL( ar.size(), 5U );

	CHECK_EQUAL( *ar.find( 18 ), 18 );
	CHECK_EQUAL( *ar.find( 8 ),  8 );
	CHECK_EQUAL( *ar.find( 10 ), 10 );
	CHECK_EQUAL( *ar.find( 1 ), 1 );
	CHECK_EQUAL( *ar.find( -1 ), -1 );

	ar.erase_by_value( -1 );

	CHECK_EQUAL( ar.size(), 4U );

	CHECK_EQUAL( ar.find( -1 ), ar.end() );

	ar.erase_by_value( -1 );
	CHECK_EQUAL( ar.size(), 4U );
	ar.erase_by_value( 18 );
	ar.erase_by_value( 10 );
	CHECK_EQUAL( ar.size(), 2U );
	ar.erase_by_value( 1 );
	ar.erase_by_value( 8 );
	CHECK_EQUAL( ar.size(), 0U );
	CHECK_EQUAL( ar.capacity(), 8U );
}

// test_static_array.cpp


TEST( DynamicArray_testIndex )
{
	DynamicArray< size_t >	floatArray(10);

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		floatArray[ i ] = i;
	}

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		CHECK_EQUAL( i, floatArray[ i ] );
	}
}


TEST( DynamicArray_testAssign )
{
	DynamicArray< int16 >	testArray(100);

	// Fill with stuff
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		testArray[ i ] = static_cast<int16>(i + 123);
	}

	// Assign 0
	testArray.assign( 0);
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 0, testArray[ i ] );
	}

	// Assign 321
	testArray.assign( 321 );
	for ( size_t i = 0; i < testArray.size(); i++ )
	{
		CHECK_EQUAL( 321, testArray[ i ] );
	}	
}

TEST( DynamicArray_testGrow )
{
	DynamicArray< size_t >	testArray;
	testArray.reserve( 128 );

	size_t expectedSize = 2 * sizeof(size_t) + sizeof(size_t*);
	CHECK_EQUAL(sizeof(testArray), expectedSize );
	CHECK_EQUAL( 128U, testArray.capacity());

	for ( size_t i = 0; i < 32; i++ )
	{
		testArray.push_back( i );
	}

	CHECK_EQUAL( 32U, testArray.size());
	CHECK_EQUAL( 128U, testArray.capacity());

	for ( size_t i = 0; i < 32; i++ )
	{
		CHECK_EQUAL( i, testArray[i] );
	}

	testArray.clear();
	CHECK_EQUAL( testArray.size(), 0U );
	CHECK_EQUAL( testArray.capacity(), 128U );
}


TEST( DynamicArray_testEraseSwapLast )
{
	DynamicArray< float >	testArray3(4);

	testArray3[0] = -100.0f;
	testArray3[1] = 1.0f;
	testArray3[2] = 0.01f;
	testArray3[3] = -103.0f;
	// test erase_swap_last
	CHECK_EQUAL( testArray3.size(), 4U);
	CHECK_EQUAL( testArray3.capacity(), 4U);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 3U);
	CHECK_CLOSE( testArray3[0], -103.0f, 0.001f);
	CHECK_CLOSE( testArray3[1], 1.0f, 0.001f);
	CHECK_CLOSE( testArray3[2], 0.01f, 0.001f);
	testArray3.erase_swap_last(1);
	CHECK_EQUAL( testArray3.size(), 2U);
	CHECK_CLOSE( testArray3[0], -103.f, 0.001f);
	CHECK_CLOSE( testArray3[1], 0.01f, 0.001f);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 1U);
	CHECK_CLOSE( testArray3[0], 0.01f, 0.001f);
	testArray3.erase_swap_last(0);
	CHECK_EQUAL( testArray3.size(), 0U);
}

TEST( ExternalArray_testInit )
{
	ExternalArray<int> array1;
	CHECK( !array1.valid() );

	int data[] = {1,2,3};
	array1.assignExternalStorage( data, 3, 3 );
	CHECK( array1.valid() );

	array1.reset();
	CHECK( !array1.valid() );

	ExternalArray<int> array2( data, 3, 2 );
	CHECK( array2.valid() );
	CHECK_EQUAL( 3U, array2.capacity() );
	CHECK_EQUAL( 2U, array2.size() );

	array2.reset();
	CHECK( !array2.valid() );
}

TEST( ExternalArray_testSize )
{	
	BW::vector<uint32> data;
	data.push_back( 1 );

	ExternalArray< uint32 >		unitArray1( &data[0], 1, 1 );
	CHECK_EQUAL( 1U,			unitArray1.size() );
	CHECK_EQUAL( 1U,			unitArray1.capacity() );

	ExternalArray< uint32 >		unitArray2( &data[0], 1, 0 );
	CHECK_EQUAL( 0U,			unitArray2.size() );
	CHECK_EQUAL( 1U,			unitArray2.capacity() );

	data.resize( 1000 * 1000 );
	ExternalArray< uint32 >	bigArray1( &data[0], data.size(), data.size() );

	CHECK_EQUAL( 1000U * 1000U,	bigArray1.size() );
	CHECK_EQUAL( 1000U * 1000U,	bigArray1.capacity() );

	ExternalArray< uint32 >	bigArray2( &data[0], data.size(), data.size()/2 );
	CHECK_EQUAL( data.size()/2,	bigArray2.size() );
	CHECK_EQUAL( data.size(),	bigArray2.capacity() );

}

TEST( ExternalArray_testIndex )
{
	BW::vector<float> data;
	data.resize( 10 );

	ExternalArray<float>	floatArray( &data[0], data.capacity(), data.size() );

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		floatArray[ i ] = (float)i;
	}

	for ( size_t i = 0; i < floatArray.size(); i++ )
	{
		CHECK( isEqual( (float)i, floatArray[ i ] ) );
	}
}



BW_END_NAMESPACE

// test_static_array.cpp
