/**
 *	@file
 */

#ifndef WORLDTRIANGLE_HPP
#define WORLDTRIANGLE_HPP

#include "cstdmf/bw_vector.hpp"

#include "cstdmf/stdmf.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/planeeq.hpp"


BW_BEGIN_NAMESPACE

// Enable this to see debug lines for intersection tests.
//#define BW_WORLDTRIANGLE_DEBUG

#ifdef BW_WORLDTRIANGLE_DEBUG
BW_END_NAMESPACE
#include "math/matrix.hpp"
BW_BEGIN_NAMESPACE
#endif

enum
{
	// Nothing will ever collide with this triangle
	TRIANGLE_NOT_IN_BSP			= 0xff,

	TRIANGLE_CAMERANOCOLLIDE	= 1 << 0,
	TRIANGLE_TRANSPARENT		= 1 << 1,
	TRIANGLE_BLENDED			= 1 << 2,
	TRIANGLE_TERRAIN			= 1 << 3,

	// Player does not collide. Camera still does
	TRIANGLE_NOCOLLIDE			= 1 << 4,

	TRIANGLE_DOUBLESIDED		= 1 << 5,
	TRIANGLE_WATER				= 1 << 6,
	TRIANGLE_PROJECTILENOCOLLIDE = 1 << 7,

	TRIANGLE_COLLISIONFLAG_MASK = 0xff,
	TRIANGLE_MATERIALKIND_MASK	= 0xff << 8,
	TRIANGLE_MATERIALKIND_SHIFT	= 8		// how many bits to shift to get to the byte we want
};

/**
 *	This class is used to represent a triangle in the collision scene.
 */
class WorldTriangle
{
public:
	typedef uint16 Flags;
	typedef uint16 Padding;

	// construction

	WorldTriangle( const Vector3 & v0,
		const Vector3 & v1,
		const Vector3 & v2,
		Flags flags = 0 );
	WorldTriangle();

	// tests

	bool intersects( const WorldTriangle & triangle ) const;

	bool intersects( const Vector3 &	start,
					 const Vector3 &	dir,
					 float &			dist ) const;

	bool intersects( const WorldTriangle &	triangle,
					 const Vector3 &		offset ) const;

	// utility
	Vector2 project( const Vector3 & onTri ) const;

	const Vector3 & v0() const;
	const Vector3 & v1() const;
	const Vector3 & v2() const;
	const Vector3 & v( uint32 index ) const;

	Vector3 normal() const;

	void bounce(Vector3 & v,
		float elasticity = 1.f) const;

	// flags 

	static Flags packFlags( int collisionFlags, int materialKind );
	static int collisionFlags( Flags flags )
	{ return (flags & TRIANGLE_COLLISIONFLAG_MASK); }
	static int materialKind( Flags flags )
	{ return (flags & TRIANGLE_MATERIALKIND_MASK) >> TRIANGLE_MATERIALKIND_SHIFT; }
	Flags	flags() const;
	void	flags( Flags newFlags );
	bool	isTransparent() const;
	bool	isBlended() const;

	uint	collisionFlags() const	{ return collisionFlags( flags_ ); }
	uint	materialKind() const	{ return materialKind( flags_ ); }

	bool operator==(const WorldTriangle& rhs) const
	{
		return v_[0] == rhs.v_[0] && v_[1] == rhs.v_[1] && v_[2] == rhs.v_[2]
			&& flags_ == rhs.flags_ && padding_ == rhs.padding_;
	}

#ifdef BW_WORLDTRIANGLE_DEBUG
	static void	BeginDraw( const Matrix& transform, uint32 lifeTime = 0,
							uint32 hitFailureTime = 0 );
	static void	EndDraw();
private:
	// Enable to draw debug triangles in collision tests. If you do you must also
	// set transform to correct mapping from chunk to world space.
	static bool		s_debugTriangleDraw_;
	static Matrix	s_debugTriangleTransform_;
	// If the debug triangles are drawn every frame, then life = 0 (the default).
	// Otherwise set this to indicate how many frames to draw them.
	static uint32	s_debugTriangleLife_;
	// How long to draw the failure for a hit test
	static uint32	s_debugHitTestFailureLife_;
#endif

private:
	Vector3 v_[3];
	Flags	flags_;
	Padding padding_;
};

// Convenience macro to what we don't want to collide against
const WorldTriangle::Flags BW_TRIANGLE_NOCOLLIDE =
	(TRIANGLE_NOCOLLIDE | TRIANGLE_WATER);


/// This type stores a collection of triangles.
typedef BW::vector<const WorldTriangle *> WTriangleSet;
typedef BW::vector<WorldTriangle> RealWTriangleSet;

#ifdef CODE_INLINE
    #include "worldtri.ipp"
#endif

BW_END_NAMESPACE

#endif // WORLDTRIANGLE_HPP
