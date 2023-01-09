// lineeq3.ipp

/**
 *	Default constructor
 */
inline LineEq3::LineEq3() :
	point_( Vector3(0.0f, 0.0f, 0.0f) ),
	direction_( Vector3(1.0f, 0.0f, 0.0f) ),
	isValid_( false )
{
}


/**
 *	Constructor that takes two points in space to define the line.
 *	The points should not be coincident
 */
inline LineEq3::LineEq3( const Vector3& point1, const Vector3& point2 ) :
		point_( point1 ), direction_( point2 - point1 ), isValid_( true )
{
	if ( direction_.length() == 0.0f )
	{
		isValid_ = false;
		direction_ = Vector3(1.0f, 0.0f, 0.0f);
		ERROR_MSG( "LineEq3::LineEq3 - The points defining the line are coincident!" );
	}
}


inline bool LineEq3::isValid() const
{
	return isValid_;
}


inline void LineEq3::isValid( bool isValid )
{
	isValid_ = isValid;
}


inline float LineEq3::perpDistance( const Vector3& point ) const
{
	Vector3 cp = direction_.crossProduct(point_ - point);
	return cp.length() / direction_.length();
}


// lineeq3.ipp
