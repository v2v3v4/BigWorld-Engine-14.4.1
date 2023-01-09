#include "pch.hpp"

#include "bwclient_filter_environment.hpp"

#include "entity.hpp"
#include "entity_manager.hpp"

#include "space/space_manager.hpp"
#include "space/client_space.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_space.hpp"

#include "cstdmf/guard.hpp"

#include "terrain/base_terrain_block.hpp"

#include "moo/line_helper.hpp"

BW_BEGIN_NAMESPACE


namespace // (anonymous)
{

/**
 *	This is a collision callback used by filterDropPoint() to find the closest
 *	solid triangle.
 */
class SolidCollisionCallback : public CollisionCallback
{
public:
	SolidCollisionCallback() :
		closestTriangle_(),
		closestDistance_( -1.0f )
	{}

	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle,
		float distance );

	float distance() const
	{ return closestDistance_; }

	const WorldTriangle & triangle() const
	{ return closestTriangle_; }

private:
	WorldTriangle closestTriangle_;
	float closestDistance_;
};


int SolidCollisionCallback::operator()( const CollisionObstacle & obstacle,
	   const WorldTriangle & triangle,
	   float distance )
{
	BW_GUARD;

	// Look in both directions if triangle is not solid.

	if (triangle.flags() & TRIANGLE_NOCOLLIDE)
	{
		return COLLIDE_ALL;
	}

	if (closestDistance_ == -1.0f || distance < closestDistance_)
	{
		closestDistance_ = distance;
		closestTriangle_ = triangle;
	}

	return COLLIDE_BEFORE;
}


} // end namespace (anonymous)


/**
 *	Constructor.
 */
BWClientFilterEnvironment::BWClientFilterEnvironment(
			float maxFilterDropPointDistance ):
		maxFilterDropPointDistance_( maxFilterDropPointDistance )
{}


bool BWClientFilterEnvironment::filterDropPoint(
		SpaceID spaceID, const Position3D & startPosition,
		Position3D & landPosition, Vector3 * pGroundNormal )
{
	BW_GUARD;

	Position3D adjustedStartPosition = startPosition;
	adjustedStartPosition.y += 0.1f;

	SolidCollisionCallback scc;

	landPosition = adjustedStartPosition;

	// Add some fudge to avoid collision error.
	adjustedStartPosition.x += 0.005f;
	adjustedStartPosition.z += 0.005f;

	SmartPointer< ClientSpace > pSpace = SpaceManager::instance().space( spaceID );

	if (pSpace == NULL)
	{
		return false;
	}

	Position3D endPosition( adjustedStartPosition );
	endPosition.y -= maxFilterDropPointDistance_;

	pSpace->collide( adjustedStartPosition, endPosition, scc );

	if (scc.distance() < 0.f)
	{
		return false;
	}

	if (pGroundNormal != NULL)
	{
		*pGroundNormal = scc.triangle().normal();
		pGroundNormal->normalise();
	}

	landPosition.y -= scc.distance();

	return true;
}


bool BWClientFilterEnvironment::resolveOnGroundPosition( Position3D & position,
		bool & isOnGround )
{
	BW_GUARD;

	isOnGround = false;

	if (position.y < -12000.0f)
	{
		float terrainHeight =
			Terrain::BaseTerrainBlock::getHeight( position.x, position.z );

		if (terrainHeight == Terrain::BaseTerrainBlock::NO_TERRAIN)
		{
			return false;
		}

		position.y = terrainHeight;
		isOnGround = true;
	}
	return true;
}


void BWClientFilterEnvironment::transformIntoCommon( SpaceID spaceID,
		EntityID vehicleID, Position3D & position, Direction3D & direction )
{
	BW_GUARD;
	Entity * pVehicle = ConnectionControl::instance().findEntity( vehicleID );

	if (pVehicle != NULL)
	{
		pVehicle->transformVehicleToCommon( position, direction );
	}
}


void BWClientFilterEnvironment::transformFromCommon( SpaceID spaceID,
		EntityID vehicleID, Position3D & position, Direction3D & direction )
{
	BW_GUARD;

	Entity * pVehicle = ConnectionControl::instance().findEntity( vehicleID );

	if (pVehicle != NULL)
	{
		pVehicle->transformCommonToVehicle( position, direction );
	}
}


/**
 * This function allows interactive testing of terrain collision.  It will
 * draw a red line if the test "missed" terrain, or a white line if collision
 * was successful.
 */
static void adfTest( SpaceID spaceID, const Vector4ProviderPtr pos )
{
	BW_GUARD;
	// Generate start and end points
	Vector4 vec;
	pos->output( vec );

	Vector3 start = Vector3( vec.x, vec.y, vec.z );
	Vector3 end = start;
	end.y -= 100.0f;

	// Draw
	LineHelper::instance().drawLine(start, end, 0xFFFFFFFF, 300 );

	// Collide
	ClientSpacePtr space = SpaceManager::instance().space( spaceID );
	SolidCollisionCallback scc;

	space->collide( start, end,  scc );
	if ( scc.distance() < 0.0f )
	{
		// draw miss state
		LineHelper::instance().drawLine(start, end, 0xFFFF0000, 300 );
	}
}

PY_AUTO_MODULE_FUNCTION( RETVOID, adfTest, ARG( uint32, ARG( Vector4ProviderPtr, END ) ), BigWorld )

BW_END_NAMESPACE

// bwclient_filter_environment.cpp
