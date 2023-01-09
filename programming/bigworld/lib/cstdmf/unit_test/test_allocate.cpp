#include "pch.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/bwrandom.hpp"
#include "cstdmf/fixed_sized_allocator.hpp"

BW_BEGIN_NAMESPACE

struct TestObject
{
	uint32 a_;
	uint32 b_;
};

TEST( Allocator_testHeapAllocZero )
{
	void* ptr = BW::Allocator::heapAllocate( 0 );
	// most allocators allocate on zero size
	CHECK( ptr != NULL );
	BW::Allocator::heapDeallocate( ptr );
}

TEST( Allocator_testDeallocateNULL )
{
	// Nothing to CHECK, just hopefully won't crash or assert.
	BW::Allocator::deallocate( NULL );
}

TEST( Allocator_testHeapDeallocateNULL )
{
	// Nothing to CHECK, just hopefully won't crash or assert.
	BW::Allocator::heapDeallocate( NULL );
}

TEST( Allocator_testBwMAllocZero )
{
	void* ptr = bw_malloc( 0 );
	// most allocators allocate on zero size
	CHECK( ptr != NULL );
	bw_free( ptr );
}

TEST( Allocator_testBWFreeNULL )
{
	// Nothing to CHECK, just hopefully won't crash or assert.
	bw_free( NULL );
}

TEST( Allocator_testHeapAlloc )
{
	char * string = reinterpret_cast<char*>( BW::Allocator::heapAllocate( 128 ) );
	CHECK( string );
	BW::Allocator::heapDeallocate( string );
}

TEST( Allocator_testBWMalloc )
{
	char * string = reinterpret_cast<char*>( bw_malloc( 128 ) );
	CHECK( string );
	bw_free( string );
}

TEST( Allocator_testNew )
{
	TestObject * obj = new TestObject();
	CHECK( obj );
	bw_safe_delete( obj );
	CHECK( !obj );
}

TEST( Allocator_testNewNoThrow )
{
	void * p = ::operator new( sizeof( TestObject ), std::nothrow );
	CHECK( p != NULL);
	::operator delete( p, std::nothrow );
}

TEST( Allocator_testNewArray )
{
	void * p = ::operator new[]( 1 );
	CHECK( p != NULL );
	::operator delete[]( p );
}

TEST( Allocator_testNewArrayNoThrow )
{
	void * p = ::operator new[]( 1, std::nothrow );
	CHECK( p != NULL );
	::operator delete[]( p, std::nothrow );
}

TEST( Allocator_testNewZero )
{
	void * p0 = ::operator new( 0 );
	CHECK( p0 != NULL );
	void * p1 = ::operator new( 0 );
	CHECK( p1 != NULL );
	CHECK( p0 != p1 );
	::operator delete( p1 );
	::operator delete( p0 );
}

TEST( Allocator_testNewNoThrowZero )
{
	void * p0 = ::operator new( 0, std::nothrow );
	CHECK( p0 != NULL );
	void * p1 = ::operator new( 0, std::nothrow );
	CHECK( p1 != NULL );
	CHECK( p0 != p1 );
	::operator delete( p1, std::nothrow );
	::operator delete( p0, std::nothrow );
}

TEST( Allocator_testArrayNewZero )
{
	void * p0 = ::operator new[]( 0 );
	CHECK( p0 != NULL );
	void * p1 = ::operator new[]( 0 );
	CHECK( p1 != NULL );
	CHECK( p0 != p1 );
	::operator delete[]( p0 );
	::operator delete[]( p1 );
}

TEST( Allocator_testArrayNewNoThrowZero )
{
	void * p0 = ::operator new[]( 0, std::nothrow );
	CHECK( p0 != NULL );
	void * p1 = ::operator new[]( 0, std::nothrow );
	CHECK( p1 != NULL );
	CHECK( p0 != p1 );
	::operator delete[]( p0, std::nothrow );
	::operator delete[]( p1, std::nothrow );
}

