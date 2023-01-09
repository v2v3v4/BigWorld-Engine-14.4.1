
#include "pch.hpp"

#include "cstdmf/guard.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bwrandom.hpp"
#include "math/math_extra.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "animation_test_helper.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Creates a random channel based on @a seed, with @a numKeys key frames
 *	(of random types) and a maximum duration of @a numSeconds.
 *	@a readCompressionFactors is passed as the corresponding
 *	InterpolatedAnimationChannel constructor parameter.
 *	
 *	@pre @a numKeys must be at least 3 because a valid channel requires at
 *		least one of each type of key frame.
 *	@post isValid() is true for the return value.
 *	@post The return value has @a numKeys key frames in total (across the
 *		three types).
 *	@return The random channel.
*/
InterpolatedAnimationChannelPtr createRandomInterpolatedAnimationChannel(
	int seed, int numKeys, float numSeconds, bool readCompressionFactors )
{
	BW_GUARD;

	MF_ASSERT( numKeys >= 3 );
	MF_ASSERT( numSeconds >= 0.0f );
	
	InterpolatedAnimationChannelPtr result(
		new Moo::InterpolatedAnimationChannel(
			false, readCompressionFactors ) );

	//The channel needs at least one of each of the three types of key
	//frames to be valid. Adding this transform directly should add
	//one of each type of key frame.
	BWRandom random( seed );
	result->addKey( 0.0f, createRandomTransform( random, -1.0f, 1.0f ) );
	MF_ASSERT( result->nPositionKeys() == 1 );
	MF_ASSERT( result->nRotationKeys() == 1 );
	MF_ASSERT( result->nScaleKeys() == 1 );
	MF_ASSERT( result->valid() );

	//The rest of the key frames are of random types.
	for (int key = 3; key < numKeys; ++key)
	{
		const Vector3 vec3 = createRandomVector3( random, -1.0f, 1.0f );
		const float time = random( 0.0f, numSeconds );

		switch (key % 3)
		{
		case 0:
			result->addScaleKey( time, vec3 );
			break;

		case 1:
			result->addPositionKey( time, vec3 );
			break;

		case 2:
			result->addRotationKey(
				time, Quaternion( vec3, random( 0.0f, 1.0f ) ) );
			break;
		}
	}

	MF_ASSERT( numKeys == (result->nScaleKeys() +
		result->nPositionKeys() + result->nRotationKeys()) );
	MF_ASSERT( result->valid() );

	return result;
}


/**
 *	@return True if @a lhs is data-equivalent to @a rhs. This means that
 *		internal reference counts and pointer values can differ, but the
 *		animation data must be the same.
 */
bool animationChannelsEquivalent(
	const InterpolatedAnimationChannelPtr & lhs,
	const InterpolatedAnimationChannelPtr & rhs )
{
	BW_GUARD;

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

	return lhs->compareChannel( *rhs );
}


/**
 *	@return A node with random transforms, etc, generated from @a seed.
 *		The node will have no children or parent.
 */
Moo::NodePtr createRandomNode( int seed, const BW::string & identifier )
{
	BW_GUARD;

	Moo::NodePtr result( new Moo::Node );

	BWRandom random( seed );
	result->transform( createRandomTransform( random, -1.0f, 1.0f ) );
	std::srand( (seed + 1) * 10 );
	result->worldTransform( createRandomTransform( random, -1.0f, 1.0f ) );
	result->blend(
		random( 0, 10 ) * 16,
		random( 0.0f, 1.0f ) );
	result->identifier( identifier );

	return result;
}


/**
 *	This function checks whether @a lhs is data-equivalent to @a rhs (including
 *	child nodes but ignoring parents). I.e., internal reference counts and
 *	pointer values can differ, but the animation data must be the same.
 *	
 *	@return True if @a lhs is considered data-equivalent to @a rhs (ignoring
 *		parents).
 */
bool compareNodeHierarchies(
	const Moo::Node & lhs, const Moo::Node & rhs, bool expectRootNodes )
{
	BW_GUARD;

	if (expectRootNodes)
	{
		MF_ASSERT( !lhs.parent().hasObject() );
		MF_ASSERT( !rhs.parent().hasObject() );
	}
	
	if (!lhs.compareNodeData( rhs ))
	{
		return false;
	}

	if (lhs.nChildren() != rhs.nChildren())
	{
		return false;
	}

	//Use this to make sure the mapping is bijective in the case where
	//node names aren't distinct.
	BW::set<uint32> usedRHSIndices;

	//Check children are equivalent (recursively).
	for(uint32 lhsChildIndex = 0;
		lhsChildIndex < lhs.nChildren();
		++lhsChildIndex)
	{
		Moo::ConstNodePtr lhsChild = lhs.child(lhsChildIndex);

		MF_ASSERT( lhsChild.hasObject() );
		MF_ASSERT( lhsChild->parent() == & lhs );

		//Search for a matching rhs child.
		Moo::ConstNodePtr rhsChild;
		for(uint32 rhsChildIndex = 0;
			rhsChildIndex < rhs.nChildren();
			++rhsChildIndex)
		{
			if (usedRHSIndices.find( rhsChildIndex ) != usedRHSIndices.end())
			{
				//Already a mapping to this child.
				continue;
			}
			Moo::ConstNodePtr rhsTestChild = rhs.child( rhsChildIndex );

			MF_ASSERT( rhsTestChild.hasObject() );
			MF_ASSERT( rhsTestChild->parent() == & rhs );

			//Note: compareNodeHierarchies will check that the
			//identifiers match.
			if (compareNodeHierarchies( * lhsChild, * rhsTestChild, false ))
			{
				//Found a rhs node with the same identifier and
				//recursively equivalent children.
				rhsChild = rhsTestChild;
				usedRHSIndices.insert( rhsChildIndex );
				break;
			}
		}
		if ( !rhsChild.hasObject() )
		{
			return false;
		}
	}
	MF_ASSERT( usedRHSIndices.size() == lhs.nChildren() );

	return true;
}


///@see the other compareNodeHierarchies function.
bool compareNodeHierarchies(
	const Moo::NodePtr & lhs, const Moo::NodePtr & rhs, bool expectRootNodes )
{
	BW_GUARD;

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

	return compareNodeHierarchies( *lhs, *rhs, expectRootNodes );
}

BW_END_NAMESPACE
