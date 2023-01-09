#ifndef ACCELERATE_ALONG_PATH_CONTROLLER_HPP
#define ACCELERATE_ALONG_PATH_CONTROLLER_HPP

#include "base_acceleration_controller.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class forms a controller that moves an entity along a list of
 *	waypoints. The controller accelerates the entity though
 *	each waypoint, finally coming to a halt at the last one.
 */
class AccelerateAlongPathController : public BaseAccelerationController
{
	DECLARE_CONTROLLER_TYPE( AccelerateAlongPathController )

public:
	AccelerateAlongPathController();
	AccelerateAlongPathController(	BW::vector< Position3D > & waypoints,
									float acceleration,
									float maxSpeed,
									Facing facing );

	virtual void		writeRealToStream( BinaryOStream & stream );
	virtual bool		readRealFromStream( BinaryIStream & stream );
	void				update();

private:
	BW::vector< Position3D >	waypoints_;
	uint						progress_;
};

BW_END_NAMESPACE

#endif // ACCELERATE_ALONG_PATH_CONTROLLER_HPP
