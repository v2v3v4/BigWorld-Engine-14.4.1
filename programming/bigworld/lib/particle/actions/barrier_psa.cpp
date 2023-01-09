#include "pch.hpp"

#include "barrier_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "barrier_psa.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/// Enumeration Type Names
const BW::string BarrierPSA::shapeTypeNames_[BarrierPSA::SHAPE_MAX] = { "None", "Vertical Cylinder", "Box", "Sphere" };
const BW::string BarrierPSA::reactionTypeNames_[BarrierPSA::REACTION_MAX] = { "Bounce", "Remove", "Allow", "Wrap" };


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */

void BarrierPSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD;

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active or if the barrier is shapeless.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}
	else if ( (shape_ == NONE) || (reaction_ == ALLOW) )
	{
		return;
	}

	if (particleSystem.isLocal())
	{
		this->lateExecute( particleSystem, dTime );
	}
	else
	{
		ParticleSystemAction::pushLateUpdate( *this, particleSystem, dTime );
	}
}


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void BarrierPSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( BarrierPSA_execute );

	// Check for collision depending on the shape.
	switch ( shape_ )
	{
	case VERTICAL_CYLINDER:
		collideWithCylinder( particleSystem, dTime );
		break;

	case BOX:
		collideWithBox( particleSystem, dTime );
		break;

	case SPHERE:
		collideWithSphere( particleSystem, dTime );
		break;

	case NONE:
	case SHAPE_MAX:
		break;
	}
}


// -----------------------------------------------------------------------------
// Section: Private Helper Methods for BarrierPSA.
// -----------------------------------------------------------------------------


/**
 *	This method checks if any particles in the particle system have collided
 *	with the vertical cylinder and bounces the particles back.
 *
 *	@param particleSystem	The particles to be checked.
 *	@param dTime			Time span over which to be checked.
 *
 *	@return True if the particle collided.
 */
void BarrierPSA::collideWithCylinder( ParticleSystem & particleSystem,
		float dTime ) const
{
	BW_GUARD;
	// This is a vertical cylinder of infinite height described by a point
	// on the axis (vecA_) and a radius (radius_.)
	// The x and z values of the the vector used for distance determination
	// from the axis.
	float radiusSquared = radius_ * radius_;

	Matrix objectToWorld = particleSystem.objectToWorld();
	Vector3 positionOfCylinder = objectToWorld.applyPoint(vecA_);
	Vector2 cylinderXZ( positionOfCylinder.x, positionOfCylinder.z );

	// Iterate through the particles.
	BW::vector< Particle > newParticles;
	Particles::iterator current = particleSystem.begin();
	//note : can't save the end() during iteration because particles
	//may be erased, thus moving the end iterator. ( i.e. this is ok )
	while (current != particleSystem.end())
	{
		Particle & particle = *current;

		if (!particle.isAlive())
		{
			++current;
			continue;
		}

		float xDiff = positionOfCylinder.x - particle.position().x;
		float zDiff = positionOfCylinder.z - particle.position().z;
		float beforeSquared = xDiff * xDiff + zDiff * zDiff;
		Vector3 posAfter;
		particleSystem.predictPosition( particle, dTime, posAfter );
		xDiff = positionOfCylinder.x - posAfter.x;
		zDiff = positionOfCylinder.z - posAfter.z;
		float afterSquared = xDiff * xDiff + zDiff * zDiff;

		//test for entirely outside the volume
		bool now = ( beforeSquared > radiusSquared );
		bool later = ( afterSquared > radiusSquared );		

		switch (reaction_)
		{
		case REMOVE:
			{
				if ( now != later )
				{
					current = particleSystem.removeParticle( current );
				}
				else
				{
					++current;
				}
				break;
			}

		case BOUNCE:
			{
				if ( now != later )
				{
					// Bounce the particle back towards the centre.
					// We'll just cancel the velocity component in the
					// direction away from the cylinder and add in one
					// towards the centre of the cylinder.
					Vector3 inwardDir( positionOfCylinder.x - particle.position().x,
						0.0f, positionOfCylinder.z - particle.position().z );
					inwardDir.normalise();

					Vector3 velocity;
					particle.getVelocity(velocity);
					float dotProduct = velocity.dotProduct( inwardDir );
					particle.setVelocity( velocity - ( 1.0f + resilience_ ) * dotProduct * inwardDir );				
				}

				++current;
				break;
			}

		case WRAP:
			{
				if ( later )
				{
					Particle newParticle( particle );
					Vector3 & pos = newParticle.position();

					Vector2 delta( pos.x, pos.z );
					delta -= cylinderXZ;
					delta.normalise();
					pos.x -= delta.x * radius_ * 2.f;
					pos.z -= delta.y * radius_ * 2.f;

					current = particleSystem.removeParticle( current );
					newParticles.push_back( newParticle );
				}
				else
				{
					++current;
				}

				break;
			}

		default:
			++current;
			break;
		}		
	}

	this->renewParticles( particleSystem, newParticles );
}

