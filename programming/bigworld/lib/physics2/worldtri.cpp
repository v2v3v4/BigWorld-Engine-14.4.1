/**
 *	@file
 */

#include "pch.hpp"

#include "worldtri.hpp"
#include "worldpoly.hpp"

#include "math/vector2.hpp"
#include "cstdmf/debug.hpp"

#ifndef CODE_INLINE
BW_BEGIN_NAMESPACE
    #include "worldtri.ipp"
BW_END_NAMESPACE
#endif

#ifdef BW_WORLDTRIANGLE_DEBUG

#include "moo/line_helper.hpp"

BW_BEGIN_NAMESPACE

bool	WorldTriangle::s_debugTriangleDraw_			= false;
Matrix	WorldTriangle::s_debugTriangleTransform_	= Matrix::identity;
uint32	WorldTriangle::s_debugTriangleLife_			= 0;
uint32	WorldTriangle::s_debugHitTestFailureLife_	= 0;

BW_END_NAMESPACE

// Call this at top of function to set up a transformed triangle. Later macros
// will need this otherwise _dv won't be defined.
#define BW_WT_DEBUG_INIT()\
	Vector3 _dv[3];												\
	if ( s_debugTriangleDraw_ )									\
	{															\
		_dv[0] = s_debugTriangleTransform_.applyPoint( v_[0] );	\
		_dv[1] = s_debugTriangleTransform_.applyPoint( v_[1] );	\
		_dv[2] = s_debugTriangleTransform_.applyPoint( v_[2] );	\
	}

// Draw this triangle with a colour
#define BW_WT_DEBUG_DRAW( colour )\
	if ( s_debugTriangleDraw_ )											\
	{																	\
		LineHelper::instance().drawTriangle(							\
			_dv[0], _dv[1], _dv[2], (colour), s_debugTriangleLife_ );	\
	}

// Draw the test prism with a colour
#define BW_WT_DEBUG_DRAW_PRISM( triangle, offset, colour )\
	if ( s_debugTriangleDraw_ )													\
	{																			\
		Vector3 ttA[3], ttB[3];													\
																				\
		/* build and draw sides */												\
		for ( uint32 i = 0; i < 3; i++ )										\
		{																		\
			ttA[i] = s_debugTriangleTransform_.applyPoint( triangle.v_[i] );	\
			ttB[i] = ttA[i] + (offset);											\
			LineHelper::instance().drawLine( ttA[i], ttB[i], (colour) );		\
		}																		\
																				\
		/* draw ends */															\
		LineHelper::instance().drawTriangle( ttA[0], ttA[1], ttA[2], (colour) );\
		LineHelper::instance().drawTriangle( ttB[0], ttB[1], ttB[2], (colour) );\
	}

// Draw the test line with a colour
#define BW_WT_DEBUG_DRAW_LINE( start, end, colour )				\
	if ( s_debugTriangleDraw_ )									\
	{															\
		Vector3 a = s_debugTriangleTransform_.applyPoint( start );\
		Vector3 b = s_debugTriangleTransform_.applyPoint( end );\
		LineHelper::instance().drawLine( a, b, (colour), s_debugHitTestFailureLife_  );			\
	}

#else

// Stub versions of above functions
#define BW_WT_DEBUG_INIT()
#define BW_WT_DEBUG_DRAW( colour )
#define BW_WT_DEBUG_DRAW_PRISM( triangle, offset, colour )
#define BW_WT_DEBUG_DRAW_LINE( start, end, colour )

#endif

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: WorldTriangle
// -----------------------------------------------------------------------------
/**
 *	This method packs collision flags + materialKind into a Flags type.
 */
WorldTriangle::Flags WorldTriangle::packFlags( int collisionFlags, int materialKind )
{
	return collisionFlags |
		(
			(materialKind << TRIANGLE_MATERIALKIND_SHIFT) & 
			TRIANGLE_MATERIALKIND_MASK
		);
}

