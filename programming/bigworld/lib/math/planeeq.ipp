#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro
    #define INLINE
#endif


/**
 *	The default constructor. If this constructor is used, care should be taken
 *	that the plane equation is set up later.
 */
INLINE
PlaneEq::PlaneEq()
{
}


/**
 *	This constructor takes a normal to the plane and the value of the dot
 *	product between this normal and a point on the plane.
 *
 *	@param normal	The normal to the plane. (The plane has sideness). The
 *					normal should point out of the front of the plane.
 *	@param d		The dot product between the input normal and any point on
 *					the plane.
 */
INLINE
PlaneEq::PlaneEq( const Vector3 & normal,
				 const float d ) :
	normal_( normal ),
	d_( d )
{
}


/**
 *	This constructor takes a normal to the plane and the value of the dot
 *	product between this normal and a point on the plane.
 *
 *	@param normal	The normal to the plane. (The plane has sideness). The
 *					normal should point out of the front of the plane.
 *	@param d		The dot product between the input normal and any point on
 *					the plane.
 */
INLINE void PlaneEq::init( const Vector3 & normal,
	const float d )
{
	normal_ = normal;
	d_ = d;
}


/**
 *	This constructor takes a point on the plane and a normal to the plane.
 *
 *	@param point	Any point on the plane.
 *	@param normal	The normal to the plane. (The plane has sideness). The
 *					normal should point out of the front of the plane.
 */
INLINE
PlaneEq::PlaneEq( const Vector3 & point,
				 const Vector3 & normal ) :
	normal_( normal ),
	d_( normal.dotProduct( point ) )
{
}


/**
 *	This constructor takes a point on the plane and a normal to the plane.
 *
 *	@param point	Any point on the plane.
 *	@param normal	The normal to the plane. (The plane has sideness). The
 *					normal should point out of the front of the plane.
 */
INLINE
void PlaneEq::init( const Vector3 & point,
				 const Vector3 & normal )
{
	normal_ = normal;
	d_ = normal.dotProduct( point );
}


/**
 *	This constructor takes three points that lie on the plane. The points
 *	should be in a clockwise order when looked at from the front side of the
 *	plane. The points must not be colinear.
 *
 *	@param p0			A point on the plane.
 *	@param p1			A point on the plane.
 *	@param p2			A point on the plane.
 *	@param normalise	If true, the normal is normalised.
 */
INLINE
PlaneEq::PlaneEq( const Vector3 & p0,
	const Vector3 & p1,
	const Vector3 & p2,
	ShouldNormalise normalise )
{
	this->init( p0, p1, p2, normalise );
}


/**
 *	This method initialises the plane equation so that the input points
 *	lie on it. The points should be in a clockwise order when looked at from the
 *	front of the plane. The points must not be colinear.
 *
 *	@param p0			A point on the plane.
 *	@param p1			A point on the plane.
 *	@param p2			A point on the plane.
 *	@param normalise	If true, the normal is normalised.
 */
INLINE
void PlaneEq::init( const Vector3 & p0,
	const Vector3 & p1,
	const Vector3 & p2,
	ShouldNormalise normalise )
{
	normal_.crossProduct(p1 - p0, p2 - p0);

	if (normalise == SHOULD_NORMALISE)
		normal_.normalise();

	d_ = normal_.dotProduct( p0 );
}


/**
 * This method returns the normal of the plane. The normal has unit length.
 */
INLINE
const Vector3 & PlaneEq::normal() const
{
	return this->normal_;
}


/**
 *	This method returns the constant in the plane equation. This value is equal
 *	to the dot product of this plane's normal with any point on the plane.
 */
INLINE
float PlaneEq::d() const
{
	return d_;
}


/**
 *	This method return the y coordinate of the plane for the input x and
 *	z coordinates.
 *
 *	@param x	The x-coordinate of the point to find.
 *	@param z	The z-coordinate of the point to find.
 *
 *	@return The y-coordinate of the point on the plane with the input x and z
 *		coordinates.
 *
 *	@warning	This method should not be called on a plane that is parallel to
 *				the y axis.
 */
INLINE
float PlaneEq::y(float x, float z) const
{
	if (isZero( normal_[Y_COORD] ))
		return 0.f;	// avoid division by zero
	return (d_ - normal_[X_COORD] * x - normal_[Z_COORD] * z) / normal_[Y_COORD];
}


/**
 *	This method returns the distance of the input point from the plane. If the
 *	input point is behind the plane a negative value is returned.
 *
 *	@param point	The point to find the distance from the plane.
 *
 *	@return		The distance from the input point to the nearest point on the
 *				plane.
 */
INLINE
float PlaneEq::distanceTo(const Vector3 & point) const
{
	return normal_.dotProduct(point) - d_;
}


/**
 *	This method returns whether or not the input point is in front of the plane,
 *	allowing for a small buffer zone.
 *
 *	@return True if the input point is in front of the plane, otherwise false.
 */
INLINE
bool PlaneEq::isInFrontOf(const Vector3 & point) const
{
	if ( almostEqual( normal_.dotProduct(point), d_ ) )
		return true;
	return normal_.dotProduct(point) > d_;
}


/**
 *	This method returns whether or not the input point is in front of the plane.
 *
 *	@return True if the input point is in front of the plane, otherwise false.
 */
INLINE
bool PlaneEq::isInFrontOfExact(const Vector3 & point) const
{
	return normal_.dotProduct(point) > d_;
}


/**
 *	This method sets this plane to be hidden. This is related to back-face
 *	culling.
 */
INLINE
void PlaneEq::setHidden()
{
	normal_.set(0, 0, 0);
	d_ = 1;
}


/**
 *	This method sets this plane so that it is always visible. This is related to
 *	back-face culling.
 */
INLINE
void PlaneEq::setAlwaysVisible()
{
	normal_.set(0, 0, 0);
	d_ = -1;
}




/**
 *	This method intersects a ray from 'source' going in direction 'dir'
 *	with this plane.
 */
INLINE
Vector3 PlaneEq::intersectRay( const Vector3 & source,
	const Vector3 & dir ) const
{
	return source +
		this->intersectRayHalf( source, normal_.dotProduct( dir ) ) * dir;
}


/**
 *	This method intersects a ray from 'source' going in a given direction
 *	with this plane. The second argument is the precalculated dot product
 *	the plane's normal with the direction.
 *
 *	@return amount to multiply direction by from source to get to the plane
 */
INLINE
float PlaneEq::intersectRayHalf( const Vector3 & source,
	float normalDotDir ) const
{
	if (isZero( normalDotDir ))
		return 0.f;	// avoid division by zero
	return (d_ - normal_.dotProduct( source )) / normalDotDir;
}

/**
 *	This method intersects a ray from 'source' going in a given direction
 *	with this plane. The second argument is one over the precalculated
 *	dot product the plane's normal with the direction.
 *
 *	@return amount to multiply direction by from source to get to the plane
 */
INLINE
float PlaneEq::intersectRayHalfNoCheck( const Vector3 & source,
	float oneOverNormalDotDir ) const
{
	return (d_ - normal_.dotProduct( source )) * oneOverNormalDotDir;
}
