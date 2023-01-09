#include "pch.hpp"
#include "solid_shape_mesh.hpp"

#include "moo/render_context.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"

BW_BEGIN_NAMESPACE

/**
 * Constructor for spherepicker object.
 * @param position position of the sphere
 * @param radius radius of the sphere
 * @param transform transform of the sphere
 */
SpherePicker::SpherePicker( const Vector3& position, float radius, const Matrix& transform )
: position_( position ),
  radius_( radius ),
  radiusSquared_( radius * radius ),
  transform_( transform )
{
	BW_GUARD;

	invTransform_.invert( transform_ );
}

/**
 * This method intersects a ray with the sphere. It only returns true if the
 * ray intersects the outside of the sphere in the positive direction of the ray.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param intersection pointer to a vector to return the intersection point in
 * @return whether the ray intersected with the sphere or not
 */
bool SpherePicker::intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection ) const
{
	BW_GUARD;

	bool ret = false;
	// compute local origin and local direction, normalise the direction
	Vector3 lo = invTransform_.applyPoint( origin );
	Vector3 ld = invTransform_.applyVector( direction );
	ld.normalise();

	// find the distance from lo along the ray ld that the position of the sphere is
	// 90degrees on the line
	float t = (position_ - lo).dotProduct( ld );
	if (t > 0)
	{
		Vector3 crossPos = lo + (ld * t);
		float lengthSq = (crossPos - position_).lengthSquared();
		if (lengthSq <= radiusSquared_)
		{
			float distanceFromEdge = lengthSq < radiusSquared_ ? sqrtf( radiusSquared_ - lengthSq ) : 0;
			float distanceFromOrigin = t - distanceFromEdge;
			if (distanceFromOrigin > 0)
			{
				ret = true;
				if (intersection)
				{
					*intersection = transform_.applyPoint( lo + (ld * distanceFromOrigin) );
				}
			}
		}
	}

	return ret;
}

/**
 * Constructor for BoxPicker object.
 * @param minValues the minimum (lower front left) coordinate of the box
 * @param maxValues the maximum (top back right) coordinate of the box
 * @param transform transform of the box
 */
BoxPicker::BoxPicker( const Vector3& minValues, const Vector3& maxValues, const Matrix& transform )
: min_( minValues ),
  max_( maxValues ),
  transform_( transform )
{
	BW_GUARD;

	invTransform_.invert( transform );
}

/**
 * This method intersects a ray with the box. It only returns true if the
 * ray intersects the outside of the box in the positive direction of the ray.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param intersection pointer to a vector to return the intersection point in
 * @return whether the ray intersected with the box or not
 */
bool BoxPicker::intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection ) const
{
	BW_GUARD;

	// compute local origin and local direction
	Vector3 lo = invTransform_.applyPoint( origin );
	Vector3 ld = invTransform_.applyVector( direction );

	Vector3 bounds[2] = {min_, max_};
	Vector3 ptOnPlane;

	bool ret = false;
	float st = 0;

	for (int bound = 0; bound < 2; bound++)
	{
		for (int axis = X_AXIS; axis <= Z_AXIS; axis++)
		{
			if (ld[axis] != 0.f)
			{
				float t = (bounds[bound][axis] - lo[axis]) / ld[axis];

				if (t > 0.f)
				{
					ptOnPlane = lo + t * ld;

					int nextAxis = (axis + 1) % 3;
					int prevAxis = (axis + 2) % 3;

					if (min_[nextAxis] < ptOnPlane[nextAxis] && ptOnPlane[nextAxis] < max_[nextAxis] &&
						min_[prevAxis] < ptOnPlane[prevAxis] && ptOnPlane[prevAxis] < max_[prevAxis])
					{
						if (!intersection)
						{
							return true;
						}
						if ( ret == false ||
							 t < st )
						{
							*intersection = ptOnPlane;
							st = t;
						}
						ret = true;
					}
				}
			}
		}
	}
	return ret;

}

/**
 * Constructor for CylinderPicker object.
 * @param position the position of the front end of the cylinder.
 * @param radius the radius of the cylinder
 * @param the length of the cylinder along the z axis
 * @param capFront whether the front end of the cylinder is capped
 * @param capBack whether the back end of the cylinder is capped
 * @param transform the transform of the cylinder
 */
