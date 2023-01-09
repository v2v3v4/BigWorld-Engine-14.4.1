#include "pch.hpp"

#include "collide_psa.hpp"
#include "source_psa.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system.hpp"
#include "romp/romp_collider.hpp"
#include "physics2/worldtri.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "collide_psa.ipp"
#endif


/**
 *	This is the serialiser for CollidePSA properties
 */
template < typename Serialiser >
void CollidePSA::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, CollidePSA, spriteBased );
	BW_PARITCLE_SERIALISE( s, CollidePSA, elasticity );
	BW_PARITCLE_SERIALISE( s, CollidePSA, minAddedRotation );
	BW_PARITCLE_SERIALISE( s, CollidePSA, maxAddedRotation );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundTag );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundEnabled );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundSrcIdx );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundProject );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundGroup );
	BW_PARITCLE_SERIALISE( s, CollidePSA, soundName );
}


void CollidePSA::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< CollidePSA >( pSect, this ) );
}


void CollidePSA::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< CollidePSA >( pSect, this ) );
}


ParticleSystemActionPtr CollidePSA::clone() const
{
	BW_GUARD;
	CollidePSA * psa = new CollidePSA();
	ParticleSystemAction::clone( psa );
	this->serialise( BW::CloneObject< CollidePSA >( this, psa ) );
	return psa;
}


/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void CollidePSA::execute( ParticleSystem & particleSystem, float dTime )
{
	BW_GUARD;
	
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
void CollidePSA::lateExecute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( CollidePSA_execute );
	const SourcePSA * pSource = static_cast< const SourcePSA * >( 
		&*particleSystem.pAction( PSA_SOURCE_TYPE_ID ) );

	if (!pSource)
	{
		return;
	}

	const RompColliderPtr pGS = pSource->groundSpecifier();
	if (!pGS)
	{
		return;
	}

	uint64	tend = timestamp() + stampsPerSecond() / 2000;

	bool soundHit = false;
	float maxVelocity = 0;
	Vector3 soundPos;
	uint materialKind = 0;

	Particles::iterator it = particleSystem.begin();
	Particles::iterator end = particleSystem.end();

	WorldTriangle tri;

	//bust out of the loop if we take more than 0.5 msec

	//Sprite particles don't calculate spin
	while ( it != end && timestamp()<tend )
	{
		Particle & particle = *it++;

		if (!particle.isAlive())
		{		
			continue;
		}

		//note - particles get moved after actions.
		Vector3 velocity;
		particle.getVelocity(velocity);
		Vector3 pos;
		Vector3 newPos;

		if (particleSystem.isLocal())
		{
			Matrix world = particleSystem.worldTransform();
			pos = world.applyPoint(particle.position());
			Vector3 nPos;
			particleSystem.predictPosition( particle, dTime, nPos );
			newPos = world.applyPoint(nPos);
		}
		else
		{
			pos = particle.position();
			particleSystem.predictPosition( particle, dTime, newPos );
		}


		float tValue = pGS->collide( pos, newPos, tri );
		if ( tValue >= 0.f && tValue <= 1.f )
		{
			// calc v as a dotprod of the two normalised vectors (before and after collision)
			Vector3 oldVel = velocity / velocity.length();
			tri.bounce( velocity, elasticity_ );
			particle.setVelocity( velocity );
			float newSpeed = velocity.length();
			Vector3 newVel(velocity / newSpeed);
			float severity = oldVel.dotProduct(newVel);
			//DEBUG_MSG("severity: %1.3f, speed=%1.3f\n", severity, newSpeed);
			float v = (1 - severity) * newSpeed;

			//now spin the particle ( mesh only )
			if (!spriteBased_)
			{
				//first, calculate the current rotation, and update the pitch/yaw value.
				Matrix currentRot;
				currentRot.setRotate( particle.yaw(), particle.pitch(), 0.f );
				Matrix spin;
                float spinSpeed = particle.meshSpinSpeed();
                Vector3 meshSpinAxis = particle.meshSpinAxis();
                // If there is no spin direction then creating a rotation 
                // matrix can create weird matrices - e.g. matrices with
                // negative scale components and a translation.  We choose the
                // velocity as the spin direction (aribitrarily choosing, for
                // example up looks weird).
                if (meshSpinAxis == Vector3::zero())
                {
                    meshSpinAxis = velocity;
                    meshSpinAxis.normalise();
                }

				D3DXMatrixRotationAxis
                ( 
                    &spin, 
                    &meshSpinAxis, 
                    spinSpeed * (particle.age()-particle.meshSpinAge()) 
                );

				currentRot.preMultiply( spin );		
				particle.pitch( currentRot.pitch() );
				particle.yaw( currentRot.yaw() );

				//now, reset the age of the spin 
				particle.meshSpinAge( particle.age() );

				//finally, update the spin ( stored in the particle's colour )
				float addedSpin = unitRand() * (maxAddedRotation_-minAddedRotation_) + minAddedRotation_;
				addedSpin *= min( newSpeed, 1.f );				
				spinSpeed = Math::clamp( 0.f, spinSpeed + addedSpin, 1.f );
                particle.meshSpinSpeed(spinSpeed);
                particle.meshSpinAxis(meshSpinAxis);
			}

			if (soundEnabled_ && v > 0.5f)
			{
				soundHit = true;
				if (v > maxVelocity) {
					maxVelocity = v;
					soundPos = particle.position();
					materialKind = tri.materialKind();
				}
			}
		}
	}
}


