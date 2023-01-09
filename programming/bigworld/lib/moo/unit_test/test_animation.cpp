#include "pch.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/guard.hpp"
#include "animation_test_helper.hpp"
#include "math/math_extra.hpp"
#include "moo/animation.hpp"
#include "moo/animation_manager.hpp"
#include "moo/node.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "moo/streamed_animation_channel.hpp"
#include "moo/moo_math.hpp"
#include "resmgr/resmgr_test_utils.hpp"

BW_USE_NAMESPACE


namespace
{
	/**
	 *	Type used for all programmatically generated test animation channels
	 *	used in this file.
	 *	The type is important since the base AnimationChannel interface
	 *	doesn't support comparisons.
	 */
	typedef Moo::InterpolatedAnimationChannel
		TestAnimationChannel;
	typedef SmartPointer<TestAnimationChannel>
		TestAnimationChannelPtr;


	/**
	 *	Compares channels that were created for testing in this class.
	 *	It needs to assume the type because AnimationChannel objects don't
	 *	support comparison in general. Eventually they should to allow for
	 *	testing.
	 *	
	 *	@return True if the animation channels are equivalent.
	 */
	bool animationChannelsEquivalent(
		const Moo::AnimationChannelPtr & lhs,
		const Moo::AnimationChannelPtr & rhs )
	{
		if (lhs.hasObject())
		{
			if (!rhs.hasObject())
			{
				return false;
			}
		}
		else
		{
			if (rhs.hasObject())
			{
				return false;
			}

			return true;
		}

		MF_ASSERT( lhs.hasObject() );
		MF_ASSERT( rhs.hasObject() );

		const TestAnimationChannel * lhsTyped =
			dynamic_cast<const TestAnimationChannel *>(lhs.get());
		const TestAnimationChannel * rhsTyped =
			dynamic_cast<const TestAnimationChannel *>(rhs.get());

		//Only support comparison of TestAnimationChannel type.
		MF_ASSERT( lhsTyped != NULL );
		MF_ASSERT( rhsTyped != NULL );

		return lhsTyped->compareChannel( *rhsTyped );
	}


	/**
	 *	Checks the channel binders for equivalance. If @a ignoreNULLNodes is
	 *	true, the nodes in the channel binder are not checked (same as if
	 *	equivalent) if either of them is NULL. This is useful when checking
	 *	channel binders loaded from an animation file, since only the
	 *	channels are loaded (i.e., nodes are all NULL).
	 *	
	 *	@return True if @a lhs is data-equivalent to @a rhs. This means that
	 *		internal reference counts and pointer values can differ, but the
	 *		animation data must be the same (see note above about
	 *		@a ignoreNULLNodes though).
	 */
	bool channelBindersEquivalent(
		const Moo::ChannelBinder & lhs, const Moo::ChannelBinder & rhs,
		bool ignoreNULLNodes = false )
	{
		bool nodesEqual = false;
		if (ignoreNULLNodes &&
			(!lhs.node().hasObject() || !rhs.node().hasObject()))
		{
			nodesEqual = true;
		}
		else
		{
			nodesEqual =
				compareNodeHierarchies( lhs.node(), rhs.node(), false );
		}

		if (!nodesEqual)
		{
			return false;
		}

		return animationChannelsEquivalent( lhs.channel(), rhs.channel() );
	}