CylinderPicker::CylinderPicker( const Vector3& position, float radius, float length, bool capFront, bool capBack, const Matrix& transform )
: position_( position ),
  radius_( radius ),
  radiusSquared_( radius * radius ),
  length_( length ),
  capFront_( capFront ),
  capBack_( capBack ),
  transform_( transform )
{
	BW_GUARD;

	invTransform_.invert( transform_ );
}

/*
 * Helper function for intersecting with the caps.
 */
bool CylinderPicker::intersectsDisc( const Vector3& discPosition, float discSign, const Vector3& origin, const Vector3& direction, Vector3* intersection ) const
{
	BW_GUARD;

	bool ret = false;
	Vector3 dsOrigin = origin - discPosition;
	if (discSign * direction.z < 0 &&
		dsOrigin.z * discSign > 0)
	{
		Vector3 dsPos = dsOrigin + (direction / direction.z) * - dsOrigin.z;
		if (dsPos.length() <= radius_)
		{
			ret = true;
			if (intersection)
			{
				*intersection = dsPos + discPosition;
			}
		}
	}
	return ret;
}

/**
 * This method intersects a ray with the cylinder. It only returns true if the
 * ray intersects the outside of the cylinder in the positive direction of the ray.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param intersection pointer to a vector to return the intersection point in
 * @return whether the ray intersected with the cylinder or not
 */
bool CylinderPicker::intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection ) const
{
	BW_GUARD;

	bool ret = false;

	// compute local origin and local direction, normalise the direction
	Vector3 lo = invTransform_.applyPoint( origin );
	Vector3 ld = invTransform_.applyVector( direction );

	Vector3 position = position_;

	position.z /= length_;
	lo.z /= length_;
	ld.z /= length_;
	ld.normalise();

	Vector3 result;
	float resultDist = 0;

	// pick caps
	if ( ld.z != 0 )
	{
		if (capFront_)
		{
			if (intersectsDisc( position, -1, lo, ld, &result ))
			{
				ret = true;
			}
		}
		if (capBack_ && ret == false)
		{
			if (intersectsDisc( position + Vector3(0, 0, 1), 1, lo, ld, &result ))
			{
				ret = true;
			}
		}
	}

	// pick the cylinder
	if ((ld.x != 0 || ld.y != 0) && ret == false)
	{
		Vector2 dir( ld.x, ld.y );
		Vector2 orig( lo.x, lo.y );
		Vector2 pos( position.x, position.y );
		float length = dir.length();
		dir /= length;

		float t = (pos - orig).dotProduct( dir );
		if (t > 0)
		{
			Vector2 crossPos = orig + (dir * t);
			float lengthSq = (crossPos - pos).lengthSquared();
			if (lengthSq <= radiusSquared_)
			{
				float distanceFromEdge = lengthSq < radiusSquared_ ? sqrtf( radiusSquared_ - lengthSq ) : 0;
				float distanceFromOrigin = t - distanceFromEdge;
				if (distanceFromOrigin > 0)
				{
					Vector3 actualPos = lo + ld * ( distanceFromOrigin / length );
					if (actualPos.z >= position.z &&
						actualPos.z <= (position.z + 1))
					{
						ret = true;
						result = actualPos;
					}
				}
			}
		}
	}

	if (intersection && ret == true)
	{
		result.z *= length_;
		*intersection = transform_.applyPoint( result );
	}

	return ret;
}

/**
 * Constructor for ConePicker object.
 * @param position the position of the flat end of the cone.
 * @param radius the radius of the cone at it's extreme
 * @param the length of the cone along the z axis
 * @param cap whether the flat end of the cone is capped
 * @param transform the transform of the cone
 */
ConePicker::ConePicker( const Vector3& position, float radius, float length, bool cap, const Matrix& transform )
: position_( position ),
  radius_( radius ),
  length_( length ),
  cap_( cap ),
  transform_( transform )
{
	BW_GUARD;

	invTransform_.invert( transform_ );
}

/**
 * This method intersects a ray with the cone. It only returns true if the
 * ray intersects the outside of the cone in the positive direction of the ray.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param intersection pointer to a vector to return the intersection point in
 * @return whether the ray intersected with the cone or not
 */