/** 
 *	This internal method updates the sound tag, and should be called whenever
 *	one of the soundProject_, soundGroup_ or soundName_ variables have changed.
 *
 *	If any of the constituent variables are empty, the sound tag is invalid and
 *	will be set to an empty string.
 */
void CollidePSA::updateSoundTag()
{
	if (!soundProject_.empty() && !soundGroup_.empty() && !soundName_.empty())
	{		
		soundTag_ = "/" + soundProject_ + "/" + soundGroup_ + "/" + soundName_;
	}
	else
	{
		soundTag_ = "";
	}
}

// -----------------------------------------------------------------------------
// Section: Python Interface to the PyCollidePSA.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyCollidePSA )

/*~ function Pixie.CollidePSA
 *	Factory function to create and return a new PyCollidePSA object. CollidePSA is
 *	a ParticleSystemAction that collides particles with the collision scene.
 *	@param elasticity Elasticity of the collisions.
 *	@return A new PyCollidePSA object.
 */
PY_FACTORY_NAMED( PyCollidePSA, "CollidePSA", Pixie )

PY_BEGIN_METHODS( PyCollidePSA )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyCollidePSA )
	PY_ATTRIBUTE( spriteBased )
	/*~ attribute PyCollidePSA.elasticity
	 *	This sets the amount of elasticity in the collision. 0 indicates no
	 *	reflection of velocity in the direction of the collision normal. 1
	 *	indicates full reflection of velocity in the direction of collision
	 *	normal. The default value is 0.
	 *	@type Float. Recommend values are 0.0 to 1.0.
	 */
	PY_ATTRIBUTE( elasticity )
	/*~ attribute PyCollidePSA.minAddedRotation
	 *	This specifies the minimum amount of spin added to the particle
	 *	when it collides ( dynamically scaled by velocity ).
	 *	The actual value varies randomly between min and max added rotation.
	 *	The value is interpreted as an absolute amount, so a particle that
	 *	is not currently spinning can begin spinning upon a collision.
	 *	If negative, the spin will decrease / be dampened.
	 *	@type Float. Recommend values are 0.0 to 1.0.
	 */
	PY_ATTRIBUTE( minAddedRotation )
	/*~ attribute PyCollidePSA.maxAddedRotation
	 *	This specifies the maximum amount of spin added to the particle
	 *	when it collides ( dynamically scaled by velocity ).
	 *	The actual value varies randomly between min and max added rotation.
	 *	The value is interpreted as an absolute amount, so a particle that
	 *	is not currently spinning can begin spinning upon a collision.
	 *	If negative, the spin will decrease / be dampened.
	 *	@type Float. Recommend values are 0.0 to 1.0.
	 */
	PY_ATTRIBUTE( maxAddedRotation )
	/*~ attribute PyCollidePSA.soundEnabled
	 *	Not supported.
	 */
	PY_ATTRIBUTE( soundEnabled )
	/*~ attribute PyCollidePSA.soundSrcIdx
	 *	Not supported.
	 */
	PY_ATTRIBUTE( soundSrcIdx )
	/*~ attribute PyCollidePSA.soundTag
	 *	Not supported.
	 */
	PY_ATTRIBUTE( soundTag )
PY_END_ATTRIBUTES()


/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python.
 */
PyObject *PyCollidePSA::pyNew( PyObject *args )
{
	BW_GUARD;
	float elasticity;

	if ( PyArg_ParseTuple( args, "f", &elasticity ) )
	{
		CollidePSAPtr pAction = new CollidePSA(elasticity);
		return new PyCollidePSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "PyCollidePSA:"
			"Expected a float value ( elasticity )." );
		return NULL;
	}
}


PY_SCRIPT_CONVERTERS( PyCollidePSA )

BW_END_NAMESPACE

// collide_psa.cpp

