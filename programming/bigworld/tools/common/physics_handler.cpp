
#include "pch.hpp"

#include "physics_handler.hpp"

#include "collision_advance.hpp"
#include "physics2/collision_obstacle.hpp"

DECLARE_DEBUG_COMPONENT2( "WPGen", 0 )


BW_BEGIN_NAMESPACE

template <class N> N sqr( const N n )
{
	return n * n;
}

class ClosestDropObstacle : public CollisionCallback
{
public:
	/**
	 *	the overridden operator()
	 */
	virtual int operator()( const CollisionObstacle& obstacle,
		const WorldTriangle& triangle, float /*dist*/ )
	{
		if (triangle.flags() & TRIANGLE_NOCOLLIDE)
		{
			return COLLIDE_ALL;
		}

		return COLLIDE_BEFORE;
	}
};

class ClosestDroppingObstacle : public CollisionCallback
{
	Vector3 refPoint_;
public:
	ClosestDroppingObstacle( const Vector3& refPoint )
		: refPoint_( refPoint )
	{
	}

	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float /*dist*/ )
	{
		BW_GUARD;

		const Matrix& obstacleTransform = obstacle.transform();
		PlaneEq peq( obstacleTransform.applyPoint( triangle.v0() ),
			obstacleTransform.applyVector( triangle.normal() ) );
		bool frontFacing = peq.isInFrontOf( refPoint_ );

		if (!frontFacing)
		{
			return COLLIDE_ALL;
		}

		if (triangle.flags() & TRIANGLE_NOCOLLIDE)
		{
			return COLLIDE_ALL;
		}

		return COLLIDE_BEFORE;
	}
};

PhysicsHandler::PhysicsHandler( ChunkSpace * pSpace, Girth gSpec ) :
		pChunkSpace_( pSpace )
{
	BW_GUARD;

	modelWidth_ = gSpec.width();
	modelHeight_ = gSpec.height();
	modelDepth_ = gSpec.depth();
	scrambleHeight_ = modelHeight_/3.0f;
	debug_ = false;
}

Vector3 PhysicsHandler::getGirth() const
{
	return Vector3( modelWidth_, modelHeight_, modelDepth_ );
}

float PhysicsHandler::getScrambleHeight() const
{
	return scrambleHeight_;
}