bool ConePicker::intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection  ) const
{
	BW_GUARD;

	bool ret = false;

	Vector3 lo = invTransform_.applyPoint( origin );
	Vector3 ld = invTransform_.applyVector( direction );

	Vector3 out;
	if (cap_ && ld.z != 0)
	{
		if ( intersectsDisc( position_, length_ > 0 ? -1.f : 1.f, lo, ld, &out ) )
		{
			ret = true;
		}
	}

	ld.normalise();

	if (!ret)
	{
		float fEpsilon = 1e-06f;
		float cosConeAngle = fabsf(length_) / ( sqrtf( (length_ * length_) + (radius_ * radius_) ) );
		Vector3 pos = position_ + Vector3( 0, 0, length_ );
		Vector3 dir = Vector3( 0.f, 0.f, length_ > 0 ? -1.f : 1.f );

		// check if origin is inside cone, if it is, pretend we don't intersect.
		Vector3 pointInCone = lo - pos;
		pointInCone.normalise();
		if (dir.dotProduct( pointInCone ) < cosConeAngle)
		{
			// set up quadratic Q(t) = c2*t^2 + 2*c1*t + c0
			float fAdD = dir.dotProduct( ld );
			float fDdD = ld.dotProduct(ld);
			float fCosSqr = cosConeAngle * cosConeAngle;
			Vector3 kE = lo - pos;
			float fAdE = dir.dotProduct( kE );
			float fDdE = ld.dotProduct(kE);
			float fEdE = kE.dotProduct(kE);
			float fC2 = fAdD*fAdD - fCosSqr*fDdD;
			float fC1 = fAdD*fAdE - fCosSqr*fDdE;
			float fC0 = fAdE*fAdE - fCosSqr*fEdE;
			// Solve the quadratic.  Keep only those X for which Dot(A,X-V) &gt; 0.
			if ( fabs(fC2) >= fEpsilon )
			{
				// c2 != 0
				float fDiscr = fC1*fC1 - fC0*fC2;
				if ( fDiscr > 0.0f )
				{
					// two distinct real roots
					float fRoot = sqrtf(fDiscr);
					float fInvC2 = 1.0f/fC2;

					float t = (-fC1 - fRoot)*fInvC2;
					float t2 = (-fC1 + fRoot)*fInvC2;

					Vector3 v = lo + ld * t;
					Vector3 v2 = lo + ld * t2;

					bool valid1 = (v - pos).dotProduct(dir) >= 0;
					bool valid2 = (v2 - pos).dotProduct(dir) >= 0;

					if (valid1&&valid2)
					{
						ret = true;
						if (t < t2)
							out = v;
						else
							out = v2;
					}
					else if (valid1)
					{
						ret = true;
						out = v;
					}
					else if (valid2)
					{
						ret = true;
						out = v2;
					}

				}
				else if ( fDiscr == 0.0f )
				{
					// one repeated real root

					float t = - (fC1/fC2);
					if ( t >= 0 )
					{
						out = ld * t + lo;
						ret = true;
					}
				}
			}
			else if ( fabs(fC1) >= fEpsilon )
			{
				// c2 = 0, c1 != 0
				float t = - (0.5f*fC0/fC1);
				if (t >= 0)
				{
					out = lo + ld * t;
					ret = true;
				}
			}
		}
	}

	// Find out if our intersection point is really inside the cone's bounds.
	float val = (out.z - position_.z) / length_;
	if ( val < 0 || val > 1 )
	{
		ret = false;
	}

	if (ret && intersection)
	{
		*intersection = transform_.applyPoint( out );
	}

	return ret;
}

/*
 * Helper function to intersect with the flat end of the cone
 */
bool ConePicker::intersectsDisc( const Vector3& discPosition, float discSign, const Vector3& origin, const Vector3& direction, Vector3* intersection ) const
{
	BW_GUARD;

	bool ret = false;
	Vector3 dsOrigin = origin - discPosition;
	if (discSign * direction.z < 0 &&
		dsOrigin.z * discSign > 0)
	{
		Vector3 dsPos = dsOrigin + (direction / direction.z) * - dsOrigin.z;
		if (dsPos.length() <= radius_)
		{
			ret = true;
			if (intersection)
			{
				*intersection = dsPos + discPosition;
			}
		}
	}
	return ret;
}

/**
 * Constructor for DiscPicker object.
 * @param position the position of the disc
 * @param innerRadius the inner radius of the disc
 * @param outerRadius the outer radius of the disc
 * @param transform the transform of the disc
 */