/**
 *	This method checks if any particles of particle system has collided with
 *	the box and bounces the particles back.
 *
 *	@param particleSystem	The particles to be checked.
 *	@param dTime			Time span over which to be checked.
 *
 *	@return True if the particle collided.
 */
void BarrierPSA::collideWithBox( ParticleSystem &particleSystem,
		float dTime ) const
{
	BW_GUARD;
	// This is a box that has sides parallel to the x,y,z axes. It is
	// described its two opposite corners, vecA_ and vecB_.


	// Precalculate our bounds for each axis component. We could probably
	// precaculate this and store it as a private data member, but it is
	// a trade off between space and speed.

	Matrix objectToWorld = particleSystem.objectToWorld();
	Vector3 vecA_world = objectToWorld.applyPoint(vecA_);
	Vector3 vecB_world = objectToWorld.applyPoint(vecB_);	

	float minX, maxX;
	if ( vecA_world.x > vecB_world.x )
	{
		minX = vecB_world.x;
		maxX = vecA_world.x;
	}
	else
	{
		minX = vecA_world.x;
		maxX = vecB_world.x;
	}

	float minY, maxY;
	if ( vecA_world.y > vecB_world.y )
	{
		minY = vecB_world.y;
		maxY = vecA_world.y;
	}
	else
	{
		minY = vecA_world.y;
		maxY = vecB_world.y;
	}

	float minZ, maxZ;
	if ( vecA_world.z > vecB_world.z )
	{
		minZ = vecB_world.z;
		maxZ = vecA_world.z;
	}
	else
	{
		minZ = vecA_world.z;
		maxZ = vecB_world.z;
	}

	float xRange = maxX - minX;
	float yRange = maxY - minY;
	float zRange = maxZ - minZ;

	// Iterate through the particles.
	BW::vector<Particle> newParticles;
	Particles::iterator current = particleSystem.begin();
	while ( current != particleSystem.end() )
	{
		Particle &particle = *current;

		if (!particle.isAlive())
		{
			++current;
			continue;
		}

		Vector3 posNow = particle.position();
		Vector3 velocity;
		particle.getVelocity(velocity);
		Vector3 posAfter;		
		particleSystem.predictPosition( particle, dTime, posAfter );

		// test for whether the particle is entirely outside the box
		uint8 now = (posNow.x < minX) | ((posNow.y < minY) << 1) | ((posNow.z < minZ) << 2) | ((posNow.x > maxX) << 3) | ((posNow.y > maxY) << 4) | ((posNow.z > maxZ) << 5);
		uint8 later = (posAfter.x < minX) | ((posAfter.y < minY) << 1) | ((posAfter.z < minZ) << 2) | ((posAfter.x > maxX) << 3) | ((posAfter.y > maxY) << 4) | ((posAfter.z > maxZ) << 5);		

		// check to see if one of the box planes has been crossed
		uint8 crossedBoundary = now ^ later;

		switch ( reaction_ )
		{
		case REMOVE:
			{
				if (crossedBoundary)
				{
					current = particleSystem.removeParticle( current );
				}
				else
				{
					current++;
				}
				break;
			}
		case BOUNCE:
			{
				if (crossedBoundary & (0x9))
				{
					// crossed an yz plane
					particle.setXVelocity( velocity.x * -resilience_ );
				}

				if (crossedBoundary & ((0x9) << 1))
				{
					// crossed an xz plane
					particle.setYVelocity( velocity.y * -resilience_ );
				}

				if (crossedBoundary & ((0x9) << 2))
				{
					// crossed an xy plane
					particle.setZVelocity( velocity.z * -resilience_ );
				}

				current++;
				break;
			}
		case WRAP:
			{
				if (later)
				{
					Particle newParticle( particle );
					Vector3& pos = newParticle.position();

					if (pos.x < minX)
					{
						pos.x += xRange;
					}
					else if (pos.x > maxX)
					{
						pos.x -= xRange;
					}

					if (pos.y < minY)
					{
						pos.y += yRange;
					}
					else if (pos.y > maxY)
					{
						pos.y -= yRange;
					}

					if (pos.z < minZ)
					{
						pos.z += zRange;
					}
					else if (pos.z > maxZ)
					{
						pos.z -= zRange;
					}

					current = particleSystem.removeParticle( current );
					newParticles.push_back( newParticle );
				}
				else
				{
					current++;
				}

				break;
			}			
		default:
			current++;
			break;
		}
	}

	this->renewParticles( particleSystem, newParticles );
}

