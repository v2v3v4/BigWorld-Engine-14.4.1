/**
 *	@file
 *
 *	@ingroup Math
 */


/**
 *	This constructor does not initialise the components of the vector for
 *	performance reasons. To create a zero vector, copy Vector2::s_zero or use
 *	Vector2::setZero.
 */
INLINE
Vector2::Vector2()
{
}


/**
 *	This constructor sets the elements of the vector to the input values.
 *
 *	@param a	The value that element 0 is set to.
 *	@param b	The value that element 1 is set to.
 */
INLINE
Vector2::Vector2( float a, float b ) : Vector2Base( a, b )
{
}


/**
 *	This method constructs this Vector2 from Vector2Base. If we are using
 *	DirectX, this is D3DXVECTOR2.
 */
INLINE
Vector2::Vector2( const Vector2Base & v2 )
{
	*static_cast< Vector2Base * >( this ) = v2;
}


/**
 *	This method sets all elements of the vector to 0.
 */
INLINE
void Vector2::setZero()
{
	x = 0.f;
	y = 0.f;
}


/**
 *	This method sets the elements of the vector to the input values.
 *
 *	@param a	The value that element 0 is set to.
 *	@param b	The value that element 1 is set to.
 */
INLINE
void Vector2::set( float a, float b )
{
	x = a;
	y = b;
};


/**
 *	This method sets this vector to the input vector scaled by the input float.
 */
INLINE
void Vector2::scale( const Vector2& v, float s )
{
	this->x = v.x * s;
	this->y = v.y * s;
};


/**
 *	This method returns the squared length of this vector.
 */
INLINE
float Vector2::lengthSquared() const
{
#ifdef EXT_MATH
	return XPVec2LengthSq( this );
#else
	return x*x + y*y;
#endif
}


/**
 *	This method returns the length of this vector.
 */
INLINE
float Vector2::length() const
{
#ifdef EXT_MATH
	return XPVec2Length( this );
#else
    float lengthSquared = this->lengthSquared();

    return (lengthSquared > 0) ?  sqrtf( lengthSquared ) : 0.f;
#endif
}


/**
 *	This method normalises this vector. That is, the direction of the vector
 *	will stay the same and its length will become 1.
 */
INLINE
void Vector2::normalise()
{
#ifdef EXT_MATH
	XPVec2Normalize( this, this );
#else
	const float length = this->length();

	if (!almostZero( length, 0.00000001f ))
	{
		float rcp = 1.f / length;
		*this *= rcp;
	}
#endif
}


/**
 *	This function returns a copy of this vector that has been normalised to
 *	unit length.
 *
 *	@return		A copy of this vector normalised to unit length.
 */
INLINE
Vector2 Vector2::unitVector() const
{
	Vector2 result( *this );
	result.normalise();
	return result;
}


#ifdef __BORLANDC__
/**
 *	This method returns an element of the vector corresponding to the input
 *	index. The first element has index 0.
 *
 *	@param i	The index of the element to return.
 *
 *	@return		The value at the input index.
 *
 *	@note <i>i</i> must be either 0 or 1.
 */
INLINE
float& Vector2::operator[]( int i )
{
	MF_ASSERT_DEBUG( i >= 0 && i < 2 );

	return  this->operator float *()[ i ];
}


/**
 *	This method returns an element of the vector corresponding to the input
 *	index. The first element has index 0.
 *
 *	@param i	The index of the element to return.
 *
 *	@return		The value at the input index.
 *
 *	@note <i>i</i> must be either 0 or 1.
 */
INLINE
float Vector2::operator[]( int i ) const
{
	MF_ASSERT_DEBUG( i >= 0 && i < 2 );

	return  this->operator const float *()[ i ];
}
#endif


/**
 *	This method adds the input vector to this vector.
 */
INLINE
void Vector2::operator +=( const Vector2& v )
{
	this->x += v.x;
	this->y += v.y;
}


/**
 *	This method subtracts the input vector from this vector.
 */
INLINE
void Vector2::operator -=( const Vector2& v )
{
	this->x -= v.x;
	this->y -= v.y;
}


/**
 *	This method scales this vector by the input value.
 */
INLINE
void Vector2::operator *=( float s )
{
	this->x *= s;
	this->y *= s;
}


