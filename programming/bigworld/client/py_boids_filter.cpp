#include "pch.hpp"

#include "py_boids_filter.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PyBoidsFilter )

PY_BEGIN_METHODS( PyBoidsFilter )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBoidsFilter )

	/*~ attribute BoidsFilter.influenceRadius
	 *
	 *	The influence radius simply determines how close
	 *	a boid must be to another to affect it at all.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( influenceRadius )

	/*~ attribute BoidsFilter.collisionFraction
	 *
	 *	The collision fraction defines the proportion of the influence radius
	 *	within which neighbours will be considered colliding.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( collisionFraction )

	/*~ attribute BoidsFilter.approachRadius
	 *
	 *	When the boids are in the process of landing, and
	 *	a boid is inside the approach radius, then it will
	 *	point directly at its goal, instead of smoothly turning
	 *	towards it.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( approachRadius )

	/*~ attribute BoidsFilter.stopRadius
	 *
	 *	When the boids are in the process of landing, and
	 *	a boid is inside the stop radius, it will be positioned
	 *	exactly at the goal, and its speed will be set to 0.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( stopRadius )

	/*~ attribute BoidsFilter.speed
	 *
	 *	The normal speed of the boids.  While boids will speed
	 *	up and slow down during flocking, their speed will always
	 *	be damped toward this speed.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( speed )

	/*~ attribute BoidsFilter.timeMultiplier
	 *
	 *	Multiplies the change in time used to update the boids. Affects the
	 *	overall speed that they all move at. Don't make this too large or the
	 *	birds will start trying to escape the radius.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( timeMultiplier )

	/*~ attribute BoidsFilter.state
	 *
	 * Controls whether the boids are flying or landing. By default they are in
	 * the flying state which is specified with 0. The landing state will cause
	 * the boids to land and is specified with 1. When a boid lands the method
	 * boidsLanded is called.
	 *
	 *	@type	unsigned integer
	 */
	PY_ATTRIBUTE( state )

	/*~ attribute BoidsFilter.radius
	 *
	 *	This is the max distance from the entity that the boids are
	 *	clamped to. A boid is only allowed to fly a percentage of this distance
	 *	per frame. If it does get outside the radius it will be spawned back at
	 *	the centre. Don't make it too small or the boids will start getting
	 *	stuck on the edges and spawning back at the centre.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( radius )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyBoidsFilter )

/*~ function BigWorld.BoidsFilter
 *
 *	This function creates a new BoidsFilter.  This is used to filter the
 *	movement of an Entity which consists of several models (boids) for which
 *	flocking behaviour is desired.
 *
 *	@return a new BoidsFilter
 */
PY_FACTORY_NAMED( PyBoidsFilter, "BoidsFilter", BigWorld )


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter.
 */
PyBoidsFilter::PyBoidsFilter( PyTypeObject * pType ) :
	PyAvatarFilter( pType ),
	influenceRadius_( 10.f ),
	collisionFraction_( 0.5f ),
	normalSpeed_( 0.5f ),
	timeMultiplier_( 30.0f ),
	state_( 0 ),
	goalApproachRadius_( 10.f ),
	goalStopRadius_( 1.f ),
	radius_( 1000.0f )
{
	BW_GUARD;
}


#define GET_SET_CACHED( TYPE, FIELDNAME )									\
/**																			\
 *	This method returns FIELDNAME for this object's managed BoidsFilter, if	\
 *	it exists, and its cache of the value if not							\
 */																			\
TYPE PyBoidsFilter::FIELDNAME() const										\
{																			\
	BW_GUARD;																\
																			\
	const BoidsFilter * pFilter = this->pAttachedFilter();					\
																			\
	if (pFilter == NULL)													\
	{																		\
		return FIELDNAME##_;												\
	}																		\
																			\
	return pFilter->FIELDNAME();											\
}																			\
																			\
																			\
/**																			\
 *	This method updates FIELDNAME for this object and its managed			\
 *	BoidsFilter																\
 */																			\
void PyBoidsFilter::FIELDNAME( TYPE newValue )								\
{																			\
	BW_GUARD;																\
																			\
	if (newValue == FIELDNAME##_)											\
	{																		\
		return;																\
	}																		\
																			\
	FIELDNAME##_ = newValue;												\
																			\
	BoidsFilter * pFilter = this->pAttachedFilter();						\
																			\
	if (pFilter != NULL)													\
	{																		\
		pFilter->FIELDNAME( FIELDNAME##_ );									\
	}																		\
}


GET_SET_CACHED( float, influenceRadius )
GET_SET_CACHED( float, collisionFraction )
GET_SET_CACHED( float, normalSpeed )
GET_SET_CACHED( float, timeMultiplier )
GET_SET_CACHED( uint, state )
GET_SET_CACHED( float, goalApproachRadius )
GET_SET_CACHED( float, goalStopRadius )
GET_SET_CACHED( float, radius )


/**
 *	This method returns the BoidsFilter * we created, if not yet lost,
 *	and NULL otherwise.
 */
BoidsFilter * PyBoidsFilter::pAttachedFilter()
{
	BW_GUARD;

	return static_cast< BoidsFilter * >( this->PyFilter::pAttachedFilter() );
}


/**
 *	This method returns the const BoidsFilter * we created, if not yet
 *	lost, and NULL otherwise.
 */
const BoidsFilter * PyBoidsFilter::pAttachedFilter() const
{
	BW_GUARD;

	return static_cast< const BoidsFilter * >(
		this->PyFilter::pAttachedFilter() );
}


/**
 *	This method provides a new BoidsFilter
 */
BoidsFilter * PyBoidsFilter::getNewFilter()
{
	BW_GUARD;

	BoidsFilter * pNewFilter = new BoidsFilter( this );

	pNewFilter->influenceRadius( influenceRadius_ );
	pNewFilter->collisionFraction( collisionFraction_ );
	pNewFilter->normalSpeed( normalSpeed_ );
	pNewFilter->timeMultiplier( timeMultiplier_ );
	pNewFilter->state( state_ );
	pNewFilter->goalApproachRadius( goalApproachRadius_ );
	pNewFilter->goalStopRadius( goalStopRadius_ );
	pNewFilter->radius( radius_ );

	return pNewFilter;
}


/**
 *	This method updates our cached filter properties before the existing filter
 *	is deleted.
 */
void PyBoidsFilter::onLosingAttachedFilter()
{
	BW_GUARD;

	BoidsFilter * pFilter = this->pAttachedFilter();

	influenceRadius_ = pFilter->influenceRadius();
	collisionFraction_ = pFilter->collisionFraction();
	normalSpeed_ = pFilter->normalSpeed();
	timeMultiplier_ = pFilter->timeMultiplier();
	state_ = pFilter->state();
	goalApproachRadius_ = pFilter->goalApproachRadius();
	goalStopRadius_ = pFilter->goalStopRadius();
	radius_ = pFilter->radius();
}


BW_END_NAMESPACE

// py_boids_filter.cpp