DiscPicker::DiscPicker( const Vector3& position, float innerRadius, float outerRadius, const Matrix& transform )
:	position_( position ),
	innerRadius_( innerRadius ),
	outerRadius_( outerRadius ),
	innerRadiusSquared_( innerRadius * innerRadius ),
	outerRadiusSquared_( outerRadius * outerRadius ),
	transform_( transform )
{
	BW_GUARD;

	invTransform_.invert( transform_ );
}

/**
 * This method intersects a ray with the cone. It only returns true if the
 * ray intersects disc in the positive direction of the ray.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param intersection pointer to a vector to return the intersection point in
 * @return whether the ray intersected with the cone or not
 */
bool DiscPicker::intersects( const Vector3& origin, const Vector3& direction, Vector3* intersection  ) const
{
	BW_GUARD;

	bool res = false;
	Vector3 lo = invTransform_.applyPoint( origin );
	Vector3 ld = invTransform_.applyVector( direction );

	if (ld.z != 0)
	{
		Vector3 ldd = ld;
		Vector3 loo = lo;
		ld /= ld.z;
		lo += ( position_.z - lo.z ) * ld;
		float lsq = ( lo - position_ ).lengthSquared();
		if (lsq <= outerRadiusSquared_ && lsq >= innerRadiusSquared_)
		{
			if ( ldd.dotProduct(lo - loo) > 0 )
			{
				res = true;
				if (intersection)
				{
					*intersection = transform_.applyPoint( lo );
				}
			}
		}
	}

	return res;
}

/*
 * Helper struct for generating geometric objects.
 */
struct MiniTri
{
	MiniTri( int aa, int bb, int cc )
	: a(aa),
	  b(bb),
	  c(cc)
	{
	}
	int a,b,c;
};

/*
 * Helper struct for generating geometric objects.
 */
struct MiniEdge
{
	MiniEdge( int aa, int bb )
	: a(aa),
	  b(bb)
	{
	}
	bool operator == ( const MiniEdge& me ) const
	{
		return	(me.a == a && me.b == b) ||
				(me.a == b && me.b == a);
	}
	int a, b;
};

typedef BW::vector< Vector3 > PointVector;
typedef BW::vector< MiniTri > TriVector;
typedef BW::vector< MiniEdge > EdgeVector;

/*
 * Helper class for splitting triangles in the sphere
 */
class SphereSplits
{
public:
	SphereSplits( PointVector& p )
	: ps_( p )
	{
	}

	int split( int a, int b )
	{
		BW_GUARD;

		MiniEdge edge( a, b );
		int splitIndex = -1;
		int index = 0;
		EdgeVector::iterator it = edges_.begin();
		EdgeVector::iterator end = edges_.end();
		while (it!=end)
		{
			if (*it++ == edge )
			{
				splitIndex = splitIndices_[index];
				break;
			}
			index++;
		}

		if (splitIndex == -1)
		{
			splitIndex = static_cast<int>(ps_.size());
			splitIndices_.push_back( splitIndex );
			edges_.push_back( edge );
			ps_.push_back( (ps_[edge.a] * 0.5f) + (ps_[edge.b] * 0.5f) );
			ps_.back().normalise();
		}

		return splitIndex;
	}

private:
	PointVector&		ps_;
	EdgeVector			edges_;
	BW::vector<int>	splitIndices_;

};



void sphereSplit( PointVector& points, TriVector& triangles )
{
	BW_GUARD;

	SphereSplits splits( points );

	int nBeginEntries = static_cast<int>(triangles.size());

	int ti = 0;

	while (ti < nBeginEntries)
	{
		const MiniTri tri = triangles[ti++];

		int ia = splits.split( tri.a, tri.b );
		int ib = splits.split( tri.b, tri.c );
		int ic = splits.split( tri.c, tri.a );

		triangles.push_back( MiniTri( tri.a, ia, ic ) );
		triangles.push_back( MiniTri( tri.b, ib, ia ) );
		triangles.push_back( MiniTri( tri.c, ic, ib ) );
		triangles.push_back( MiniTri( ia, ib, ic ) );
	}

	triangles.erase( triangles.begin(), triangles.begin() + nBeginEntries );
}

/**
 * This method adds a spehere to the mesh and transforms it by the current transform
 * @param pos the position of the sphere
 * @param radius the radius of the sphere
 * @param colour the colour of the sphere
 * @param callback the functor to call when a ray intersects with the sphere
 */
