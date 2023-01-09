#ifndef PROPELLOR_HPP
#define PROPELLOR_HPP

#include "motor.hpp"
#include "pyscript/script.hpp"
#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class moves an object with the effect of a propellor
 *	or other point source of force.
 */
class Propellor : public Motor
{
	Py_Header( Propellor, Motor )

public:
	Propellor( PyTypeObject * pType = &Propellor::s_type_ );
	~Propellor();

	virtual void rev( float dTime );

	float xDrag() const		{ return xDrag_; }
	float yDrag() const		{ return yDrag_; }
	float zDrag() const		{ return zDrag_; }

	float thrust() const	{ return throttle_ * maxThrust_; }

	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( throttle_, throttle )
	PY_RW_ATTRIBUTE_DECLARE( wheelPosition_, wheelPosition )

	PY_RW_ATTRIBUTE_REF_DECLARE( momentum_, momentum )

	PY_RW_ATTRIBUTE_DECLARE( xDrag_, xDrag )
	PY_RW_ATTRIBUTE_DECLARE( yDrag_, yDrag )
	PY_RW_ATTRIBUTE_DECLARE( zDrag_, zDrag )

	PY_RW_ATTRIBUTE_DECLARE( turnRate_, turnRate )
	PY_RW_ATTRIBUTE_DECLARE( maxThrust_, maxThrust )

	PY_RW_ATTRIBUTE_DECLARE( timeScale_, timeScale )
	PY_RW_ATTRIBUTE_DECLARE( actualTurn_, actualTurn )
	PY_RW_ATTRIBUTE_DECLARE( turnHalflife_, turnHalflife )

private:
	void runPhysics( float dTime );

	float throttle_;
	float wheelPosition_;

	Vector3 momentum_;

	float xDrag_;
	float yDrag_;
	float zDrag_;

	float turnRate_;

	float maxThrust_;

	float timeScale_;

	float actualTurn_;		///< Used to dampen the wheel's response.
	float turnHalflife_;	///< How quickly the desired turn is approached
};

BW_END_NAMESPACE

#endif // PROPELLOR_HPP
