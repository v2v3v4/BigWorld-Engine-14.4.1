#include "pch.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/collisions/collision_callbacks.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"

BW_BEGIN_NAMESPACE

extern bool s_collideTerrainHoles;


namespace Snap
{


/**
 * snaps a float value to the nearest allowable position
 *
 * @param v the float value to be snapped
 * @param snaps the snap grid to be snapped with
 *
 * @return float a copy of the newly snapped float
 */
float value( float& v, float snapSize )
{
	if ( snapSize > 0.f )
	{
		float halfSnap = snapSize / 2.f;

		if (v > 0.f)
		{
			v += halfSnap;

			v -= ( fmodf( v, snapSize ) );
		}
		else
		{
			v -= halfSnap;

			v += ( fmodf( -v, snapSize ) ); 
		}
	}

    return v;
}


/**
 * snaps a vector3 to the nearest allowable position,
 * changing the input vector
 *
 * @param v the Vector to be snapped and changed.
 * @param snaps the Vector of snaps to be snapped with
 *
 * @return Vector3	a reference to the newly snapped vector,
 *					so that this method can be used in an
 *					expression.
 */
Vector3& vector3( Vector3& v, const Vector3& snaps, const Vector3& origin )
{
	v += origin;
	value( v.x, snaps.x );
	value( v.y, snaps.y );
	value( v.z, snaps.z );
	v -= origin;

	return v;
}


/**
 * snaps a const vector3 to the nearest allowable position
 *
 * @param v the Vector to be snapped
 * @param snaps the Vector of snaps to be snapped with
 *
 * @return Vector3	a copy of the newly snapped vector
 */
Vector3 vector3( const Vector3& v, const Vector3& snaps, const Vector3& origin )
{
	static Vector3 workingSpace;
	workingSpace = v;

	return vector3( workingSpace, snaps, origin );
}


/**
 *	snaps a vector3's y value to the terrain, if the vector
 *	is currently located in an outside ( terrain ) chunk.
 */
Vector3 toGround( const Vector3& v )
{
	BW_GUARD;

	Vector3 pos( v );
	pos.y = MAX_CHUNK_HEIGHT;

	Vector3 groundPos = toGround(pos, Vector3(0.f, -1.f, 0.f), MAX_CHUNK_HEIGHT - MIN_CHUNK_HEIGHT );
	if (groundPos != pos)
		return groundPos;

	return v;
}


Vector3 toGround( const Vector3& startPosition, const Vector3& direction, 
				 float distance, bool ignoreHoles)
{
	BW_GUARD;

	ClosestTerrainObstacle terrainCallback;

	Vector3 extent = startPosition + ( direction * distance );

	s_collideTerrainHoles = ignoreHoles;
	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if ( space )
	{
		space->collide(	startPosition,
						extent,
						terrainCallback );
	}
	s_collideTerrainHoles = false;

	if (terrainCallback.collided())
	{
		// return the ground position
		return startPosition +
				( direction * terrainCallback.dist() );
	}

	// ground not found, return the start position
	return startPosition;
}


Vector3 toObstacle( const Vector3& startPosition, const Vector3& direction, 
				   bool snapToBoundingBox, float distance, bool *snapped)
{
	BW_GUARD;

	Vector3 extent = startPosition + ( direction * distance );

	ObstacleLockCollisionCallback* pCallback = &ObstacleLockCollisionCallback::s_default;
	pCallback->dist_ = 1e23f;

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	distance = space->collide(	startPosition,
								extent,
								*pCallback );

	if ( pCallback->dist_ > 0.f && pCallback->dist_ < 1e23f )
	{
        if (snapped != NULL)
            *snapped = true;

		Vector3	pos = startPosition + ( direction * pCallback->dist_ );

		// modify the position and normal if want to select on bounding box
		if (snapToBoundingBox && pCallback->pItem_->pOwnSect())
		{
			// use bounding box
			BoundingBox bb;
			pCallback->pItem_->edBounds(bb);

			// find apporpriate side of box
			// and determine the new normal
			Vector3 axisX(1.f, 0.f, 0.f);
			Vector3 axisY(0.f, 1.f, 0.f);
			Vector3 axisZ(0.f, 0.f, 1.f);
			Vector3 triangleNormal = pCallback->triangleNormal_;

			float dotX = triangleNormal.dotProduct(axisX);
			float dotY = triangleNormal.dotProduct(axisY);
			float dotZ = triangleNormal.dotProduct(axisZ);

			Vector3 boxNormal;
			float temp = 0.f;

			if (fabs(dotX) > fabs(dotY))
			{
				temp = dotX;
				boxNormal = axisX;
				if (dotX < 0.f)
					boxNormal *= -1.f;
			}
			else
			{
				temp = dotY;
				boxNormal = axisY;
				if (dotY < 0.f)
					boxNormal *= -1.f;
			}
	
			if (fabs(dotZ) > fabs(temp))
			{
				boxNormal = axisZ;
				if (dotZ < 0.f)
					boxNormal *= -1.f;
			}

			// shift the selected point to the surface of the bounding box
			Matrix trans = pCallback->pItem_->edTransform();
			trans.postMultiply(pCallback->pItem_->chunk()->transform());
			
			Matrix invTrans = trans;
			invTrans.invert();
			Vector3 localPos = invTrans.applyPoint(pos);
			float dist = boxNormal.dotProduct(bb.minBounds() - localPos);
			if (dist < 0.f)
			{
				dist = boxNormal.dotProduct(bb.maxBounds() - localPos);
			}
			
			pCallback->normal_ = trans.applyVector( boxNormal );;
			pos += dist * pCallback->normal_;
		}

		return pos;
	}
    else
    {
        if (snapped != NULL)
            *snapped = false;
    }

	return startPosition;
}

Vector3 toObstacleNormal( const Vector3& startPosition, const Vector3& direction, 
	float distance)
{
	BW_GUARD;

	Vector3 extent = startPosition + ( direction * distance );

	ObstacleLockCollisionCallback* pCallback = &ObstacleLockCollisionCallback::s_default;
	pCallback->dist_ = 1e23f;

	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	distance = space->collide(	startPosition,
								extent,
								*pCallback );

	if ( pCallback->dist_ > 0.f && pCallback->dist_ < 1e23f )
		return pCallback->normal_;

	return Vector3( 0, 1, 0 );
}


/**
 * snaps a vector to the nearest angle that is a multiple of snapAngle
 *
 * @param vector the Vector to be modified
 */
Vector3& angle( Vector3& vector, float snapAngle )
{
	BW_GUARD;

    float pitch;
	float yaw;

    bool flipYaw;

    if (vector.x != 0.f)
    {
        yaw = atanf( vector.z / vector.x );
        flipYaw = ( vector.x < 0 );
    }
    else
    {
        yaw = MATH_PI / 2;
        flipYaw = ( vector.z < 0 );
    }

    pitch = acosf( Math::clamp(-1.0f, vector.y, +1.0f) );

    //snap the angles
    yaw	    = value( yaw, DEG_TO_RAD( snapAngle ) );
    //snap the angle
    pitch	= value( pitch, DEG_TO_RAD( snapAngle ) );

    //mirror octants
   	if ( flipYaw )
    	yaw = yaw - MATH_PI;

    //recalc the orientation
    float cosYaw = cosf( yaw );
    float sinYaw = sinf( yaw );
    float cosPitch = cosf( pitch );
    float sinPitch = sinf( pitch );

    vector.set( cosYaw * sinPitch, cosPitch, sinYaw * sinPitch );

    vector.normalise();

	return vector;
}


/**
 * snaps the vectors in a matrix to the nearest angle
 * that is a multiple of snapAngle.
 *
 * @param m the Matrix whose orientatio vectors should be snapped.
 * @param snapSize the snapAngle
 *
 * @return Matrix& reference to the snapped matrix, returned so
 *				that this method can be used in an expression.
 */
Matrix& angles( Matrix& m, float snapSize )
{
	if ( snapSize > 0.f )
	{
		angle( *(Vector3*)&m.m[0], snapSize );
		angle( *(Vector3*)&m.m[1], snapSize );
		angle( *(Vector3*)&m.m[2], snapSize );
	}

	return m;
}

/**
 * Returns an approximation of the Least Common Multiple of a and b.
 *
 * If one is 0.f, the other is returned. If both are 0.f, 0.f is returned.
 */
float satisfy( float a, float b )
{
	if (a > 0.f)
	{
		if (b > 0.f)
		{
			// We convert to integers because this can't be done right with an
			// arbitary amount of decimal places, we allow units as small as
			// 0.001, which is enough granularity for user snaps.

			int x = std::max<int>( (int) (a * 100.f), (int) (b * 100.f) );
			int y = std::min<int>( (int) (a * 100.f), (int) (b * 100.f) );

			if ( y == 0 || x % y == 0)
			{
				return x / 100.f;
			}
			else
			{
				// It'd be nice if we worked out the Least Common Multiple
				// of x and y here, rather than using this approximation
				int t = x * y;
				while ((t / 10) % x == 0 && (t / 10) % y == 0)
					t /= 10;

				return t / 100.f;
			}
		}
		else
		{
			return a;
		}
	}

	return b;
}

}	//end namespace Snaps

BW_END_NAMESPACE

