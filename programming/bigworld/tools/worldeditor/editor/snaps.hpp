#ifndef SNAPS_HPP
#define SNAPS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/world_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Snap
{
	float value( float& v, float snapSize );
	Vector3& vector3( Vector3& v, const Vector3& snaps, const Vector3& origin = Vector3(0,0,0) );
	Vector3 vector3( const Vector3& v, const Vector3& snaps, const Vector3& origin = Vector3(0,0,0) );
	Vector3& angle( Vector3& vector, float snapAngle );

	// snap an arbitrary ray to the ground
	Vector3 toGround( const Vector3& startPosition, const Vector3& direction, 
						float distance = WorldManager::instance().getMaxFarPlane(),
						bool ignoreHoles = false );

	// snap a point's Y value to the ground
	Vector3 toGround( const Vector3& v );

	// snap an arbitrary ray to the closest obstacle
	Vector3 toObstacle( const Vector3& startPosition, const Vector3& direction, 
						bool snapToBoundingBox = false,
						float distance = WorldManager::instance().getMaxFarPlane(),
                        bool *snapped = NULL );

	Vector3 toObstacleNormal( const Vector3& startPosition, const Vector3& direction,
		float distance = WorldManager::instance().getMaxFarPlane());

	Matrix& angles( Matrix& v, float snapSize );

	// calculate a value that will satisfy both snap values a and b
	float satisfy( float a, float b );
};

BW_END_NAMESPACE

#endif // SNAPS_HPP
