#ifndef CLOSEST_TRIANGLE_HPP
#define CLOSEST_TRIANGLE_HPP

#include "chunk/chunk_obstacle.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class finds the closest triangle hit and records it
 */
class ClosestTriangle : public CollisionCallback
{
public:
	explicit ClosestTriangle( bool shouldUseTriangleFlags = true ) :
		CollisionCallback(),
		shouldUseTriangleFlags_( shouldUseTriangleFlags ),
		hasTriangle_( false )
	{
	}

	bool hasTriangle() const { return hasTriangle_; }

	const WorldTriangle & triangle() const { return tri_; }

protected:
	void triangle( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle )
	{
		hasTriangle_ = true;

		// Transform into world space
		tri_ = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ),
			(shouldUseTriangleFlags_) ? triangle.flags() : 0 );
	}

private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float /*dist*/ )
	{
		this->triangle( obstacle, triangle );
		return COLLIDE_BEFORE;
	}

	WorldTriangle tri_;
	bool shouldUseTriangleFlags_;
	bool hasTriangle_;
};

BW_END_NAMESPACE

#endif