TEST( Allocator_testNewBadAlloc )
{
	void * p = NULL;
	bool badAllocThrown = false;
	try 
	{
		p = ::operator new( std::numeric_limits<size_t>::max() );
	}
	catch (std::bad_alloc)
	{
		badAllocThrown = true;
	}
	CHECK( badAllocThrown );
	CHECK( p == NULL );
	::operator delete( p );
}

TEST( Allocator_testNewArrayBadAlloc )
{
	void * p = NULL;
	bool badAllocThrown = false;
	try 
	{
		p = ::operator new[]( std::numeric_limits<size_t>::max() );
	}
	catch (std::bad_alloc)
	{
		badAllocThrown = true;
	}
	CHECK( badAllocThrown );
	CHECK( p == NULL );
	::operator delete[]( p );
}

TEST( Allocator_testNewNoThrowBadAlloc )
{
	void * p = NULL;
	bool badAllocThrown = false;
	try 
	{
		p = ::operator new( std::numeric_limits<size_t>::max(), std::nothrow );
	}
	catch (std::bad_alloc)
	{
		badAllocThrown = true;
	}
	CHECK( !badAllocThrown );
	CHECK( p == NULL );
	::operator delete( p, std::nothrow );
}

TEST( Allocator_testNewArrayNoThrowBadAlloc )
{
	void * p = NULL;
	bool badAllocThrown = false;
	try 
	{
		p = ::operator new[]( std::numeric_limits<size_t>::max(), std::nothrow );
	}
	catch (std::bad_alloc)
	{
		badAllocThrown = true;
	}
	CHECK( !badAllocThrown );
	CHECK( p == NULL );
	::operator delete[]( p, std::nothrow );
}

TEST( Allocator_testHeapReallocNew )
{
	char * string = NULL;
	string = reinterpret_cast<char*>( BW::Allocator::heapReallocate( string, 128 ) );
	CHECK( string );
	BW::Allocator::heapDeallocate( string );
}

TEST( Allocator_testBWReallocNew )
{
	char * string = NULL;
	string = reinterpret_cast<char*>( bw_realloc( string, 128 ) );
	CHECK( string );
	bw_free( string );
}

TEST( Allocator_testStlAllocator )
{
	bool badAllocThrown = false;
	std::vector< int, BW::StlAllocator< int > > vector;
	try 
	{
		vector.reserve( std::numeric_limits< size_t >::max() / sizeof(int) );
	}
	catch (std::bad_alloc)
	{
		badAllocThrown = true;
	}
	CHECK( badAllocThrown );
}

void setString( const char* string, char* var, size_t len, TestResult & result_, const char * m_name )
{
	CHECK( var );
	for ( size_t i = 0; i < len; ++i )
		var[i] = string[i];
}

void checkString( const char* string, const char* var, size_t len, TestResult & result_, const char * m_name )
{
	for ( size_t i = 0; i < len; ++i )
		CHECK_EQUAL( string[i], var[i] );
}

static const char s_testString[] = "0123456789abcdef0123456789abcdef";

TEST( Allocator_testHeapReallocGrow )
{
	char * string = reinterpret_cast<char*>( BW::Allocator::heapAllocate( 32 ) );
	setString( s_testString, string, 32, result_, m_name );
	checkString( s_testString, string, 32, result_, m_name );
	string = reinterpret_cast<char*>( BW::Allocator::heapReallocate( string, 64 ) );
	checkString( s_testString, string, 32, result_, m_name );
	BW::Allocator::heapDeallocate( string );
}

TEST( Allocator_testBWReallocGrow )
{
	char * string = reinterpret_cast<char*>( bw_malloc( 32 ) );
	setString( s_testString, string, 32, result_, m_name );
	checkString( s_testString, string, 32, result_, m_name );
	string = reinterpret_cast<char*>( bw_realloc( string, 48 ) );
	checkString( s_testString, string, 32, result_, m_name );
	string = reinterpret_cast<char*>( bw_realloc( string, 64 ) );
	checkString( s_testString, string, 32, result_, m_name );
	bw_free( string );
}

