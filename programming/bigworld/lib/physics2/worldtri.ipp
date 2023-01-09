/**
 *	@file
 */

#ifdef CODE_INLINE
#define INLINE inline
#else
/// INLINE macro
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
INLINE WorldTriangle::WorldTriangle( const Vector3 & v0,
		const Vector3 & v1,
		const Vector3 & v2,
		Flags flags ) :
	flags_( flags ),
	padding_( 0 )
{
	v_[0] = v0;
	v_[1] = v1;
	v_[2] = v2;
}


/**
 *	Default constructor
 */
INLINE WorldTriangle::WorldTriangle() :
	flags_( 0 ),
	padding_( 0 )
{
	v_[0] = Vector3::zero();
	v_[1] = Vector3::zero();
	v_[2] = Vector3::zero();	
}


// -----------------------------------------------------------------------------
// Section: Accessors
// -----------------------------------------------------------------------------

/**
 *	This method returns the first vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v0() const
{
	return v_[0];
}


/**
 *	This method returns the second vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v1() const
{
	return v_[1];
}


/**
 *	This method returns the third vertex of this triangle.
 */
INLINE
const Vector3 & WorldTriangle::v2() const
{
	return v_[2];
}



/**
 *	This method returns vertex at given index.
 */
INLINE
const Vector3 & WorldTriangle::v( uint32 index ) const
{
	MF_ASSERT( index < 3 );
	return v_[index];	
}

/**
 *	This method returns the normal of this triangle.
 */
INLINE
Vector3 WorldTriangle::normal() const
{
	return (v_[2] - v_[0]).crossProduct(v_[2] - v_[1]);
}


/**
 *	This method returns the flags associated with this triangle.
 */
INLINE
WorldTriangle::Flags WorldTriangle::flags() const
{
	return flags_;
}


/**
 *	This method sets the flags associated with this triangle.
 */
INLINE
void WorldTriangle::flags( Flags newFlags )
{
	flags_ = newFlags;
}


/**
 *	This method returns whether or not this triangle is transparent.
 */
INLINE
bool WorldTriangle::isTransparent() const
{
	return (flags_ & TRIANGLE_TRANSPARENT) != 0;
}


/**
 *	This method returns whether or not this triangle uses blended transparency.
 */
INLINE
bool WorldTriangle::isBlended() const
{
	return (flags_ & TRIANGLE_BLENDED) != 0;
}



// ---------------------------------------------------------------------
// Section: Intersection
// ---------------------------------------------------------------------

//
// Macros
//

/// Helper macro
#define ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1) \
              isect0=VV0+(VV1-VV0)*D0/(D0-D1);    \
              isect1=VV0+(VV2-VV0)*D0/(D0-D2);


/// Helper macro
#define COMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,isect0,isect1) \
  if(D0D1>0.0f)                                         \
  {                                                     \
    /* here we know that D0D2<=0.0 */                   \
    /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else if(D0D2>0.0f)                                    \
  {                                                     \
    /* here we know that d0d1<=0.0 */                   \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if (D1*D2>0.0f || !almostZero( D0 ))             \
  {                                                     \
    /* here we know that d0d1<=0.0 or that D0!=0.0 */   \
    ISECT(VV0,VV1,VV2,D0,D1,D2,isect0,isect1);          \
  }                                                     \
  else if (!almostZero( D1 ))                           \
  {                                                     \
    ISECT(VV1,VV0,VV2,D1,D0,D2,isect0,isect1);          \
  }                                                     \
  else if (!almostZero( D2 ))                           \
  {                                                     \
    ISECT(VV2,VV0,VV1,D2,D0,D1,isect0,isect1);          \
  }                                                     \
  else                                                  \
  {                                                     \
    /* triangles are coplanar (Treat as false) */       \
    return false;                                       \
  }


//
// Helper methods
//

/// This function will swap the values in a and b if a is greater than b.
template <class T>
inline void sort(T & a, T & b)
{
	if (a > b)
	{
		T temp = a;
		a = b;
		b = temp;
	}
}

// worldtri.ipp
