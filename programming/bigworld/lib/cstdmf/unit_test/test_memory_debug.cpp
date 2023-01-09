#include "pch.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_memory.hpp"
#include "cstdmf/memory_debug.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

#if ENABLE_MEMORY_DEBUG

namespace
{

class OverloadedNewWithArgClass
{
public:
	OverloadedNewWithArgClass() {}

	void * operator new( size_t size, const char * some_var )
	{
		MF_ASSERT( some_var );
		s_overloadNewCalled_ = true;
		return bw_malloc( size );
	}

	void operator delete( void * ptr, size_t size )
	{
		s_overloadDelCalled_ = true;
		bw_free( ptr );
	}

	static bool overloadNewCalled()
	{
		return s_overloadNewCalled_;
	}

	static bool overloadDelCalled()
	{
		return s_overloadNewCalled_;
	}
private:
	static bool s_overloadNewCalled_;
	static bool s_overloadDelCalled_;
	uint32 var1_;
	uint32 var2_;
};

/*static */ bool OverloadedNewWithArgClass::s_overloadNewCalled_ = false;
/*static */ bool OverloadedNewWithArgClass::s_overloadDelCalled_ = false;

class OverloadedNewNoArgClass
{
public:
	OverloadedNewNoArgClass() {}

	void * operator new( size_t size )
	{
		s_overloadNewCalled_ = true;
		return bw_malloc( size );
	}

	void operator delete( void * ptr )
	{
		s_overloadDelCalled_ = true;
		bw_free( ptr );
	}

	static bool overloadNewCalled()
	{
		return s_overloadNewCalled_;
	}

	static bool overloadDelCalled()
	{
		return s_overloadNewCalled_;
	}
private:
	static bool s_overloadNewCalled_;
	static bool s_overloadDelCalled_;
	uint32 var1_;
	uint32 var2_;
};

/*static */ bool OverloadedNewNoArgClass::s_overloadNewCalled_ = false;
/*static */ bool OverloadedNewNoArgClass::s_overloadDelCalled_ = false;

#ifdef _WIN32
// no matching delete operator for placement new operator
#pragma warning(push)
#pragma warning(disable: 4291)
#endif

class OverloadedWithArgClassDisableWarn
{
public:
	OverloadedWithArgClassDisableWarn() {}

	void * operator new( size_t size, uint32 some_var )
	{
		MF_ASSERT( some_var );
		s_overloadNewCalled_ = true;
		return bw_malloc( size );
	}

	void operator delete( void * ptr )
	{
		s_overloadDelCalled_ = true;
		bw_free( ptr );
	}

	static bool overloadNewCalled()
	{
		return s_overloadNewCalled_;
	}

	static bool overloadDelCalled()
	{
		return s_overloadNewCalled_;
	}
private:
	static bool s_overloadNewCalled_;
	static bool s_overloadDelCalled_;
	uint32 var1_;
	uint32 var2_;
};

/*static */ bool OverloadedWithArgClassDisableWarn::s_overloadNewCalled_ = false;
/*static */ bool OverloadedWithArgClassDisableWarn::s_overloadDelCalled_ = false;


} // anonymous namespace


TEST( MemoryDebug_testCounts )
{
	struct TestObject
	{
		uint32 a_;
		uint32 b_;
	};
	// We need this test object to be 8 bytes for later on
	CHECK_EQUAL( 8u, sizeof(TestObject) );
	CHECK_EQUAL( 8u, sizeof(OverloadedNewWithArgClass) );
	CHECK_EQUAL( 8u, sizeof(OverloadedNewNoArgClass) );
	CHECK_EQUAL( 8u, sizeof(OverloadedWithArgClassDisableWarn) );


	// Get baseline
	BW::Allocator::AllocationStats baseLine;
	BW::Allocator::readAllocationStats( baseLine );

	// Perform some allocations
	char * string1	= (char*)BW::Allocator::allocate( 256 ); // 256 bytes
	char * string2  = (char*)bw_malloc( 128 );	// 128 bytes
	uint32 * array	= new uint32[16];			// 64  bytes
	TestObject* obj	= new TestObject;			// 8   bytes
	OverloadedNewWithArgClass * obj2 = 
		new( "test" ) OverloadedNewWithArgClass();	// 8 bytes
	OverloadedNewNoArgClass * obj3 = 
		new OverloadedNewNoArgClass();			// 8 bytes
	OverloadedWithArgClassDisableWarn * obj4 = 
		new( 1u ) OverloadedWithArgClassDisableWarn();	// 8 bytes
												// 468 bytes total.
	// Get totals
	BW::Allocator::AllocationStats totals;
	BW::Allocator::readAllocationStats( totals );

	CHECK_EQUAL( baseLine.currentAllocs_ + 7, totals.currentAllocs_ );
	CHECK_EQUAL( baseLine.currentBytes_ + 480, totals.currentBytes_ );

	CHECK( OverloadedNewWithArgClass::overloadNewCalled() );
	CHECK( OverloadedNewWithArgClass::overloadDelCalled() );
	CHECK( OverloadedNewNoArgClass::overloadNewCalled() );
	CHECK( OverloadedNewNoArgClass::overloadDelCalled() );
	CHECK( OverloadedWithArgClassDisableWarn::overloadNewCalled() );
	CHECK( OverloadedWithArgClassDisableWarn::overloadDelCalled() );

	// Deallocate test allocations, we should be back to baseline.
	BW::Allocator::deallocate( string1 );
	bw_free( string2 );
	delete[] array;
	delete obj;
	delete obj2;
	delete obj3;
	delete obj4;

	BW::Allocator::readAllocationStats( totals );

	CHECK_EQUAL( baseLine.currentAllocs_, totals.currentAllocs_ );
	CHECK_EQUAL( baseLine.currentBytes_, totals.currentBytes_ );
}

