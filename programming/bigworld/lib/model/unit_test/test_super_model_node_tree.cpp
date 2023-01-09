#include "pch.hpp"

#include <algorithm>
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "moo/node.hpp"
#include "moo/node_catalogue.hpp"
#include "model/super_model_node_tree.hpp"


namespace
{
	using namespace BW;

	/**
	 *	Used to describe node graphs for testing purposes.
	 */
	class TestTreeNode
	{
	public:
		/**
		 *	@pre A tree cannot be constructed that contains cycles.
		 */
		TestTreeNode(
			const BW::string identifier,
			const BW::set<TestTreeNode> & children = BW::set<TestTreeNode>() )
			:
			identifier_( identifier )
			, children_( children )
		{
			BW_GUARD;
		}

		/**
		 *	@return The number of nodes in the tree starting at this node
		 *		(including this node).
		 */
		int getNumTreeNodes() const
		{
			BW_GUARD;

			int result = 1;

			for (BW::set<TestTreeNode>::const_iterator itr = children_.begin();
				itr != children_.end();
				++itr)
			{
				result += itr->getNumTreeNodes();
			}

			return result;
		}

		/**
		 *	Inserts this node's identifier and those of its children into
		 *	@a identifiers.
		 */
		void getAllIdentifiers( BW::set<BW::string> & identifiers ) const
		{
			BW_GUARD;

			identifiers.insert( identifier_ );

			for (BW::set<TestTreeNode>::const_iterator itr = children_.begin();
				itr != children_.end();
				++itr)
			{
				itr->getAllIdentifiers( identifiers );
			}
		}

		bool operator == ( const TestTreeNode & rhs ) const
		{
			BW_GUARD;

			return
				(identifier_ == rhs.identifier_)
				&&
				(children_ == rhs.children_);
		}

		bool operator < ( const TestTreeNode & rhs ) const
		{
			BW_GUARD;

			if (identifier_ == rhs.identifier_)
			{
				return children_ < rhs.children_;
			}
			return identifier_ < rhs.identifier_;
		}

		///The name for this node.
		const BW::string identifier_;
		///The children for this node.
		const BW::set<TestTreeNode> children_;
	};


	///Helper for createMooNodeFromTestSpec that adds children recursively.
	void addChildToMooNode( Moo::Node & mooNode,
		const TestTreeNode & testSpecChildNode )
	{
		BW_GUARD;

		Moo::NodePtr mooChildNode( new Moo::Node() );
		mooChildNode->identifier( testSpecChildNode.identifier_ );
		//Add to the catalogue.
		Moo::NodeCatalogue::findOrAdd( mooChildNode );
		mooNode.addChild( mooChildNode );

		for (BW::set<TestTreeNode>::const_iterator itr =
				testSpecChildNode.children_.begin();
			itr != testSpecChildNode.children_.end();
			++itr)
		{
			addChildToMooNode( *mooChildNode, *itr );
		}
	}


	/**
	 *	@pre @a testSpecNode doesn't specify a cycle.
	 *	@post Returned a NodePtr with children created (recursively) to match
	 *		the graph for testSpecNode.
	 */
	Moo::NodePtr createMooNodeFromTestSpec(
		const TestTreeNode & testSpecNode )
	{
		BW_GUARD;

		Moo::NodePtr mooNode( new Moo::Node() );
		mooNode->identifier( testSpecNode.identifier_ );
		//Add to the catalogue.
		Moo::NodeCatalogue::findOrAdd( mooNode );
		
		for (BW::set<TestTreeNode>::const_iterator itr =
				testSpecNode.children_.begin();
			itr != testSpecNode.children_.end();
			++itr)
		{
			addChildToMooNode( *mooNode, *itr );
		}

		return mooNode;
	}


	/**
	 *	Attempts to add nodes matching the graph of @a nodeToAdd into @a tree.
	 *	@return True if the addition was successful. On false return @a tree
	 *		state is indeterminate and it should not be used.
	 */
	bool addNodesFromTestSpec(
		SuperModelNodeTree & tree,
		const TestTreeNode & nodeToAdd )
	{
		BW_GUARD;

		Moo::NodePtr mooNodeToAdd = createMooNodeFromTestSpec( nodeToAdd );
		return tree.addAllNodes( *mooNodeToAdd );
	}