void SolidShapeMesh::addSphere( const Vector3& pos, float radius, uint32 colour, ShapePartPtr callback  )
{
	BW_GUARD;

	pickers_.push_back( new SpherePicker( pos, radius, transform_ ) );
	callbacks_.push_back( callback );

	radius = fabsf(radius);
	PointVector ps;
	TriVector ts;

	Vector3 v( 0, 0, 1 );

	Matrix m, m2, m3;
	m.setRotateX( DEG_TO_RAD( 60 ) );
	m2.setRotateZ( DEG_TO_RAD( 72 ) );
	m3.setRotateZ( DEG_TO_RAD( 72 / 2 ) );

	ps.push_back( v );

	v = m.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	v = m.applyVector( v );
	v = m3.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	v = m2.applyVector( v );
	ps.push_back( v );
	ps.push_back( Vector3( 0, 0, -1 ) );

	for (int i = 0; i < 5; i++)
	{
		ts.push_back( MiniTri( 0, i + 1, ((i + 1) % 5) + 1 ) );
	}

	for (int i = 1; i < 6;i++)
	{
		ts.push_back( MiniTri( (i % 5) + 1, ((i-1) % 5 ) + 6, (i % 5) + 6 ) );
		ts.push_back( MiniTri( (i % 5) + 1, (i % 5 ) + 6, ((i + 1) % 5) + 1 ) );
	}

	for (int i = 0; i < 5; i++)
	{
		ts.push_back( MiniTri( (i % 5) + 6, 11, ((i + 1) % 5) + 6 ) );
	}

	sphereSplit( ps, ts );
	sphereSplit( ps, ts );

	MF_ASSERT( vertices_.size() <= USHRT_MAX );
	uint16 vertexBase = ( uint16 ) vertices_.size();

	Moo::VertexXYZND vert;
	vert.colour_ = colour;

	PointVector::iterator pit = ps.begin();
	PointVector::iterator pend = ps.end();
	while (pit != pend)
	{
		const Vector3& p = *pit++;
		vert.normal_ = p;
		vert.pos_ = (p * radius) + pos;
		addVertex( vert );
	}

	TriVector::iterator tit = ts.begin();
	TriVector::iterator tend = ts.end();
	while (tit!=tend)
	{
		const MiniTri& t = *tit++;
		indices_.push_back( t.a + vertexBase );
		indices_.push_back( t.b + vertexBase );
		indices_.push_back( t.c + vertexBase );
	}
}

/**
 * This method adds a cone to the mesh and transforms it by the current transform
 * @param pos the position of the flat end of the cone
 * @param radius the radius of the cone at the flat end
 * @param length the length of the cone along the z-axis
 * @param cap whether to cap the flat end of the cone or not
 * @param colour the colour of the cone
 * @param callback the functor to call when a ray intersects with the cone
 */
void SolidShapeMesh::addCone( const Vector3& pos, float radius, float length, bool cap, uint32 colour, ShapePartPtr callback )
{
	BW_GUARD;

	pickers_.push_back( new ConePicker( pos, radius, length, cap, transform_ ) );
	callbacks_.push_back( callback );

	radius = fabsf(radius);
	uint16	vertexBase = static_cast<uint16>(vertices_.size());
	int		indexBase  = static_cast<int>(indices_.size());

	bool invert = length < 0;

	Moo::VertexXYZND vert;
	vert.colour_ = colour;
	vert.pos_ = pos + Vector3( 0, 0, length );
	vert.normal_ = invert ? Vector3( 0, 0, -1 ) : Vector3( 0, 0, 1 );

	addVertex( vert );

	Vector3 offset( radius, 0, 0 );
	Matrix m;
	m.setRotateZ( DEG_TO_RAD( 10 ) );

	for (int i = 0; i < 36;i++)
	{
		vert.normal_.crossProduct( Vector3( -offset.x, -offset.y, length ),
									Vector3( offset.y, -offset.x, 0 ) );
		if (invert)
			vert.normal_ *= -1;
		vert.normal_.normalise();
		vert.pos_ = offset + pos;
		addVertex( vert );
		offset = m.applyVector( offset );
	}

	for (int i = 0; i < 36; i++)
	{
		indices_.push_back( vertexBase );
		if (!invert)
		{
			indices_.push_back( vertexBase + 1 + i);
			indices_.push_back( vertexBase + 1 + ((i+1) % 36) );
		}
		else
		{
			indices_.push_back( vertexBase + 1 + ((i+1) % 36) );
			indices_.push_back( vertexBase + 1 + i);
		}
	}

	if (cap)
	{
		vertexBase = static_cast<uint16>(vertices_.size());
		offset.set( radius, 0, 0 );
		vert.normal_ = invert ? Vector3( 0, 0, 1 ) : Vector3( 0, 0, -1 );
		for (int i = 0; i < 36;i++)
		{
			vert.pos_ = offset + pos;
			addVertex( vert );
			offset = m.applyVector( offset );
		}

		for (int i = 0; i < 35; i++)
		{
			indices_.push_back( vertexBase );
			if (invert)
			{
				indices_.push_back( vertexBase + 1 + i);
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
			}
			else
			{
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
				indices_.push_back( vertexBase + 1 + i);
			}
		}
	}
}

