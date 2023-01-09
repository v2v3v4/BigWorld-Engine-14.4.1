#ifdef _MSC_VER 
#pragma once
#endif

#ifndef NODEFULL_MODEL_ANIMATION_HPP
#define NODEFULL_MODEL_ANIMATION_HPP


#include "moo/animation.hpp"

#include "model_animation.hpp"
//#include "nodefull_model.hpp"


BW_BEGIN_NAMESPACE

class NodefullModel;


/**
 *	NodefullModel's animation
 */
class NodefullModelAnimation : public ModelAnimation
{
public:
	NodefullModelAnimation( NodefullModel & rOwner, Moo::AnimationPtr anim,
		DataSectionPtr pAnimSect );
	virtual ~NodefullModelAnimation();

	virtual bool valid() const;

	virtual void tick( float dtime, float otime, float ntime );
	virtual void play( float time, float blendRatio, int flags );

	virtual void flagFactor( int flags, Matrix & mOut ) const;
	virtual const Matrix & flagFactorBit( int bit ) const;

	virtual uint32 sizeInBytes() const;

	virtual Moo::AnimationPtr getMooAnim() { return anim_; }

private:

	void precalcItinerant( int flags );

	NodefullModel			& rOwner_;	// needed for fn above
	Moo::AnimationPtr		anim_;		//  (but not terribly bad)

	int			firstFrame_;	///< Starts at beginning of this frame
	int			lastFrame_;		///< Stops  at beginning of this frame
	float		frameRate_;		///< This many frames per second
	int			frameCount_;	///< This many frames in total

	Moo::NodeAlphas				* alphas_;

	Moo::AnimationChannelPtr	alternateItinerants_[8];
	Matrix						flagFactors_[3];
	Moo::AnimationChannelPtr	flagFactor2_;

	BW::string					cognate_;	// for coordination


	NodefullModelAnimation( const NodefullModelAnimation & rOther );
	NodefullModelAnimation & operator=( const NodefullModelAnimation & rOther );
};

BW_END_NAMESPACE


#endif // NODEFULL_MODEL_ANIMATION_HPP
