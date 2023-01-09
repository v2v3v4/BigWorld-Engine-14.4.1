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
 *	This constructor creates a zero matrix. That is, a matrix whose elements are
 *	all 0.
 */
INLINE
Matrix::Matrix()
{
	this->setZero();
}

/**
 * This constructor creates a matrix with four Vector4s representing the rows
 * of the matrix.
 */
 INLINE
 Matrix::Matrix(	const Vector4& v0,
					const Vector4& v1,
					const Vector4& v2,
					const Vector4& v3 )
{
	row( 0, v0);
	row( 1, v1);
	row( 2, v2);
	row( 3, v3);					
}

/**
 *	This method sets this matrix to the zero matrix. The zero matrix is the
 *	matrix whose elements are all 0.
 */
INLINE
void Matrix::setZero()
{
	memset( this, 0, sizeof( *this ) );
}


/**
 *	This method returns the determinant of this matrix. For a non-scaling,
 *	non-skewing transform, this should be 1.
 */
INLINE
float Matrix::getDeterminant() const
{
#ifdef EXT_MATH
	return XPMatrixfDeterminant( this );
#else
	// TODO: This only considers the 3x3 part of the matrix.
	float det = 0;

	det += m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
	det -= m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
	det += m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

	return det;
#endif
}


/**
 *	This method implements an operator that can be used to access individual
 *	elements of this matrix.
 *
 *	@param column	The column number of the desired element. This value must be
 *					in the range [0, 3].
 *	@param row		The row number of the desired element. This value must be in
 *					the range [0, 3].
 *
 *	@return A reference to the specified element.
 */
INLINE
float& Matrix::operator ()( uint32 column, uint32 row )
{
	MF_ASSERT_DEBUG( column < 4 );
	MF_ASSERT_DEBUG( row	< 4 );

	return m[column][row];
}


/**
 *	This method implements an operator that can be used to access individual
 *	elements of this matrix.
 *
 *	@param column	The column number of the desired element. This value must be
 *					in the range [0, 3].
 *	@param row		The row number of the desired element. This value must be in
 *					the range [0, 3].
 *
 *	@return A copy of the specified element.
 */
INLINE
float Matrix::operator ()( uint32 column, uint32 row ) const
{
	MF_ASSERT_DEBUG( column < 4 );
	MF_ASSERT_DEBUG( row	< 4 );

	return m[column][row];
}


/**
 *	This method pre multiplies this matrix by the matrix that translates by the
 *	input amount.
 *
 *	@param v	The amount to translate by.
 */
INLINE
void Matrix::preTranslateBy(const Vector3 & v)
{
	(*this)[3] += this->applyVector(v);
}


/**
 *	This method post multiplies this matrix by the matrix that translates by the
 *	input amount.
 *
 *	@param v	The amount to translate by.
 */
INLINE
void Matrix::postTranslateBy(const Vector3 & v)
{
	(*this)[3] += v;
}

/**
 *	This method multiplies the point represented by the input vector with this
 *	matrix. The input vector is on the left of the multiplication. i.e. it
 *	produces vM, where v is the input point and M is this matrix.
 *
 *	@return The resulting point represented by a Vector3.
 */
INLINE
Vector3 Matrix::applyPoint( const Vector3& v2 ) const
{
	Vector4 res;
	this->applyPoint( res, v2 );
	return Vector3( res.x, res.y, res.z );
}


/**
 *	This method multiplies the point represented by the input v2 with this
 *	matrix. The input vector is on the left of the multiplication. i.e. it
 *	produces vM, where v1 is the input point and M is this matrix. The resulting
 *	point is placed in v1.
 *
 *	@param v1	The vector to place the resulting point in.
 *	@param v2	The vector representing the point to be transformed.
 */
INLINE
void Matrix::applyPoint( Vector3& v1, const Vector3& v2 ) const
{
	v1 = this->applyPoint( v2 );
}


/**
 *	This method applies this matrix to an input Vector3 and produces a resulting
 *	Vector4.
 *
 *	@param v1	The Vector4 that will be set to the result.
 *	@param v2	The Vector3 that the matrix will be applied to.
 */