	/**
	 *	@return a TestTreeNode representing the same graph as @a tree.
	 */
	TestTreeNode getTestSpecFromTreeNode(
		const SuperModelNodeTree & tree, int treeNodeIndex )
	{
		BW_GUARD;

		const BW::SuperModelNode & node = tree.getNode( treeNodeIndex );

		BW::set<TestTreeNode> children;
		for(int childNum = 0; childNum < node.getNumChildren(); ++childNum)
		{
			children.insert(
				getTestSpecFromTreeNode(tree, node.getChild(childNum) ) );
		}

		return TestTreeNode( node.getIdentifier(), children );
	}


	/**
	 *	This functor tests that graph of a SuperModelNodeTree is as expected
	 *	and that searches for all of the nodes work as expected.
	 */
	class CompareSuperModelNodeTreeToTestTree
	{
	public:
		/**
		 *	@param expectedRootNodes All of the root hierarchies expected to be
		 *		present in the tree.
		 *	@param tree The tree to test against to @a expectedRootNodes.
		 */
		CompareSuperModelNodeTreeToTestTree(
			const BW::set<TestTreeNode> & expectedRootNodes,
			const SuperModelNodeTree & tree )
			:
			expectedRootNodes_( expectedRootNodes )
			, tree_( tree )
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;


			RETURN_ON_FAIL_CHECK( expectedRootNodes_.size() ==
				static_cast<std::size_t>(tree_.getNumRootNodes()) );


			//Total expected number of nodes.
			int expectedNumNodes = 0;
			BW::set<BW::string> allNodeIdentifiers;
			for (BW::set<TestTreeNode>::const_iterator itr =
					expectedRootNodes_.begin();
				itr != expectedRootNodes_.end();
				++itr)
			{
				expectedNumNodes += itr->getNumTreeNodes();
				itr->getAllIdentifiers( allNodeIdentifiers );
			}
			MF_ASSERT( allNodeIdentifiers.size() ==
				static_cast<std::size_t>(expectedNumNodes) );
			RETURN_ON_FAIL_CHECK( expectedNumNodes == tree_.getNumNodes() );

			
			//Access every node.
			BW::set<int> allNodeIndices;
			for (BW::set<BW::string>::const_iterator itr =
					allNodeIdentifiers.begin();
				itr != allNodeIdentifiers.end();
				++itr)
			{
				const int nodeIndex = tree_.getNodeIndex( *itr );

				if ( nodeIndex >= 0 )
				{
					const SuperModelNode & node = tree_.getNode( nodeIndex );
					CHECK_EQUAL( *itr, node.getIdentifier() );
					CHECK_EQUAL( *itr, node.getMooNode()->identifier() );
					allNodeIndices.insert( nodeIndex );
				}
				else
				{
					RETURN_ON_FAIL_CHECK( false && "Node search failed. " );
				}
			}
			RETURN_ON_FAIL_CHECK(
				allNodeIndices.size() == allNodeIdentifiers.size() );
			

			//For each root node, extract out a tree from tree_ and
			//check that they match.
			for (BW::set<TestTreeNode>::const_iterator itr =
					expectedRootNodes_.begin();
				itr != expectedRootNodes_.end();
				++itr)
			{
				const int rootNodeIndex =
					tree_.getNodeIndex( itr->identifier_ );
				MF_ASSERT( rootNodeIndex >= 0 );

				//Expect no parent for this node.
				CHECK( tree_.getNode( rootNodeIndex ).getParentIndex() < 0 );

				const TestTreeNode graphFromRootNode(
					getTestSpecFromTreeNode( tree_, rootNodeIndex ) );
				CHECK( *itr == graphFromRootNode );
			}
		}

