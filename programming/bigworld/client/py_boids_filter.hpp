#ifndef PY_BOIDS_FILTER_HPP
#define PY_BOIDS_FILTER_HPP

#include "boids_filter.hpp"
#include "py_avatar_filter.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.BoidsFilter
 *
 *	This subclass of AvatarFilter implements a filter which implements flocking
 *	behaviour for the models that make up an entity.  This would normally
 *	be used for an entity such as a flock of birds or a school of fish which
 *	has many models driven by one entity.
 *
 *	The interpolation of the Entity position is done using the same logic
 *	as its parent AvatarFilter.  However, it also updates the positions
 *	of individual models (which are known, according to flocking conventions,
 *	as boids) that are attached to the entity, using standard
 *	flocking rules.
 *
 *	When the flock lands, the boidsLanded method is called on the entity.
 *
 *	A new BoidsFilter is created using the BigWorld.BoidsFilter function.
 */
/**
 *	This is a Python object to manage a BoidsFilter instance, matching the
 *	lifecycle of a Python object to the (shorter) lifecycle of a MovementFilter.
 */
class PyBoidsFilter : public PyAvatarFilter
{
	Py_Header( PyBoidsFilter, PyAvatarFilter );

public:
	PyBoidsFilter( PyTypeObject * pType = &s_type_ );
	virtual ~PyBoidsFilter() {}

	float influenceRadius() const;
	void influenceRadius( float newValue );

	float collisionFraction() const;
	void collisionFraction( float newValue );

	float normalSpeed() const;
	void normalSpeed( float newValue );

	float timeMultiplier() const;
	void timeMultiplier( float newValue );

	uint state() const;
	void state( uint newValue );

	float goalApproachRadius() const;
	void goalApproachRadius( float newValue );

	float goalStopRadius() const;
	void goalStopRadius( float newValue );

	float radius() const;
	void radius( float newValue );

	// Python Interface
	PY_AUTO_CONSTRUCTOR_FACTORY_DECLARE( PyBoidsFilter, END );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, influenceRadius, influenceRadius );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, collisionFraction,
		collisionFraction );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, goalApproachRadius,
		approachRadius );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, goalStopRadius, stopRadius );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, normalSpeed, speed );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, timeMultiplier, timeMultiplier );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( uint, state, state );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, radius, radius );

	// Implementation of PyFilter
	virtual BoidsFilter * pAttachedFilter();
	virtual const BoidsFilter * pAttachedFilter() const;

protected:
	// Implementation of PyFilter
	virtual BoidsFilter * getNewFilter();
	virtual void onLosingAttachedFilter();

private:
	// These attributes are caches of the values in BoidsFilter, but
	// they are only used if this->pAttachedFilter() is NULL
	float influenceRadius_;
	float collisionFraction_;
	float normalSpeed_;
	float timeMultiplier_;
	uint state_;
	float goalApproachRadius_;
	float goalStopRadius_;
	float radius_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyBoidsFilter );

BW_END_NAMESPACE

#endif // PY_BOIDS_FILTER_HPP