bool PhysicsHandler::findDropPoint( const Vector3& pos, float& y )
{
	BW_GUARD;

	Vector3 bottomlessPit( pos.x, -10000.0f, pos.z );

	ClosestDropObstacle co;
	float dist = pChunkSpace_->collide( pos, bottomlessPit, co );

	if ( dist >= 0.f )
	{
		y = pos.y - dist + 0.01f;
		if( dist > 1000000 )
		{
			return findDropPoint( Vector3( pos.x, y + 50, pos.z ), y );
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

bool PhysicsHandler::findDropSeedPoint( const Vector3& pos, float& y )
{
	BW_GUARD;

	Vector3 bottomlessPit( pos.x, -10000.0f, pos.z );

	ClosestDroppingObstacle co( pos );
	float dist = pChunkSpace_->collide( pos, bottomlessPit, co );

	if ( dist >= 0.f )
	{
		y = pos.y - dist + 0.01f;
		if( dist > 1000000 )
		{
			return findDropPoint( Vector3( pos.x, y + 50, pos.z ), y );
		}
	}
	else
	{
		return false;
	}
	
	return true;
}

/**
 *	This function prevents a collision between newPos and oldPos,
 *	adjusting newPos if necessary.
 *
 *	Copied from bigworld/src/client/app.cpp
 */
void PhysicsHandler::preventCollision( const Vector3 &oldPos, Vector3 &newPos,
	float hopMax, float modelWidth, float modelHeight, float modelDepth )
{
	BW_GUARD;

	ChunkSpace * pSpace = pChunkSpace_;
	if ( pSpace == NULL )
		return;

	Vector3 delta = newPos - oldPos;
	delta.y = 0.f;

	const float maxAdvance = delta.length();
	if ( maxAdvance < 0.0001f ) return;

	Vector3 flatDir = delta / maxAdvance;

	// Construct a rectangle that will be advanced. This rectangle corresponds
	// to the width and height of the model with the bottom (hop height) cut
	// off.
	const Vector3 widthVector( modelWidth * flatDir.z, 0.f, -modelWidth * flatDir.x );
	const Vector3 halfWidthVector = 0.5f * widthVector;
	const Vector3 heightVector( 0.f, modelHeight - hopMax, 0.f );
	Vector3 bottomLeft = oldPos - halfWidthVector;
	bottomLeft.y += hopMax;

	// Make sure that we are at least this far away before we run into an object.
	const float frontDistance = 0.5f * sqrtf( sqr(modelDepth) + sqr(modelWidth));

	// COLLISION_FUDGE is a bit of a fudge factor.
	const Vector3 frontBuffer = frontDistance * flatDir;

	CollisionAdvance collisionAdvance( bottomLeft,
		widthVector, heightVector, flatDir, maxAdvance + frontDistance );

	// For now, we are just sweeping the triangle that has one edge as the base
	// of the rectangle and the other vertex is at the top, centre.
	WorldTriangle sweepTriangle(
		bottomLeft,
		bottomLeft + heightVector + halfWidthVector,
		bottomLeft + widthVector );

	Vector3 dest = bottomLeft + delta + frontBuffer;

	pSpace->collide( sweepTriangle, dest, collisionAdvance );

	float clear = collisionAdvance.advance() - frontDistance;

	newPos = oldPos;

	if ( clear > 0.f )
	{
		// move as far as we can
		newPos += flatDir * clear;
	}

	if ( debug_ )
	{
		DEBUG_MSG( "preventCollision: maxAdvance %f, clear %f, flatDir(%f,%f,%f)\n",
			maxAdvance, clear, flatDir.x, flatDir.y, flatDir.z );
	}
}

void PhysicsHandler::applyGravity( Vector3 &newPos, float hopMax,
	float modelWidth, float modelDepth )
{
	BW_GUARD;

	ChunkSpace * pSpace = pChunkSpace_;
	if ( pSpace == NULL ) return;

	float dropDistance = 50.f;

	newPos.y += hopMax;

	// see where we'd land from here
	Vector3	landPos;

	// we only need to check the max distance we can fall
	float maxDrop = dropDistance;

	// TODO: Should pass in the flat direction.
	Vector3 flatDir( 0.f, 0.f, 1.f );

	Vector3 perpFlatDir( flatDir.z, 0.f, -flatDir.x );

	modelWidth = modelDepth;
	
	// Find the corners of the triangles to test.
	Vector3 corners[4];
	corners[0] = newPos - flatDir * modelDepth*0.5f - perpFlatDir * modelWidth*0.5f;
	corners[1] = newPos - flatDir * modelDepth*0.5f + perpFlatDir * modelWidth*0.5f;
	corners[2] = newPos + flatDir * modelDepth*0.5f - perpFlatDir * modelWidth*0.5f;
	corners[3] = newPos + flatDir * modelDepth*0.5f + perpFlatDir * modelWidth*0.5f;

	// make the object that checks them
	// origin, axis1, axis2, direction, maxDist.
	CollisionAdvance collisionAdvance( corners[0],
		corners[1] - corners[0], corners[2] - corners[0],
		Vector3( 0.f, -1.f, 0.f ), maxDrop );
	collisionAdvance.shouldFindCentre( true );

	// and test the two triangles
	WorldTriangle dropOne( corners[1], corners[2], corners[0] );
	WorldTriangle dropTwo( corners[1], corners[2], corners[3] );
	Vector3 dest( corners[1][0], corners[1][1] - maxDrop - 0.1f, corners[1][2] );

	pSpace->collide( dropOne, dest, collisionAdvance );
	pSpace->collide( dropTwo, dest, collisionAdvance );

	newPos.y -= collisionAdvance.advance();
	
	if ( collisionAdvance.advance() < maxDrop ) 
	{
		newPos.y += 0.01f;
	}
}



void PhysicsHandler::adjustMove( const Vector3& src, const Vector3& dst,
		Vector3& dst2 )
{
	BW_GUARD;

	Vector3 newPos = src;
	while ( Vector2(newPos.x-dst.x,newPos.z-dst.z).lengthSquared() > 0.001f )
	{
		Vector3 halfPos = newPos;
		newPos = dst;
		this->preventCollision( halfPos, newPos, scrambleHeight_,
			modelWidth_, modelHeight_, modelDepth_ );
		
		if ( debug_ )
		{
			DEBUG_MSG( "adjustMove: newPosA is (%f,%f,%f)\n",
				newPos.x, newPos.y, newPos.z );
		}

		// apply gravity to our base square. this is what the client does
		// for player characters.
		this->applyGravity( newPos, scrambleHeight_,
			modelWidth_, modelDepth_ );

		// save the height.. 
		float heightSave = newPos.y;

		// now apply gravity to a point - this is what the client does for
		// NPC's to save time.
			this->applyGravity( newPos, scrambleHeight_, 0.005f, 0.005f );
		
		// if there is too much difference then we are standing "over" the
		// edge of something (ie. one foot on a ledge). The client will drop
		// us right down. 
		if ( fabs( newPos.y - heightSave ) < scrambleHeight_ )
		{
			// stops possibility of walking "through" something.
			newPos.y = heightSave;
		}
		else
		{
			// keep the low height.
		}


		if ( debug_ )
		{
			DEBUG_MSG( "adjustMove: newPosB is (%f,%f,%f)\n",
				newPos.x, newPos.y, newPos.z );
		}

		if ( Vector2( newPos.x-halfPos.x, newPos.z-halfPos.z ).
			lengthSquared() < 0.001f ) break;
	}
	dst2 = newPos;
	float y;
	if( !findDropPoint( dst2, y ) )
	{// bottomless point, raise it to drop again
		if( !findDropPoint( Vector3( dst2.x, dst2.y + modelHeight_, dst2.z ), y ) )
		{// cannot find bottom, mark it as invalid
			dst2.y = -10000.f;
			return;
		}

		dst2.y = y;
		return;
	}
	findDropPoint( Vector3( dst.x, dst2.y + modelHeight_, dst.z ), y );
	if( y - dst2.y > DROP_FUDGE )
		dst2.y = -10000.f;// cannot find bottom, mark it as invalid
	else
		dst2.y = y;
}

BW_END_NAMESPACE
