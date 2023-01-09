#include "pch.hpp"
#include "worldeditor/editor/chunk_obstacle_locator.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/collisions/collision_callbacks.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

BW_BEGIN_NAMESPACE

//------------------------------------------------------------
//Section : ChunkObstacleToolLocator
//------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ChunkObstacleToolLocator::

PY_TYPEOBJECT( ChunkObstacleToolLocator )

PY_BEGIN_METHODS( ChunkObstacleToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkObstacleToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkObstacleToolLocator, Locator )

/**
 *	constructor.
 */
ChunkObstacleToolLocator::ChunkObstacleToolLocator(
	CollisionCallback& collisionRoutine,
	PyTypeObject * pType ):
		ChunkToolLocator( pType ),
		callback_( &collisionRoutine ),
		positionValid_( false )
{
}


/**
 *	This method calculates the position of the tool, defined
 *	as the nearest solid object along the camera's world ray.
 *
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool this locator applies to.
 */
void ChunkObstacleToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if ( !space )
		return;

	if ( callback_ == &ObstacleLockCollisionCallback::s_default )
	{
		ObstacleLockCollisionCallback* pCallback = &ObstacleLockCollisionCallback::s_default;

		Vector3 rayStart = Moo::rc().invView().applyToOrigin();
		Vector3 pos = Snap::toObstacle( rayStart, worldRay );

		if ( pos != rayStart )
		{
			// found something

			// set transform so that the y-axis is aligned with the surface normal,
			// and the x and z axes point along the surface
			pCallback->normal_.normalise();
			if ( fabsf(pCallback->normal_.y) < 0.95f )
			{
				transform_.lookAt( pos, pCallback->normal_, Vector3( 0.f, 1.f, 0.f ) );			
			}
			else
			{
				transform_.lookAt( pos, pCallback->normal_, Vector3( 0.f, 0.f, 1.f ) );				
			}
			transform_.invert();
			transform_.translation( Vector3(0,0,0) );
			transform_.preRotateX( DEG_TO_RAD(90.f) );
			transform_.translation( pos );
			positionValid_ = true;
		}
		else
		{
			positionValid_ = false;
		}

		pCallback->clear();
	}
	else
	{
		float distance = 0.f;
		Vector3 extent = Moo::rc().invView().applyToOrigin() +
			( worldRay * WorldManager::instance().getMaxFarPlane() );

		distance = space->collide( 
			Moo::rc().invView().applyToOrigin(),
			extent,
			*callback_ );

		if ( distance > 0.f )
		{
			Vector3	pos = Moo::rc().invView().applyToOrigin() + ( worldRay * distance );
			transform_.setTranslate( pos );
		}
	}
}



/**
 *	Python factory method
 */
PyObject * ChunkObstacleToolLocator::pyNew( PyObject * pArgs )
{
	BW_GUARD;

	if (PyTuple_Size( pArgs ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "ChunkObstacleToolLocator() "
			"expects no arguments" );
		return NULL;
	}

	return new ChunkObstacleToolLocator( ObstacleLockCollisionCallback::s_default );
}

BW_END_NAMESPACE

