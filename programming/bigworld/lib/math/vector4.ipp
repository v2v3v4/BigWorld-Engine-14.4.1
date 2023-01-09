/**
 *	@file
 *
 *	@ingroup Math
 */

#ifdef CODE_INLINE
    #define INLINE    inline
#else
	/// INLINE macro.
    #define INLINE
#endif

/**
 *	This constructor does not initialise the components of the vector for
 *	performance reasons. To create a zero vector, copy Vector4::s_zero or use
 *	Vector4::setZero.
 */
INLINE
Vector4::Vector4()
{
}


/**
 *	This constructor sets the elements of the vector to the input values.
 *
 *	@param a	The value that element 0 is set to.
 *	@param b	The value that element 1 is set to.
 *	@param c	The value that element 2 is set to.
 *	@param d	The value that element 3 is set to.
 */
INLINE
Vector4::Vector4( float a, float b, float c, float d ) :
	Vector4Base( a, b, c, d )
{
}


/**
 *	This method constructs this Vector4 from Vector4Base. If we are using
 *	DirectX, this is D3DXVECTOR4.
 */
INLINE
Vector4::Vector4( const Vector4Base & v4 )
{
	*static_cast< Vector4Base * >( this ) = v4;
}


/**
 *	This method constructs this Vector4 from a Vector3 and an additional float.
 */
INLINE
Vector4::Vector4( const Vector3 & v, float w ) :
	Vector4Base( v.x, v.y, v.z, w )
{
}


/**
 *	This method constructs this Vector4 from an SSE register
 */
#ifdef SSE_MATH3
INLINE
Vector4::Vector4( __m128 v4 )
{
	_mm_storeu_ps( &x, v4 );
}
#endif

	/**
 *	This method sets all elements of the vector to 0.
 */
INLINE
void Vector4::setZero()
{
#ifdef SSE_MATH3
	_mm_storeu_ps( &x, _mm_setzero_ps() );
#else
	x = 0.f;
	y = 0.f;
	z = 0.f;
	w = 0.f;
#endif
}


/**
 *	This method sets the elements of the vector to the input values.
 *
 *	@param a	The value that element 0 is set to.
 *	@param b	The value that element 1 is set to.
 *	@param c	The value that element 2 is set to.
 *	@param d	The value that element 3 is set to.
 */
INLINE
void Vector4::set( float a, float b, float c, float d )
{
	x = a;
	y = b;
	z = c;
	w = d;
};


/**
 *	This method sets this vector to the input vector scaled by the input float.
 */
INLINE
void Vector4::scale( const Vector4& v, float s )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_mul_ps( _mm_loadu_ps( &v.x ), _mm_set1_ps( s ) );
	_mm_storeu_ps( &x, v4 );
#else
	x = v.x * s;
	y = v.y * s;
	z = v.z * s;
	w = v.w * s;
#endif
};


/**
 *	This method returns the squared length of this vector.
 */
INLINE
float Vector4::lengthSquared() const
{
#ifdef EXT_MATH
	return XPVec4LengthSq( this );
#else
    return (x * x) + (y * y) + (z * z) + (w * w);
#endif
}


/**
 *	This method returns the length of this vector.
 */
INLINE
float Vector4::length() const
{
#ifdef EXT_MATH
	return XPVec4Length( this );
#else

#ifdef __BORLANDC__
	return sqrt( this->lengthSquared() );
#else
	return sqrtf( this->lengthSquared() );
#endif

#endif
}


/**
 *	This method normalises this vector. That is, the direction of the vector
 *	will stay the same and its length will become 1.
 */
INLINE
void Vector4::normalise()
{
#ifdef EXT_MATH
	XPVec4Normalize( this, this );
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
Vector4 Vector4::unitVector() const
{
	Vector4 result( *this );
	result.normalise();
	return result;
}


/**
 *	This method returns the outcode associated with this vector.
 */
INLINE
Outcode Vector4::calculateOutcode() const
{
	Outcode oc=0;

	if( x < ( -w ) )
		oc |= OUTCODE_LEFT;

	if( x > w )
		oc |= OUTCODE_RIGHT;

	if( y < ( -w ) )
		oc |= OUTCODE_BOTTOM;

	if( y > w )
		oc |= OUTCODE_TOP;

	if( z < 0 )
		oc |= OUTCODE_NEAR;

	if( z > w )
		oc |= OUTCODE_FAR;

	return oc;
}


/**
 *	This method adds the input vector to this vector.
 */
INLINE
void Vector4::operator +=( const Vector4& v )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_add_ps( _mm_loadu_ps( &x ), _mm_loadu_ps( &v.x ) );
	_mm_storeu_ps( &x, v4 );
#else
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
#endif
}