/**
 *	This method returns whether the input triangle intersects this triangle.
 */
bool WorldTriangle::intersects(const WorldTriangle & triangle) const
{
	// How it works:
	//	Two (non-coplanar) triangles can only intersect in 3D space along an
	//	interval that lies on the line that is the intersection of the two
	//	planes that the triangles lie on. What we do is find the interval of
	//	intersecton each triangle with this line. If these intervals overlap,
	//	the triangles intersect. We only need to test if they overlap in one
	//	coordinate.

	const WorldTriangle & triA = *this;
	const WorldTriangle & triB = triangle;

	PlaneEq planeEqA( triA.v0(), triA.v1(), triA.v2(),
		PlaneEq::SHOULD_NOT_NORMALISE );

	float dB0 = planeEqA.distanceTo(triB.v0());
	float dB1 = planeEqA.distanceTo(triB.v1());
	float dB2 = planeEqA.distanceTo(triB.v2());

	float dB0dB1 = dB0 * dB1;
	float dB0dB2 = dB0 * dB2;

	// Is triangle B on one side of the plane equation A.
	if (dB0dB1 > 0.f && dB0dB2 > 0.f)
		return false;

	PlaneEq planeEqB( triB.v0(), triB.v1(), triB.v2(),
		PlaneEq::SHOULD_NOT_NORMALISE );

	float dA0 = planeEqB.distanceTo(triA.v0());
	float dA1 = planeEqB.distanceTo(triA.v1());
	float dA2 = planeEqB.distanceTo(triA.v2());

	float dA0dA1 = dA0 * dA1;
	float dA0dA2 = dA0 * dA2;

	// Is triangle A on one side of the plane equation B.
	if (dA0dA1 > 0.f && dA0dA2 > 0.f)
		return false;

	Vector3 D(planeEqA.normal().crossProduct(planeEqB.normal()));

	float max = fabsf(D.x);
	int index = X_AXIS;

	float temp = fabsf(D.y);

	if (temp > max)
	{
		max = temp;
		index = Y_AXIS;
	}

	temp = fabsf(D.z);

	if (temp > max)
	{
		index = Z_AXIS;
	}

	Vector3 projA(
		triA.v0()[index],
		triA.v1()[index],
		triA.v2()[index]);

	Vector3 projB(
		triB.v0()[index],
		triB.v1()[index],
		triB.v2()[index]);

	float isect1[2], isect2[2];

	/* compute interval for triangle A */
	COMPUTE_INTERVALS(projA.x, projA.y, projA.z,
		dA0, dA1,dA2,
		dA0dA1, dA0dA2,
		isect1[0], isect1[1]);

	/* compute interval for triangle B */
	COMPUTE_INTERVALS(projB.x, projB.y, projB.z,
		dB0, dB1,dB2,
		dB0dB1, dB0dB2,
		isect2[0], isect2[1]);

	sort(isect1[0],isect1[1]);
	sort(isect2[0],isect2[1]);

	return(isect1[1] >= isect2[0] &&
		isect2[1] >= isect1[0]);
}

/// This constant stores that value that if two floats are within this value,
///	they are considered equal.
const float EPSILON = 0.000001f;

/**
 *	This method tests whether the input interval intersects this triangle.
 *	The interval is from start to (start + length * dir). If it intersects, the
 *	input float is set to the value such that (start + length * dir) is the
 *	intersection point.
 */
