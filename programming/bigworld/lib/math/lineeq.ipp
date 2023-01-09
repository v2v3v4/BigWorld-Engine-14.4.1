// lineeq.ipp

/**
 *	Line-segment constructor
 */
inline LineEq::LineEq( const Vector2 & v0, const Vector2 & v1,
	bool needToNormalise )
{
	normal_.x = v0.y - v1.y;
	normal_.y = v1.x - v0.x;
	if (needToNormalise) normal_.normalise();

	d_ = normal_.dotProduct( v0 );
}



/**
 *	This method intersects this line with the other, returning the
 *	distance along the line that the intersection occurs.
 */
inline float LineEq::intersect( const LineEq & other ) const
{
	const Vector2 & on = other.normal_;
	return (other.d_ - d_ * on.dotProduct( normal_ ) ) /
		on.crossProduct( normal_ );
}


/**
 *	Return the point that corresponds tot he given parameter
 */
inline Vector2 LineEq::param( float t ) const
{
	return Vector2(
		normal_.x * d_ + normal_.y * t,
		normal_.y * d_ - normal_.x * t );
}


/**
 *	Return the parameter of the given point projected onto the line
 */
inline float LineEq::project( const Vector2 & point ) const
{
	Vector2 dir( normal_.y, -normal_.x );	// also unit length
	return dir.dotProduct( point - normal_ * d_ );
}


/**
 *	Returns whether or not the given line is a minimum cutter of our line.
 *
 *	A minimum cutter means that if the intersection is greater than the
 *	minimum, this intersection forms the new minimum.
 */
inline bool LineEq::isMinCutter( const LineEq & cutter ) const
{
	return normal_.crossProduct( cutter.normal_ ) > 0.f;
}


/**
 *	Returns whether or not this line is parallel with the other
 */
inline bool LineEq::isParallel( const LineEq & other ) const
{
	return fabs( normal_.crossProduct( other.normal_ ) ) < 0.001f;
}


/**
 *	Returns whether or not the given point is in front of the line
 */
inline bool LineEq::isInFrontOf( const Vector2 & point ) const
{
	return normal_.dotProduct( point ) > d_;
}




// lineeq.ipp