/**
 *	This method checks if any particles in the particle system have collided
 *	with the sphere and bounces the particles back.
 *
 *	@param particleSystem	The particles to be checked.
 *	@param dTime			Time span over which to be checked.
 *
 *	@return True if the particle collided.
 */
void BarrierPSA::collideWithSphere( ParticleSystem &particleSystem,
		float dTime ) const
{
	BW_GUARD;
	// The sphere is described by a centre (vecA_) and a radius (radius_.)
	float radiusSquared = radius_ * radius_;

	Matrix objectToWorld = particleSystem.objectToWorld();
	Vector3 positionOfSphere = objectToWorld.applyPoint(vecA_);

	// Iterate through the particles.
	BW::vector<Particle> newParticles;
	Particles::iterator current = particleSystem.begin();
	while ( current != particleSystem.end() )
	{
		Particle &particle = *current;

		if (!particle.isAlive())
		{
			++current;
			continue;
		}

		Vector3 posNow = particle.position();
		Vector3 velocity;
		particle.getVelocity( velocity );
		Vector3 posAfter;
		particleSystem.predictPosition( particle, dTime, posAfter );

		Vector3 towardNow = positionOfSphere - posNow;
		Vector3 towardAfter = positionOfSphere - posAfter;

		float distSqrNow = towardNow.lengthSquared();
		float distSqrAfter = towardAfter.lengthSquared();

		bool now = ( distSqrNow > radiusSquared );
		bool later = ( distSqrAfter > radiusSquared );

		// if cross boundary, do something

		switch ( reaction_ )
		{
		case REMOVE:
			{
				if (now != later)
				{
					current = particleSystem.removeParticle( current );
				}
				else
				{
					current++;
				}
				break;
			}

		case BOUNCE:
			{
				if (now != later)
				{
					// Bounce the particle back where they came from.
					towardNow.normalise();
					float dotProduct = velocity.dotProduct( towardNow );
					particle.setVelocity( velocity - (1.0f + resilience_) * dotProduct * towardNow );					
				}				
				current++;
				break;
			}			

		case WRAP:
			{
				if (later)
				{
					Particle newParticle( particle );
					Vector3& pos = newParticle.position();

					Vector3 delta = positionOfSphere - pos;
					delta.normalise();					
					pos = pos + delta * radius_ * 2.f;

					current = particleSystem.removeParticle( current );
					newParticles.push_back( newParticle );
				}
				else
				{
					current++;
				}
				break;
			}

		default:
			{
				current++;
				break;
			}
		}
	}

	this->renewParticles( particleSystem, newParticles );
}

/**
 *	Put back wrapping particles
 */