bool WorldTriangle::intersects(const Vector3 & start,
								const Vector3 & dir,
								float & length) const
{
	BW_WT_DEBUG_INIT();
	BW_WT_DEBUG_DRAW( 0xFFFFFFFF );

	// Find the vectors for the edges sharing v0.
	const Vector3 edge1(v1() - v0());
	const Vector3 edge2(v2() - v0());

	// begin calculating determinant - also used to calculate U parameter
	const Vector3 p(dir.crossProduct(edge2));

	// if determinant is near zero, ray lies in plane of triangle
	float det = edge1.dotProduct(p);

	if ( almostZero( det, EPSILON ) )
	{
		//BW_WT_DEBUG_DRAW_LINE( start, start + dir, 0xFFFF0000)
		return false;
	}

	float inv_det = 1.f / det;

	// calculate distance from vert0 to ray origin
	const Vector3 t(start - v0());

	// calculate U parameter and test bounds
	float u = t.dotProduct(p) * inv_det;

	if (u < 0.f || 1.f < u)
	{
		//BW_WT_DEBUG_DRAW_LINE( start, start + dir, 0xFFFF0000)
		return false;
	}

	// prepare to test V parameter
	const Vector3 q(t.crossProduct(edge1));

	// calculate V parameter and test bounds
	float v = dir.dotProduct(q) * inv_det;

	if (v < 0.f || 1.f < u + v)
	{
		//BW_WT_DEBUG_DRAW_LINE( start, start + dir, 0xFFFF0000)
		return false;
	}

	// calculate intersection point = start + dist * dir
	float distanceToPlane = edge2.dotProduct(q) * inv_det;

	if (0.f < distanceToPlane && distanceToPlane < length)
	{
		length = distanceToPlane;
		return true;
	}
	else
	{
		BW_WT_DEBUG_DRAW_LINE( start, start + dir, 0xFFFF0000);
		return false;
	}
}

/**
 *	Helper function to determine which side a given vector is
 *	of another vector. Returns true if the second vector is in
 *	the half-plane on the anticlockwise side of the first vector.
 *	Colinear vectors are considered to be on that side (returns true)
 */
inline bool inside( const Vector2 & va, const Vector2 & vb )
{
	return (va.x * vb.y - va.y * vb.x) >= 0;
}


/**
 *	This method returns whether a triangular prism intersects this triangle.
 *	The prism is described by a triangle and an offset.
 *
 *	@param	triangle	The triangle to test against, assumed to be in the same
 *						space as this triangle.
 *	@param	offset		The offset of end triangle in volume.
 */
