//boundbox.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


INLINE AABB::AABB() :
	min_( FLT_MAX, FLT_MAX, FLT_MAX ),
	max_( -FLT_MAX, -FLT_MAX, -FLT_MAX )
{
}

INLINE AABB::AABB( const Vector3 & min, const Vector3 & max ) :
	min_( min ),
	max_( max )
{
}

INLINE bool AABB::operator==( const AABB & bb ) const
{
	return (min_ == bb.min_) && (max_ == bb.max_); 
}

INLINE bool AABB::operator!=( const AABB & bb ) const
{
	return (min_ != bb.min_) || (max_ != bb.max_); 
}

/**
 *	Constructor
 *	Defaults to the unset/inside out state
 */
INLINE BoundingBox::BoundingBox() :
	oc_( 0 ),
	combinedOc_( 0)
{
}


/**
 *	Constructor taking an aabb
 */
INLINE BoundingBox::BoundingBox( const AABB& other ) :
	AABB( other ),
	oc_( 0 ),
	combinedOc_( 0)
{

}


/**
 *	Constructor taking min and max
 */
INLINE BoundingBox::BoundingBox( const Vector3 & min, const Vector3 & max ) :
	AABB(min, max),
	oc_( 0 ),
	combinedOc_( 0)
{
}

/**
 *	This method tests if two bounding boxen are equal
 */
INLINE bool BoundingBox::operator==( const BoundingBox & bb ) const
{
	return AABB::operator==(bb);
}

/**
 *	This method tests if two bounding boxen are not equal
 */
INLINE bool BoundingBox::operator!=( const BoundingBox & bb ) const
{
	return AABB::operator!=(bb);
}

/**
 *	This method gets the min bounds
 */
INLINE const Vector3 & AABB::minBounds() const
{
	MF_ASSERT_DEBUG( !this->insideOut() );

	return min_;
}

/**
 *	This method gets the max bounds
 */
INLINE const Vector3 & AABB::maxBounds() const
{
	MF_ASSERT_DEBUG( !this->insideOut() );

	return max_;
}

/**
 *	This method sets the bounds
 */
INLINE void AABB::setBounds( const Vector3 & min, const Vector3 & max )
{
	min_ = min;
	max_ = max;
}

/**
 *  This gets the width (x-direction) of the bounding box.
 */
INLINE float AABB::width() const
{
	if (insideOut())
		return 0.0f;
	else
		return max_.x - min_.x;
}

/**
 *  This gets the height (y-direction) of the bounding box.
 */
INLINE float AABB::height() const
{
	if (insideOut())
		return 0.0f;
	else
		return max_.y - min_.y;
}

/**
 *  This gets the depth (z-direction) of the bounding box.
 */
INLINE float AABB::depth() const
{
	if (insideOut())
		return 0.0f;
	else
		return max_.z - min_.z;
}

INLINE void AABB::addYBounds( float y )
{
	if( min_[1] > y )
		min_[1] = y;
	if( max_[1] < y )
		max_[1] = y;
}

/**
 *	This method expands the bounding box to contain the given point
 */
INLINE void AABB::addBounds( const Vector3 & v )
{
	if( this->insideOut() )
	{
		min_ = v;
		max_ = v;
		return;
	}

	if (min_[0] > v[0])
		min_[0] = v[0];

	if (min_[1] > v[1])
		min_[1] = v[1];

	if (min_[2] > v[2])
		min_[2] = v[2];

	if (max_[0] < v[0])
		max_[0] = v[0];

	if (max_[1] < v[1])
		max_[1] = v[1];

	if (max_[2] < v[2])
		max_[2] = v[2];
}

/**
 * This method expands the bounding box by the given amounts in each direction.
 * Empty bounding boxes are left alone.  Offseting inwards too much can make, 
 * for example, min_.x > max_.x which in turns makes the box inside out as 
 * you'd expect.
 */
INLINE void AABB::expandSymmetrically( float dx, float dy, float dz )
{
	if (!insideOut())
	{
		min_.x -= dx; min_.y -= dy; min_.z -= dz;
		max_.x += dx; max_.y += dy; max_.z += dz;
	}
}