	private:
		///Expected nodes for test tree.
		const BW::set<TestTreeNode> expectedRootNodes_;
		///The test tree.
		const SuperModelNodeTree tree_;
	};


	/**
	 *	This functor creates a tree by adding all nodes from a given set of
	 *	root nodes. It then checks it using
	 *	CompareSuperModelNodeTreeToTestTree.
	 */
	class CheckSuperModelNodeTreeCreation
	{
	public:
		/**
		 *	@param testRootNodes All of the root hierarchies expected to be
		 *		present in the tree that's tested.
		 *	@pre @a testRootNodes defines non-interesting sets of node
		 *		identifiers with no cycles.
		 */
		explicit CheckSuperModelNodeTreeCreation(
			const BW::set<TestTreeNode> & testRootNodes )
			:
			testRootNodes_( testRootNodes )
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;
			
			SuperModelNodeTree testTree;

			//We do the addition twice, because the second addition should
			//succeed and have no effect (all should already exist in the
			//tree and be compatible).
			for(int pass = 0; pass < 2; ++pass)
			{
				for (BW::set<TestTreeNode>::const_iterator itr =
						testRootNodes_.begin();
					itr != testRootNodes_.end();
					++itr)
				{
					RETURN_ON_FAIL_CHECK(
						addNodesFromTestSpec( testTree, *itr ) );
				}

				CompareSuperModelNodeTreeToTestTree checkTreeFunc(
					testRootNodes_, testTree );
				TEST_SUB_FUNCTOR( checkTreeFunc );
			}
		}

	private:
		///Root nodes to use in the test.
		const BW::set<TestTreeNode> testRootNodes_;
	};


	/**
	 *	This functor is used to check that the result from getPathIndices is
	 *	as expected.
	 */
	class CheckPathIndicesCalculation
	{
	public:
		/**
		 *	@param tree Tree to test on.
		 *	@param startIndex/endIndex The index of the start/end node on
		 *		the path.
		 *	@param expectedPath The expected path to be generated. If this is
		 *		empty, it indicates that no path is expected to be found.
		 *	@pre @a startIndex and @a endIndex are both in the
		 *		range [0, tree.getNumNodes()).
		 *	@post Created object ready to run the test.
		 */
		CheckPathIndicesCalculation(
			const SuperModelNodeTree & tree,
			int startIndex, int endIndex,
			const vector<int> & expectedPath )
			:
			tree_( tree )
			, startIndex_( startIndex )
			, endIndex_( endIndex )
			, expectedPath_( expectedPath )
		{
			BW_GUARD;

			MF_ASSERT( 0 <= startIndex_ );
			MF_ASSERT( startIndex_ < tree.getNumNodes() );
			MF_ASSERT( 0 <= endIndex_ );
			MF_ASSERT( endIndex_ < tree.getNumNodes() );
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;
			
			//Forward.
			BW::vector<int> outputPath;
			bool searchRes =
				tree_.getPathIndices( outputPath, startIndex_, endIndex_ );
			if ( expectedPath_.empty() )
			{
				CHECK_EQUAL( false, searchRes );
				CHECK( outputPath.empty() );
			}
			else
			{
				CHECK( expectedPath_ == outputPath );
			}
			
			//Reverse.
			searchRes =
				tree_.getPathIndices( outputPath, endIndex_, startIndex_ );
			if ( expectedPath_.empty() )
			{
				CHECK_EQUAL( false, searchRes );
				CHECK( outputPath.empty() );
			}
			else
			{
				RETURN_ON_FAIL_CHECK(
					outputPath.size() == expectedPath_.size() );
				CHECK(
					std::equal( expectedPath_.rbegin(), expectedPath_.rend(),
						outputPath.begin() ) );
			}
		}

	private:
		///Tree to test on.
		const SuperModelNodeTree tree_;
		///Node index of path start.
		int startIndex_;
		///Node index of path end.
		int endIndex_;
		///Tree to test on.
		const vector<int> expectedPath_;
	};
}


TEST( SuperModelNodeTree_NoNodes )
{
	BW_GUARD;

	SuperModelNodeTree testTree;
	CHECK_EQUAL( 0, testTree.getNumNodes() );
	CHECK_EQUAL( 0, testTree.getNumRootNodes() );
	CHECK( testTree.getNodeIndex( "anything" ) < 0 );

	BW::set<TestTreeNode> expectedRootNodes;
	CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
	TEST_SUB_FUNCTOR( checkTreeFunc );
};


TEST( SuperModelNodeTree_OneNode )
{
	BW_GUARD;

	const TestTreeNode a( "a" );
	BW::set<TestTreeNode> expectedRootNodes;
	expectedRootNodes.insert( a );

	CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
	TEST_SUB_FUNCTOR( checkTreeFunc );
};


TEST( SuperModelNodeTree_TwoNodes )
{
	BW_GUARD;

	//A(B())
	{
		const TestTreeNode b( "b" );
		BW::set<TestTreeNode> aChildren;
		aChildren.insert( b );
		const TestTreeNode a( "a", aChildren );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}

	//A();B()
	{
		const TestTreeNode b( "b" );
		const TestTreeNode a( "a" );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );
		expectedRootNodes.insert( b );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}
};


