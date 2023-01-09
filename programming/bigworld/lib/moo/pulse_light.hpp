#ifndef _PULSE_LIGHT_HPP
#define _PULSE_LIGHT_HPP

#include "moo_math.hpp"
#include "omni_light.hpp"
#include "animation.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/vectornodest.hpp"
#include "cstdmf/static_array.hpp"
#include "math/linear_animation.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;
namespace Moo {

class PulseLight;
typedef SmartPointer< PulseLight > PulseLightPtr;

/**
 * This class represents an animated omni-directional light with a colour,
 * position and an attenuation range.
 */
class PulseLight : public OmniLight
{
public:
	PulseLight();
	PulseLight( const Colour & colour, const Vector3 & position,
		float innerRadius, float outerRadius, const char * animationName,
		const ExternalArray<Vector2> & animFrames, float loopDuration );
	~PulseLight();

	void tick( float dTime );

private:
	Vector3 basePosition_;
	Colour baseColour_;
	SmartPointer<Animation> pAnimation_;
	LinearAnimation<float> colourAnimation_;
	float positionAnimFrame_,
	      colourAnimFrame_;
};

} // namespace Moo

//typedef BW::vector< Moo::PulseLightPtr > PulseLightVector;
typedef VectorNoDestructor< Moo::PulseLightPtr > PulseLightVector;

BW_END_NAMESPACE

#endif // _PULSE_LIGHT_HPP