/**
 * This method adds a cylinder to the mesh and transforms it by the current transform
 * @param pos the position of one end of the cylinder
 * @param radius the radius of the cylinder
 * @param length the length of the cylinder along the z-axis
 * @param capFront whether to cap the front end of the cylinder
 * @param capBack whether to cap the back end of the cylinder
 * @param colour the colour of the cylinder
 * @param callback the functor to call when a ray intersects with the cylinder
 */
void SolidShapeMesh::addCylinder( const Vector3& pos, float radius, float length, bool capFront, bool capBack, uint32 colour, ShapePartPtr callback )
{
	BW_GUARD;

	pickers_.push_back( new CylinderPicker( pos, radius, length, capFront, capBack, transform_ ) );
	callbacks_.push_back( callback );

	radius = fabsf(radius);
	uint16	vertexBase = static_cast<uint16>(vertices_.size());
	int		indexBase  = static_cast<int>(indices_.size());

	bool invert = length < 0;

	Moo::VertexXYZND vert;
	vert.colour_ = colour;

	Vector3 offset( 1, 0, 0 );
	Matrix m;
	m.setRotateZ( DEG_TO_RAD( 10 ) );

	for (int i = 0; i < 36;i++)
	{
		vert.normal_ = offset;
		vert.pos_ = (offset * radius) + pos;
		addVertex( vert );
		offset = m.applyVector( offset );
	}

	offset.set( 1, 0, 0 );
	for (int i = 0; i < 36;i++)
	{
		vert.normal_ = offset;
		vert.pos_ = (offset * radius) + pos;
		vert.pos_.z += length;
		addVertex( vert );
		offset = m.applyVector( offset );
	}


	for (int i = 0; i < 36; i++)
	{
		if (!invert)
		{
			indices_.push_back( vertexBase + i );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) + 36 );
			indices_.push_back( vertexBase + i );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) + 36 );
			indices_.push_back( vertexBase + i + 36 );
		}
		else
		{
			indices_.push_back( vertexBase + i );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) + 36 );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) );
			indices_.push_back( vertexBase + i );
			indices_.push_back( vertexBase + i + 36 );
			indices_.push_back( vertexBase + (( i + 1 ) % 36) + 36 );
		}
	}

	if (capFront)
	{
		vertexBase = static_cast<uint16>(vertices_.size());
		offset.set( radius, 0, 0 );
		vert.normal_ = invert ? Vector3( 0, 0, 1 ) : Vector3( 0, 0, -1 );
		for (int i = 0; i < 36;i++)
		{
			vert.pos_ = offset + pos;
			addVertex( vert );
			offset = m.applyVector( offset );
		}

		for (int i = 0; i < 35; i++)
		{
			indices_.push_back( vertexBase );
			if (invert)
			{
				indices_.push_back( vertexBase + 1 + i);
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
			}
			else
			{
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
				indices_.push_back( vertexBase + 1 + i);
			}
		}
	}

	if (capBack)
	{
		vertexBase = static_cast<uint16>(vertices_.size());
		offset.set( radius, 0, 0 );
		vert.normal_ = invert ? Vector3( 0, 0, -1 ) : Vector3( 0, 0, 1 );
		for (int i = 0; i < 36;i++)
		{
			vert.pos_ = offset + pos;
			vert.pos_.z += length;
			addVertex( vert );
			offset = m.applyVector( offset );
		}

		for (int i = 0; i < 35; i++)
		{
			indices_.push_back( vertexBase );
			if (!invert)
			{
				indices_.push_back( vertexBase + 1 + i);
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
			}
			else
			{
				indices_.push_back( vertexBase + 1 + ((i+1) % 35) );
				indices_.push_back( vertexBase + 1 + i);
			}
		}
	}

}

