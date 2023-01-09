#ifndef RECTT_IPP
#define RECTT_IPP


/**
 *	This constructs an empty rectangle.
 */
template< typename COORDINATE >
inline RectT< COORDINATE >::RectT()
{
	x_.set( 0, 0 );
	y_.set( 0, 0 );
}


/**
 *	This constructs a rectangle.
 *
 *	@param xMin		The minimum x coordinate.
 *	@param yMin		The minimum y coordinate.
 *	@param xMax		The maximum x coordinate.
 *	@param yMax		The maximum y coordinate.
 */
template< typename COORDINATE >
inline RectT< COORDINATE >::RectT( 
	typename RectT< COORDINATE >::Coordinate xMin, 
	typename RectT< COORDINATE >::Coordinate yMin, 
	typename RectT< COORDINATE >::Coordinate xMax, 
	typename RectT< COORDINATE >::Coordinate yMax )
{
	x_.set( xMin, xMax );
	y_.set( yMin, yMax );
}


/**
 *	This gets the area of the rectangle.
 *
 *	@return			The area of the rectangle.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Coordinate 
	RectT< COORDINATE >::area() const	
{ 
	return x_.length() * y_.length(); 
}


/**
 *	This expands or contracts the rectangle.
 *
 *	@param value	The amount to expand or contract by.  If value is positive
 *					then the rectangle is expanded.  If value is negative then
 *					the rectangle is contracted.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::inflateBy( 
	typename RectT< COORDINATE >::Coordinate value )
{
	x_.inflateBy( value );
	y_.inflateBy( value );
}


/**
 *	This expands or contracts the rectangle. It makes sure not to increase a
 *	value that is already at numeric_limits.
 *
 *	@param value	The amount to expand or contract by.  If value is positive
 *					then the rectangle is expanded.  If value is negative then
 *					the rectangle is contracted.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::safeInflateBy( 
	typename RectT< COORDINATE >::Coordinate value )
{
	x_.safeInflateBy( value );
	y_.safeInflateBy( value );
}


/**
 *	This returns whether this rectangle intersects another rectangle.
 *
 *	@param rect		The other rectangle to test intersection against.
 *	@return			True if this rectangle and rect intersect.
 */
template< typename COORDINATE >
inline bool RectT< COORDINATE >::intersects( const RectT & rect ) const
{
	return x_.intersects( rect.x_ ) && y_.intersects( rect.y_ );
}


/**
 *	This returns whether the given point is inside the rectangle.
 *
 *	@param x		The x coordinate of the point.
 *	@param y		The y coordinate of the point.
 *	@return			True if the point is inside the rectangle, false if it is
 *					outside.  The edges and corners are considered to be 
 *					inside the rectangle.
 */
template< typename COORDINATE >
inline bool RectT< COORDINATE >::contains( 
	typename RectT< COORDINATE >::Coordinate x, 
	typename RectT< COORDINATE >::Coordinate y ) const
{
	return x_.contains( x ) && y_.contains( y );
}


/**
 *	This returns whether the given rectangle is inside this rectangle.
 *
 *	@param rect		The (potentially contained) rectangle.
 *	@return			True if the rect is inside this rectangle, false if it is
 *					outside.  The edges and corners are considered to be 
 *					inside the rectangle.
 */
template< typename COORDINATE >
inline bool RectT< COORDINATE >::contains( const RectT & rect ) const
{
	return x_.contains( rect.x_ ) && y_.contains( rect.y_ );
}


/**
 *	This returns the distance of the given point to the edges of the rectangle.
 *
 *	@param x		The x coordinate of the point.
 *	@param y		The y coordinate of the point.
 *	@return			The L_\inf distance of the point from the edges.
 */
template< typename COORDINATE >
inline float RectT< COORDINATE >::distTo( 
	typename RectT< COORDINATE >::Coordinate x, 
	typename RectT< COORDINATE >::Coordinate y ) const
{
	return std::max( x_.distTo( x ), y_.distTo( y ) );
}


