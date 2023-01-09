#include "pch.hpp"

#include "cstdmf/dependency_graph.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"



BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	The output ordering is not strongly defined for the dependency graph,
	 *	since multiple sorts could satisfy the dependencies.
	 *	This complicates testing by making it so that the expected result
	 *	must be given as a set of (node, nodes that must appear after it
	 *	in the ordering).
	 *	This type maps a key onto a vector node keys that must appear
	 *	later in the order (because those keys depend on the first one).
	 */
	typedef
		BW::unordered_map< BW::string, BW::vector<BW::string> >
		ExpectedOrderings;


	/**
	 *	@return True if @a ordering is valid given @a expectedOrderings.
	 */
	bool checkExpectedOrdering(
		const BW::vector<BW::string> & ordering,
		const ExpectedOrderings & expectedOrderings )
	{
		BW_GUARD;

		//Map key to its index in @a ordering.
		typedef BW::vector<BW::string> OrderingVector;
		typedef OrderingVector::size_type OrderingVectorSizeT;
		typedef
			BW::unordered_map<BW::string, OrderingVectorSizeT>
			OrderingIndexMap;
		OrderingIndexMap orderingIndexMap;
		for (OrderingVectorSizeT index = 0; index < ordering.size(); ++index)
		{
			orderingIndexMap[ordering[index]] = index;
		}
		if (orderingIndexMap.size() != ordering.size())
		{
			DEBUG_MSG( "Non-unique elements in DependencyGraph ordering.\n" );
			return false;
		}
		if (orderingIndexMap.size() != expectedOrderings.size())
		{
			DEBUG_MSG( "Expected and actual order sizes are different.\n" );
			return false;
		}

		//Check nodes that should be later in the order really are.
		for (ExpectedOrderings::const_iterator expItr =
				expectedOrderings.begin();
			expItr != expectedOrderings.end();
			++expItr)
		{
			OrderingIndexMap::const_iterator srchKeyRes =
				orderingIndexMap.find( expItr->first );
			if (srchKeyRes == orderingIndexMap.end())
			{
				DEBUG_MSG( "Node missing from ordering.\n" );
				return false;
			}

			for (BW::vector<BW::string>::const_iterator laterItr =
					expItr->second.begin();
				laterItr != expItr->second.end();
				++laterItr)
			{
				OrderingIndexMap::const_iterator srchLaterKeyRes =
					orderingIndexMap.find( *laterItr );
				if (srchLaterKeyRes == orderingIndexMap.end())
				{
					DEBUG_MSG( "Node missing from ordering.\n" );
					return false;
				}

				if ( srchKeyRes->second >= srchLaterKeyRes->second )
				{
					return false;
				}
			}
		}

		return true;
	}

	/**
	 *	This functor takes an expected ordering, and uses that to generate
	 *	a graph and check that it gives back a compatible ordering.
	 */
	class TestGraphOrdering
	{
	public:
		/**
		 *	@param expectedOrderings Describes dependencies to use to create
		 *		a tree, them the ordering generated from the tree will be
		 *		checked for compatibility.
		 *	@param expectOrderingSuccess If this is false, it signals that
		 *		ordering in the graph is expected to fail (e.g.,
		 *		due to a cycle).
		 */
		TestGraphOrdering(
			const ExpectedOrderings & expectedOrderings,
			bool expectOrderingSuccess = true )
			:
			expectedOrderings_( expectedOrderings )
			, expectOrderingSuccess_( expectOrderingSuccess )
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			//Generate a graph with the correct dependencies.
			DependencyGraph<BW::string> graph;
			addExpectedNodesToGraph( graph );
			RETURN_ON_FAIL_CHECK(
				expectedOrderings_.size() == graph.size() );
			addExpectedDependenciesToGraph( graph );
			
			BW::vector<BW::string> ordering;
			//Shouldn't matter what the input state of ordering is.
			ordering.push_back( "something" );
			RETURN_ON_FAIL_CHECK(
				expectOrderingSuccess_ == graph.getOrder( ordering ) );
			if (expectOrderingSuccess_)
			{
				RETURN_ON_FAIL_CHECK(
					checkExpectedOrdering( ordering, expectedOrderings_ ) );
			}
			else
			{
				RETURN_ON_FAIL_CHECK( ordering.empty() );
			}


			//Should be able to add again without affecting the result.
			addExpectedNodesToGraph( graph );
			RETURN_ON_FAIL_CHECK(
				expectedOrderings_.size() == graph.size() );
			addExpectedDependenciesToGraph( graph );

			ordering.clear();
			RETURN_ON_FAIL_CHECK(
				expectOrderingSuccess_ == graph.getOrder( ordering ) );
			if (expectOrderingSuccess_)
			{
				RETURN_ON_FAIL_CHECK(
					checkExpectedOrdering( ordering, expectedOrderings_ ) );
			}
			else
			{
				RETURN_ON_FAIL_CHECK( ordering.empty() );
			}


			//Should be able to regenerate after doing a clear.
			graph.clear();
			addExpectedNodesToGraph( graph );
			RETURN_ON_FAIL_CHECK(
				expectedOrderings_.size() == graph.size() );
			addExpectedDependenciesToGraph( graph );

			ordering.clear();
			RETURN_ON_FAIL_CHECK(
				expectOrderingSuccess_ == graph.getOrder( ordering ) );
			if (expectOrderingSuccess_)
			{
				RETURN_ON_FAIL_CHECK(
					checkExpectedOrdering( ordering, expectedOrderings_ ) );
			}
			else
			{
				RETURN_ON_FAIL_CHECK( ordering.empty() );
			}
		}

	private:
		/**
		 *	@pre True.
		 *	@post Added the nodes from expectedOrderings_ to @a graph.
		 */
		void addExpectedNodesToGraph(
			DependencyGraph<BW::string> & graph )
		{
			for (ExpectedOrderings::const_iterator expItr =
					expectedOrderings_.begin();
				expItr != expectedOrderings_.end();
				++expItr)
			{
				graph.addNode( expItr->first );

				for (BW::vector<BW::string>::const_iterator laterItr =
						expItr->second.begin();
					laterItr != expItr->second.end();
					++laterItr)
				{
					graph.addNode( *laterItr );
				}
			}
		}

		/**
		 *	@pre All expected nodes are already in @a graph.
		 *	@post Added the dependencies from expectedOrderings_ to @a graph.
		 */
		void addExpectedDependenciesToGraph(
			DependencyGraph<BW::string> & graph )
		{
			for (ExpectedOrderings::const_iterator expItr =
					expectedOrderings_.begin();
				expItr != expectedOrderings_.end();
				++expItr)
			{
				for (BW::vector<BW::string>::const_iterator laterItr =
						expItr->second.begin();
					laterItr != expItr->second.end();
					++laterItr)
				{
					graph.addDependency( *laterItr, expItr->first );
				}
			}
		}


		///The node ordering requirements for the graph getOrder output.
		const ExpectedOrderings expectedOrderings_;
		///Indicates whether ordering is expected to be possible.
		bool expectOrderingSuccess_;
	};
}