/**
 * This method adds a box to the mesh and transforms it by the current transform
 * @param minValues the minimum values of the box (lower left front)
 * @param minValues the maximum values of the box (upper right back)
 * @param colour the colour of the box
 * @param callback the functor to call when a ray intersects with the box
 */
void SolidShapeMesh::addBox( const Vector3& minValues, const Vector3& maxValues, uint32 colour, ShapePartPtr callback )
{
	BW_GUARD;

	pickers_.push_back( new BoxPicker( minValues, maxValues, transform_ ) );
	callbacks_.push_back( callback );

	uint16 vertexBase = static_cast<uint16>(vertices_.size());
	Moo::VertexXYZND vert;
	vert.colour_ = colour;

	vert.normal_.set( 0, 0, -1 );
	vert.pos_.set( minValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2);
	indices_.push_back( vertexBase + 3 );

	vertexBase = static_cast<uint16>(vertices_.size());
	vert.normal_.set( 0, 0, 1 );
	vert.pos_.set( minValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 3 );
	indices_.push_back( vertexBase + 2);

	vertexBase = static_cast<uint16>(vertices_.size());
	vert.normal_.set( 0, -1, 0 );
	vert.pos_.set( minValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 3 );
	indices_.push_back( vertexBase + 2);

	vertexBase = static_cast<uint16>(vertices_.size());
	vert.normal_.set( 0, 1, 0 );
	vert.pos_.set( minValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2);
	indices_.push_back( vertexBase + 3 );

	vertexBase = static_cast<uint16>(vertices_.size());
	vert.normal_.set( -1, 0, 0 );
	vert.pos_.set( minValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( minValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2);
	indices_.push_back( vertexBase + 3 );

	vertexBase = static_cast<uint16>(vertices_.size());
	vert.normal_.set( 1, 0, 0 );
	vert.pos_.set( maxValues.x, minValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, minValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, minValues.y, maxValues.z );
	addVertex( vert );
	vert.pos_.set( maxValues.x, maxValues.y, maxValues.z );
	addVertex( vert );
	indices_.push_back( vertexBase );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 2 );
	indices_.push_back( vertexBase + 1 );
	indices_.push_back( vertexBase + 3 );
	indices_.push_back( vertexBase + 2);
}

/**
 * This method adds a disc to the mesh and transforms it by the current transform
 * @param pos the position of the disc
 * @param innerRadius the inner radius of the disc
 * @param outerRadius the outer radius of the disc
 * @param callback the functor to call when a ray intersects with the disc
 */
void SolidShapeMesh::addDisc( const Vector3& pos, float innerRadius, float outerRadius, uint32 colour, ShapePartPtr callback )
{
	BW_GUARD;

	pickers_.push_back( new DiscPicker( pos, innerRadius, outerRadius, transform_ ) );
	callbacks_.push_back( callback );

	uint16 vertexBase = static_cast<uint16>(vertices_.size());
	Vector3 offset( 1, 0, 0 );
	Matrix m;
	m.setRotateZ( DEG_TO_RAD( 10 ) );
	Moo::VertexXYZND vert;
	vert.normal_.set( 0, 0, 1 );
	vert.colour_ = colour;
	for (int i = 0; i < 36; i++)
	{
		vert.pos_ = offset * innerRadius + pos;
		addVertex( vert );
		vert.pos_ = offset * outerRadius + pos;
		addVertex( vert );
		offset = m.applyVector( offset );
	}

	for (int i = 0; i < 72; i += 2)
	{
		indices_.push_back( vertexBase + i );
		indices_.push_back( vertexBase + ((i + 1) % 72) );
		indices_.push_back( vertexBase + ((i + 2) % 72) );
		indices_.push_back( vertexBase + ((i + 2) % 72) );
		indices_.push_back( vertexBase + ((i + 1) % 72) );
		indices_.push_back( vertexBase + ((i + 3) % 72) );
	}

	vertexBase = static_cast<uint16>(vertices_.size());
	offset.set( 1, 0, 0 );
	vert.normal_.set( 0, 0, -1 );
	m.setRotateZ( DEG_TO_RAD( -10 ) );
	for (int i = 0; i < 36; i++)
	{
		vert.pos_ = offset * innerRadius + pos;
		addVertex( vert );
		vert.pos_ = offset * outerRadius + pos;
		addVertex( vert );
		offset = m.applyVector( offset );
	}

	for (int i = 0; i < 72; i += 2)
	{
		indices_.push_back( vertexBase + i );
		indices_.push_back( vertexBase + ((i + 1) % 72) );
		indices_.push_back( vertexBase + ((i + 2) % 72) );
		indices_.push_back( vertexBase + ((i + 2) % 72) );
		indices_.push_back( vertexBase + ((i + 1) % 72) );
		indices_.push_back( vertexBase + ((i + 3) % 72) );
	}
}


