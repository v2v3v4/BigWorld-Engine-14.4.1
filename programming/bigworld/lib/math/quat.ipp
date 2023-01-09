
#ifdef CODE_INLINE
#define INLINE inline
#else
/// INLINE macro
#define INLINE
#endif



/**
 *	This method sets the values of this quaternion to the input values.
 */
INLINE
void Quaternion::set( const Vector3 &v, float _w )
{
	this->x	    = v.x;
	this->y     = v.y;
	this->z     = v.z;
	this->w		= _w;
}

/**
 *	The default constructor.
 */
INLINE
Quaternion::Quaternion()
{
}


/**
 *	This constructor sets up this quaternion based on the input matrix.
 *
 *	@see fromMatrix
 */
INLINE
Quaternion::Quaternion( const Matrix &m )
{
	this->fromMatrix( m );
}


/**
 *	This constructor sets up this quaternion based on the input values.
 */
INLINE
Quaternion::Quaternion( float x, float y, float z, float w ) :
    QuaternionBase( x, y, z, w )
{
}


/**
 *	This constructor sets up this quaternion based on the input values.
 */
INLINE
Quaternion::Quaternion( const Vector3 &v, float w )
{
	this->set( v, w );
}


/**
 *	This method sets all values of this quaternion to 0.
 */
INLINE
void Quaternion::setZero()
{
	x = y = z = w = 0.0f;
}


/**
 *	This method sets the values of this quaternion to the input values.
 */
INLINE
void Quaternion::set( float _x, float _y, float _z, float _w )
{
	this->x	    = _x;
	this->y     = _y;
	this->z     = _z;
	this->w		= _w;
}

/**
 *	This method pre-multiplies this quaternion by the input quaternion. This
 *	quaternion is set to the result.
 *
 *	@see multiply
 */
INLINE
void Quaternion::preMultiply( const Quaternion& q )
{
	this->multiply( q, *this );
}


/**
 *	This method post-multiplies this quaternion by the input quaternion. This
 *	quaternion is set to the result.
 *
 *	@see multiply
 */
INLINE
void Quaternion::postMultiply( const Quaternion& q )
{
	this->multiply( *this, q );
}

/**
 *	This method returns the dot product of this quaternion with the input
 *	quaternion.
 *
 *	@param q	The quaternion to perform the dot product with.
 *
 *	@return	The dot product of this quaternion.
 */
INLINE
float Quaternion::dotProduct( const Quaternion& q ) const
{
#ifdef EXT_MATH
	return XPQuaternionDot( this, &q );
#else
	Vector4 v1( x, y, z, w );
	Vector4 v2( q.x, q.y, q.z, q.w );
	return v1.dotProduct( v2 );
#endif
}

/**
 * This function returns the length of this quaternion.
 */
INLINE
float Quaternion::length() const
{
	Vector4 vec4(x, y, z, w);
	return vec4.length();
}

/**
 * This function returns the squared length of this quaternion.
 */
INLINE
float Quaternion::lengthSquared() const
{
	Vector4 vec4(x, y, z, w);
	return vec4.lengthSquared();
}

/**
 *	This function implements a shorthand method of multiplying two quaternions.
 */
INLINE
Quaternion operator *( const Quaternion& q1, const Quaternion& q2 )
{
	Quaternion q;
	q.multiply( q1, q2 );
	return q;
}


/**
 *	This method returns whether or not two quaternions are equal. Two
 *	quaternions are equal if all of their elements are equal.
 */
INLINE
bool operator ==( const Quaternion& q1, const Quaternion& q2 )
{
	return ( almostEqual( q1.x, q2.x ) && 
	         almostEqual( q1.y, q2.y ) &&
	         almostEqual( q1.z, q2.z ) &&
	         almostEqual( q1.w, q2.w ) );
}

/**
 *	This method makes the w member of this quaternion non-negative.
 */
INLINE
void Quaternion::minimise()
{
	if (w < 0)
	{
		x = -x;
		y = -y;
		z = -z;
		w = -w;
	}
}

// -----------------------------------------------------------------------------
// Section: DirectX / XG
// -----------------------------------------------------------------------------

#ifdef EXT_MATH
/**
 *	This method slerps from qStart when t = 0 to qEnd when t = 1. It is safe for
 *	the result quaternion (i.e. 'this') to be one of the parameters.
 *
 *	@param qStart	The quaternion to start from.
 *	@param qEnd		The quaternion to end at from.
 *	@param t		A value in the range [0, 1] that indicates the fraction to
 *					slerp between to to input quaternions.
 */
INLINE
void Quaternion::slerp( const Quaternion &qStart, const Quaternion &qEnd,
		float t )
{
	XPQuaternionSlerp( this, &qStart, &qEnd, t );
}


/**
 *	This method initialises this quaternion base on the input angle and axis.
 *	The input parameters describe a rotation in 3 dimensions.
 *
 *	@param angle	The angle in radians to rotate around the input axis.
 *	@param axis		The axis to rotate around.
 */
INLINE
void Quaternion::fromAngleAxis( float angle, const Vector3 &axis )
{
	XPQuaternionRotationAxis( this, &axis, angle );
}


/**
 *	This method initialises this quaternion based on the rotation in the input
 *	matrix. The translation present in the matrix is not used.
 *
 *	@param m	The matrix to set this quaternion from.
 */
INLINE
void Quaternion::fromMatrix( const Matrix &m )
{
	XPQuaternionRotationMatrix( this, &m );
}


/**
 *	This method sets this quaternion to be the rotation that would result by
 *	applying the rotation of q1 and then the rotation of q2.
 */
INLINE
void Quaternion::multiply( const Quaternion& q1, const Quaternion& q2 )
{
	XPQuaternionMultiply( this, &q1, &q2 );
}


/**
 *	This method normalises this quaternion. That is, it makes sure that the
 *	quaternion has a length of 1.
 */
INLINE
void Quaternion::normalise()
{
	XPQuaternionNormalize( this, this );
}


/**
 *	This method inverts this quaternion.
 */
INLINE
void Quaternion::invert()
{
	XPQuaternionInverse( this, this );
}

#endif // EXT_MATH

/*quat.ipp*/