TEST( SuperModelNodeTree_ThreeNodes )
{
	BW_GUARD;

	//A(B(),C())
	{
		const TestTreeNode c( "c" );
		const TestTreeNode b( "b" );
		BW::set<TestTreeNode> aChildren;
		aChildren.insert( b );
		aChildren.insert( c );
		const TestTreeNode a( "a", aChildren );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}

	//A(B(C()))
	{
		const TestTreeNode c( "c" );
		BW::set<TestTreeNode> bChildren;
		bChildren.insert( c );
		const TestTreeNode b( "b", bChildren );
		BW::set<TestTreeNode> aChildren;
		aChildren.insert( b );
		const TestTreeNode a( "a", aChildren );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}

	//A(B());C()
	{
		const TestTreeNode c( "c" );
		const TestTreeNode b( "b" );
		BW::set<TestTreeNode> aChildren;
		aChildren.insert( b );
		const TestTreeNode a( "a", aChildren );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );
		expectedRootNodes.insert( c );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}

	//A();B();C()
	{
		const TestTreeNode c( "c" );
		const TestTreeNode b( "b" );
		const TestTreeNode a( "a" );
		BW::set<TestTreeNode> expectedRootNodes;
		expectedRootNodes.insert( a );
		expectedRootNodes.insert( b );
		expectedRootNodes.insert( c );

		CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
		TEST_SUB_FUNCTOR( checkTreeFunc );
	}
};


namespace
{
	/**
	 *	This helper function sets @a rootNodes to contain the root nodes for
	 *	the node graph shown below.
	 *	
	 *	A(
	 *		B(D()),
	 *		C(
	 *			E(),
	 *			F(
	 *				G(
	 *					H()
	 *				)
	 *			)
	 *		)
	 *	)
	 */
	BW::set<TestTreeNode> getComplexTestTree1()
	{
		const TestTreeNode h( "h" );

		BW::set<TestTreeNode> gChildren;
		gChildren.insert( h );
		const TestTreeNode g( "g", gChildren );

		BW::set<TestTreeNode> fChildren;
		fChildren.insert( g );
		const TestTreeNode f( "f", fChildren );

		const TestTreeNode e( "e" );

		const TestTreeNode d( "d" );

		BW::set<TestTreeNode> cChildren;
		cChildren.insert( f );
		cChildren.insert( e );
		const TestTreeNode c( "c", cChildren );

		BW::set<TestTreeNode> bChildren;
		bChildren.insert( d );
		const TestTreeNode b( "b", bChildren );

		BW::set<TestTreeNode> aChildren;
		aChildren.insert( c );
		aChildren.insert( b );
		const TestTreeNode a( "a", aChildren );

		BW::set<TestTreeNode> result;
		result.insert( a );
		return result;
	}


	/**
	 *	This helper function sets @a rootNodes to contain the root nodes for
	 *	the node graph shown below.
	 *	
	 *	A(
	 *		B(C(), D()),
	 *		E()
	 *	);
	 *	F(
	 *		G(),
	 *		H(I())
	 *	)
	 */
	BW::set<TestTreeNode> getComplexTestTree2()
	{
		const TestTreeNode i( "i" );

		BW::set<TestTreeNode> hChildren;
		hChildren.insert( i );
		const TestTreeNode h( "h", hChildren );

		const TestTreeNode g( "g" );

		BW::set<TestTreeNode> fChildren;
		fChildren.insert( g );
		fChildren.insert( h );
		const TestTreeNode f( "f", fChildren );

		const TestTreeNode e( "e" );

		const TestTreeNode d( "d" );

		const TestTreeNode c( "c" );

		BW::set<TestTreeNode> bChildren;
		bChildren.insert( c );
		bChildren.insert( d );
		const TestTreeNode b( "b", bChildren );

		BW::set<TestTreeNode> aChildren;
		aChildren.insert( b );
		aChildren.insert( e );
		const TestTreeNode a( "a", aChildren );

		BW::set<TestTreeNode> result;
		result.insert( a );
		result.insert( f );
		return result;
	}

	
	/**
	 *	@pre @a testNodes describes a set of nodes compatible with
	 *		SuperModelNodeTree.
	 *	@return A SuperModelNodeTree populated with @a testNodes.
	 */
	SuperModelNodeTree getAsSuperModelNodeTree(
		const BW::set<TestTreeNode> & testNodes )
	{
		SuperModelNodeTree result;

		for (BW::set<TestTreeNode>::const_iterator itr = testNodes.begin();
			itr != testNodes.end();
			++itr)
		{
			const bool addSuccess = addNodesFromTestSpec( result, *itr );
			MF_ASSERT( addSuccess );
		}

		return result;
	}
}