#ifdef _WIN32
#pragma warning(pop)
#endif


#if ENABLE_SMARTPOINTER_TRACKING

TEST( MemoryDebug_testSmartPointerCounts )
{
	class TestObject : public SafeReferenceCount
	{
		uint32 a_;
	};
	typedef SmartPointer<TestObject> TestObjectPtr;

	BW::Allocator::SmartPointerStats overallBaseLine;
	BW::Allocator::SmartPointerStats baseLine;
	BW::Allocator::SmartPointerStats currentLine;

	// Establish a baseline
	BW::Allocator::readSmartPointerStats( overallBaseLine );
	{
		// Establish a baseline
		BW::Allocator::readSmartPointerStats( baseLine );

		// Exercise the smart pointer constructors
		TestObjectPtr spObjA = NULL; // +0 assigns, +0 current
		TestObjectPtr spObjB = new TestObject(); // +1 assign, +1 current
		TestObjectPtr spObjC = spObjB; // +1 assign, +1 current
		TestObjectPtr spObjD = spObjA; // +0 assign, +0 current (null assign)
		// Total: +2 assign, +2 current

		// CHECK current state
		BW::Allocator::readSmartPointerStats( currentLine );
		CHECK_EQUAL( baseLine.totalAssigns_ + 2, currentLine.totalAssigns_ );
		CHECK_EQUAL( baseLine.currentAssigns_ + 2, currentLine.currentAssigns_ );

		// --------------------------------------------------

		// Establish a baseline
		BW::Allocator::readSmartPointerStats( baseLine );

		// Perform null assignments
		spObjA = NULL; // +0 assigns (already this value)
		spObjB = NULL; // +1 assign, -1 current

		// CHECK current state
		BW::Allocator::readSmartPointerStats( currentLine );
		CHECK_EQUAL( baseLine.totalAssigns_ + 1, currentLine.totalAssigns_ );
		CHECK_EQUAL( baseLine.currentAssigns_ - 1, currentLine.currentAssigns_ );

		// --------------------------------------------------

		// Establish a baseline
		BW::Allocator::readSmartPointerStats( baseLine );

		// Perform a non null assignments 
		spObjD = spObjC; // +1 assign, +1 current

		// CHECK current state
		BW::Allocator::readSmartPointerStats( currentLine );
		CHECK_EQUAL( baseLine.totalAssigns_ + 1, currentLine.totalAssigns_ );
		CHECK_EQUAL( baseLine.currentAssigns_ + 1, currentLine.currentAssigns_ );

		// --------------------------------------------------

		// Establish a baseline
		BW::Allocator::readSmartPointerStats( baseLine );

		// Perform a steal reference (safely)
		TestObject* pObj = new TestObject();
		pObj->incRef();
		spObjA = TestObjectPtr( pObj, TestObjectPtr::STEAL_REFERENCE ); // +3 assign, +1 current (2 temp assigns included)
		CHECK_EQUAL( 1, pObj->refCount() );

		// CHECK current state
		BW::Allocator::readSmartPointerStats( currentLine );
		CHECK_EQUAL( baseLine.totalAssigns_ + 3, currentLine.totalAssigns_ );
		CHECK_EQUAL( baseLine.currentAssigns_ + 1, currentLine.currentAssigns_ );

		// --------------------------------------------------

		// Establish a baseline
		BW::Allocator::readSmartPointerStats( baseLine );

		// Allow all the smart pointers to go out of scope (destructors)
		// +4 assigns, -4 current
	}
	
	// CHECK current state
	BW::Allocator::readSmartPointerStats( currentLine );
	CHECK_EQUAL( baseLine.totalAssigns_ + 4, currentLine.totalAssigns_ );
	CHECK_EQUAL( baseLine.currentAssigns_ - 4, currentLine.currentAssigns_ );
	CHECK_EQUAL( overallBaseLine.currentAssigns_, currentLine.currentAssigns_);
}

#endif // ENABLE_SMARTPOINTER_TRACKING


#endif // ENABLE_MEMORY_DEBUG

BW_END_NAMESPACE

// test_memory_debug.cpp
