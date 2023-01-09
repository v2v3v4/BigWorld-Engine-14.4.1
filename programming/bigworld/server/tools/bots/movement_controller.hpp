#ifndef MOVEMENT_CONTROLLER_HPP
#define MOVEMENT_CONTROLLER_HPP

#include "math/vector3.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is the interface for movement controllers.
 */
class MovementController
{
public:
	virtual ~MovementController() {}

	virtual bool nextStep( float & speed, float dTime,
		Vector3 & pos, Direction3D & dir ) = 0;
};


/**
 *	This class is the base class for all movement factories. They are
 *	responsible for creating the movement controllers.
 */
class MovementFactory
{
public:
	MovementFactory( const char * name );
	virtual MovementController * create( const BW::string & data,
		float & speed, Vector3 & startPos ) = 0;
};

BW_END_NAMESPACE

#endif // MOVEMENT_CONTROLLER_HPP
