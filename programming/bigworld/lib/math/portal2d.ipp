// portal2d.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

/**
 *	Add the given point to the portal
 */
INLINE void Portal2D::addPoint( const Vector2 &v )
{
	points_.push_back( v );
}


// portal2D.ipp