void BarrierPSA::renewParticles( ParticleSystem & particleSystem,
	const BW::vector<Particle> & newParticles ) const
{
	for ( BW::vector<Particle>::const_iterator it = newParticles.begin(),
		end = newParticles.end(); it != end; ++it )
	{
		particleSystem.addParticle( *it,

			particleSystem.pRenderer()->isMeshStyle() );
	}
}

/**
 *	This is the serialiser for BarrierPSA properties
 */
template < typename Serialiser >
void BarrierPSA::serialise( const Serialiser & s ) const
{
	BW_GUARD;
	BW_PARITCLE_SERIALISE_ENUM( s, BarrierPSA, shape );
	BW_PARITCLE_SERIALISE_ENUM( s, BarrierPSA, reaction );
	// vecA is shared by most barrier types
	BW_PARITCLE_SERIALISE( s, BarrierPSA, vecA );
	// vecB also shared
	BW_PARITCLE_SERIALISE( s, BarrierPSA, vecB );
	// radius shared by sphere and cylinder
	BW_PARITCLE_SERIALISE( s, BarrierPSA, radius );
}


void BarrierPSA::loadInternal( DataSectionPtr pSect )
{
	this->serialise( BW::LoadFromDataSection< BarrierPSA >( pSect, this ) );
}


void BarrierPSA::saveInternal( DataSectionPtr pSect ) const
{
	this->serialise( BW::SaveToDataSection< BarrierPSA >( pSect, this ) );
}


ParticleSystemActionPtr BarrierPSA::clone() const
{
	BarrierPSA * psa = new BarrierPSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< BarrierPSA >( this, psa ) );
	return psa;
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PyBarrierPSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyBarrierPSA )

/*~ function Pixie.BarrierPSA
 *	Factory function to create and return a new PyBarrierPSA object. BarrierPSA is
 *	a ParticleSystemAction that acts as an invisible barrier to particles. 
 *	@return A new PyBarrierPSA object.
 */
PY_FACTORY_NAMED( PyBarrierPSA, "BarrierPSA", Pixie )

PY_BEGIN_METHODS( PyBarrierPSA )
	/*~ function PyBarrierPSA.verticalCylinder
	 *	Specifies that the BarrierPSA should take the shape of a vertical
	 *	cylinder. Coordinates are in world space.
	 *	@param axisPoint Sequence of 3 floats. Point on axis of cylinder.
	 *	@param radius Float. Radius of cylinder.
	 */
	PY_METHOD( verticalCylinder )
	/*~ function PyBarrierPSA.box
	 *	Specifies that the BarrierPSA should take the shape of a Box. 
	 *	Coordinates are in world space.
	 *	@param corner Sequence of 3 floats. One corner of the box.
	 *	@param oppositeCorner Sequence of 3 floats. Opposite corner of the box.
	 */
	PY_METHOD( box )
	/*~ function PyBarrierPSA.sphere
	 *	Specifies that the BarrierPSA should take the shape of a sphere.
	 *	Coordinates are in world space.
	 *	@param center Sequence of 3 floats. Center of the sphere.
	 *	@param radius Float. Radius of the sphere.
	 */
	PY_METHOD( sphere )

	/*~ function PyBarrierPSA.bounceParticles
	 *	Set the BarrierPSA to bounce particles that collide with it.
	 */
	PY_METHOD( bounceParticles )
	/*~ function PyBarrierPSA.removeParticles
	 *	Set the BarrierPSA to remove particles that collide with it.
	 */
	PY_METHOD( removeParticles )
	/*~ function PyBarrierPSA.allowParticles
	 *	Set the BarrierPSA to allow particles to collide with it (effectively 
	 *	turns it off).
	 */
	PY_METHOD( allowParticles )
	/*~ function PyBarrierPSA.wrapParticles
	 *	Set the BarrierPSA to allow particles to wrap around it.
	 */
	PY_METHOD( wrapParticles )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBarrierPSA )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. No arguments
 *					are required or looked at by the factory method.
 */
