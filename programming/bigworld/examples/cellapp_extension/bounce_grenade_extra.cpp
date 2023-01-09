#include "script/first_include.hpp"

#include "bounce_grenade_extra.hpp"

#include "common/closest_triangle.hpp"


DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( BounceGrenadeExtra )

PY_BEGIN_METHODS( BounceGrenadeExtra )

	/* function Entity bounceGrenade
	*  @components{ cell }
	*  The bounceGrenade function estimates the position at which a projectile
	*  would end up, using basic pinball physics and assuming acceleration due
	*  to gravity of 4.9. The simulated projectile only bounces off world
	*  geometry, and the coefficient of restitution for all collisions is
	*  fixed at approximately 0.3. This can be used to verify the results of a
	*  client projectile simulation.
	*
	*  @param position The position argument is the initial location of the
	*  projectile, as a tuple of 3 floats (x, y, z) in world coordinates.
	*
	*  @param velocity The velocity argument is the initial velocity of the
	*  projectile, as a tuple of 3 floats (x, y, z) in world coordinates.
	*
	*  @param elasticity The elasticity of the bounce. For each bounce, this is
	*  scalar is multiplied with the velocity.
	*
	*  @param radius     This is the radius of the grenade.
	*
	*  @param timeSample timeSample is the number of seconds in each iteration
	*  of the simulation, as a float. Smaller numbers provide more accurate
	*  simulation, but take longer to process.
	*
	*  @param maxSamples The maxSamples argument specifies the maximum number
	*  of iterations that the simulation should process. The simulation will
	*  end when either maxSamples iterations have been run, or when the
	*  projectile comes to rest. The estimated final position is the location
	*  of the projectile at the end of the simulation.
	*
	*  @param maxBounces The optional maxBounces argument specifies the
	*  maximum number of bounces that the simulation should process. The
	*  simulation will end when either maxBounces have occurred, or when the
	*  projectile comes to rest. If maxBounces is not specified, the
	*  simulation will end only when the projectile comes to rest.
	*
	*  @return A 2-tuple of 3 floats representing the estimated position
	*  (x, y, z) in world coordinates and the time taken for the simulation.
	*/
	PY_METHOD( bounceGrenade )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BounceGrenadeExtra )
PY_END_ATTRIBUTES()

const BounceGrenadeExtra::Instance< BounceGrenadeExtra >
		BounceGrenadeExtra::instance( BounceGrenadeExtra::s_getMethodDefs(),
			BounceGrenadeExtra::s_getAttributeDefs() );


/**
 *	Constructor.
 */
BounceGrenadeExtra::BounceGrenadeExtra( Entity & e ) :
	EntityExtra( e )
{
}


/**
 *	Destructor.
 */
BounceGrenadeExtra::~BounceGrenadeExtra()
{
}

/**
 *	This method is exposed to scripting. It is used by an entity to
 *	work out an initial velocity and final destination for a grenade
 *
 *	@param sourcePos  This is the starting point of the grenade
 *	@param velocity   This is the starting velocity of the grenade
 *	@param elasticity The elasticity of the bounce.
 *	@param radius     This is the radius of the grenade
 *	@param timeSample This is the amount of time for each loop
 *	@param maxSamples The maximum samples to take.
 *	@param maxBounces The maximum bounces to perform.
 *
 *	@return the final position of the grenade.
 */
PyObject * BounceGrenadeExtra::bounceGrenade( const Vector3 & sourcePos,
	const Vector3 & velocity, float elasticity,
	float radius, float timeSample,
	int maxSamples, int maxBounces ) const
{
	// TODO: This is an extremely naive implementation. It would be better
	// split into a controller that could perform calculations in the
	// background if nescessary and callback the entity with the result.
	Vector3 worldPos = sourcePos;
	Vector3 vel = velocity;
	float dTime = timeSample;

	ChunkSpace * cs = entity_.pChunkSpace();
	// TODO: remove the following when collide() query is supported
	// by all Physical Space types. Then will need to use PhysicalSpace
	// instead of ChunkSpace
	if (!cs) {
		PyErr_SetString( PyExc_TypeError,
				"BounceGrenadeExtra::bounceGrenade(): "
				"Entity physical space type is not supported " );
		return NULL;
	}

	ClosestTriangle ct( /*shouldUseTriangleFlags*/ false );
	WorldTriangle resTriangle;

	bool done = false;
	int loopCount = 0;
	int bounceCount = 0;
	while (!done)
	{
		++loopCount;
		// TODO: Should not have hard-coded gravity constant.
		vel[1] += -9.8f * dTime * 0.5f;

		bool checkForBounce = true;
		bool bounced = false;
		Vector3 endPos = worldPos;
		while (checkForBounce)
		{
			checkForBounce = false;

			Vector3 newPos = endPos + vel * dTime;
			float distance = cs->collide( endPos, newPos, ct );
			resTriangle = ct.triangle();

			if (distance > 0.f && distance <= (newPos - endPos).length())
			{
				bounced = true;
				float proportionBeforeBounce(
					(distance - radius) / vel.length() );

				if (proportionBeforeBounce < 0.0f)
				{
					proportionBeforeBounce = 0.0f;
				}
				endPos = endPos + vel * proportionBeforeBounce;
				Vector3 preVel( vel );

				if ((maxBounces != -1) && (bounceCount == maxBounces))
				{
					checkForBounce = false;
				}
				else
				{
					checkForBounce = true;
					resTriangle.bounce( vel, 1.0f );
					vel = elasticity * vel;
					endPos = endPos + vel * (dTime - proportionBeforeBounce);
				}
			}
			else
			{
				endPos = newPos;
			}
		}

		if (bounced)
		{
			++bounceCount;
			if ((maxBounces != -1) && (bounceCount > maxBounces))
			{
				done = true;
			}
		}

		// Revise the new position
		if ((endPos - worldPos).length() / dTime < 0.25f ||
					(maxSamples > 0 && loopCount >= maxSamples))
		{
			done = true;
		}
		worldPos = endPos;

		// Update velocity
		if (vel.length() < 0.25)
		{
			vel = Vector3( 0.0, vel[1], 0.0 );
		}
	}

	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, Script::getData( worldPos ) );
	PyTuple_SET_ITEM( pTuple, 1, Script::getData( loopCount * timeSample ) );

	return pTuple;
}

BW_END_NAMESPACE

// bounce_grenade_extra.cpp