	/**
	 *	Creates an animation with the given identifier and @a numLevels
	 *	levels of nodes with @a nodeChildren children at each level.
	 */
	Moo::AnimationPtr generateAnimationHierarchy(
		const BW::string identifier, int numLevels, int nodeChildren )
	{
		MF_ASSERT( numLevels > 0 );
		MF_ASSERT( nodeChildren > 0 );

		const int MAX_NUM_KEY_FRAMES = 5;
		//The minimum number of key frames is 3 because a valid channel
		//must have one of each type (position, rotation, scale).
		MF_ASSERT( MAX_NUM_KEY_FRAMES >= 3 );
		const float MAX_ANIM_DURATION = 5.0f;

		Moo::AnimationPtr result = Moo::AnimationManager::instance().
			getEmptyTestAnimation( identifier );

		unsigned int seed = numLevels * nodeChildren;
		BWRandom random( seed );
		Moo::NodePtr root = createRandomNode( seed, "root" );
		BW::vector<Moo::NodePtr> previousLevelNodes;
		previousLevelNodes.push_back( root );
		for(int level = 1; level < numLevels; ++level)
		{
			BW::vector<Moo::NodePtr> thisLevelNodes;

			for(std::size_t levelNode = 0;
				levelNode < previousLevelNodes.size();
				++levelNode)
			{
				for(int nodeChild = 0;
					nodeChild < nodeChildren;
					++nodeChild)
				{
					BW::ostringstream nodeLabelStream;
					nodeLabelStream << "Level" << (level - 1) << "_Node" <<
						levelNode << "_Child" << nodeChild;
					BW::string nodeLabel( nodeLabelStream.str().c_str() );

					Moo::NodePtr child = createRandomNode(
						++seed, nodeLabel + ".Node" );
					thisLevelNodes.push_back( child );
					previousLevelNodes.at( levelNode )->addChild( child );

					InterpolatedAnimationChannelPtr channel =
						createRandomInterpolatedAnimationChannel(
							++seed,
							3 + random( 0, MAX_NUM_KEY_FRAMES - 3 ),
							random( 0.0f, MAX_ANIM_DURATION ),
							true//true since not using compression.
							);
					channel->identifier( nodeLabel + ".Channel" );

					Moo::ChannelBinder channelBind( channel, child );
					result->addChannelBinder( channelBind );
				}
			}

			previousLevelNodes.swap( thisLevelNodes );
		}

#if !defined( _RELEASE )
		const uint32 numDescendants = root->countDescendants();
		uint32 expectedNumDescendants = 0;
		for(int level = 1; level < numLevels; ++level)
		{
			expectedNumDescendants += static_cast<uint32>(
				std::pow(
					static_cast<double>(nodeChildren), level ));
		}

		MF_ASSERT( numDescendants == expectedNumDescendants );
#endif

		return result;
	}


	/**
	 *	This functor tests that two given animations are equivalent.
	 */
	class CheckAnimsEquivalent
	{
	public:
		/**
		 *	This will compare @a a with @a b.
		 *	
		 *	If @a ignoreNULLNodes is true, then a comparison of channel
		 *	binders where either has a NULL node will only check the channel
		 *	data (i.e., it will ignore the node data). This can be useful when
		 *	checking an animation that has been restored from a file, since
		 *	only the channel data is written to the file.
		 */
		CheckAnimsEquivalent(
			Moo::ConstAnimationPtr & a, Moo::ConstAnimationPtr & b,
			bool ignoreNULLNodes )
			:
			a_( a )
			, b_( b )
			, ignoreNULLNodes_( ignoreNULLNodes )
		{
			BW_GUARD;
		}

		TEST_SUB_FUNCTOR_OPERATOR
		{
			BW_GUARD;

			RETURN_ON_FAIL_CHECK( a_.hasObject() );
			RETURN_ON_FAIL_CHECK( b_.hasObject() );

			CHECK_EQUAL( a_->identifier(), b_->identifier() );
			CHECK_EQUAL( a_->internalIdentifier(), b_->internalIdentifier() );

			RETURN_ON_FAIL_CHECK(
				a_->nChannelBinders() == b_->nChannelBinders() );
			
			for(uint32 channelBinderIndex = 0;
				channelBinderIndex < a_->nChannelBinders();
				++channelBinderIndex)
			{
				CHECK( channelBindersEquivalent(
					a_->channelBinder(channelBinderIndex),
					b_->channelBinder(channelBinderIndex),
					ignoreNULLNodes_ ) );
			}
		}

	private:
		Moo::ConstAnimationPtr a_;
		Moo::ConstAnimationPtr b_;
		bool ignoreNULLNodes_;
	};
}


