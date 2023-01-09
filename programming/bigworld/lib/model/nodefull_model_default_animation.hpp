#ifdef _MSC_VER 
#pragma once
#endif

#ifndef NODEFULL_MODEL_DEFAULT_ANIMATION_HPP
#define NODEFULL_MODEL_DEFAULT_ANIMATION_HPP


#include "model_animation.hpp"

#include "math/blend_transform.hpp"

#include "node_tree.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Nodefull model's default animation
 */
class NodefullModelDefaultAnimation : public ModelAnimation
{
public:
	NodefullModelDefaultAnimation( NodeTree & rTree,
		int itinerantNodeIndex,
		BW::vector< Matrix > & transforms );
	virtual ~NodefullModelDefaultAnimation();

	virtual bool valid() const;

	virtual void play( float time, float blendRatio, int flags );

	virtual void flagFactor( int flags, Matrix & mOut ) const;

	const BW::vector< Matrix > & cTransforms() { return cTransforms_; }

private:
	NodeTree		& rTree_;
	int				itinerantNodeIndex_;

	BW::vector< BlendTransform >		bTransforms_;
	const BW::vector< Matrix >		cTransforms_;
};

BW_END_NAMESPACE

#endif // NODEFULL_MODEL_DEFAULT_ANIMATION_HPP