/**
 * This method expands the bounding box by the given amounts in each direction.
 * Empty bounding boxes are left alone.  Offseting inwards too much can make, 
 * for example, min_.x > max_.x which in turns makes the box inside out as 
 * you'd expect.
 */
INLINE void AABB::expandSymmetrically( const Vector3 & v )
{
	expandSymmetrically(v.x, v.y, v.z);
}

/**
 *	This method expands the bounding box to contain the given bounding box
 */
INLINE void AABB::addBounds( const AABB & bb )
{
	if( bb.insideOut() )
		return;

	if( this->insideOut() )
	{
		min_ = bb.min_;
		max_ = bb.max_;
		return;
	}

	if (min_[0] > bb.min_[0])
		min_[0] = bb.min_[0];

	if (min_[1] > bb.min_[1])
		min_[1] = bb.min_[1];

	if (min_[2] > bb.min_[2])
		min_[2] = bb.min_[2];

	if (max_[0] < bb.max_[0])
		max_[0] = bb.max_[0];

	if (max_[1] < bb.max_[1])
		max_[1] = bb.max_[1];

	if (max_[2] < bb.max_[2])
		max_[2] = bb.max_[2];
}

/**
 *	This method gets the outcode
 */
INLINE Outcode BoundingBox::outcode( void ) const
{
	return oc_;
}

/**
 *	This method gets the combined outcode
 */
INLINE Outcode BoundingBox::combinedOutcode( void ) const
{
	return combinedOc_;
}

/**
 *	This method sets the outcode
 */
INLINE void BoundingBox::outcode( Outcode oc )
{
	oc_ = oc;
}

/**
 *	This method sets the combined outcode
 */
INLINE void BoundingBox::combinedOutcode( Outcode oc )
{
	combinedOc_ = oc;
}

/**
 *	This inline function is used by BoundingBox::intersects
 */
INLINE bool intervalsIntersect( float min1, float max1, float min2, float max2 )
{
	return min2 <= max1 && min1 <= max2;
}

/**
 *	This method tests if two bounding boxen intersect
 */
INLINE bool AABB::intersects( const AABB & box ) const
{
	const Vector3 & min1 = this->min_;
	const Vector3 & max1 = this->max_;

	const Vector3 & min2 = box.min_;
	const Vector3 & max2 = box.max_;

	return(
		intervalsIntersect(min1[0], max1[0], min2[0], max2[0]) &&
		intervalsIntersect(min1[1], max1[1], min2[1], max2[1]) &&
		intervalsIntersect(min1[2], max1[2], min2[2], max2[2]));
}


/**
 *	This method tests if the bounding box contains the given point
 */
INLINE bool AABB::intersects( const Vector3 & v ) const
{
	return ( v[ 0 ] >= min_[ 0 ] ) && ( v[ 1 ] >= min_[ 1 ] ) && ( v[ 2 ] >= min_[ 2 ] ) &&
		   ( v[ 0 ] <  max_[ 0 ] ) && ( v[ 1 ] <  max_[ 1 ] ) && ( v[ 2 ] <  max_[ 2 ] );
}

/**
 *	This method tests if the bounding box enlarged by the given bias
 *	(in each dimension) contains the given point
 */
INLINE bool AABB::intersects( const Vector3 &v, float bias ) const
{
	return ( v[ 0 ] >= ( min_[ 0 ] - bias ) ) && ( v[ 1 ] >= ( min_[ 1 ] - bias ) ) && ( v[ 2 ] >= ( min_[ 2 ] - bias ) ) &&
		   ( v[ 0 ] <  ( max_[ 0 ] + bias ) ) && ( v[ 1 ] <  ( max_[ 1 ] + bias ) ) && ( v[ 2 ] <  ( max_[ 2 ] + bias ) );
}


/**
 * The centre point of the bounding box
 */
INLINE Vector3 AABB::centre() const
{
	MF_ASSERT_DEBUG( !this->insideOut() );

	return (min_ + max_) / 2.f;
}


INLINE 	bool AABB::insideOut() const
{
	return min_.x > max_.x || min_.y > max_.y || min_.z > max_.z;
}


//boundbox.ipp
