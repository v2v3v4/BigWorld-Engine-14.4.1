#include "pch.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_unordered_set.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

#if ENABLE_MEMORY_DEBUG
#define PREPARE_ALLOCATION_STATS() \
	BW::Allocator::AllocationStats baseLine, totals; \
	BW::Allocator::readAllocationStats( baseLine )
#define CHECK_ALLOCATION_INCREASE() \
	BW::Allocator::readAllocationStats( totals ); \
	CHECK( baseLine.currentAllocs_ < totals.currentAllocs_ ); \
	CHECK( baseLine.currentBytes_ < totals.currentBytes_ )
#define CHECK_ALLOCATION_FREED() \
	BW::Allocator::readAllocationStats( totals ); \
	CHECK_EQUAL( baseLine.currentAllocs_, totals.currentAllocs_ ); \
	CHECK_EQUAL( baseLine.currentBytes_, totals.currentBytes_ )
#else
#define PREPARE_ALLOCATION_STATS()
#define CHECK_ALLOCATION_INCREASE()
#define CHECK_ALLOCATION_FREED()
#endif // ENABLE_MEMORY_DEBUG

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
#define _HAS_CXX11_MOVE
namespace {
// function to test move constructors
template <typename T>
T testMove( const T& temp )
{
	return temp;
}
}
#endif

