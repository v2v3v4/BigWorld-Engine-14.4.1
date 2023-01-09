///This file contains helper functions shared among Moo animation tests.

#pragma once
#ifndef ANIMATION_TEST_HELPER_HPP
#define ANIMATION_TEST_HELPER_HPP

#include "cstdmf/guard.hpp"
#include "cstdmf/bw_noncopyable.hpp"
#include "moo/animation.hpp"
#include "moo/animation_manager.hpp"
#include "moo/node.hpp"
#include "moo/node_catalogue.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "moo/moo_math.hpp"

BW_BEGIN_NAMESPACE


/**
 *	Manages the lifetime of the singletons to that need to be created for
 *	each animation test.
 */
class AnimTestsFixture
	:
	private BW::NonCopyable
{
public:
	AnimTestsFixture()
	{
		BW_GUARD;

		new Moo::NodeCatalogue();
		new Moo::AnimationManager();
	}

	~AnimTestsFixture()
	{
		BW_GUARD;

		delete Moo::AnimationManager::pInstance();
		delete Moo::NodeCatalogue::pInstance();
	}
};


typedef SmartPointer<Moo::InterpolatedAnimationChannel>
	InterpolatedAnimationChannelPtr;


InterpolatedAnimationChannelPtr createRandomInterpolatedAnimationChannel(
	int seed, int numKeys, float numSeconds, bool readCompressionFactors );


bool animationChannelsEquivalent(
	const InterpolatedAnimationChannelPtr & lhs,
	const InterpolatedAnimationChannelPtr & rhs );


Moo::NodePtr createRandomNode( int seed, const BW::string & identifier );


bool compareNodeHierarchies(
	const Moo::Node & lhs, const Moo::Node & rhs, bool expectRootNodes );


bool compareNodeHierarchies(
	const Moo::NodePtr & lhs, const Moo::NodePtr & rhs, bool expectRootNodes );

BW_END_NAMESPACE

#endif //ANIMATION_TEST_HELPER_HPP