/**
 * This method sets the current transform that gets applied to all objects added
 * to the mesh.
 * @param transform the transform to set
 */
void SolidShapeMesh::transform( const Matrix& transform )
{
	BW_GUARD;

	transform_ = transform;
	normalTransform_.invert( transform );
	normalTransform_[3].setZero();
	XPMatrixTranspose( &normalTransform_, &normalTransform_ );
}

/*
 * Helper function to transform and add a vertex to the vertex list.
 */
void SolidShapeMesh::addVertex( const Moo::VertexXYZND& vert )
{
	BW_GUARD;

	vertices_.push_back( vert );
	vertices_.back().pos_ = transform_.applyPoint( vert.pos_ );
	vertices_.back().normal_ = transform_.applyVector( vert.normal_ );
	vertices_.back().normal_.normalise();
}

/**
 * This method intersects a ray with the objects in the mesh. It only returns true if the
 * ray intersects the outside of any of the objects, and it also calls the collision
 * callback for the closest object it intersects with.
 *
 * @param origin origin of the ray
 * @param direction direction of the ray
 * @param distance pass in the maximum distance of the direction returns the distance to the closest object
 * @return the ShapePartPtr of the intersection
 */
ShapePart* SolidShapeMesh::intersects( const Vector3& origin, const Vector3& direction, float* distance )
{
	BW_GUARD;

	BW::vector< PickerPtr >::iterator it = pickers_.begin();
	BW::vector< PickerPtr >::iterator end = pickers_.end();
	BW::vector< ShapePartPtr >::iterator git = callbacks_.begin();

	Vector3 is( 0, 0, 0);
	bool ret = false;
	float dist = 0;
	ShapePartPtr part = NULL;

	while (it!=end)
	{
		PickerPtr picker = *it++;

		bool res = picker->intersects( origin, direction, &is ) && *git;
		if (res)
		{
			if (ret)
			{
				float newdist = (origin - is).length();
				if (newdist < dist)
				{
					dist = newdist;
					part = *git;
				}
			}
			else
			{
				ret = true;
				dist = (origin - is).length();
				part = *git;
			}
		}
		git++;
	}

	if (distance && part.hasObject())
	{
		if (*distance <= dist)
			return NULL;
		else
			*distance = dist;
	}

	return part.getObject();
}

void SolidShapeMesh::draw(Moo::RenderContext &rc)
{
	BW_GUARD;

	Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = 
        dib.lock(static_cast<uint32>(indices().size()));
	if (ind.size())
	{
		ind.fill(&indices_.front(), static_cast<uint32>(indices_.size()));

		dib.unlock();
		uint32 ibLockIndex = dib.lockIndex();

		typedef Moo::VertexXYZND VertexType;
		int vertexSize = sizeof(VertexType);

		Moo::DynamicVertexBufferBase2 & dvb =
			Moo::DynamicVertexBufferBase2::instance( vertexSize );

		void * verts = dvb.lock( static_cast<uint32>(vertices_.size()) );
		if (verts)
		{
			memcpy( verts, &vertices_.front(), vertexSize * vertices_.size() );

			dvb.unlock();
			uint32 vbLockIndex = dvb.lockIndex();
			
			rc.setFVF( VertexType::fvf() );
			dvb.set();
			dib.indexBuffer().set();

			rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, vbLockIndex, 0, static_cast<UINT>(vertices_.size()),
				ibLockIndex, static_cast<UINT>(indices_.size() / 3) );
		}
	}
}


void SolidShapeMesh::clear()
{
	BW_GUARD;

    vertices_ .clear();
    indices_  .clear();
    pickers_  .clear();
    callbacks_.clear();

	transform_      .setIdentity();
	normalTransform_.setIdentity();
}


SolidShapeMesh::SolidShapeMesh()
{
	transform_.setIdentity();
	normalTransform_.setIdentity();
}
BW_END_NAMESPACE