TEST( BWContainers_testBwVector )
{
	static const size_t COUNT = 5;

	PREPARE_ALLOCATION_STATS();

	{
		// check default constructor
		BW::vector< size_t > container;
		for ( size_t i = 0; i < COUNT; ++i ) 
		{
			container.push_back(i);
		}

		CHECK_ALLOCATION_INCREASE();

		size_t i = 0;
		for ( BW::vector< size_t >::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
		{
			CHECK_EQUAL( *itr, i );
			++i;
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// compilation check
		BW::vector< bool > container;
	}
	CHECK_ALLOCATION_FREED();

	{
		// check vector(size_t) constructor
		BW::vector< size_t > container( COUNT );
		CHECK_EQUAL( container.size(), COUNT );
	}
	CHECK_ALLOCATION_FREED();

	{
		// check vector( size_t, T, Allocator = Allocator() ) constructor
		BW::vector< size_t > container( COUNT, 1 );
		CHECK_EQUAL( container.size(), COUNT );
		for ( BW::vector< size_t >::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
		{
			CHECK_EQUAL( *itr, 1u );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// check vector( InputIt, InputIt, Allocator = Allocator() )
		BW::vector< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::vector< size_t > container2( container1.begin(), container1.end() );
		for (size_t i = 0; i < COUNT; ++i )
		{
			CHECK_EQUAL( container1[i], container2[i] );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// check copy constructor
		BW::vector< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::vector< size_t > container2( container1 );
		for (size_t i = 0; i < COUNT; ++i )
		{
			CHECK_EQUAL( container1[i], container2[i] );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		BW::vector< size_t > container;
	}
	CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
	{
		// check move constructor
		BW::vector< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::vector< size_t > container2 = testMove(container1);

		for (BW::vector< size_t >::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
			itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
		{
			CHECK_EQUAL( *itr1, *itr2 );
		}

		{
			BW::vector< int > a;
			BW::vector< int > b;

			a.push_back( 0 );
			a.push_back( 1 );

			b.push_back( 2 );
			b.push_back( 3 );
			b.push_back( 4 );

			std::swap( a, b );

			CHECK_EQUAL( 3, a.size() );
			CHECK_EQUAL( 2, b.size() );
		}
	}
	CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
}

TEST( BWContainers_testBwList )
{
	static const size_t COUNT = 5;

	PREPARE_ALLOCATION_STATS();

	{
		// check default constructor
		BW::list< size_t > container;
		for ( size_t i = 0; i < COUNT; ++i ) 
		{
			container.push_back(i);
		}

		CHECK_ALLOCATION_INCREASE();

		size_t i = 0;
		for ( BW::list< size_t >::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
		{
			CHECK_EQUAL( *itr, i );
			++i;
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// check list(size_t) constructor
		BW::list< size_t > container( COUNT );
		CHECK_EQUAL( container.size(), COUNT );
	}
	CHECK_ALLOCATION_FREED();

	{
		// check list( size_t, T, Allocator = Allocator() ) constructor
		BW::list< size_t > container( COUNT, 1 );
		CHECK_EQUAL( container.size(), COUNT );
		for ( BW::list< size_t >::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
		{
			CHECK_EQUAL( *itr, size_t(1) );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// check list( InputIt, InputIt, Allocator = Allocator() )
		BW::list< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::list< size_t > container2( container1.begin(), container1.end() );
		for (BW::list< size_t >::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
			itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
		{
			CHECK_EQUAL( *itr1, *itr2 );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		// check copy constructor
		BW::list< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::list< size_t > container2( container1 );
		for (BW::list< size_t >::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
			itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
		{
			CHECK_EQUAL( *itr1, *itr2 );
		}
	}
	CHECK_ALLOCATION_FREED();

	{
		BW::list< const size_t > container;
	}
	CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
	{
		// check move constructor
		BW::list< size_t > container1;
		for (size_t i = 0; i < COUNT; ++i) container1.push_back(i);

		BW::list< size_t > container2 = testMove(container1);

		for (BW::list< size_t >::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
			itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
		{
			CHECK_EQUAL( *itr1, *itr2 );
		}
	}
	CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
}

template< class ContainerClass >
class SetTester
{
public:
	TEST_SUB_FUNCTOR_OPERATOR
	{
		static const size_t COUNT = 5;

		PREPARE_ALLOCATION_STATS();

		{
			// check default constructor
			ContainerClass container;
			for ( size_t i = 0; i < COUNT; ++i ) 
			{
				container.insert(i);
			}

			CHECK_ALLOCATION_INCREASE();

			size_t i = 0;
			for (typename ContainerClass::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
			{
				CHECK_EQUAL( *itr, i );
				++i;
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check set( InputIt, InputIt, Allocator = Allocator() )
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert(i);

			ContainerClass container2( container1.begin(), container1.end() );
			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check copy constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert(i);

			ContainerClass container2( container1 );
			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
		{
			// check move constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert(i);

			ContainerClass container2 = testMove(container1);

			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
	}
};

TEST( BWContainers_testBwSet )
{
	SetTester< BW::set< size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

TEST( BWContainers_testBwMultiSet )
{
	SetTester< BW::multiset< size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

template< class ContainerClass >
class MapTester
{
public:
	TEST_SUB_FUNCTOR_OPERATOR
	{
		static const size_t COUNT = 5;

		PREPARE_ALLOCATION_STATS();

		{
			// check default constructor
			ContainerClass container;
			for ( size_t i = 0; i < COUNT; ++i ) 
			{
				container.insert( std::make_pair( i, COUNT - i ) );
			}

			CHECK_ALLOCATION_INCREASE();

			size_t i = 0;
			for (typename ContainerClass::const_iterator itr = container.begin(), end = container.end(); itr != end; ++itr )
			{
				CHECK_EQUAL( itr->first, i );
				CHECK_EQUAL( itr->second, COUNT - i );
				++i;
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check map( InputIt, InputIt, Allocator = Allocator() )
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2( container1.begin(), container1.end() );
			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check copy constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2( container1 );
			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
		{
			// check move constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2 = testMove(container1);

			for (typename ContainerClass::const_iterator itr1 = container1.begin(), itr2 = container2.begin() ;
				itr1 != container1.end() && itr2 != container2.end(); ++itr1, ++itr2 )
			{
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
	}
};

TEST( BWContainers_testBwMap )
{
	MapTester< BW::map< size_t, size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

TEST( BWContainers_testBwMultiMap )
{
	MapTester< BW::multimap< size_t, size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}


template< class ContainerClass >
class UnorderedMapTester
{
public:
	TEST_SUB_FUNCTOR_OPERATOR
	{
		static const size_t COUNT = 5;

		PREPARE_ALLOCATION_STATS();

		{
			// check default constructor
			ContainerClass container;
			for ( size_t i = 0; i < COUNT; ++i ) 
			{
				container.insert( std::make_pair( i, COUNT - i ) );
			}

			CHECK_ALLOCATION_INCREASE();

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr = container.find( i );
				CHECK_EQUAL( itr->first, i );
				CHECK_EQUAL( itr->second, COUNT - i );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check unordered_map( InputIt, InputIt )
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2( container1.begin(), container1.end() );

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( itr1->first, i );
				CHECK_EQUAL( itr1->second, COUNT - i );
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check copy constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2( container1 );

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( itr1->first, i );
				CHECK_EQUAL( itr1->second, COUNT - i );
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
		{
			// check move constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( std::make_pair( i, COUNT - i ) );

			ContainerClass container2 = testMove(container1);

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( itr1->first, i );
				CHECK_EQUAL( itr1->second, COUNT - i );
				CHECK_EQUAL( itr1->first, itr2->first );
				CHECK_EQUAL( itr1->second, itr2->second );
			}
		}
		CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
	}
};

TEST( BWContainers_testBwUnorderedMap )
{
	UnorderedMapTester< BW::unordered_map< size_t, size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

TEST( BWContainers_testBwUnorderedMultiMap )
{
	UnorderedMapTester< BW::unordered_multimap< size_t, size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

template< class ContainerClass >
class UnorderedSetTester
{
public:
	TEST_SUB_FUNCTOR_OPERATOR
	{
		static const size_t COUNT = 5;

		PREPARE_ALLOCATION_STATS();

		{
			// check default constructor
			ContainerClass container;
			for ( size_t i = 0; i < COUNT; ++i ) 
			{
				container.insert( i );
			}

			CHECK_ALLOCATION_INCREASE();

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr = container.find( i );
				CHECK_EQUAL( *itr, i );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check unordered_set( InputIt, InputIt )
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( i );

			ContainerClass container2( container1.begin(), container1.end() );

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( *itr1, i );
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();

		{
			// check copy constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( i );

			ContainerClass container2( container1 );

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( *itr1, i );
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();

#ifdef _HAS_CXX11_MOVE
		{
			// check move constructor
			ContainerClass container1;
			for (size_t i = 0; i < COUNT; ++i) container1.insert( i );

			ContainerClass container2 = testMove(container1);

			for ( size_t i = 0; i < COUNT; ++i )
			{
				typename ContainerClass::const_iterator itr1 = container1.find( i ), itr2 = container2.find( i );
				CHECK_EQUAL( *itr1, i );
				CHECK_EQUAL( *itr1, *itr2 );
			}
		}
		CHECK_ALLOCATION_FREED();
#endif // _HAS_CXX11_MOVE
	}
};

TEST( BWContainers_testBwUnorderedSet )
{
	UnorderedSetTester< BW::unordered_set< size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

TEST( BWContainers_testBwUnorderedMultiSet )
{
	UnorderedSetTester< BW::unordered_multiset< size_t > > tester;
	TEST_SUB_FUNCTOR(tester);
}

BW_END_NAMESPACE