TEST( Allocator_testHeapReallocShrink )
{
	char * string = reinterpret_cast<char*>( BW::Allocator::heapAllocate( 32 ) );
	setString( s_testString, string, 32, result_, m_name );
	checkString( s_testString, string, 32, result_, m_name );
	string = reinterpret_cast<char*>( BW::Allocator::heapReallocate( string, 16 ) );
	checkString( s_testString, string, 16, result_, m_name );
	BW::Allocator::heapDeallocate( string );
}

TEST( Allocator_testBWReallocShrink )
{
	char * string = reinterpret_cast<char*>( bw_malloc( 32 ) );
	setString( s_testString, string, 32, result_, m_name );
	checkString( s_testString, string, 32, result_, m_name );
	string = reinterpret_cast<char*>( bw_realloc( string, 24 ) );
	checkString( s_testString, string, 24, result_, m_name );
	string = reinterpret_cast<char*>( bw_realloc( string, 16 ) );
	checkString( s_testString, string, 16, result_, m_name );
	bw_free( string );
}

TEST( Allocator_testReallocOutOfPool )
{
	const size_t startSize = 64;
	const size_t endSize = 128;
	char * string = reinterpret_cast<char*>( bw_malloc( startSize ) );
	CHECK( string );
	for (size_t i = 0; i < (startSize - 1); ++i)
	{
		string[i] = 'a';
	}
	string[startSize - 1] = '\0';
	string = reinterpret_cast<char*>( bw_realloc( string, endSize ) );
	CHECK( string );
	for (size_t i = 0; i < (startSize - 1); ++i)
	{
		CHECK_EQUAL( 'a', string[i] );
	}
	CHECK_EQUAL( '\0', string[startSize - 1] );
	bw_free( string );
}

#if ENABLE_FIXED_SIZED_POOL_ALLOCATOR

void runTestFull1( const size_t size,
	TestResult & result_,
	const char * m_name );
TEST( FixedSizedAllocator_testFull1 )
{
	// Run for a bunch of sizes
	runTestFull1( 1, result_, m_name );
	runTestFull1( 32, result_, m_name );
	runTestFull1( 64, result_, m_name );
	runTestFull1( 100, result_, m_name );
	runTestFull1( 200, result_, m_name );
	runTestFull1( 300, result_, m_name );
	runTestFull1( 400, result_, m_name );
	runTestFull1( 500, result_, m_name );
	runTestFull1( 600, result_, m_name );
	runTestFull1( 700, result_, m_name );
	runTestFull1( 800, result_, m_name );
	runTestFull1( 900, result_, m_name );
	runTestFull1( 1000, result_, m_name );
	runTestFull1( 1024, result_, m_name );

	runTestFull1( BWRandom::instance()( 1, 1024 ), result_, m_name );
}

/// @param size The size block we want to test
void runTestFull1( const size_t size,
	TestResult & result_,
	const char * m_name )
{
	// Setup an empty string manager
	const size_t sizes[] = { 32, 64, 128, 256, 512, 1024 };
	const size_t poolSize( 1024 );
	const size_t maxNumItemsPerPool( poolSize / size );

	BW::FixedSizedAllocator stringManager(
		sizes, ARRAY_SIZE( sizes ), poolSize, m_name );

	stringManager.setLargeAllocationHooks(
		&BW::Allocator::heapAllocate, 
		&BW::Allocator::heapDeallocate, 
		&BW::Allocator::heapReallocate, 
		&BW::Allocator::heapMemorySize );
	CHECK( stringManager.isAllPoolsEmptyForSize( size ) );

	// Set to some big constant so we do not have to allocate for it
	const size_t maxNumPointers( poolSize );
	void* pointers[ maxNumPointers ];

	// Check there is a memory pool for this size
	CHECK( stringManager.poolExistsForSize( size ) );
	// Check we can store enough pointers to fill the pool
	const size_t initialNumFree(
		stringManager.getFirstPoolNumFreeForSize( size ) );
	CHECK( maxNumPointers >= initialNumFree );
	CHECK_EQUAL( 0u, initialNumFree );

	// Allocate one to create the next pool
	size_t allocCount( 0 );
	pointers[ allocCount ] = stringManager.allocate( size );
	++allocCount;
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	if (maxNumItemsPerPool > 1)
	{
		CHECK( !stringManager.isLastPoolFullForSize( size ) );
	}
	else
	{
		CHECK( stringManager.isLastPoolFullForSize( size ) );
	}
	{
		const size_t numUsed(
			stringManager.getNumPoolItemsForSize( size ) -
			stringManager.getFirstPoolNumFreeForSize( size ) );
		CHECK_EQUAL( 1u, numUsed );
	}

	// Fill the memory pool
	while (!stringManager.isLastPoolFullForSize( size ))
	{
		pointers[ allocCount ] = stringManager.allocate( size );
		++allocCount;
	}

	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK_EQUAL( 0u, stringManager.getFirstPoolNumFreeForSize( size ) );

	// Allocate more - should go into a new pool
	const size_t extraAllocs( 20 );
	void* extraPointers[ extraAllocs ];
	for (size_t i = 0; i < extraAllocs; ++i)
	{
		extraPointers[i] = stringManager.allocate( size );
	}

	// Deallocate from pool
	for (size_t i = 0; i < allocCount; ++i)
	{
		stringManager.deallocate( pointers[i] );
	}

	// Deallocate extras
	for (size_t i = 0; i < extraAllocs; ++i)
	{
		stringManager.deallocate( extraPointers[i] );
	}

	CHECK_EQUAL(
		initialNumFree, stringManager.getFirstPoolNumFreeForSize( size ) );

	// Check deallocate NULL.
	// Nothing to CHECK, just hopefully won't crash or assert.
	stringManager.deallocate( NULL );
}