/**
 *	This method divides the vector by the input value.
 */
INLINE
void Vector2::operator /=( float s )
{
	float div = 1.f/s;
	this->x *= div;
	this->y *= div;
}


/**
 *	This method returns the dot product of this vector and the input vector.
 *
 *	@param v	The vector to perform the dot product with.
 *
 *	@return The dot product of this vector and the input vector.
 */
INLINE
float Vector2::dotProduct( const Vector2& v ) const
{
#ifdef EXT_MATH
	return XPVec2Dot( this, &v );
#else
	return this->x * v.x + this->y * v.y;
#endif
}


/**
 *	This method returns the cross product of this vector and the input vector.
 *
 *	@param v	The vector to perform the cross product with.
 *
 *	@return The cross product of this vector and the input vector.
 */
INLINE
float Vector2::crossProduct( const Vector2& v ) const
{
	return this->x * v.y - this->y * v.x;
}


/**
 *	This method sets this vector to vector1 projected onto vector2.
 */
INLINE
void Vector2::projectOnto( const Vector2& v1, const Vector2& v2 )
{
	*this = v2 * ( v1.dotProduct( v2 ) / v2.lengthSquared() );
}


/**
 *	This method returns this vector projected onto the input vector.
 */
INLINE
Vector2 Vector2::projectOnto(const Vector2 & v) const
{
	Vector2 result;
	result.projectOnto( *this, v );

	return result;
}


/**
 *	This function returns the sum of the two input vectors.
 */
INLINE
Vector2 operator +( const Vector2& v1, const Vector2& v2 )
{
	return Vector2( v1.x+v2.x, v1.y+v2.y );
}


/**
 *	This function returns the result of subtracting v2 from v1.
 */
INLINE
Vector2 operator -( const Vector2& v1, const Vector2& v2 )
{
	return Vector2( v1.x-v2.x, v1.y-v2.y );
}


/**
 *	This function returns the input vector scaled by the input float.
 */
INLINE
Vector2 operator *( const Vector2& v, float s )
{
	return Vector2( v.x * s, v.y * s );
}


/**
 *	This function returns the input vector scaled by the input float.
 */
INLINE
Vector2 operator *( float s, const Vector2& v )
{
	return Vector2( s * v.x, s * v.y );
}


/**
 *	This function return the input vector reduced in length by the input float.
 */
INLINE
Vector2 operator /( const Vector2& v, float s )
{
	return Vector2( v.x / s, v.y / s );
}


/**
 *	This method returns the cross product of the two input vectors.
 */
INLINE
Vector2 operator *( const Vector2& a, const Vector2& b )
{
	return Vector2( a.x * b.x, a.y * b.y );
}


/**
 *	This function returns whether or not two vectors are equal. Two vectors are
 *	considered equal if all of their corresponding elements are equal.
 *
 *	@return True if the input vectors are equal, otherwise false.
 */
INLINE
bool operator ==( const Vector2& v1, const Vector2& v2 )
{
	return isEqual( v1.x, v2.x ) && isEqual( v1.y, v2.y );
}


/**
 *	This function returns whether or not two vectors are not equal. Two vectors
 *	are considered equal if all of their corresponding elements are equal.
 *
 *	@return True if the input vectors are not equal, otherwise false.
 */
INLINE
bool operator   !=( const Vector2& v1, const Vector2& v2 )
{
	return !(isEqual( v1.x, v2.x ) && isEqual( v1.y, v2.y ));
}

/**
 *	This function returns whether or not the vector on the left is less than
 *	the vector on the right. A vector is considered less than another if
 *	its x element is less than the other. Or if the x elements are equal,
 *	then the y elements are compared.
 *
 *	@return True if the input vectors are not equal, otherwise false.
 */
INLINE bool operator < ( const Vector2& v1, const Vector2& v2 )
{
	if (v1.x < v2.x) return true;
	if (v1.x > v2.x) return false;
	return (v1.y < v2.y);
}

/**
 *	This function determines whether two Vector2's are almost the same.
 */
INLINE bool almostEqual( const Vector2& v1, const Vector2& v2, const float epsilon )
{
	return almostEqual(v1.x, v2.x, epsilon) && 
		almostEqual(v1.y, v2.y, epsilon);
}

// vector2.ipp