INLINE
void Matrix::applyPoint( Vector4& v1, const Vector3& v2 ) const
{
#if defined( SSE_MATH3 )
	__m128 v = _mm_loadu_ps( &v2.x );

	__m128 mx = _mm_loadu_ps( m[0] );
	__m128 my = _mm_loadu_ps( m[1] );
	__m128 mz = _mm_loadu_ps( m[2] );
	__m128 mw = _mm_loadu_ps( m[3] );

	__m128 vx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128 vy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	__m128 vz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );

	vx = _mm_mul_ps( mx, vx );
	vy = _mm_mul_ps( my, vy );
	vz = _mm_mul_ps( mz, vz );

	__m128 va = _mm_add_ps( vx, vy );
	__m128 vb = _mm_add_ps( vz, mw );

	v = _mm_add_ps( va, vb );
	_mm_storeu_ps( &v1.x, v );

#elif defined( EXT_MATH )
	XPVec3Transform( &v1, &v2, this );
#else
	v1[0] = v2[0] * m[0][0] + v2[1] * m[1][0] + v2[2] * m[2][0] + m[3][0];
	v1[1] = v2[0] * m[0][1] + v2[1] * m[1][1] + v2[2] * m[2][1] + m[3][1];
	v1[2] = v2[0] * m[0][2] + v2[1] * m[1][2] + v2[2] * m[2][2] + m[3][2];
	v1[3] = v2[0] * m[0][3] + v2[1] * m[1][3] + v2[2] * m[2][3] + m[3][3];
#endif
}


/**
 *	This method applies this matrix to an input Vector4 and produces a resulting
 *	Vector4.
 *
 *	@param v1	The Vector4 that will be set to the result.
 *	@param v2	The Vector4 that the matrix will be applied to.
 */
INLINE
void Matrix::applyPoint( Vector4& v1, const Vector4& v2 ) const
{
#if defined( SSE_MATH3 )
	__m128 v = _mm_loadu_ps( &v2.x );

	__m128 mx = _mm_loadu_ps( m[0] );
	__m128 my = _mm_loadu_ps( m[1] );
	__m128 mz = _mm_loadu_ps( m[2] );
	__m128 mw = _mm_loadu_ps( m[3] );

	__m128 vx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128 vy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	__m128 vz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );
	__m128 vw = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 3, 3, 3, 3 ) );

	vx = _mm_mul_ps( mx, vx );
	vy = _mm_mul_ps( my, vy );
	vz = _mm_mul_ps( mz, vz );
	vw = _mm_mul_ps( mw, vw );

	__m128 va = _mm_add_ps( vx, vy );
	__m128 vb = _mm_add_ps( vz, vw );

	v = _mm_add_ps( va, vb );
	_mm_storeu_ps( &v1.x, v );

#elif defined( EXT_MATH )
	XPVec4Transform( &v1, &v2, this );
#else
	v1[0] = v2[0] * m[0][0] + v2[1] * m[1][0] + v2[2] * m[2][0] + v2[3] * m[3][0];
	v1[1] = v2[0] * m[0][1] + v2[1] * m[1][1] + v2[2] * m[2][1] + v2[3] * m[3][1];
	v1[2] = v2[0] * m[0][2] + v2[1] * m[1][2] + v2[2] * m[2][2] + v2[3] * m[3][2];
	v1[3] = v2[0] * m[0][3] + v2[1] * m[1][3] + v2[2] * m[2][3] + v2[3] * m[3][3];
#endif
}


/**
 *	This method applies the transform to a vector. It is the same as apply point
 *	except that it does not add the translation. The v is on the left of the
 *	multiplication. i.e. it produces vM, where M is this matrix.
 *
 *	@see applyPoint
 */
INLINE
Vector3 Matrix::applyVector( const Vector3& v2 ) const
{
#if defined( SSE_MATH3 )
	__m128 v = _mm_loadu_ps( &v2.x );

	__m128 mx = _mm_loadu_ps( m[0] );
	__m128 my = _mm_loadu_ps( m[1] );
	__m128 mz = _mm_loadu_ps( m[2] );

	__m128 vx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128 vy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	__m128 vz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );

	vx = _mm_mul_ps( mx, vx );
	vy = _mm_mul_ps( my, vy );
	vz = _mm_mul_ps( mz, vz );

	__m128 va = _mm_add_ps( vx, vy );

	v = _mm_add_ps( va, vz );
	return Vector3( v );

