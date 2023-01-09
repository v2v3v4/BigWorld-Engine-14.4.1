#include "pch.hpp"

#include "pulse_light.hpp"
#include "moo_math.hpp"
#include "node.hpp"
#include "animation_manager.hpp"


BW_BEGIN_NAMESPACE

namespace Moo {

PulseLight::PulseLight() { }
/**
 *	Constructor
 */
PulseLight::PulseLight( const Colour & colour, const Vector3 & position,
	float innerRadius, float outerRadius, const char * animationName,
	const ExternalArray<Vector2> & animFrames, float loopDuration )
	: OmniLight(colour, position, innerRadius, outerRadius)
	, basePosition_(position)
	, baseColour_(colour)
	, positionAnimFrame_(0)
	, colourAnimFrame_(0)
{
	Moo::NodePtr pNode = new Moo::Node;
	pAnimation_ = Moo::AnimationManager::instance().get(animationName, pNode);
	colourAnimation_.reset();
	ExternalArray<Vector2>::const_iterator iter = animFrames.begin(),
	                                       end = animFrames.end();
	for (; iter != end; ++iter)
	{
		const Vector2 & frame = *iter;
		colourAnimation_.addKey( frame.x, frame.y );
	}
	if (loopDuration >= 0.f)
	{
		const float animDuration = (loopDuration > 0.f) ?
			loopDuration : colourAnimation_.getTotalTime();
		colourAnimation_.loop( true, animDuration );
	}
}

/**
 *	Destructor
 */
PulseLight::~PulseLight()
{
}

void PulseLight::tick( float dTime )
{
	if (pAnimation_)
	{
		positionAnimFrame_ += dTime * 30.f;
		if (positionAnimFrame_ >= pAnimation_->totalTime())
		{
			positionAnimFrame_ = fmodf(positionAnimFrame_, pAnimation_->totalTime());
		}
		Matrix m;
		Matrix res = Matrix::identity;

		for (uint32 i = 0; i < pAnimation_->nChannelBinders(); i++)
		{
			const Moo::AnimationChannelPtr pChannel = pAnimation_->channelBinder(i).channel();
			if (pChannel)
			{
				pChannel->result( positionAnimFrame_, m );
				res.preMultiply( m );
			}
		}
		const Vector3 animPosition = res.applyToOrigin();
		worldPosition_ = position_ = (basePosition_ + animPosition);
	}

	colourAnimFrame_ += dTime;
	colourAnimFrame_ = fmodf(colourAnimFrame_, colourAnimation_.getTotalTime());
	const float colourMod = colourAnimation_.animate( colourAnimFrame_ );
	this->colour( Moo::Colour( baseColour_.r * colourMod,
	                           baseColour_.g * colourMod, 
	                           baseColour_.b * colourMod,
	                           1.f ) );
}	

} // namespace Moo

BW_END_NAMESPACE

// omni_light.cpp