/**
 *	This method subtracts the input vector from this vector.
 */
INLINE
void Vector4::operator -=( const Vector4& v )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_sub_ps( _mm_loadu_ps( &x ), _mm_loadu_ps( &v.x ) );
	_mm_storeu_ps( &x, v4 );
#else
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
#endif
}


/**
 *	This method multiplies this vector by the input vector,
 *	treating each component independently (like for + and -)
 */
INLINE
void Vector4::operator *=( const Vector4& v )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_mul_ps( _mm_loadu_ps( &x ), _mm_loadu_ps( &v.x ) );
	_mm_storeu_ps( &x, v4 );
#else
	x *= v.x;
	y *= v.y;
	z *= v.z;
	w *= v.w;
#endif
}

/**
 *	This method divides this vector by the input vector,
 *	treating each component independently (like for + and -)
 */
INLINE
void Vector4::operator /=( const Vector4& v )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_div_ps( _mm_loadu_ps( &x ), _mm_loadu_ps( &v.x ) );
	_mm_storeu_ps( &x, v4 );
#else
	x /= v.x;
	y /= v.y;
	z /= v.z;
	w /= v.w;
#endif
}


/**
 *	This method scales this vector by the input value.
 */
INLINE
void Vector4::operator *=( float s )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_mul_ps( _mm_loadu_ps( &x ), _mm_set1_ps( s ) );
	_mm_storeu_ps( &x, v4 );
#else
	x *= s;
	y *= s;
	z *= s;
	w *= s;
#endif
}


/**
 *	This method returns the dot product of this vector and the input vector.
 *
 *	@param v	The vector to perform the dot product with.
 *
 *	@return The dot product of this vector and the input vector.
 */
INLINE
float Vector4::dotProduct( const Vector4& v ) const
{
#ifdef EXT_MATH
	return XPVec4Dot( this, & v );
#else
	return ( v.x * x ) + ( v.y * y ) + ( v.z * z ) + ( v.w * w );
#endif
}


/**
 *	This method multiplies the elements of this vector by the
 *	corresponding elements of the input vector.
 *
 *	@relates Vector4
 */
INLINE
void Vector4::parallelMultiply( const Vector4& v )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_mul_ps( _mm_loadu_ps( &x ), _mm_loadu_ps( &v.x ) );
	_mm_storeu_ps( &x, v4 );
#else
	x *= v.x, y *= v.y, z *= v.z, w *= v.w;
#endif
}



/**
 *	This function returns the sum of the two input vectors.
 */
INLINE
Vector4 operator +( const Vector4& v1, const Vector4& v2 )
{
#ifdef SSE_MATH3
	return Vector4( _mm_add_ps( _mm_loadu_ps( &v1.x ), _mm_loadu_ps( &v2.x ) ) );
#else
	return Vector4( v1.x+v2.x, v1.y+v2.y, v1.z+v2.z, v1.w+v2.w );
#endif
}


/**
 *	This function returns the result of subtracting v2 from v1.
 */
INLINE
Vector4 operator -( const Vector4& v1, const Vector4& v2 )
{
#ifdef SSE_MATH3
	return Vector4( _mm_sub_ps( _mm_loadu_ps( &v1.x ), _mm_loadu_ps( &v2.x ) ) );
#else
	return Vector4( v1.x-v2.x, v1.y-v2.y, v1.z-v2.z, v1.w-v2.w );
#endif
}


/**
 *	This function returns the result of multiplying v1 and v2,
 *	treating each component independently (like for + and -)
 */
INLINE
Vector4 operator *( const Vector4& v1, const Vector4& v2 )
{
#ifdef SSE_MATH3
	return Vector4( _mm_mul_ps( _mm_loadu_ps( &v1.x ), _mm_loadu_ps( &v2.x ) ) );
#else
	return Vector4( v1.x*v2.x, v1.y*v2.y, v1.z*v2.z, v1.w*v2.w );
#endif
}


/**
 *	This function returns the result of dividing v1 by v2,
 *	treating each component independently (like for + and -)
 */
INLINE
Vector4 operator /( const Vector4& v1, const Vector4& v2 )
{
#ifdef SSE_MATH3
	return Vector4( _mm_div_ps( _mm_loadu_ps( &v1.x ), _mm_loadu_ps( &v2.x ) ) );
#else
	return Vector4( v1.x/v2.x, v1.y/v2.y, v1.z/v2.z, v1.w/v2.w );
#endif
}