TEST( SuperModelNodeTree_Complex )
{
	BW_GUARD;

	{
		CheckSuperModelNodeTreeCreation testFunc( ::getComplexTestTree1() );
		TEST_SUB_FUNCTOR( testFunc );
	}

	{
		CheckSuperModelNodeTreeCreation testFunc( ::getComplexTestTree2() );
		TEST_SUB_FUNCTOR( testFunc );
	}
};


TEST( SuperModelNodeTree_SubsetAddition )
{
	BW_GUARD;

	//Structure of test tree.
	//A(
	//	B(D()),
	//	C(
	//		E(),
	//		F(
	//			G(
	//				H()
	//			)
	//		)
	//	)
	//)

	SuperModelNodeTree tree;
	BW::set<TestTreeNode> expectedRootNodes;


	//------------------------
	const TestTreeNode h( "h" );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, h ) );

	expectedRootNodes.insert( h );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	BW::set<TestTreeNode> gChildren;
	gChildren.insert( h );
	const TestTreeNode g( "g", gChildren );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, g ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( g );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	BW::set<TestTreeNode> fChildren;
	fChildren.insert( g );
	const TestTreeNode f( "f", fChildren );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, f ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( f );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	const TestTreeNode e( "e" );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, e ) );

	expectedRootNodes.insert( e );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	const TestTreeNode d( "d" );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, d ) );

	expectedRootNodes.insert( d );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	BW::set<TestTreeNode> cChildren;
	cChildren.insert( f );
	cChildren.insert( e );
	const TestTreeNode c( "c", cChildren );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, c ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( c );
	expectedRootNodes.insert( d );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	BW::set<TestTreeNode> bChildren;
	bChildren.insert( d );
	const TestTreeNode b( "b", bChildren );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, b ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( c );
	expectedRootNodes.insert( b );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	{
		//Add a with no children first, to test the trees correctly merge when
		//it's added with children.
		
		const TestTreeNode a( "a" );

		RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, a ) );

		expectedRootNodes.clear();
		expectedRootNodes.insert( a );
		expectedRootNodes.insert( b );
		expectedRootNodes.insert( c );
		TEST_SUB_FUNCTOR(
			CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );
	}


	//------------------------
	BW::set<TestTreeNode> aChildren;
	aChildren.insert( c );
	aChildren.insert( b );
	const TestTreeNode a( "a", aChildren );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, a ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( a );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );

	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, b ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, c ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, d ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, e ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, f ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, g ) );
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, h ) );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );
		

	//------------------------
	CheckSuperModelNodeTreeCreation checkTreeFunc( expectedRootNodes );
	TEST_SUB_FUNCTOR( checkTreeFunc );


	//----Other Combinations From Top----

	tree = SuperModelNodeTree();


	//------------------------
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, c ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( c );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, b ) );

	expectedRootNodes.insert( b );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, f ) );

	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );


	//------------------------
	RETURN_ON_FAIL_CHECK( addNodesFromTestSpec( tree, a ) );

	expectedRootNodes.clear();
	expectedRootNodes.insert( a );
	TEST_SUB_FUNCTOR(
		CompareSuperModelNodeTreeToTestTree( expectedRootNodes, tree ) );
};


