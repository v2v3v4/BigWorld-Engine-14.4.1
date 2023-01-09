/**
 *	@file
 *	@ingroup Math
 */

// -----------------------------------------------------------------------------
// Constructors and Destructor.
// -----------------------------------------------------------------------------

/**
 *	Default constructor for Angle. The angle is initialised to 0.
 */
inline Angle::Angle() : angle_( 0.f )
{
}

/**
 *	Float constructor for Angle.
 *
 *	@param	valueInRadians		Value of angle in radians.
 */
inline Angle::Angle( float valueInRadians )
{
	angle_ = Angle::normalise( valueInRadians );
}

/**
 *	Destructor.
 */
inline Angle::~Angle()
{
}


// -----------------------------------------------------------------------------
// Assignment Operators.
// -----------------------------------------------------------------------------

/**
 *	Operator = from Angle to Angle.
 *
 *	@param	rhs		The angle whose value we want assigned to the current
 *					angle.
 *
 *	@return	The Angle assigned.
 */
inline const Angle &Angle::operator = ( const Angle &rhs )
{
	angle_ = rhs.angle_;
	return *this;
}

/**
 *	Operator = from float to Angle.
 *
 *	@param	valueInRadians	The float whose value we want assigned to
 *                          the current angle.
 *
 *	@return	The Angle assigned.
 */
inline const Angle &Angle::operator = ( float valueInRadians )
{
	angle_ = Angle::normalise( valueInRadians );
	return *this;
}


// -----------------------------------------------------------------------------
// Unary Arithmetic Operators.
// -----------------------------------------------------------------------------

/**
 *	Operator += where the rhs is a float.
 *
 *	@param	rhs		The angle to be added to the current Angle.
 *
 *	@return	The value of the Angle after the addition.
 */
inline const Angle &Angle::operator += ( float rhs )
{
	angle_ = Angle::normalise( angle_ + rhs );
	return *this;
}


/**
 *	Operator -= where the rhs is a float.
 *
 *	@param	rhs		The angle to be subtracted from the current Angle.
 *
 *	@return	The value of the Angle after the subtraction.
 */
inline const Angle &Angle::operator -= ( float rhs )
{
	angle_ = Angle::normalise( angle_ - rhs );
	return *this;
}


// -----------------------------------------------------------------------------
// Casting Operators.
// -----------------------------------------------------------------------------

/**
 *	Casting operator for float.
 */
inline Angle::operator float () const
{
	return angle_;
}


// -----------------------------------------------------------------------------
// Static Helper Methods.
// -----------------------------------------------------------------------------

/**
 *	This method returns the normalised value between MATH_PI and minus MATH_PI.
 *
 *	@param	value		The value to be normalised.
 *
 *	@return	The value as an angle normalised between MATH_PI and minus MATH_PI.
 */
inline float Angle::normalise( float value )
{
	const float TWO_PI = (MATH_PI * 2.0f);
	const float THREE_PI = (MATH_PI * 3.0f);

	if ( value > MATH_PI )
	{
		if ( value > THREE_PI )
		{
			return value -
				(float) floor( (value + MATH_PI) / TWO_PI ) * TWO_PI;
		}
		else
		{
			return value - TWO_PI;
		}
	}
	else if ( value <= -MATH_PI )
	{
		if ( value <= -THREE_PI )
		{
			return value -
				(float) floor( (value + MATH_PI) / TWO_PI ) * TWO_PI;
		}
		else
		{
			return value + TWO_PI;
		}
	}
	else
	{
		return value;
	}
}

/**
 *	This method decays a value towards another value. The amount decayed is
 *	dependent on the half-life, which is the amount of time for the source value
 *	to move half-way towards the destination value.
 *
 *	@param	src			The original value to begin from.
 *	@param	dst			The final destination for the source value.
 *	@param	halfLife	The time taken for the source to reach half of the
 *						remaining distance.
 *	@param	dTime		Amount of time passed in seconds.
 *
 *	@return	The new value of the source.
 */
inline float Angle::decay( float src, float dst, float halfLife, float dTime )
{
	dst = Angle::sameSignAngle( src, dst );

	if ( halfLife <= 0.0f )
	{
		return dst;
	}
	else
	{
		return dst + (float) pow( 0.5f, dTime/halfLife ) *
			   ( src - dst );
	}
}

/**
 *	This method returns the closest angle to the given angle that is the same
 *	sign as the given angle. For example, if -175 was compared to 175, the
 *	closest angle returned would be -185.
 *
 *	@param	angle		The angle to be compared to.
 *	@param	closest		The angle we wish to test to get a closest angle to
 *						the previous angle.
 *
 *	@return	The closest same-signed angle.
 */
inline float Angle::sameSignAngle( float angle, float closest )
{
	if ( closest > angle + MATH_PI )
	{
		return closest - 2.0f * MATH_PI;
	}
	else if ( closest < angle - MATH_PI )
	{
		return closest + 2.0f * MATH_PI;
	}
	else
	{
		return closest;
	}
}


/**
 *	This method clamps this angle to be between the two input angles. The range
 *	is from the minimum angle to the maximum angle in the positive angle
 *	direction.
 *
 *	@return True if the value was clamped, otherwise false.
 */
inline bool Angle::clampBetween( Angle min, Angle max )
{
	float diff  = Angle::turnRange( min, *this );
	float range = Angle::turnRange( min, max );

	if (diff > range)
	{
		angle_ = (diff - range/2.f < MATH_PI) ? max : min;
		return true;
	}

	return false;
}


/**
 *	This method whether or not this angle to be between the two input angles.
 *	The range is from the minimum angle to the maximum angle in the positive
 *	angle direction.
 *
 *	@return True if in the range, otherwise false.
 */
inline bool Angle::isBetween( Angle min, Angle max ) const
{
	float diff  = Angle::turnRange( min, *this );
	float range = Angle::turnRange( min, max );

	return (diff <= range);
}


/**
 *	This method sets the angle to be the weighted blend across the shortest arc of the given angles.
 *
 *   alpha = 0.0    result = a
 *   alpha = 0.5    result = (a+b)/2 
 *   alpha = 1.0    result = b
 *
 *	@param a
 *	@param b
 *	@param alpha
 */
inline void Angle::lerp( Angle a, Angle b, float alpha )
{
	*this = ( sameSignAngle( a, b ) - a) * alpha + a;
}


/**
 *	This static helper returns the angle to turn in the positive direction to
 *	get from one angle to the other.
 *
 *	@param from	The angle to start from.
 *	@param to	The angle to finish at.
 *
 *	@return A float in the range [ 0, 2 * pi ).
 */
inline float Angle::turnRange( Angle from, Angle to )
{
	float diff = to - from;

	return (diff < 0.f) ? diff + MATH_2PI : diff;
}

// -----------------------------------------------------------------------------
// Extractors and Insertors.
// -----------------------------------------------------------------------------

/**
 *	This function implements the standard IO stream insertor for Angle.
 *
 *	@param	output		The ostream that is written to.
 *	@param	angle		The angle to write.
 *
 *	@return	The ostream that was written to.
 *
 *	@relates Angle
 */
inline std::ostream & operator << ( std::ostream &output, const Angle &angle )
{
	output << angle.angle_;
	return output;
}


/**
 *	This function implements the standard IO stream extractor for Angle.
 *
 *	@param	input		The istream that is read.
 *	@param	angle		The angle to write to.
 *
 *	@return	The istream that was read.
 *
 *	@relates Angle
 */
inline std::istream & operator >> ( std::istream &input, Angle &angle )
{
	input >> angle.angle_;
	return input;
}

/* angle.ipp */