/**
 *	This function returns the input vector scaled by the input float.
 */
INLINE
Vector4 operator *( const Vector4& v, float s )
{
#ifdef SSE_MATH3
	return Vector4( _mm_mul_ps( _mm_loadu_ps( &v.x ), _mm_set1_ps( s ) ) );
#else
	return Vector4( v.x * s, v.y * s, v.z * s, v.w * s );
#endif
}


/**
 *	This function returns the input vector scaled by the input float.
 */
INLINE
Vector4 operator *( float s, const Vector4& v )
{
#ifdef SSE_MATH3
	return Vector4( _mm_mul_ps( _mm_loadu_ps( &v.x ), _mm_set1_ps( s ) ) );
#else
	return Vector4( s * v.x, s * v.y, s * v.z, s * v.w );
#endif
}


/**
 *	This function returns the input vector scaled down by the input float.
 *
 *	@relates Vector4
 */
INLINE
Vector4 operator /( const Vector4& v, float s )
{
#ifdef SSE_MATH3
	return Vector4( _mm_div_ps( _mm_loadu_ps( &v.x ), _mm_set1_ps( s ) ) );
#else
	float oos = 1.f / s;
	return Vector4( v.x * oos, v.y * oos, v.z * oos, v.w * oos );
#endif
}


/**
 *	This function returns whether or not two vectors are equal. Two vectors are
 *	considered equal if all of their corresponding elements are equal.
 *
 *	@return True if the input vectors are equal, otherwise false.
 */
INLINE
bool operator ==( const Vector4& v1, const Vector4& v2 )
{
	return isEqual( v1.x, v2.x ) &&
		isEqual( v1.y, v2.y ) &&
		isEqual( v1.z, v2.z ) &&
		isEqual( v1.w, v2.w );
}


/**
 *	This function returns whether or not two vectors are not equal. Two vectors
 *	are considered equal if all of their corresponding elements are equal.
 *
 *	@return True if the input vectors are not equal, otherwise false.
 */
INLINE
bool operator !=( const Vector4& v1, const Vector4& v2 )
{
	return !(isEqual( v1.x, v2.x ) &&
		isEqual( v1.y, v2.y ) &&
		isEqual( v1.z, v2.z ) &&
		isEqual( v1.w, v2.w ));
}


/**
 *	This function returns whether or not the vector on the left is less than
 *	the vector on the right. A vector is considered less than another if
 *	its x element is less than the other. Or if the x elements are equal,
 *	then the y elements are compared, and so on.
 *
 *	@return True if the input vectors are not equal, otherwise false.
 */
INLINE bool operator < ( const Vector4& v1, const Vector4& v2 )
{
	if (v1.x < v2.x) return true;
	if (v1.x > v2.x) return false;
	if (v1.y < v2.y) return true;
	if (v1.y > v2.y) return false;
	if (v1.z < v2.z) return true;
	if (v1.z > v2.z) return false;
	return (v1.w < v2.w);
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
 *	@note <i>i</i> must be either 0, 1, 2 or 3.
 */
INLINE
float& Vector4::operator[]( int i )
{
	MF_ASSERT_DEBUG( i >= 0 && i < 4 );

	return this->operator float *()[ i ];
}


/**
 *	This method returns an element of the vector corresponding to the input
 *	index. The first element has index 0.
 *
 *	@param i	The index of the element to return.
 *
 *	@return		The value at the input index.
 *
 *	@note <i>i</i> must be either 0, 1, 2 or 3.
 */
INLINE
const float Vector4::operator[]( int i ) const
{
	MF_ASSERT_DEBUG( i >= 0 && i < 4 );

	return this->operator const float *()[ i ];
}
#endif // __BORLANDC__


/**
 *	This method scales this vector by the input amount.
 */
INLINE
void Vector4::scale( float s )
{
#ifdef SSE_MATH3
	__m128 v4 = _mm_mul_ps( _mm_loadu_ps( &x ), _mm_set1_ps( s ) );
	_mm_storeu_ps( &x, v4 );
#else
	this->x *= s;
	this->y *= s;
	this->z *= s;
	this->w *= s;
#endif
}

/**
 *	This method compares two Vector4s.
 */
 INLINE
 bool almostEqual(const Vector4 &v1, const Vector4 &v2, float epsilon)
 {
 	return 
 		almostEqual( v1.x, v2.x, epsilon ) &&
		almostEqual( v1.y, v2.y, epsilon ) &&
		almostEqual( v1.z, v2.z, epsilon ) &&
		almostEqual( v1.w, v2.w, epsilon );
 }

// vector4.ipp