TEST( SuperModelNodeTree_DetectIncompatibleData )
{
	//Structure of test tree.
	//A(
	//	B(D()),
	//	C(
	//		E(),
	//		F(
	//			G(
	//				H()
	//			)
	//		)
	//	)
	//)
	const SuperModelNodeTree baseTestTree( getAsSuperModelNodeTree(
		getComplexTestTree1() ) );

	//Different parent.
	{
		SuperModelNodeTree testTree( baseTestTree );

		//Add f with child of h. Should fail because h already child of g.
		const TestTreeNode h( "h" );
		BW::set<TestTreeNode> fChildren;
		fChildren.insert( h );
		const TestTreeNode f( "f", fChildren );

		Moo::NodePtr nodeToAdd = createMooNodeFromTestSpec( f );

		CHECK_EQUAL( false, testTree.addAllNodes( *nodeToAdd ) );
	}

	//Different parent.
	{
		SuperModelNodeTree testTree( baseTestTree );

		//Add h with child of b. Should fail because b already child of a.
		const TestTreeNode b( "b" );
		BW::set<TestTreeNode> hChildren;
		hChildren.insert( b );
		const TestTreeNode h( "h", hChildren );

		Moo::NodePtr nodeToAdd = createMooNodeFromTestSpec( h );

		CHECK_EQUAL( false, testTree.addAllNodes( *nodeToAdd ) );
	}

	//Cycle
	{
		SuperModelNodeTree testTree( baseTestTree );

		//Add e with child of a. Should fail because it will add a cycle.
		const TestTreeNode a( "a" );
		BW::set<TestTreeNode> eChildren;
		eChildren.insert( a );
		const TestTreeNode e( "e", eChildren );

		Moo::NodePtr nodeToAdd = createMooNodeFromTestSpec( e );

		CHECK_EQUAL( false, testTree.addAllNodes( *nodeToAdd ) );
	}
}


TEST( SuperModelNodeTree_GetPathIds )
{
	{
		//Structure of test tree.
		//A(
		//	B(D()),
		//	C(
		//		E(),
		//		F(
		//			G(
		//				H()
		//			)
		//		)
		//	)
		//)
		const SuperModelNodeTree tree( getAsSuperModelNodeTree(
			getComplexTestTree1() ) );
		BW::vector<int> expectedChain;

		//H->H
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "h" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "h" ),
			expectedChain ) );

		//H->G
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "h" ) );
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "g" ),
			expectedChain ) );

		//H->F
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "h" ) );
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		expectedChain.push_back( tree.getNodeIndex( "f" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "f" ),
			expectedChain ) );

		//H->A
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "h" ) );
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		expectedChain.push_back( tree.getNodeIndex( "f" ) );
		expectedChain.push_back( tree.getNodeIndex( "c" ) );
		expectedChain.push_back( tree.getNodeIndex( "a" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "a" ),
			expectedChain ) );

		//B->C
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "b" ) );
		expectedChain.push_back( tree.getNodeIndex( "a" ) );
		expectedChain.push_back( tree.getNodeIndex( "c" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "b" ),
			tree.getNodeIndex( "c" ),
			expectedChain ) );

		//H->D
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "h" ) );
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		expectedChain.push_back( tree.getNodeIndex( "f" ) );
		expectedChain.push_back( tree.getNodeIndex( "c" ) );
		expectedChain.push_back( tree.getNodeIndex( "a" ) );
		expectedChain.push_back( tree.getNodeIndex( "b" ) );
		expectedChain.push_back( tree.getNodeIndex( "d" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "d" ),
			expectedChain ) );

		//C->G
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "c" ) );
		expectedChain.push_back( tree.getNodeIndex( "f" ) );
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "c" ),
			tree.getNodeIndex( "g" ),
			expectedChain ) );

		//G->B
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "g" ) );
		expectedChain.push_back( tree.getNodeIndex( "f" ) );
		expectedChain.push_back( tree.getNodeIndex( "c" ) );
		expectedChain.push_back( tree.getNodeIndex( "a" ) );
		expectedChain.push_back( tree.getNodeIndex( "b" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "g" ),
			tree.getNodeIndex( "b" ),
			expectedChain ) );
	}

	
	{
		//Structure of test tree.
		//A(
		//	B(C(), D()),
		//	E()
		//);
		//F(
		//	G(),
		//	H(I())
		//);
		const SuperModelNodeTree tree( getAsSuperModelNodeTree(
			getComplexTestTree2() ) );
		BW::vector<int> expectedChain;

		//A->F
		expectedChain.clear();
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "a" ),
			tree.getNodeIndex( "f" ),
			expectedChain ) );

		//H->D
		expectedChain.clear();
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "h" ),
			tree.getNodeIndex( "d" ),
			expectedChain ) );

		//A->D
		expectedChain.clear();
		expectedChain.push_back( tree.getNodeIndex( "a" ) );
		expectedChain.push_back( tree.getNodeIndex( "b" ) );
		expectedChain.push_back( tree.getNodeIndex( "d" ) );
		TEST_SUB_FUNCTOR( CheckPathIndicesCalculation(
			tree,
			tree.getNodeIndex( "a" ),
			tree.getNodeIndex( "d" ),
			expectedChain ) );
	}
}