bool WorldTriangle::intersects( const WorldTriangle &	triangle,
								const Vector3 &			offset ) const
{
	// Initialise debug triangle - this compiles out if BW_WORLDTRIANGLE_DEBUG
	// is not defined. Triangle rejections are colour coded below for easier
	// debugging.
	BW_WT_DEBUG_INIT()

	// Draw the input volume ( light gray )
	BW_WT_DEBUG_DRAW_PRISM( triangle, offset, 0xFFAAAAAA );

	// first find the distance along 'translation' of the points of
	// 'triangle' from our plane eq.
	PlaneEq planeEq( v_[0], v_[1], v_[2], PlaneEq::SHOULD_NOT_NORMALISE );
	float unitDotProduct = planeEq.normal().unitVector().dotProduct( offset.unitVector() );
	float ndt = planeEq.normal().length() * offset.length() * unitDotProduct;

	float vd0 = 0, vd1 = 0, vd2 = 0;

	bool bruteCheck = false;
	// If the offset is parallel to the world triangles plane we do a brute 
	// force check against the volume created by the triangle prism, this is 
	// done because the efficient version of the collision fails in this case
	if (almostZero( unitDotProduct, 0.005f ))
	{
		bruteCheck = true;
	}
	else
	{
		float indt = 1.f / ndt;
		vd0 = planeEq.intersectRayHalfNoCheck( triangle.v_[0], indt );
		vd1 = planeEq.intersectRayHalfNoCheck( triangle.v_[1], indt );
		vd2 = planeEq.intersectRayHalfNoCheck( triangle.v_[2], indt );
		// if they are all too near or too close, then we consider the
		// volume to not cross the plane.
		if (vd0 <  0.f && vd1 <  0.f && vd2 <  0.f) 
		{
			// debug - ORANGE
			BW_WT_DEBUG_DRAW( 0xFFFF6600 );
			return false;
		}
		if (vd0 >= 1.f && vd1 >= 1.f && vd2 >= 1.f) 
		{
			// debug - YELLOW
			BW_WT_DEBUG_DRAW( 0xFFFFFF33 );
			return false;
		}
		
		//If either end of the volume intersects planeEq we do a brute force 
		// check against the volume, this is 
		// done because the efficient version of the collision fails in 
		// this case
		if (vd0 < 0.f || vd1 < 0.f || vd2 < 0.f ||
			vd0 >= 1.f || vd1 >= 1.f || vd2 >= 1.f)
		{
			bruteCheck = true;
		}
	}
	
	// The brute force version of the check clips the triangle against the 
	// planes that make up the triangle prism
	if ( bruteCheck )
	{ 
		// Get the start of the prism and make sure its plane is pointing in the
		// right direction
		WorldTriangle wt1;
		PlaneEq pe1( triangle.v0(), triangle.v1(), triangle.v2(), PlaneEq::SHOULD_NOT_NORMALISE );

		if (pe1.normal().dotProduct( offset ) < 0.f)
		{
			pe1 = PlaneEq( -pe1.normal(), -pe1.d() );
			wt1 = WorldTriangle( triangle.v2(), triangle.v1(), triangle.v0() );
		}
		else
		{
			wt1 = triangle;
		}

		WorldPolygon wp(8);
		wp.resize(3);

		// Initialise with our triangles
		wp[0] = v_[0];
		wp[1] = v_[1];
		wp[2] = v_[2];

		// Check the first plane
		if (!wp.chop(pe1))
		{
			// debug - RED
			BW_WT_DEBUG_DRAW( 0xFFFF0000 );
			return false;
		}

		// Create the second cap plane
		Vector3 invNormal(-pe1.normal().x, -pe1.normal().y, -pe1.normal().z);

		PlaneEq pe2( invNormal, invNormal.dotProduct( wt1.v0() + offset ) );

		// Check against second cap plane
		if (!wp.chop(pe2))
		{
			// debug - RED
			BW_WT_DEBUG_DRAW( 0xFFFF0000 );
			return false;
		}

		// Create the triangle at the other end of the triangle prism
		WorldTriangle wt2( wt1.v0() + offset, 
			wt1.v1() + offset, wt1.v2() + offset );

		// Check against the sides of the triangle prism
		PlaneEq plane;
		for (uint32 i = 0; i < 3; i++)
		{
			plane.init( wt1.v(i), wt2.v(i), wt1.v((i + 1) %3),
				PlaneEq::SHOULD_NOT_NORMALISE );
			if (!wp.chop(plane))
			{
				// debug - RED
				BW_WT_DEBUG_DRAW( 0xFFFF0000 );
				return false;
			}
		}

		// We intersect the triangle prism
		// debug - PINK
		BW_WT_DEBUG_DRAW( 0xFFFF00FF );
		return true;
	}
	else //a more efficient version of checking
	{
		
		// figure out a good cartesian plane to project everything onto
		const Vector3 & norm = planeEq.normal();
		float max = fabsf(norm.x);
		int indexU = Y_AXIS;
		int indexV = Z_AXIS;

		float temp = fabsf(norm.y);
		if (temp > max)   { max = temp; indexU = X_AXIS; indexV = Z_AXIS; }
		if (fabsf(norm.z) > max) { indexU = X_AXIS; indexV = Y_AXIS; }

		// now project both triangles onto it
		Vector3 triB3D[3] = {
			triangle.v_[0] + offset * vd0,
			triangle.v_[1] + offset * vd1,
			triangle.v_[2] + offset * vd2 };
		Vector2	triA[3] = {
			Vector2( v_[0][indexU], v_[0][indexV] ),
			Vector2( v_[1][indexU], v_[1][indexV] ),
			Vector2( v_[2][indexU], v_[2][indexV] ) };
		Vector2 triB[3] = {
			Vector2( triB3D[0][indexU], triB3D[0][indexV] ),
			Vector2( triB3D[1][indexU], triB3D[1][indexV] ),
			Vector2( triB3D[2][indexU], triB3D[2][indexV] ) };


		// now intersect the two triangles (yay!)
		// we make sure at least one point of each triangle is on the inside
		//  side of each segment of the other triangle
		bool signA = inside( triA[1] - triA[0], triA[2] - triA[0] );
		bool signB = inside( triB[1] - triB[0], triB[2] - triB[0] );
		Vector2 segmentA, segmentB;

		for (int i1 = 0, i2 = 2; i1 <= 2; i2 = i1, i1++)
		{
			segmentA = triA[i1] - triA[i2];
			if (inside( segmentA, triB[0] - triA[i2] ) != signA &&
				inside( segmentA, triB[1] - triA[i2] ) != signA &&
				inside( segmentA, triB[2] - triA[i2] ) != signA) 
			{
				// debug - GREEN
				BW_WT_DEBUG_DRAW( 0xFF00FF00 );		
				return false;
			}

			segmentB = triB[i1] - triB[i2];
			if (inside( segmentB, triA[0] - triB[i2] ) != signB &&
				inside( segmentB, triA[1] - triB[i2] ) != signB &&
				inside( segmentB, triA[2] - triB[i2] ) != signB)
			{
				// debug - BLUE
				BW_WT_DEBUG_DRAW( 0xFF0000FF );		
				return false;
			}
		}

		// so they both have at least one point on the inside side of every
		// segment of the other triangle. this means they intersect
	}

	// debug - WHITE
	BW_WT_DEBUG_DRAW( 0xFFFFFFFF );
	return true;
}


