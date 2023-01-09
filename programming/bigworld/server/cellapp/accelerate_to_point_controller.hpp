#ifndef ACCELERATE_TO_POINT_CONTROLLER_HPP
#define ACCELERATE_TO_POINT_CONTROLLER_HPP


#include "base_acceleration_controller.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This movement controller accelerates the entity toward a point in space.
 *	It can be configured to rotate the entity to match its current movement
 *	and to come gently to a halt at its final destination.
 */
class AccelerateToPointController : public BaseAccelerationController
{
	DECLARE_CONTROLLER_TYPE( AccelerateToPointController )

public:
	AccelerateToPointController( Vector3 destination = Vector3(0,0,0),
								float acceleration = 0.0f,
								float maxSpeed = 0.0f,
								Facing facing = FACING_NONE,
								bool stopAtDestination = true );

	virtual void		writeRealToStream( BinaryOStream & stream );
	virtual bool		readRealFromStream( BinaryIStream & stream );
	void				update();

private:
	Vector3				destination_;
	bool				stopAtDestination_;
};

BW_END_NAMESPACE

#endif // ACCELERATE_TO_POINT_CONTROLLER_HPP
