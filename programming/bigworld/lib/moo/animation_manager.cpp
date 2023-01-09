#include "pch.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/concurrency.hpp"

#include "moo_math.hpp"
#include "math/blend_transform.hpp"
#include "animation.hpp"
#include "interpolated_animation_channel.hpp"
#include "discrete_animation_channel.hpp"

#include "animation_manager.hpp"

#ifndef CODE_INLINE
#include "animation_manager.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( Moo::AnimationManager )


namespace Moo
{

AnimationManager::AnimationManager() :
	fullHouse_( false )
{
}


/**
 *	This method retrieves the given animation resource, tied to the hierarchy
 *	starting from the input rootNode.
 */
AnimationPtr AnimationManager::get( const BW::string& resourceID, NodePtr rootNode )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( rootNode )
	{
		return NULL;
	}

/*	MEMORYSTATUS preStat;
	GlobalMemoryStatus( &preStat );*/

	AnimationPtr res;

	AnimationPtr found = this->find( resourceID );
	if (found)
	{
		res = new Animation( &*found, rootNode );
	}

/*	static SIZE_T animationMem = 0;
	MEMORYSTATUS postStat;

	GlobalMemoryStatus( &postStat );
	animationMem += preStat.dwAvailPhys - postStat.dwAvailPhys;
	TRACE_MSG( "AnimationManager: Loaded animation '%s' animations used memory: %d\n",
		resourceID.c_str(), animationMem / 1024 );*/

	return res;
}


/**
 *	This method retrieves the given animation resource, tied to the nodes
 *	recorded in the input catalogue.
 */
AnimationPtr AnimationManager::get( const BW::string& resourceID )
{
	BW_GUARD;	
/*	MEMORYSTATUS preStat;
	GlobalMemoryStatus( &preStat );*/

	AnimationPtr res;

	AnimationPtr found = this->find( resourceID );
	if (found.getObject() != NULL)
	{
		res = new Animation( &*found );
	}

/*	static SIZE_T animationMem = 0;
	MEMORYSTATUS postStat;

	GlobalMemoryStatus( &postStat );
	animationMem += preStat.dwAvailPhys - postStat.dwAvailPhys;
	TRACE_MSG( "AnimationManager: Loaded animation '%s' animations used memory: %d\n",
		resourceID.c_str(), animationMem / 1024 );*/

	return res;
}


/**
 *	This method is used for testing purposes to get a blank animation object
 *	for configuration.
 *	@pre No animation resource using the specified name already exists in
 *		the manager.
 *	@return Pointer to a default-constructed Animation object that has had
 *		the id @a resourceID set. It will be managed by this object.
 */
AnimationPtr AnimationManager::getEmptyTestAnimation(
	const BW::string& resourceID )
{
	BW_GUARD;

	AnimationPtr result;

	MF_ASSERT( this->find( resourceID ).get() == NULL );

	result = new Animation;
	result->identifier( resourceID );
	result->internalIdentifier( resourceID );
	
	animationsLock_.grab();
	animations_.insert( std::make_pair( resourceID, result.get() ) );
	animationsLock_.give();

	MF_ASSERT( this->find( resourceID ).get() == result.get() );

	return result;
}


/**
 *	This method removes the given animation from our map, if it is present
 */
void AnimationManager::del( Animation * pAnimation )
{
	BW_GUARD;
	SimpleMutexHolder smh( animationsLock_ );

	for (AnimationMap::iterator it = animations_.begin();
		it != animations_.end();
		it++)
	{
		if (it->second == pAnimation)
		{
			animations_.erase( it );
			break;
		}
	}
}


/**
 *	Set whether or not we have a full house
 *	(and thus cannot load any new animations)
 */
void AnimationManager::fullHouse( bool noMoreEntries )
{
	fullHouse_ = noMoreEntries;
}



/**
 *	This private method returns the animation manager's private copy of the
 *	the input animation resource. The AnimationManager keeps a map of
 *	animations that it has loaded, and if the input name exists in the vector,
 *	it returns that one; otherwise it loads a new one and returns it
 *	(after storing it in the vector for next time).
 */
AnimationPtr AnimationManager::find( const BW::string & resourceID )
{
	BW_GUARD;
	AnimationPtr res;

	{
		SimpleMutexHolder smh( animationsLock_ );
		AnimationMap::iterator it = animations_.find( resourceID );
		if (it != animations_.end())
		{
			MF_ASSERT( it->second->refCount() );
			res = it->second;
		}
	}

	if (!res)
	{
		if (fullHouse_)
		{
			CRITICAL_MSG( "AnimationManager::getAnimationResource: "
				"Loading the animation '%s' into a full house!\n",
				resourceID.c_str() );
		}

		res = new Animation;

		if (!res->load( resourceID ))
		{
			res = NULL;	// will delete by refcount
		}
		else
		{
			SimpleMutexHolder smh( animationsLock_ );
			animations_.insert( std::make_pair( resourceID, &*res) );
		}
	}

	return res;
}

BW::string AnimationManager::resourceID( Animation * pAnim )
{
	BW_GUARD;
	while (pAnim->pMother_.getObject() != NULL)
	{
		pAnim = pAnim->pMother_.getObject();
	}

	SimpleMutexHolder smh( animationsLock_ );

	AnimationMap::iterator it = animations_.begin();
	AnimationMap::iterator end = animations_.end();

	while (it != end)
	{
		if (it->second == pAnim)
			return it->first;
		it++;
	}

	return BW::string();
}


} // namespace Moo

BW_END_NAMESPACE

// animation_manager.cpp
