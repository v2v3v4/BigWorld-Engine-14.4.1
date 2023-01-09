#include "pch.hpp"

#include "nodefull_model_default_animation.hpp"

#include "moo/node.hpp"

#include "model.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor for the default animation (i.e. initial pose)
 *	of a nodefull model.
 */
NodefullModelDefaultAnimation::NodefullModelDefaultAnimation(
		NodeTree & rTree,
		int itinerantNodeIndex,
		BW::vector< Matrix > & transforms ) :
	rTree_( rTree ),
	itinerantNodeIndex_( itinerantNodeIndex ),
	cTransforms_( transforms ),
	bTransforms_( transforms.size() )
{
	BW_GUARD;
	for (uint i = 0; i < transforms.size(); i++)
	{
		bTransforms_[i].init( transforms[i] );
	}
}

/**
 *	Destructor for the default animation of a nodefull model.
 */
NodefullModelDefaultAnimation::~NodefullModelDefaultAnimation()
{
	BW_GUARD;	
}


bool NodefullModelDefaultAnimation::valid() const
{
	BW_GUARD;
	if (!this->ModelAnimation::valid())
		return false;

	if (bTransforms_.empty())
		return false;

	// Only check and report blend transform errors in debug mode. This
	// will be upgraded to hybrid in a future release. Currently, it is 
	// undesirable due the amount of assets this might affect ( before 
	// the exporter rules are changed ).

#if _DEBUG
	for ( size_t i = 0; i < bTransforms_.size(); i++ )
	{
		BW::string reason;
		if ( !bTransforms_[i].valid( &reason ) )
		{
			WARNING_MSG("NodefullModelDefaultAnimation::valid() :"
				" transform %d is invalid, reason: %s \n", i, reason.c_str() );
			// Enable return false once exporters are verified
			// return false;
		}
	}
#endif

	return true;
}


/**
 *	This method applies the default animation (i.e. initial pose) with the
 *	given parameters to the global node table.
*/
void NodefullModelDefaultAnimation::play( float time, float blendRatio,
						int flags )
{
	BW_GUARD;
	/* Previously the doxygen comment above included:
	 *		"If flags are -1, then the animation is only applied to those nodes
	 *		 that have not yet been touched with the current value of
	 *		s_blendCookie_.",
	 *	but this behaivour has been
	 *	moved into the model's dress call because always adding the default
	 *	animation is a pain, and I can't think of any reason why you wouldn't
	 *	want to use it if nothing else set the node. Plus originally I only
	 *	intended this to be used with subordinate model parts, whereas it should
	 *	rightly be used with all, since not every animation moves every node.
	 */
	for (uint i = 0; i < bTransforms_.size(); i++)
	{
		Moo::Node * pNode = rTree_[i].pNode;

		float oldBr = pNode->blend( Model::blendCookie() );
		float sumBr = oldBr + blendRatio;

		if (oldBr == 0.f)
		{
			pNode->blendTransform() = bTransforms_[i];
		}
		else
		{
			pNode->blendTransform().blend(
				blendRatio / sumBr, bTransforms_[i] );
		}

		pNode->blend( Model::blendCookie(), sumBr );
	}

	/*
	}
	else // if flags == -1
	{
		for (int i = 0; i < bTransforms_.size(); i++)
		{
			Moo::Node * pNode = rTree_[i].pNode;
			if (pNode->blend( s_blendCookie_ ) == 0.f)
			{
				pNode->blendTransform() = transforms_[i];
				pNode->blend( s_blendCookie_, blendRatio );
			}
			// hmmm.. should we add the appropriate blendratio to it anyway?
			//  or conversely, should we always pretend a blendratio of 0...?
		}
	}
	*/
}


/**
 *	Get the factor matrix for the given flags. If flags are -1,
 *	get the itinerant root node's initial transform
 */
void NodefullModelDefaultAnimation::flagFactor( int flags, Matrix & mOut ) const
{
	BW_GUARD;
	if (flags == -1 && uint(itinerantNodeIndex_) < cTransforms_.size())
	{
		mOut = cTransforms_[itinerantNodeIndex_];
	}
	else
	{
		mOut.setIdentity();
	}
}

BW_END_NAMESPACE

// nodefull_model_default_animation.cpp
