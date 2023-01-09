#ifdef _MSC_VER 
#pragma once
#endif

#ifndef SWITCHED_MODEL_ANIMATION_HPP
#define SWITCHED_MODEL_ANIMATION_HPP


#include "model_animation.hpp"


BW_BEGIN_NAMESPACE

template< class BULK >
class SwitchedModel;


/**
 *	SwitchedModel's animation
 */
template < class BULK >
class SwitchedModelAnimation : public ModelAnimation
{
public:
	SwitchedModelAnimation( SwitchedModel<BULK> & owner, const BW::string & name, float frameRate ) :
		owner_( owner ), identifier_( name ), frameRate_( frameRate )
	{
		BW_GUARD;	
	}

	virtual ~SwitchedModelAnimation()
	{
		BW_GUARD;	
	}

	virtual void play( float time, float blendRatio, int /*flags*/ )
	{
		BW_GUARD;
		// Animations should not be added to the list if
		//  they have no frames_!
		if (owner_.blend( Model::blendCookie() ) < blendRatio)
		{
			owner_.setFrame( frames_[ uint32(time*frameRate_) % frames_.size() ] );
			owner_.blend( Model::blendCookie(), blendRatio );
		}
	}

	void addFrame( BULK frame )
	{
		BW_GUARD;
		frames_.push_back( frame );
		duration_ = float(frames_.size()) / frameRate_;
		looped_ = true;
	}

	int numFrames()
	{
		MF_ASSERT( frames_.size() <= INT_MAX );
		return ( int ) frames_.size();
	}

	BW::vector<BULK> & frames() { return frames_; }

private:
	SwitchedModel<BULK> & owner_;

	BW::string			identifier_;
	float				frameRate_;
	BW::vector<BULK>	frames_;
};

BW_END_NAMESPACE

#endif // SWITCHED_MODEL_ANIMATION_HPP
