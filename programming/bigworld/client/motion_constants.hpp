#ifndef MOTION_CONSTANTS_HPP
#define MOTION_CONSTANTS_HPP

#include "math/angle.hpp"


BW_BEGIN_NAMESPACE

class Physics;

/**
 *	This helper struct contains constant parameters related to motion.
 */
struct MotionConstants
{
	MotionConstants( const Physics & physics );

	Angle		modelYaw;	// not really a constant
	Angle		modelPitch;	// nor this...
	float		modelWidth;
	float		modelHeight;
	float		modelDepth;
	float		scrambleHeight;
};

BW_END_NAMESPACE

#endif // MOTION_CONSTANTS_HPP