TEST_F( AnimTestsFixture, Animation_SimpleStructure )
{
	BW_GUARD;

	//Create test data.
	Moo::NodePtr nodeA = createRandomNode( 0, "nodeA" );
	InterpolatedAnimationChannelPtr channelA =
		createRandomInterpolatedAnimationChannel( 0, 5, 5.0f,
		true//true since not using compression.
		);
	Moo::ChannelBinder channelBindA( channelA, nodeA );

	Moo::NodePtr nodeB = createRandomNode( 1, "nodeB" );
	InterpolatedAnimationChannelPtr channelB =
		createRandomInterpolatedAnimationChannel( 1, 3, 0.5f,
		true//true since not using compression.
		);
	Moo::ChannelBinder channelBindB( channelB, nodeB );

	const char * const TEST_RESOURCE_ID =
		"Animation_SimpleStructure.animation";
	RETURN_ON_FAIL_CHECK( ResMgrTest::eraseResource( TEST_RESOURCE_ID ) );
	Moo::AnimationPtr anim = Moo::AnimationManager::instance().
		getEmptyTestAnimation( TEST_RESOURCE_ID );

	//Empty
	CHECK_EQUAL( 0, anim->nChannelBinders() );
	CHECK( animationChannelsEquivalent(
		Moo::AnimationChannelPtr(), anim->findChannel( nodeA ) ) );
	CHECK( animationChannelsEquivalent(
		Moo::AnimationChannelPtr(), anim->findChannel( nodeB ) ) );

	//Contains A.
	anim->addChannelBinder( channelBindA );
	CHECK_EQUAL( 1, anim->nChannelBinders() );
	CHECK( channelBindersEquivalent( channelBindA, anim->channelBinder(0) ) );
	{
		const Moo::AnimationChannelPtr & a = channelA;
		const Moo::AnimationChannelPtr & b = anim->findChannel( nodeA );
		CHECK( animationChannelsEquivalent( a, b ) );
	}
	CHECK( animationChannelsEquivalent(
		Moo::AnimationChannelPtr(), anim->findChannel( nodeB ) ) );

	//Contains A and B.
	anim->addChannelBinder( channelBindB );
	CHECK_EQUAL( 2, anim->nChannelBinders() );
	CHECK( channelBindersEquivalent( channelBindA, anim->channelBinder(0) ) );
	CHECK( channelBindersEquivalent( channelBindB, anim->channelBinder(1) ) );
	{
		const Moo::AnimationChannelPtr & a = channelA;
		const Moo::AnimationChannelPtr & b = anim->findChannel( nodeA );
		CHECK( animationChannelsEquivalent( a, b ) );
	}
	CHECK( channelBindersEquivalent( channelBindB, anim->channelBinder(1) ) );
	{
		const Moo::AnimationChannelPtr & a = channelB;
		const Moo::AnimationChannelPtr & b = anim->findChannel( nodeB );
		CHECK( animationChannelsEquivalent( a, b ) );
	}

	const char * const TEST_ID = "testIdentifier";
	const char * const TEST_INTERNAL_ID = "testInternalIdentifier";
	anim->identifier( TEST_ID );
	anim->internalIdentifier( TEST_INTERNAL_ID );
	CHECK_EQUAL( TEST_ID, anim->identifier() );
	CHECK_EQUAL( TEST_INTERNAL_ID, anim->internalIdentifier() );
};


TEST_F( AnimTestsFixture, Animation_HierarchyIO )
{
	BW_GUARD;

	for(int numLevels = 1; numLevels <= 5; ++numLevels)
	{
		for(int numChildren = 1; numChildren <= 3; ++numChildren)
		{
			BW::ostringstream resourceIdStr;
			resourceIdStr << "Animation_HierarchyIO_" << numLevels <<
				"Levels_" << numChildren << "Children";
			const BW::string resourceId =
				BW::string( resourceIdStr.str().c_str() ) +
					".animation";
			const BW::string resourceRestoredId =
				BW::string( resourceIdStr.str().c_str() ) +
					"Restored.animation";

			//Ensure no pre-existing files to interfere.
			RETURN_ON_FAIL_CHECK(
				ResMgrTest::eraseResource( resourceId ) );
			RETURN_ON_FAIL_CHECK(
				ResMgrTest::eraseResource( resourceRestoredId ) );

			Moo::AnimationPtr animPtr = generateAnimationHierarchy(
				resourceId, numLevels, numChildren );

			RETURN_ON_FAIL_CHECK( animPtr->save( resourceId ) );

			Moo::AnimationPtr animRestoredPtr = Moo::AnimationManager::
				instance().getEmptyTestAnimation( resourceRestoredId );
			const bool loadSuccess = animRestoredPtr->load( resourceId );
			if (!loadSuccess)
			{
				CHECK( loadSuccess );
				continue;
			}

			CheckAnimsEquivalent checkAnimsFunc(
				animPtr, animRestoredPtr, true );
			TEST_SUB_FUNCTOR( checkAnimsFunc );

			RETURN_ON_FAIL_CHECK( ResMgrTest::eraseResource( resourceId ) );
		}
	}
}


TEST_F( AnimTestsFixture, Animation_BasicFileIO )
{
	BW_GUARD;

	//Very basic sanity check for animation loading from a typical file.

	//Want to test I/O from .animation files, not to the streaming cache.
	MF_ASSERT( Moo::StreamedAnimation::s_savingAnimCache_ == NULL );

	const char * const TEST_RESOURCE_ID = "TestArmsCrossed.animation";

	Moo::AnimationPtr loadedAnimPtr =
		Moo::AnimationManager::instance().get( TEST_RESOURCE_ID );

	RETURN_ON_FAIL_CHECK( loadedAnimPtr.hasObject() );
	CHECK_EQUAL( "armscrossed", loadedAnimPtr->identifier() );
	CHECK_EQUAL( "armscrossed", loadedAnimPtr->internalIdentifier() );
	CHECK_EQUAL( 31, loadedAnimPtr->nChannelBinders() );
	CHECK_EQUAL( 126.0f, loadedAnimPtr->totalTime() );
}