/**
 *	This method <i>bounces</i> the input vector off a plane parallel to this
 *	triangle.
 *
 *	@param v			The vector to be bounced. This is passed by reference
 *						and obtains the result.
 *	@param elasticity	The elasticity of the bounce. 0 indicates that the
 *						resulting vector has no component in the direction of
 *						the plane's normal. 1 indicates that the the component
 *						in the direction of the plane's normal is reversed.
 */
void WorldTriangle::bounce( Vector3 & v,
							float elasticity ) const
{
	Vector3 normal = this->normal();
	normal.normalise();

	float proj = normal.dotProduct(v);
	v -= (1 + elasticity) * proj * normal;
}



/**
 *	This method project the given 3D point to the basis defined by
 *	the sides of this triangle. (0,0) is at v0, (1,0) is at v1,
 *	(0,1) is at v2, etc.
 *
 *	Currently it only uses a vertical projection.
 */
Vector2 WorldTriangle::project( const Vector3 & onTri ) const
{
	// always use a vertical projection
	Vector2 vs( v_[1][0]	- v_[0][0], v_[1][2]	- v_[0][2] );
	Vector2 vt( v_[2][0]	- v_[0][0], v_[2][2]	- v_[0][2] );
	Vector2 vp( onTri[0]	- v_[0][0], onTri[2]	- v_[0][2] );

	// do that funky linear interpolation
	float sXt = vs[0]*vt[1] - vs[1]*vt[0];
	float ls = (vp[0]*vt[1] - vp[1]*vt[0])/sXt;
	float lt = (vp[1]*vs[0] - vp[0]*vs[1])/sXt;

	return Vector2( ls, lt );
}
// worldtri.cpp

#ifdef BW_WORLDTRIANGLE_DEBUG

void WorldTriangle::BeginDraw(	const Matrix&	transform, 
								uint32			lifeTime /* = 0  */,
								uint32			hitFailureLife /* = 0 */ )
{
	s_debugTriangleDraw_		= true;
	s_debugTriangleTransform_	= transform;
	s_debugTriangleLife_		= lifeTime;
	s_debugHitTestFailureLife_	= hitFailureLife;
}

void WorldTriangle::EndDraw()
{
	s_debugTriangleDraw_		= false;
}

#endif

BW_END_NAMESPACE

// worldtri.cpp