void runTestFull2( const size_t size,
	TestResult & result_,
	const char * m_name );
TEST( FixedSizedAllocator_testFull2 )
{
	// Run for a bunch of sizes
	runTestFull2( 1, result_, m_name );
	runTestFull2( 32, result_, m_name );
	runTestFull2( 64, result_, m_name );
	runTestFull2( 100, result_, m_name );
	runTestFull2( 200, result_, m_name );
	runTestFull2( 300, result_, m_name );
	runTestFull2( 400, result_, m_name );
	runTestFull2( 500, result_, m_name );
	runTestFull2( 600, result_, m_name );
	runTestFull2( 700, result_, m_name );
	runTestFull2( 800, result_, m_name );
	runTestFull2( 900, result_, m_name );
	runTestFull2( 1000, result_, m_name );
	runTestFull2( 1024, result_, m_name );


	runTestFull2( BWRandom::instance()( 1, 1024 ), result_, m_name );
}

/// @param size The size block we want to test
void runTestFull2( const size_t size,
	TestResult & result_,
	const char * m_name )
{
	// Setup an empty string manager
	const size_t sizes[] = { 32, 64, 128, 256, 512, 1024 };
	const size_t poolSize( 1024 );
	const size_t maxNumItemsPerPool( poolSize / size );

	BW::FixedSizedAllocator stringManager(
		sizes, ARRAY_SIZE( sizes ), poolSize, m_name );

	stringManager.setLargeAllocationHooks(
		&BW::Allocator::heapAllocate, 
		&BW::Allocator::heapDeallocate, 
		&BW::Allocator::heapReallocate, 
		&BW::Allocator::heapMemorySize );
	CHECK( stringManager.isAllPoolsEmptyForSize( size ) );

	// Set to some big constant so we do not have to allocate for it
	const size_t maxNumPointers( poolSize );
	void* firstPoolPointers[ maxNumPointers ];
	void* secondPoolPointers[ maxNumPointers ];
	void* thirdPoolPointers[ maxNumPointers ];

	// Check there is a memory pool for this size
	CHECK( stringManager.poolExistsForSize( size ) );
	// Check we can store enough pointers to fill the pool
	const size_t initialNumFree(
		stringManager.getFirstPoolNumFreeForSize( size ) );
	CHECK( maxNumPointers >= initialNumFree );
	CHECK_EQUAL( 0u, initialNumFree );

	// 1 - Fill the first memory pool
	
	// Allocate one to create the next pool
	size_t firstPoolAllocCount( 0 );
	firstPoolPointers[ firstPoolAllocCount ] = stringManager.allocate( size );
	++firstPoolAllocCount;
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	if (maxNumItemsPerPool > 1)
	{
		CHECK( !stringManager.isLastPoolFullForSize( size ) );
	}
	else
	{
		CHECK( stringManager.isLastPoolFullForSize( size ) );
	}
	{
		const size_t numUsed(
			stringManager.getNumPoolItemsForSize( size ) -
			stringManager.getFirstPoolNumFreeForSize( size ) );
		CHECK_EQUAL( 1u, numUsed );
	}

	while (!stringManager.isLastPoolFullForSize( size ))
	{
		firstPoolPointers[ firstPoolAllocCount ] =
			stringManager.allocate( size );
		++firstPoolAllocCount;
	}

	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK_EQUAL( 0u, stringManager.getFirstPoolNumFreeForSize( size ) );

	// Allocate more - should go into a new pool
	// 2 - Fill the second memory pool

	// Allocate one to create the next pool
	size_t secondPoolAllocCount( 0 );
	secondPoolPointers[ secondPoolAllocCount ] =
		stringManager.allocate( size );
	++secondPoolAllocCount;
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	if (maxNumItemsPerPool > 1)
	{
		CHECK( !stringManager.isLastPoolFullForSize( size ) );
	}
	else
	{
		CHECK( stringManager.isLastPoolFullForSize( size ) );
	}
	{
		const size_t numUsed(
			stringManager.getNumPoolItemsForSize( size ) -
			stringManager.getFirstPoolNumFreeForSize( size ) );
		CHECK_EQUAL( 1u, numUsed );
	}
	
	// Fill the next pool
	while (!stringManager.isLastPoolFullForSize( size ))
	{
		secondPoolPointers[ secondPoolAllocCount ] =
			stringManager.allocate( size );
		++secondPoolAllocCount;
	}

	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK_EQUAL( 0u, stringManager.getFirstPoolNumFreeForSize( size ) );

	// Allocate more - should go into a new pool
	// 3 - Fill the third memory pool

	// Allocate one to create the next pool
	size_t thirdPoolAllocCount( 0 );
	thirdPoolPointers[ thirdPoolAllocCount ] = stringManager.allocate( size );
	++thirdPoolAllocCount;
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	if (maxNumItemsPerPool > 1)
	{
		CHECK( !stringManager.isLastPoolFullForSize( size ) );
	}
	else
	{
		CHECK( stringManager.isLastPoolFullForSize( size ) );
	}
	{
		const size_t numUsed(
			stringManager.getNumPoolItemsForSize( size ) -
			stringManager.getFirstPoolNumFreeForSize( size ) );
		CHECK_EQUAL( 1u, numUsed );
	}

	// Fill the next pool
	while (!stringManager.isLastPoolFullForSize( size ))
	{
		thirdPoolPointers[ thirdPoolAllocCount ] =
			stringManager.allocate( size );
		++thirdPoolAllocCount;
	}

	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK_EQUAL( 0u, stringManager.getFirstPoolNumFreeForSize( size ) );

	// 4 - Deallocate
	// Now we deallocate and check that the linked list of pools works

	// Deallocate from second pool - in middle of linked list
	for (size_t i = 0; i < secondPoolAllocCount; ++i)
	{
		stringManager.deallocate( secondPoolPointers[i] );
	}
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );

	// Deallocate from first pool - third pool becomes top level pool
	for (size_t i = 0; i < firstPoolAllocCount; ++i)
	{
		stringManager.deallocate( firstPoolPointers[i] );
	}
	CHECK( stringManager.isLastPoolFullForSize( size ) );
	CHECK( !stringManager.isAllPoolsEmptyForSize( size ) );

	// Deallocate from third pool - last pool
	for (size_t i = 0; i < thirdPoolAllocCount; ++i)
	{
		stringManager.deallocate( thirdPoolPointers[i] );
	}
	CHECK_EQUAL(
		initialNumFree, stringManager.getFirstPoolNumFreeForSize( size ) );
	CHECK( stringManager.isAllPoolsEmptyForSize( size ) );
}

#endif // ENABLE_FIXED_SIZED_POOL_ALLOCATOR

BW_END_NAMESPACE

// test_allocate.cpp