TEST( DependencyGraph_Empty )
{
	BW_GUARD;

	ExpectedOrderings expectedOrderings;

	TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
}


TEST( DependencyGraph_OneNode )
{
	BW_GUARD;

	ExpectedOrderings expectedOrderings;
	expectedOrderings["a"];

	TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
}


TEST( DependencyGraph_TwoNodes )
{
	BW_GUARD;

	//No dependencies
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"];
		expectedOrderings["b"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}

	//b depends on a
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"] = BW::vector<BW::string>( 1, "b" );
		expectedOrderings["b"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}
}


TEST( DependencyGraph_ThreeNodes )
{
	BW_GUARD;

	//No dependencies
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"];
		expectedOrderings["b"];
		expectedOrderings["c"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}

	//(b->a;c)
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"] = BW::vector<BW::string>( 1, "b" );
		expectedOrderings["b"];
		expectedOrderings["c"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}

	//(c->b->a)
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"] = BW::vector<BW::string>( 1, "b" );
		expectedOrderings["b"] = BW::vector<BW::string>( 1, "c" );
		expectedOrderings["c"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}

	//((c,b)->a)
	{
		ExpectedOrderings expectedOrderings;
		{
			BW::vector<BW::string> appearAfterA;
			appearAfterA.push_back( "b" );
			appearAfterA.push_back( "c" );
			expectedOrderings["a"] = appearAfterA;
		}
		expectedOrderings["b"];
		expectedOrderings["c"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}
}


TEST( DependencyGraph_ManyNodes )
{
	BW_GUARD;

	//((c,b,f)->a);(g->b);(d->c);(e->d);
	{
		ExpectedOrderings expectedOrderings;
		{
			BW::vector<BW::string> appearAfterA;
			appearAfterA.push_back( "b" );
			appearAfterA.push_back( "c" );
			appearAfterA.push_back( "f" );
			expectedOrderings["a"] = appearAfterA;
		}
		expectedOrderings["b"] = BW::vector<BW::string>( 1, "g" );
		expectedOrderings["c"] = BW::vector<BW::string>( 1, "d" );
		expectedOrderings["d"] = BW::vector<BW::string>( 1, "e" );
		expectedOrderings["e"];
		expectedOrderings["f"];
		expectedOrderings["g"];

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings ) );
	}
}


TEST( DependencyGraph_Cycles )
{
	BW_GUARD;

	//(a->b);(b->a);
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"] = BW::vector<BW::string>( 1, "b" );
		expectedOrderings["b"] = BW::vector<BW::string>( 1, "a" );

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings, false ) );
	}

	//(a->b);(b->c);(c->a);
	{
		ExpectedOrderings expectedOrderings;
		expectedOrderings["a"] = BW::vector<BW::string>( 1, "b" );
		expectedOrderings["b"] = BW::vector<BW::string>( 1, "c" );
		expectedOrderings["c"] = BW::vector<BW::string>( 1, "a" );

		TEST_SUB_FUNCTOR( TestGraphOrdering( expectedOrderings, false ) );
	}
}

BW_END_NAMESPACE

// test_dependency_graph.cpp