/**
 *	This sets the rectangle to be the intersection of the two rectangles.  If 
 *	the two rectangles do no intersect then the rectangle is set to be 
 *	(0, 0, 0, 0).
 *
 *	@param r1		Rectangle 1.
 *	@param r2		Rectangle 2.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::setToIntersection( 
	const RectT & r1,
	const RectT & r2 )
{
	xMin_ = std::max( r1.xMin_, r2.xMin_ );
	yMin_ = std::max( r1.yMin_, r2.yMin_ );
	xMax_ = std::min( r1.xMax_, r2.xMax_ );
	yMax_ = std::min( r1.yMax_, r2.yMax_ );

	if ((xMax_ < xMin_) || (yMax_ < yMin_))
	{
		x_.set( 0.f, 0.f );
		y_.set( 0.f, 0.f );
	}
}


/**
 *	This sets the rectangle to be the smallest rectangle containing the two
 *	rectangles.
 *
 *	@param r1		Rectangle 1.
 *	@param r2		Rectangle 2.		
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::setToUnion( 
	const RectT & r1, 
	const RectT & r2 )
{
	xMin_ = std::min( r1.xMin_, r2.xMin_ );
	yMin_ = std::min( r1.yMin_, r2.yMin_ );
	xMax_ = std::max( r1.xMax_, r2.xMax_ );
	yMax_ = std::max( r1.yMax_, r2.yMax_ );
}


/**
 *	This gets the given range of the rectangle.
 *
 *	@param getY		Determines whether we get the y or x range.
 *	@return			The y range of the rectangle if getY is true,
 *					the x range of the rectangle if getY is false.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Range & 
	RectT< COORDINATE >::range1D( bool getY )
{ 
	return getY ? y_ : x_; 
}


/**
 *	This gets the given range of the rectangle.
 *
 *	@param getY		Determines whether we get the y or x range.
 *	@return			The y range of the rectangle if getY is true,
 *					the x range of the rectangle if getY is false.
 */
template< typename COORDINATE >
inline const typename RectT< COORDINATE >::Range & 
	RectT< COORDINATE >::range1D( bool getY ) const	
{ 
	return getY ? y_ : x_; 
}


/**
 *	This gets the x minimum of the rectangle.
 *
 *	@return			The x minimum of the rectangle.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Coordinate 
	RectT< COORDINATE >::xMin() const
{ 
	return xMin_; 
}


/**
 *	This gets the x maximum of the rectangle.
 *
 *	@return			The x maximum of the rectangle.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Coordinate 
	RectT< COORDINATE >::xMax() const
{ 
	return xMax_; 
}


/**
 *	This gets the y minimum of the rectangle.
 *
 *	@return			The y minimum of the rectangle.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Coordinate 
	RectT< COORDINATE >::yMin() const
{ 
	return yMin_; 
}


/**
 *	This gets the y maximum of the rectangle.
 *
 *	@return			The y maximum of the rectangle.
 */
template< typename COORDINATE >
inline typename RectT< COORDINATE >::Coordinate 
	RectT< COORDINATE >::yMax() const
{ 
	return yMax_; 
}


/**
 *	This sets the x minimum of the rectangle.
 *
 *	@param value	The new x minimum.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::xMin(
	typename RectT< COORDINATE >::Coordinate value )
{ 
	xMin_ = value; 
}


/**
 *	This sets the x maximum of the rectangle.
 *
 *	@param value	The new x maximum.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::xMax(
	typename RectT< COORDINATE >::Coordinate value )
{ 
	xMax_ = value; 
}


/**
 *	This sets the y minimum of the rectangle.
 *
 *	@param value	The new y minimum.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::yMin(
	typename RectT< COORDINATE >::Coordinate value )
{ 
	yMin_ = value; 
}


/**
 *	This sets the y maximum of the rectangle.
 *
 *	@param value	The new y maximum.
 */
template< typename COORDINATE >
inline void RectT< COORDINATE >::yMax(
	typename RectT< COORDINATE >::Coordinate value )
{ 
	yMax_ = value; 
}


/**
 *	This gets the x range of the rectangle.
 *
 *	@return			The x range of the rectangle.
 */
template< typename COORDINATE >
inline const typename RectT< COORDINATE >::Range & 
	RectT< COORDINATE >::xRange() const
{ 
	return x_; 
}


/**
 *	This gets the y range of the rectangle.
 *
 *	@return			The y range of the rectangle.
 */
template< typename COORDINATE >
inline const typename RectT< COORDINATE >::Range & 
	RectT< COORDINATE >::yRange() const
{ 
	return y_; 
}


/**
 *  This function compares 2 arguments for equality
 *
 *  @return true if arguments are equal, false otherwise.
 */
template< typename COORDINATE >
bool
	operator==( const RectT< COORDINATE > & l, const RectT< COORDINATE > & r )
{
	return
		isEqual(l.xMin_, r.xMin_) &&
		isEqual(l.yMin_, r.yMin_) &&
		isEqual(l.xMax_, r.xMax_) &&
		isEqual(l.yMax_, r.yMax_);
}


/**
 *  This function compares 2 arguments for inequality
 *
 *  @return true if arguments are not equal, false otherwise.
 */
template< typename COORDINATE >
inline bool
	operator!=( const RectT< COORDINATE > & l, const RectT< COORDINATE > & r )
{
	return !operator==( l, r );
}

#endif // RECTT_IPP