#elif defined( EXT_MATH )
	Vector3 vout;
	XPVec3TransformNormal( &vout, &v2, this );
	return vout;
#else
	return Vector3(
		v2.x * m[0][0] + v2.y * m[1][0] + v2.z * m[2][0],
		v2.x * m[0][1] + v2.y * m[1][1] + v2.z * m[2][1],
		v2.x * m[0][2] + v2.y * m[1][2] + v2.z * m[2][2]);
#endif
}

/**
 *	This method applies the transform to a vector. It is the same as apply point
 *	except that it does not add the translation. The v1 is on the left of the
 *	multiplication. i.e. it produces vM, where M is this matrix.
 *
 *	@param v1	The vector that is set to the resulting vector.
 *	@param v2	The vector that is to be transformed.
 *
 *	@see applyPoint
 */
INLINE
void Matrix::applyVector( Vector3& v1, const Vector3& v2 ) const
{
#if defined( SSE_MATH3 )
	__m128 v = _mm_loadu_ps( &v2.x );

	__m128 mx = _mm_loadu_ps( m[0] );
	__m128 my = _mm_loadu_ps( m[1] );
	__m128 mz = _mm_loadu_ps( m[2] );

	__m128 vx = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	__m128 vy = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	__m128 vz = _mm_shuffle_ps( v, v, _MM_SHUFFLE( 2, 2, 2, 2 ) );

	vx = _mm_mul_ps( mx, vx );
	vy = _mm_mul_ps( my, vy );
	vz = _mm_mul_ps( mz, vz );

	__m128 va = _mm_add_ps( vx, vy );

	vx = _mm_add_ps( va, vz );
	vy = _mm_shuffle_ps( vx, vx, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	vz = _mm_shuffle_ps( vx, vx, _MM_SHUFFLE( 2, 2, 2, 2 ) );

	_mm_store_ss( &v1.x, vx );
	_mm_store_ss( &v1.y, vy );
	_mm_store_ss( &v1.z, vz );

#elif defined( EXT_MATH )
	XPVec3TransformNormal( &v1, &v2, this );
#else
	v1.set(
		v2.x * m[0][0] + v2.y * m[1][0] + v2.z * m[2][0],
		v2.x * m[0][1] + v2.y * m[1][1] + v2.z * m[2][1],
		v2.x * m[0][2] + v2.y * m[1][2] + v2.z * m[2][2]);
#endif
}


/**
 *	This method returns the vector that would result from applying the transform
 *	to the unit vector along the input axis. This corresponds to one of the
 *	columns of the matrix.
 */
INLINE
const Vector3 & Matrix::applyToUnitAxisVector(int axis) const
{
	return *reinterpret_cast<const Vector3 *>( m[axis] );
}

/**
 *	This method returns the point that would result from applying the transform
 *	to the point at the origin. This is the same as the translation part of the
 *	matrix.
 */
INLINE
const Vector3 & Matrix::applyToOrigin() const
{
	return (*this)[3];
}

/**
 *	This method sets a row of the matrix as a Vector4. The rows are indexed
 *	from 0 to 3.
 *
 *	@param i		The index of the desired row.
 *	@param value	The values to set.
 */
INLINE
void Matrix::row( int i, const Vector4& value )
{
 	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
 	
	m[i][0] = value.x;
	m[i][1] = value.y;
	m[i][2] = value.z;
	m[i][3] = value.w;
}


/**
 *	This method returns a row of the matrix as a Vector4. The rows are indexed
 *	from 0 to 3.
 *
 *	@param i	The index of the desired row.
 *
 *	@return The row with the input index.
 */
INLINE
const Vector4 & Matrix::row( int i ) const
{
#ifndef __BORLANDC__
	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
#endif
	return *reinterpret_cast< const Vector4 * >( m[i] );
}


/**
 *	This method returns a column of the matrix as a Vector4.  The columns are
 *	indexed from 0 to 3.
 *
 *  @param i	The index of the desired column.
 *
 *  @return		The column with the input index.
 */
INLINE
Vector4 Matrix::column( int i ) const
{
	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
	return Vector4( m[0][i],  m[1][i], m[2][i], m[3][i] );
}


/**
 *	This method sets a column of the matrix as a Vector4.  The columns are
 *	indexed from 0 to 3.
 *
 *  @param i	The index of the desired column.
 *	@param v	The new value of the column.
 */
INLINE
void Matrix::column( int i, const Vector4 & v )
{
#ifndef __BORLANDC__
	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
#endif
	m[0][i] = v.x;
	m[1][i] = v.y;
	m[2][i] = v.z;
	m[3][i] = v.w;
}


/**
 *	This method returns a row of the matrix as a Vector3. The rows are indexed
 *	from 0 to 3.
 *
 *	@param i	The index of the desired row.
 *
 *	@return The row with the input index.
 */
INLINE
Vector3 & Matrix::operator []( int i )
{
	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
	return *reinterpret_cast<Vector3 *>( m[i] );
}


/**
 *	This method returns a row of the matrix as a Vector3. The rows are indexed
 *	from 0 to 3.
 *
 *	@param i	The index of the desired row.
 *
 *	@return The row with the input index.
 */
INLINE
const Vector3 & Matrix::operator []( int i ) const
{
	MF_ASSERT_DEBUG( 0 <= i && i < 4 );
	return *reinterpret_cast< const Vector3 * >( m[i] );
}


/**
 *	This method sets this matrix to a scaling matrix. It does not contain any
 *	rotation and translation.
 *
 *	@param	scale	The amount to scale in each coordinate.
 */
INLINE
void Matrix::setScale( const Vector3 & scale )
{
	this->setScale( scale.x, scale.y, scale.z );
}

/**
 *	This method sets this matrix to be a translation matrix. It contains no
 *	rotation or scaling.
 *
 *	@param pos	The amount to translate by.
 */
INLINE
void Matrix::setTranslate( const Vector3 & pos )
{
	this->setTranslate( pos.x, pos.y, pos.z );
}


/**
 *	This method inverts this matrix. It only works on matrices that are
 *	orthonormal. That is, the axis vectors must be orthogonal and have unit
 *	length. This matrix is set to the result.
 */
INLINE
void Matrix::invertOrthonormal()
{
	const Matrix m( *this );
	this->invertOrthonormal( m );
}


/**
 *	This method inverts this matrix. This matrix is set to the result.
 *
 *	@warning	The determinant of this matrix should not be 0.
 */
INLINE
bool Matrix::invert()
{
#ifdef EXT_MATH
	return (XPMatrixInverse( this, NULL, this ) != NULL);
#else
	const Matrix m( *this );
	return this->invert( m );
#endif
}


/**
 *	This method transposes this matrix
 */
INLINE
void Matrix::transpose()
{
#ifdef EXT_MATH
	XPMatrixTranspose( this, this );
#else
	const Matrix m( *this );
	this->transpose( m );
#endif
}


/**
 *	This method sets this matrix to a left-handed orthogonal projection matrix.
 */
INLINE
void Matrix::orthogonalProjection( float w, float h, float zn, float zf )
{
#ifdef EXT_MATH
	XPMatrixOrthoLH( this, w, h, zn, zf );
#else
	this->row(0, Vector4( 2.f/w,	0.f,	0.f,		0.f ) );
	this->row(1, Vector4( 0.f,	2.f/h,	0.f,			0.f ) );
	this->row(2, Vector4( 0.f,	0.f,	1.f/(zf-zn),	0.f ) );
	this->row(3, Vector4( 0.f,	0.f,	zn/(zn-zf),		1.f ) );	
#endif
}

/**
 *	This method sets this matrix to a left-handed perspective projection matrix.
 */
INLINE
void Matrix::perspectiveProjection( float fov, float aspectRatio,
	float nearPlane, float farPlane )
{
#ifdef EXT_MATH
	XPMatrixPerspectiveFovLH( this, fov, aspectRatio, nearPlane, farPlane );
#else
	float cot = 1 / (float)tan(fov * 0.5f);
	float rcp = 1 / (farPlane - nearPlane);

	m[0][0] = (cot / aspectRatio);
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;

	m[1][0] = 0;
	m[1][1] = cot;
	m[1][2] = 0;
	m[1][3] = 0;

	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = rcp * farPlane;
	m[2][3] = 1;

	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = - rcp  * farPlane * nearPlane;
	m[3][3] = 0;
#endif
}


/**
 *	This method sets the translation component of this matrix.
 */
INLINE
void Matrix::translation( const Vector3& v )
{
	(*this)[3] = v;
	m[3][3] = 1;
}

/**
 *	This method sets this matrix to the identity matrix.
 */
INLINE
void Matrix::setIdentity()
{
#ifdef EXT_MATH
	XPMatrixIdentity( this );
#else
	(*this) = Matrix::identity;
#endif
}


/**
 *	Return the yaw of the rotation part of this matrix
 */
INLINE float Matrix::yaw() const
{
	Vector3 zdir = this->applyToUnitAxisVector(2);
	zdir.normalise();

	// Can have problems when the y direction is one because that implies that
	// both zdir.x and zdir.z are zero if this is orthonormal. According to the
	// standard atan2f returns zero in this case.
#ifdef __BORLANDC__
	return atan2( zdir.x, zdir.z );
#else
	return atan2f( zdir.x, zdir.z );
#endif
}


/**
 *	Return the pitch of the rotation part of this matrix
 */
INLINE float Matrix::pitch() const
{
	Vector3 zdir = this->applyToUnitAxisVector(2);
	zdir.normalise();
#ifdef __BORLANDC__
	return -asin( zdir.y );
#else
	return -asinf( Math::clamp(-1.0f, zdir.y, +1.0f) );
#endif
}


/**
 *	Return the roll of the rotation part of this matrix
 */
INLINE float Matrix::roll() const
{
	Vector3 xdir = this->applyToUnitAxisVector(0);
	Vector3 ydir = this->applyToUnitAxisVector(1);
	xdir.normalise();
	ydir.normalise();

#ifdef __BORLANDC__
	return atan2( xdir.y, ydir.y );
#else
	return atan2f( xdir.y, ydir.y );
#endif
}



/**
 *	Return the uniform scale of this matrix
 *  Matrix has to be orthogonal and scaled uniformly
 *	For non-uniform scaled matrix use the version which returns Vector3
 */
INLINE float	Matrix::uniformScale() const
{
	return this->applyToUnitAxisVector( 0 ).length();
}

/**
 *	Return the non uniform scale of this matrix
 */
INLINE Vector3	Matrix::nonUniformScale() const
{
	return Vector3( this->applyToUnitAxisVector( 0 ).length(),
					this->applyToUnitAxisVector( 1 ).length(),
					this->applyToUnitAxisVector( 2 ).length() );

}


// -----------------------------------------------------------------------------
// Section: DirectX inline methods
// -----------------------------------------------------------------------------

#ifdef EXT_MATH

/**
 *	This method sets this matrix to a scaling matrix. It does not contain any
 *	rotation and translation.
 *
 *	@param	x	The amount to scale the x-coordinate.
 *	@param	y	The amount to scale the y-coordinate.
 *	@param	z	The amount to scale the z-coordinate.
 */
INLINE
void Matrix::setScale( const float x, const float y, const float z )
{
	XPMatrixScaling( this, x, y, z );
}


/**
 *	This method sets this matrix to be a translation matrix. It contains no
 *	rotation or scaling.
 *
 *	@param x	The amount to translate along the x-axis.
 *	@param y	The amount to translate along the y-axis.
 *	@param z	The amount to translate along the z-axis.
 */
INLINE
void Matrix::setTranslate( const float x, const float y, const float z )
{
	XPMatrixTranslation( this, x, y, z );
}


/**
 *	This method sets this matrix to the result of multiplying the two input
 *	matrices.
 *
 *	@param m1	The left-hand side of the multiplication.
 *	@param m2	The right-hand side of the multiplication.
 */
INLINE
void Matrix::multiply( const Matrix& m1, const Matrix& m2 )
{
	XPMatrixMultiply( this, &m1, &m2 );
}


/**
 *	This method sets this matrix to the result of multiply the input matrix by
 *	this matrix. The input matrix is on the left-hand side of the multiplication
 *	and this matrix is on the right.
 *
 *	@param m	The matrix to pre-multiply this matrix by.
 */
INLINE
void Matrix::preMultiply( const Matrix& m )
{
	XPMatrixMultiply( this, &m, this );
}


/**
 *	This method sets this matrix to the result of multiply this matrix by the
 *	input matrix. The input matrix is on the right-hand side of the
 *	multiplication and this matrix is on the left.
 *
 *	@param m	The matrix to post-multiply this matrix by.
 */
INLINE
void Matrix::postMultiply( const Matrix& m )
{
	XPMatrixMultiply( this, this, &m );
}


/**
 *	This method sets this matrix to be a rotation matrix around the X axis. A
 *	positive value rotates the Y axis towards the Z axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::setRotateX( const float angle )
{
	XPMatrixRotationX( this, angle );
}


/**
 *	This method sets this matrix to be a rotation matrix around the Y axis. A
 *	positive value rotates the Z axis towards the X axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::setRotateY( const float angle )
{
	XPMatrixRotationY( this, angle );
}


/**
 *	This method sets this matrix to be a rotation matrix around the Z axis. A
 *	positive rotates the X axis towards the Y axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::setRotateZ( const float angle )
{
	XPMatrixRotationZ( this, angle );
}


/**
 *	This method sets this matrix to be the rotation matrix equivalent to the
 *	input quaternion.
 */
INLINE
void Matrix::setRotate( const Quaternion & q )
{
	XPMatrixRotationQuaternion( this, (const QuaternionBase *)&q );
}


/**
 *	This method sets this matrix to be the rotation matrix equivalent to the
 *	input yaw, pitch and roll values.
 */
INLINE void Matrix::setRotate( float yaw, float pitch, float roll )
{
	XPMatrixRotationYawPitchRoll( this, yaw, pitch, roll );
}


/**
 *	This method inverts the input matrix. This matrix is set to the result.
 *
 *	@warning	The determinant of the input matrix should not be 0.
 */
INLINE bool Matrix::invert( const Matrix & matrix )
{
	return (XPMatrixInverse( this, NULL, &matrix ) != NULL);
}


/**
 *	This method transposes the input matrix
 */
INLINE void Matrix::transpose( const Matrix & matrix )
{
	XPMatrixTranspose( this, &matrix );
}


/**
 *	This method creates a <i>look at</i> matrix from two directions and a
 *	position.
 *
 *	@param postion		The position to look from.
 *	@param direction	The direction to look.
 *	@param up			The direction of <i>"up"</i>.
 */
INLINE
void Matrix::lookAt( const Vector3& position, const Vector3& direction,
		const Vector3& up )
{
	Vector3 at = position + direction;
	XPMatrixLookAtLH( this, &position, &at, &up );
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the X axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::postRotateX( const float angle )
{
	Matrix m;
	XPMatrixRotationX( &m, angle );
	this->postMultiply( m );
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::postRotateY(const float angle)
{
	Matrix m;
	XPMatrixRotationY( &m, angle );
	this->postMultiply( m );
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the Z axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::postRotateZ(const float angle)
{
	Matrix m;
	XPMatrixRotationZ( &m, angle );
	this->postMultiply( m );
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the X axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::preRotateX(const float angle)
{
	Matrix m;
	XPMatrixRotationX( &m, angle );
	this->preMultiply( m );
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::preRotateY(const float angle)
{
	Matrix m;
	XPMatrixRotationY( &m, angle );
	this->preMultiply( m );
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
INLINE
void Matrix::preRotateZ(const float angle)
{
	Matrix m;
	XPMatrixRotationZ( &m, angle );
	this->preMultiply( m );
}

#endif // EXT_MATH


/**
 *	@return True if all corresponding elements of @a a and @a b return true
 *		for almostEqual( aVal, bVal, @a epsilon ).
 */
INLINE
bool almostEqual( const Matrix & a, const Matrix & b,
	const float epsilon)
{
	for (int i = 0; i < 4; ++i)
	{
		if (!almostEqual( a.row( i ), b.row( i ), epsilon ))
		{
			return false;
		}
	}
	return true;
}

// matrix.ipp
