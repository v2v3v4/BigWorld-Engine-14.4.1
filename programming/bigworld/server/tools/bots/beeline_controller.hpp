#ifndef BEELINE_CONTROLLER_HPP
#define BEELINE_CONTROLLER_HPP

#include "movement_controller.hpp"


BW_BEGIN_NAMESPACE

namespace Beeline
{
	// This could probably derive from GraphTraverser itself, but I want to do
	// it as a standalone class as a learning experience
	class BeelineController : public MovementController
	{
	public:
		BeelineController( const Vector3 &destinationPos );
		virtual bool nextStep (float &speed,
							   float dTime,
							   Vector3 &pos,
							   Direction3D &dir);

	protected:
		Vector3 destinationPos_;
	};
};

BW_END_NAMESPACE

#endif // BEELINE_CONTROLLER_HPP
