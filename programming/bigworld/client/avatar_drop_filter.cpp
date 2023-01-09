#include "pch.hpp"

#include "avatar_drop_filter.hpp"

#include "entity.hpp"
#include "py_avatar_drop_filter.hpp"
#include "py_entity.hpp"

#include "moo/geometrics.hpp"

#include "script/script_object.hpp"

#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

typedef WeakPyPtr< PyEntity > WeakPyEntityPtr;
typedef WeakPyPtr< PyAvatarDropFilter > WeakAvatarDropFilterPtr;
typedef ScriptObjectPtr< PyAvatarDropFilter > PyAvatarDropFilterPtr;

static WeakAvatarDropFilterPtr g_wpDebugFilter;
static WeakPyEntityPtr g_wpDebugFilterTarget;


/**
 *	Constructor
 *
 *	@param	pType	The python object defining the type of the filter. 
 */
AvatarDropFilter::AvatarDropFilter( PyAvatarDropFilter * pOwner ) :
	AvatarFilter( pOwner ),
	alignToGround_( false ),
	groundNormal_( 0, 1, 0 )
{
	BW_GUARD;	
}


/**
 *	Destructor
 */
AvatarDropFilter::~AvatarDropFilter()
{
	BW_GUARD;
}


/**
 *	This method drops the output position to the ground.
 */
void AvatarDropFilter::onEntityPositionUpdated( Entity & entity )
{
	BW_GUARD;

	if (entity.pVehicle())
	{
		// TODO: Consider colliding against the vehicle
		return;
	}

	if (g_wpDebugFilter.exists() && g_wpDebugFilter.get() == this->pOwner())
	{
		MF_ASSERT( !entity.isDestroyed() );
		g_wpDebugFilterTarget = entity.pPyEntity().get();
		MF_ASSERT( g_wpDebugFilterTarget.exists() );
	}

	SpaceID spaceID = entity.spaceID();
	Position3D localPos = entity.localPosition();
	Direction3D localDir = entity.localDirection();

	if (spaceID == NULL_SPACE_ID)
	{
		return;
	}

	static DogWatch dwAvatarDropFilter( "AvatarDropFilter" );
	dwAvatarDropFilter.start();

	groundNormal_.set( 0, 1, 0 );

	this->filterDropPoint( spaceID, localPos, localPos, &groundNormal_ );
	groundNormal_.normalise();

	if (alignToGround_)
	{
		Matrix rotation;
		rotation.setIdentity();
		rotation.setRotateY( localDir.yaw );

		// Note: As the normal and up vectors converge the rotation of the
		// quaternion becomes zero making this cross product safe.
		Vector3 slopeAxes( Vector3(0,1,0).crossProduct( groundNormal_ ) );

		Quaternion slopeRotation( slopeAxes,
			1 + Vector3(0,1,0).dotProduct( groundNormal_ ) );
		slopeRotation.normalise();

		Matrix slopeRotationMatrix;
		slopeRotationMatrix.setIdentity();
		slopeRotationMatrix.setRotate( slopeRotation );
		rotation.postMultiply( slopeRotationMatrix );

		localDir.pitch = rotation.pitch();
		localDir.roll = rotation.roll();
	}

	// Setting position clears velocity, so save it and restore it
	Vector3 localVelocity = entity.localVelocity();
	entity.setSpaceVehiclePositionAndDirection( spaceID, NULL_ENTITY_ID,
		localPos, localDir );
	entity.localVelocity( localVelocity );

	dwAvatarDropFilter.stop();
}


/**
 *	The normal of the world polygon intersected during the last output drop
 *	collide, otherwise (0,1,0).
 */
const Vector3 & AvatarDropFilter::groundNormal() const
{
	BW_GUARD;
	return groundNormal_;
}


/**
 *	This method will grab the history from another AvatarDropFilter if one is
 *	so provided
 */
bool AvatarDropFilter::tryCopyState( const MovementFilter & rOtherFilter )
{
	const AvatarDropFilter * pOtherAvatarDropFilter =
		dynamic_cast< const AvatarDropFilter * >( &rOtherFilter );

	if (pOtherAvatarDropFilter == NULL)
	{
		return this->AvatarFilter::tryCopyState( rOtherFilter );
	}

	alignToGround_ = pOtherAvatarDropFilter->alignToGround_;
	groundNormal_ = pOtherAvatarDropFilter->groundNormal_;

	MF_VERIFY( this->AvatarFilter::tryCopyState( rOtherFilter ) );

	return true;
}


namespace
{
/*
 * This function draw a plane given a center point on the plane
 * and two different vectors on the plane
 */
void drawPlane(const Vector3 & center, const Vector3 & normal, float size, Moo::Colour & colour, bool drawAlways)
{
	BW_GUARD;

	Vector3 v1 = Vector3(0,1,0).crossProduct( normal );
	v1.normalise();

	if ( v1 == Vector3(0,0,0) )
		v1 = Vector3(1,0,0);

	Vector3 v2 = normal.crossProduct( v1 );

	v1 = v1 * size;
	v2 = v2 * size;

	Geometrics::drawLine(center + v1 - v2, 
						 center + v1 + v2, colour, drawAlways);
	Geometrics::drawLine(center - v1 - v2, 
						 center - v1 + v2, colour, drawAlways);
	Geometrics::drawLine(center - v1 - v2, 
						 center + v1 - v2, colour, drawAlways);
	Geometrics::drawLine(center - v1 + v2, 
						 center + v1 + v2, colour, drawAlways);
	Geometrics::drawLine(center - v1 - v2, 
						 center + v1 + v2, colour, drawAlways);
	Geometrics::drawLine(center - v1 + v2, 
						 center + v1 - v2, colour, drawAlways);
}


}

/*
 * This function draws the models plane on the terrain
 */
void AvatarDropFilter::drawDebugStuff()
{
	if (!g_wpDebugFilter.exists() || !g_wpDebugFilterTarget.exists() ||
		g_wpDebugFilterTarget->isDestroyed() ||
		g_wpDebugFilter.get()->pAttachedFilter() == NULL ||
		g_wpDebugFilterTarget->pEntity()->pFilter() !=
			g_wpDebugFilter.get()->pAttachedFilter())
	{
		return;
	}

	Moo::Colour blue(0, 0, 1, 0);

	Position3D position = g_wpDebugFilterTarget->pEntity()->position();
	Vector3 groundNormal = g_wpDebugFilter->pAttachedFilter()->groundNormal();

	drawPlane(position, groundNormal, 1.0f, blue, true);
}


/*
 * BigWorld.debugDropFilter
 * This function accepts a PyAvatarDropFilter to debug visually
 */
static bool debugDropFilter( PyAvatarDropFilterPtr pPyFilter )
{
	// clear last camera state
	g_wpDebugFilterTarget = NULL;
	g_wpDebugFilter = NULL;

	if (pPyFilter == NULL)
	{
		return true;
	}

	g_wpDebugFilter = pPyFilter.get();

	if (!g_wpDebugFilter.exists())
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.debugDropFilter: "
			"failed to take weak reference to PyAvatarDropFilter\n" );
		return false;
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION(
	RETOK, debugDropFilter,
	OPTARG( PyAvatarDropFilterPtr, PyAvatarDropFilterPtr(), END ), BigWorld )


BW_END_NAMESPACE

// avatar_drop_filter.cpp