PyObject *PyBarrierPSA::pyNew( PyObject *args )
{
	BW_GUARD;
	BarrierPSAPtr pAction = new BarrierPSA;
	return new PyBarrierPSA( pAction );
}


/**
 *	This method allows the Python script to specify that the BarrierPSA take
 *	the shape of a vertical cylinder.
 *
 *	@param args		The list of parameters passed from Python. The expected
 *					arguments are a triple of floats (point) and a float 
 *					(radius.)
 */
PyObject *PyBarrierPSA::py_verticalCylinder( PyObject *args )
{
	BW_GUARD;
	float x, y, z;
	float radius;

	if ( PyArg_ParseTuple( args, "(fff)f", &x, &y, &z, &radius ) )
	{
		pAction_->verticalCylinder( Vector3( x, y, z ), radius );
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "BarrierPSA.verticalCylinder: "
			"Expected a triple of floats and a float." );
		return NULL;
	}
}


/**
 *	This method allows the Python script to specify that the BarrierPSA take
 *	the shape of a box.
 *
 *	@param args		The list of parameters passed from Python. The expected
 *					arguments are two triples of floats (corner and opposite
 *					corner.)
 */
PyObject *PyBarrierPSA::py_box( PyObject *args )
{
	BW_GUARD;
	float x1, y1, z1;
	float x2, y2, z2;

	if ( PyArg_ParseTuple( args, "(fff)(fff)",
		&x1, &y1, &z1, &x2, &y2, &z2 ) )
	{
		pAction_->box( Vector3( x1, y1, z1 ), Vector3( x2, y2, z2 ) );
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "BarrierPSA.box: "
			"Expected two triples of floats." );
		return NULL;
	}
}


/**
 *	This method allows the Python script to specify that the BarrierPSA take
 *	the shape of a sphere.
 *
 *	@param args		The list of parameters passed from Python. The expected
 *					arguments are a triple of floats (centre) and a float
 *					(radius.)
 */
PyObject *PyBarrierPSA::py_sphere( PyObject *args )
{
	BW_GUARD;
	float x, y, z;
	float radius;

	if ( PyArg_ParseTuple( args, "(fff)f", &x, &y, &z, &radius ) )
	{
		pAction_->sphere( Vector3( x, y, z ), radius );
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "BarrierPSA.sphere: "
			"Expected a triple of floats and a float." );
		return NULL;
	}
}


/**
 *	This method allows the Python script to set the reaction of the BarrierPSA
 *	to bouncing particles that collide with it.
 *
 *	@param args		The list of parameters passed from Python. No arguments
 *					are required or looked at the this method.
 */
PyObject *PyBarrierPSA::py_bounceParticles( PyObject *args )
{
	BW_GUARD;
	pAction_->bounceParticles();
	Py_RETURN_NONE;
}


/**
 *	This method allows the Python script to set the reaction of the BarrierPSA
 *	to removing particles that collide with it.
 *
 *	@param args		The list of parameters passed from Python. No arguments
 *					are required or looked at the this method.
 */
PyObject *PyBarrierPSA::py_removeParticles( PyObject *args )
{
	BW_GUARD;
	pAction_->removeParticles();
	Py_RETURN_NONE;
}


/**
 *	This method allows the Python script to set the reaction of the BarrierPSA
 *	to allowing particles to collide with it.
 *
 *	@param args		The list of parameters passed from Python. No arguments
 *					are required or looked at the this method.
 */
PyObject *PyBarrierPSA::py_allowParticles( PyObject *args )
{
	BW_GUARD;
	pAction_->allowParticles();
	Py_RETURN_NONE;
}


/**
 *	This method allows the Python script to set the reaction of the BarrierPSA
 *	to allowing particles to wrap around with it.
 *
 *	@param args		The list of parameters passed from Python. No arguments
 *					are required or looked at the this method.
 */
PyObject *PyBarrierPSA::py_wrapParticles( PyObject *args )
{
	BW_GUARD;
	pAction_->wrapParticles();
	Py_RETURN_NONE;
}


PY_SCRIPT_CONVERTERS( PyBarrierPSA )


BW_END_NAMESPACE

// barrier_psa.cpp
