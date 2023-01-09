#include "pch.hpp"

#include "matrix.hpp"

#include <iomanip>
#include <math.h>

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include "quat.hpp"
#include "vector3.hpp"

DECLARE_DEBUG_COMPONENT2( "Math", 0 )

#define ENABLE_ROTATION_MATRIX_GATHERING 0
#if ENABLE_ROTATION_MATRIX_GATHERING
// Debug code that can be used to write out unique rotation matrices.
// collectUniqueRotationMats appends rotation matrices it has not been 
// given previously to a given file. There are currently no calls to this
// so would need to call it from somewhere.
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_util.hpp"

BW_BEGIN_NAMESPACE

void collectUniqueRotationMats(const char * fileName, const Matrix& m)
{
	static vector<Matrix> uniqueRotationMats;
	// Find if we have already seen this matrix, if not then append it
	// to the given file

	for( vector<Matrix>::const_iterator it = uniqueRotationMats.cbegin(); 
			it != uniqueRotationMats.cend(); ++it )
	{
		const Matrix& x = *it;
		Vector3 row0(x.row(0).x, x.row(0).y, x.row(0).z);
		Vector3 row1(x.row(1).x, x.row(1).y, x.row(1).z);
		Vector3 row2(x.row(2).x, x.row(2).y, x.row(2).z);

		Vector3 rowM0(m.row(0).x, m.row(0).y, m.row(0).z);
		Vector3 rowM1(m.row(1).x, m.row(1).y, m.row(1).z);
		Vector3 rowM2(m.row(2).x, m.row(2).y, m.row(2).z);

		if (almostEqual(row0, rowM0) &&
			almostEqual(row1, rowM1) &&
			almostEqual(row2, rowM2) )
		{
			return;
		}
	}
	uniqueRotationMats.push_back( m );
	FILE* f = bw_fopen( fileName, "a" );

	for( int i = 0; i < 3; ++i )
	{
		const Vector4& row = m.row(i);
		fprintf( f, "%f, %f, %f,", row.x, row.y, row.z );
	}				
	fprintf( f, "\n" );

	fclose( f );
}

BW_END_NAMESPACE
#endif // ENABLE_ROTATION_MATRIX_GATHERING


BW_BEGIN_NAMESPACE

#define ENABLE_TEST_ROTATION_MATRICES 0
#if ENABLE_TEST_ROTATION_MATRICES
// Function to test the conversion from rotation matrix to yaw, pitch, roll
// and back again. The matrices should generally be similar although there
// cases like gimbal lock where this might not be true. If the matrix
// has scale applied then this will also fail the test. This function is not
// currently called from anywhere so you would need to add a call to it
// where you need to test.
void testRotationMatrixConversion( const Matrix& m )
{
	const float yaw = m.yaw();
	const float pitch = m.pitch();
	const float roll = m.roll();

	Matrix a;
	a = m;
	a.setRotate( yaw, pitch, roll );
	a.translation( m.applyToOrigin() );

#if 0
	// Output the rotation matrix and the angles
	const float yawDeg = RAD_TO_DEG(yaw);
	const float pitchDeg = RAD_TO_DEG(pitch);
	const float rollDeg = RAD_TO_DEG(roll);

	TRACE_MSG( "AfterN %f %f %f\n", a(0,0), a(1,0), a(2,0) );
	TRACE_MSG( "       %f %f %f\n", a(0,1), a(1,1), a(2,1) );
	TRACE_MSG( "       %f %f %f\n", a(0,2), a(1,2), a(2,2) );
	TRACE_MSG( " ypr   %f %f %f\n", yawDeg, pitchDeg, rollDeg );
#endif

	// Count the number of matrices that fail comparison
	if (!almostEqual( m, a, 0.01f ))
	{
		static int notEqualCount = 0;
		++notEqualCount;
		TRACE_MSG("New not equal %d\n", notEqualCount);
	}
}

#endif // ENABLE_TEST_ROTATION_MATRICES


#ifndef CODE_INLINE
    #include "matrix.ipp"
#endif

const Matrix Matrix::identity(	Vector4( 1.0f, 0.0f, 0.0f, 0.0f ),
								Vector4( 0.0f, 1.0f, 0.0f, 0.0f ),
								Vector4( 0.0f, 0.0f, 1.0f, 0.0f ),
								Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );

/**
 *	This method returns whether or not this matrix is mirrored. That is, whether
 *	applying this matrix to a coordinate system would change it from a
 *	left-handed coordinate system, to a right-handed coordinate system.
 *
 *	@return	True if matrix is mirrored, otherwise false.
 */
bool Matrix::isMirrored() const
{
	Vector3 v;
	v.crossProduct( (*this)[ 0 ], (*this)[ 2 ] );
	return ( v.dotProduct( (*this)[ 1 ] ) > 0.f );
}


/**
 *	This method returns whether or not this matrix is uniformly scaled
 */
bool	Matrix::isUniformlyScaled() const
{
	float l0 = this->applyToUnitAxisVector( 0 ).length();
	float l1 = this->applyToUnitAxisVector( 1 ).length();
	float l2 = this->applyToUnitAxisVector( 2 ).length();
	bool res = almostEqual(l0, l1);
	res &= almostEqual(l1, l2);
	res &= almostEqual(l2, l0);
	return res;
}
/**
 *	This method inverts the input matrix. It only works on matrices that are
 *	orthonormal. That is, the axis vectors must be orthogonal and have unit
 *	length. This matrix is set to the result.
 */
void Matrix::invertOrthonormal( const Matrix& matrix )
{
	m[0][0] = matrix.m[0][0];
	m[0][1] = matrix.m[1][0];
	m[0][2] = matrix.m[2][0];
	m[0][3] = 0.f;

	m[1][0] = matrix.m[0][1];
	m[1][1] = matrix.m[1][1];
	m[1][2] = matrix.m[2][1];
	m[1][3] = 0.f;

	m[2][0] = matrix.m[0][2];
	m[2][1] = matrix.m[1][2];
	m[2][2] = matrix.m[2][2];
	m[2][3] = 0.f;

	m[3][0] = -(matrix.m[3][0] * m[0][0] + matrix.m[3][1] * m[1][0] + matrix.m[3][2] * m[2][0]);
	m[3][1] = -(matrix.m[3][0] * m[0][1] + matrix.m[3][1] * m[1][1] + matrix.m[3][2] * m[2][1]);
	m[3][2] = -(matrix.m[3][0] * m[0][2] + matrix.m[3][1] * m[1][2] + matrix.m[3][2] * m[2][2]);
	m[3][3] = 1.f;
}

/// Private helper function
/* If this defintion is changed, also update PYAUTO_WRITE_VALID_ANGLE
 * and PYAUTO_WRITE_VALID_ANGLE_VECTOR3 in src/lib/pyscript/script_math.hpp
 * TODO: Might be better to export this at some point, since other callers
 * of the relevant functions probably need to be able to validate this.
 */
static inline bool isValidAngle( float angle )
{
	return (-100.f < angle) && (angle < 100.f);
}

/**
 *	This method sets this matrix to be the inverse of the rotation matrix
 *	equivalent to the input yaw, pitch and roll values. The order of
 *	application in the non-inverted matrix is: roll, then pitch, then yaw.
 */
void Matrix::setRotateInverse( float yaw, float pitch, float roll )
{
	MF_ASSERT_DEBUG( isValidAngle( yaw ) );
	MF_ASSERT_DEBUG( isValidAngle( pitch ) );
	MF_ASSERT_DEBUG( isValidAngle( roll ) );

#if 1
	// This version gives same results as D3DXMatrixRotationYawPitchRoll
	this->setRotateY( yaw );
	this->preRotateX( pitch );
	this->preRotateZ( roll );
	this->invertOrthonormal();
#else
	// TODO: Remove or fix this optimised version. It does not work in its
	// current form.

	// TODO: Floating point or other errors can lead to inaccuracies of
	// up to 5% in this function! This must be improved. The DirectX
	// functions appear to have far less problems.

	const double sya = sin( yaw );
	const double cya = cos( yaw );
	const double sxa = sin( pitch );
	const double cxa = cos( pitch );
	const double sza = sin( roll );
	const double cza = cos( roll );

	m[0][0] = float( cya * cza );
	m[0][1] = float(-cya * sza );
	m[0][2] = float( sya * cxa );
	m[0][3] =  0.f;

	m[1][0] = float( cxa * sza );
	m[1][1] = float( cxa * cza );
	m[1][2] = float(-sxa );
	m[1][3] =  0.f;

	m[2][0] = float(-sya * cza + cya * sza * sxa );
	m[2][1] = float( sya * sza + cya * cza * sxa );
	m[2][2] = float( cxa * cya );
	m[2][3] =  0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
#endif
}


/**
 *	@return A string (intended for debugging) showing the matrix values.
 */
BW::string Matrix::desc(
	const BW::string & rowStart, const BW::string & rowEnd ) const
{
	BW::ostringstream resultStream;
	resultStream.precision( 4 );
	resultStream << std::fixed;

	for(int row = 0; row < 4; ++row)
	{
		resultStream << rowStart << " ";
		for(int col = 0; col < 4; ++col)
		{
			resultStream << std::setw( 10 ) << ((*this)( col, row )) <<
				((col < 3) ? ", " : " ");
		}
		resultStream << rowEnd;
	}

	return resultStream.str();
}



// -----------------------------------------------------------------------------
// Section: Methods that are inlined if using DirectX math
// -----------------------------------------------------------------------------

#ifndef EXT_MATH

/**
 *	This method sets this matrix to a scaling matrix. It does not contain any
 *	rotation and translation.
 *
 *	@param	x	The amount to scale the x-coordinate.
 *	@param	y	The amount to scale the y-coordinate.
 *	@param	z	The amount to scale the z-coordinate.
 */
void Matrix::setScale( const float x, const float y, const float z )
{
	m[0][0] = x;
	m[0][1] = 0.f;
	m[0][2] = 0.f;
	m[0][3] = 0.f;

	m[1][0] = 0.f;
	m[1][1] = y;
	m[1][2] = 0.f;
	m[1][3] = 0.f;

	m[2][0] = 0.f;
	m[2][1] = 0.f;
	m[2][2] = z;
	m[2][3] = 0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}


/**
 *	This method sets this matrix to be a translation matrix. It contains no
 *	rotation or scaling.
 *
 *	@param x	The amount to translate along the x-axis.
 *	@param y	The amount to translate along the y-axis.
 *	@param z	The amount to translate along the z-axis.
 */
void Matrix::setTranslate( const float x, const float y, const float z )
{
	m[0][0] = 1.f;
	m[0][1] = 0.f;
	m[0][2] = 0.f;
	m[0][3] = 0.f;

	m[1][0] = 0.f;
	m[1][1] = 1.f;
	m[1][2] = 0.f;
	m[1][3] = 0.f;

	m[2][0] = 0.f;
	m[2][1] = 0.f;
	m[2][2] = 1.f;
	m[2][3] = 0.f;

	m[3][0] = x;
	m[3][1] = y;
	m[3][2] = z;
	m[3][3] = 1.f;
}


/**
 *	This method sets this matrix to the result of multiplying the two input
 *	matrices.
 *
 *	@param m1	The left-hand side of the multiplication.
 *	@param m2	The right-hand side of the multiplication.
 */
void Matrix::multiply( const Matrix& m1, const Matrix& m2 )
{
	MF_ASSERT_DEBUG( this != &m1 && this != &m2 );

	m00 = m1.m00 * m2.m00 + m1.m01 * m2.m10 + m1.m02 * m2.m20 + m1.m03 * m2.m30;
	m01 = m1.m00 * m2.m01 + m1.m01 * m2.m11 + m1.m02 * m2.m21 + m1.m03 * m2.m31;
	m02 = m1.m00 * m2.m02 + m1.m01 * m2.m12 + m1.m02 * m2.m22 + m1.m03 * m2.m32;
	m03 = m1.m00 * m2.m03 + m1.m01 * m2.m13 + m1.m02 * m2.m23 + m1.m03 * m2.m33;

	m10 = m1.m10 * m2.m00 + m1.m11 * m2.m10 + m1.m12 * m2.m20 + m1.m13 * m2.m30;
	m11 = m1.m10 * m2.m01 + m1.m11 * m2.m11 + m1.m12 * m2.m21 + m1.m13 * m2.m31;
	m12 = m1.m10 * m2.m02 + m1.m11 * m2.m12 + m1.m12 * m2.m22 + m1.m13 * m2.m32;
	m13 = m1.m10 * m2.m03 + m1.m11 * m2.m13 + m1.m12 * m2.m23 + m1.m13 * m2.m33;

	m20 = m1.m20 * m2.m00 + m1.m21 * m2.m10 + m1.m22 * m2.m20 + m1.m23 * m2.m30;
	m21 = m1.m20 * m2.m01 + m1.m21 * m2.m11 + m1.m22 * m2.m21 + m1.m23 * m2.m31;
	m22 = m1.m20 * m2.m02 + m1.m21 * m2.m12 + m1.m22 * m2.m22 + m1.m23 * m2.m32;
	m23 = m1.m20 * m2.m03 + m1.m21 * m2.m13 + m1.m22 * m2.m23 + m1.m23 * m2.m33;

	m30 = m1.m30 * m2.m00 + m1.m31 * m2.m10 + m1.m32 * m2.m20 + m1.m33 * m2.m30;
	m31 = m1.m30 * m2.m01 + m1.m31 * m2.m11 + m1.m32 * m2.m21 + m1.m33 * m2.m31;
	m32 = m1.m30 * m2.m02 + m1.m31 * m2.m12 + m1.m32 * m2.m22 + m1.m33 * m2.m32;
	m33 = m1.m30 * m2.m03 + m1.m31 * m2.m13 + m1.m32 * m2.m23 + m1.m33 * m2.m33;
}


/**
 *	This method sets this matrix to the result of multiply the input matrix by
 *	this matrix. The input matrix is on the left-hand side of the multiplication
 *	and this matrix is on the right.
 *
 *	@param m	The matrix to pre-multiply this matrix by.
 */
void Matrix::preMultiply( const Matrix& m )
{
	MF_ASSERT_DEBUG( this != &m );

	Matrix tmp( *this );

	multiply( m, tmp );
}


/**
 *	This method sets this matrix to the result of multiply this matrix by the
 *	input matrix. The input matrix is on the right-hand side of the
 *	multiplication and this matrix is on the left.
 *
 *	@param m	The matrix to post-multiply this matrix by.
 */
void Matrix::postMultiply( const Matrix& m )
{
	MF_ASSERT_DEBUG( this != &m );

	Matrix tmp( *this );

	multiply( tmp, m );
}


/**
 *	This method sets this matrix to be a rotation matrix around the X axis. A
 *	positive value rotates the Y axis toward the Z axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::setRotateX( const float angle )
{
	MF_ASSERT_DEBUG( isValidAngle( angle ) );

	float sa = (float)sin( angle );
	float ca = (float)cos( angle );

	m[0][0] = 1.f;
	m[0][1] = 0.f;
	m[0][2] = 0.f;
	m[0][3] = 0.f;

	m[1][0] = 0.f;
	m[1][1] = ca;
	m[1][2] = sa;
	m[1][3] = 0.f;

	m[2][0] = 0.f;
	m[2][1] = -sa;
	m[2][2] = ca;
	m[2][3] = 0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}


/**
 *	This method sets this matrix to be a rotation matrix around the Y axis. A
 *	positive value rotates the Z axis toward the X axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::setRotateY( const float angle )
{
	MF_ASSERT_DEBUG( isValidAngle( angle ) );

	float sa = (float)sin( angle );
	float ca = (float)cos( angle );

	m[0][0] = ca;
	m[0][1] = 0.f;
	m[0][2] = -sa;
	m[0][3] = 0.f;

	m[1][0] = 0.f;
	m[1][1] = 1.f;
	m[1][2] = 0.f;
	m[1][3] = 0.f;

	m[2][0] = sa;
	m[2][1] = 0.f;
	m[2][2] = ca;
	m[2][3] = 0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}


/**
 *	This method sets this matrix to be a rotation matrix around the Z axis. A
 *	positive rotates the X axis toward the Y axis.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::setRotateZ( const float angle )
{
	MF_ASSERT_DEBUG( isValidAngle( angle ) );

	float sa = (float)sin( angle );
	float ca = (float)cos( angle );

	m[0][0] = ca;
	m[0][1] = sa;
	m[0][2] = 0.f;
	m[0][3] = 0.f;

	m[1][0] = -sa;
	m[1][1] = ca;
	m[1][2] = 0.f;
	m[1][3] = 0.f;

	m[2][0] = 0.f;
	m[2][1] = 0.f;
	m[2][2] = 1.f;
	m[2][3] = 0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
}


/**
 *	This method sets this matrix to be the rotation matrix equivalent to the
 *	input quaternion.
 */
void Matrix::setRotate( const Quaternion & q )
{
	const Vector3 v( q.x, q.y, q.z ); 

	//calculate coefficents
	float xx = v[ 0 ] * v[ 0 ] * 2.f;
	float xy = v[ 0 ] * v[ 1 ] * 2.f;
	float xz = v[ 0 ] * v[ 2 ] * 2.f;

	float yy = v[ 1 ] * v[ 1 ] * 2.f;
	float yz = v[ 1 ] * v[ 2 ] * 2.f;

	float zz = v[ 2 ] * v[ 2 ] * 2.f;

	float wx = q.w * v[ 0 ] * 2.f;
	float wy = q.w * v[ 1 ] * 2.f;
	float wz = q.w * v[ 2 ] * 2.f;

	m[0][0] = 1.f - ( yy + zz );
	m[1][0] = xy - wz;
	m[2][0] = xz + wy;

	m[0][1] = xy + wz;
	m[1][1] = 1.f - ( xx + zz );
	m[2][1] = yz - wx;

	m[0][2] = xz - wy;
	m[1][2] = yz + wx;
	m[2][2] = 1.f - ( xx + yy );

	m[0][3] = 0.f;
	m[1][3] = 0.f;
	m[2][3] = 0.f;

	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

/**
 *	This method sets this matrix to be the rotation matrix equivalent to the
 *	input yaw, pitch and roll values. The order of application is: roll, then
 *	pitch, then yaw.
 */
void Matrix::setRotate( float yaw, float pitch, float roll )
{
	MF_ASSERT_DEBUG( isValidAngle( yaw ) );
	MF_ASSERT_DEBUG( isValidAngle( pitch ) );
	MF_ASSERT_DEBUG( isValidAngle( roll ) );

#if 1 
	// This version gives same results as D3DXMatrixRotationYawPitchRoll
	this->setRotateY( yaw );
	this->preRotateX( pitch );
	this->preRotateZ( roll );
#else
	// TODO: Either remove this entirely or create an optimised version of
	// these operations that actually works.

	// TODO: Floating point or other errors can lead to inaccuracies of
	// up to 5% in this function! This must be improved. The DirectX
	// functions appear to have far less problems.

	const double sya = sin( yaw );
	const double cya = cos( yaw );
	const double sxa = sin( pitch );
	const double cxa = cos( pitch );
	const double sza = sin( roll );
	const double cza = cos( roll );

	m[0][0] =  cya * cza;
	m[0][1] =  cxa * sza;
	m[0][2] = -sya * cza + cya * sza * sxa;
	m[0][3] =  0.f;

	m[1][0] = -cya * sza;
	m[1][1] =  cxa * cza;
	m[1][2] =  sya * sza + cya * cza * sxa;
	m[1][3] =  0.f;

	m[2][0] =  sya * cxa;
	m[2][1] = -sxa;
	m[2][2] =  cxa * cya;
	m[2][3] =  0.f;

	m[3][0] = 0.f;
	m[3][1] = 0.f;
	m[3][2] = 0.f;
	m[3][3] = 1.f;
#endif
}


/**
 *	This method inverts the input matrix. This matrix is set to the result.
 *
 *	@warning	The determinant of the input matrix should not be 0.
 */
bool Matrix::invert( const Matrix& matrix )
{
	const float determinant = matrix.getDeterminant();

	if (isZero( determinant ))
	{
		ERROR_MSG( "Matrix::invert: "
				"Attempted to invert a matrix with zero determinant\n" );
		return false;
	}

	// TODO: Need to consider the invert of a 4x4. This is for a 3x4.

	float rcp = 1 / determinant;

	m[0][0] = matrix.m[1][1] * matrix.m[2][2] - matrix.m[1][2] * matrix.m[2][1];
	m[0][1] = matrix.m[0][2] * matrix.m[2][1] - matrix.m[0][1] * matrix.m[2][2];
	m[0][2] = matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1];
	m[1][0] = matrix.m[1][2] * matrix.m[2][0] - matrix.m[1][0] * matrix.m[2][2];
	m[1][1] = matrix.m[0][0] * matrix.m[2][2] - matrix.m[0][2] * matrix.m[2][0];
	m[1][2] = matrix.m[0][2] * matrix.m[1][0] - matrix.m[0][0] * matrix.m[1][2];
	m[2][0] = matrix.m[1][0] * matrix.m[2][1] - matrix.m[1][1] * matrix.m[2][0];
	m[2][1] = matrix.m[0][1] * matrix.m[2][0] - matrix.m[0][0] * matrix.m[2][1];
	m[2][2] = matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0];

	m[0][0] *= rcp;
	m[0][1] *= rcp;
	m[0][2] *= rcp;

	m[1][0] *= rcp;
	m[1][1] *= rcp;
	m[1][2] *= rcp;

	m[2][0] *= rcp;
	m[2][1] *= rcp;
	m[2][2] *= rcp;

	m[3][0] = -(matrix.m[3][0] * m[0][0] + matrix.m[3][1] * m[1][0] + matrix.m[3][2] * m[2][0]);
	m[3][1] = -(matrix.m[3][0] * m[0][1] + matrix.m[3][1] * m[1][1] + matrix.m[3][2] * m[2][1]);
	m[3][2] = -(matrix.m[3][0] * m[0][2] + matrix.m[3][1] * m[1][2] + matrix.m[3][2] * m[2][2]);

	return true;
}


/**
 *	This method transposes the input matrix.
 */
void Matrix::transpose( const Matrix & matrix )
{
	m[0][0] = matrix[0][0];
	m[1][1] = matrix[1][1];
	m[2][2] = matrix[2][2];
	m[3][3] = matrix[3][3];

// Need to be careful if this == &matrix
#define TRANSPOSE_SWAP( A, B )	\
	temp = matrix[A][B];		\
	m[A][B] = matrix[B][A];		\
	m[B][A] = temp;				\

	float temp;
	TRANSPOSE_SWAP( 0, 1 )
	TRANSPOSE_SWAP( 0, 2 )
	TRANSPOSE_SWAP( 0, 3 )
	TRANSPOSE_SWAP( 1, 2 )
	TRANSPOSE_SWAP( 1, 3 )
	TRANSPOSE_SWAP( 2, 3 )
}


/**
 *	This method creates a <i>look at</i> matrix from two directions and a
 *	position.
 *
 *	@param position		The position to look from.
 *	@param direction	The direction to look.
 *	@param up			The direction of <i>"up"</i>.
 */
void Matrix::lookAt( const Vector3& position, const Vector3& direction,
		const Vector3& up )
{
	Vector3 Up;
	Vector3 Direction( direction );
	Vector3 Right;

	Direction.normalise( );
	Right.crossProduct( up, Direction );
	Right.normalise( );
	Up.crossProduct( Direction, Right );

	m[0][0] = Right.x;
	m[1][0] = Right.y;
	m[2][0] = Right.z;
	m[3][0] = 0.f;

	m[0][1] = Up.x;
	m[1][1] = Up.y;
	m[2][1] = Up.z;
	m[3][1] = 0.f;

	m[0][2] = Direction.x;
	m[1][2] = Direction.y;
	m[2][2] = Direction.z;
	m[3][2] = 0.f;

	m[3][0] = -position.dotProduct( Right );
	m[3][1] = -position.dotProduct( Up );
	m[3][2] = -position.dotProduct( Direction );
	m[3][3] = 1.f;
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the X axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::postRotateX( const float angle )
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	// Need to store this cell value because it changes during the
	// calculation.
	const float cell_01 = m[0][1];

	m[0][1]= cell_01 * ca + m[0][2] * -sa;
	m[0][2]= cell_01 * sa + m[0][2] * ca;

	const float cell_11 = m[1][1];

	m[1][1]= cell_11 * ca + m[1][2] * -sa;
	m[1][2]= cell_11 * sa + m[1][2] * ca;

	const float cell_21 = m[2][1];

	m[2][1]= cell_21 * ca + m[2][2] * -sa;
	m[2][2]= cell_21 * sa + m[2][2] * ca;

	const float cell_31 = m[3][1];

	m[3][1]= cell_31 * ca + m[3][2] * -sa;
	m[3][2]= cell_31 * sa + m[3][2] * ca;
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::postRotateY(const float angle)
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	const float cell_00 = m[0][0];

	m[0][0]= cell_00 *  ca + m[0][2] * sa;
	m[0][2]= cell_00 * -sa + m[0][2] * ca;

	const float cell_10 = m[1][0];

	m[1][0]= cell_10 *  ca + m[1][2] * sa;
	m[1][2]= cell_10 * -sa + m[1][2] * ca;

	const float cell_20 = m[2][0];

	m[2][0]= cell_20 *  ca + m[2][2] * sa;
	m[2][2]= cell_20 * -sa + m[2][2] * ca;

	const float cell_30 = m[3][0];

	m[3][0]= cell_30 *  ca + m[3][2] * sa;
	m[3][2]= cell_30 * -sa + m[3][2] * ca;
}


/**
 *	This method post multiplies this matrix by the matrix that rotates around
 *	the Z axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::postRotateZ(const float angle)
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	const float cell_00 = m[0][0];

	m[0][0]= cell_00 * ca + m[0][1] * -sa;
	m[0][1]= cell_00 * sa + m[0][1] * ca;

	const float cell_10 = m[1][0];

	m[1][0]= cell_10 * ca + m[1][1] * -sa;
	m[1][1]= cell_10 * sa + m[1][1] * ca;

	const float cell_20 = m[2][0];

	m[2][0]= cell_20 * ca + m[2][1] * -sa;
	m[2][1]= cell_20 * sa + m[2][1] * ca;

	const float cell_30 = m[3][0];

	m[3][0]= cell_30 * ca + m[3][1] * -sa;
	m[3][1]= cell_30 * sa + m[3][1] * ca;
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the X axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::preRotateX(const float angle)
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	const Vector3 v1 = (*this)[1];

	m[1][0]= ca * v1.x + sa * m[2][0];
	m[1][1]= ca * v1.y + sa * m[2][1];
	m[1][2]= ca * v1.z + sa * m[2][2];

	m[2][0]= -sa * v1.x + ca * m[2][0];
	m[2][1]= -sa * v1.y + ca * m[2][1];
	m[2][2]= -sa * v1.z + ca * m[2][2];
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::preRotateY(const float angle)
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	const Vector3 v0 = (*this)[0];

	m[0][0]= ca * v0[0] + -sa * m[2][0];
	m[0][1]= ca * v0[1] + -sa * m[2][1];
	m[0][2]= ca * v0[2] + -sa * m[2][2];

	m[2][0]= sa * v0[0] + ca * m[2][0];
	m[2][1]= sa * v0[1] + ca * m[2][1];
	m[2][2]= sa * v0[2] + ca * m[2][2];
}


/**
 *	This method pre multiplies this matrix by the matrix that rotates around
 *	the Y axis by the input amount.
 *
 *	@param angle	The angle in radians to rotate by.
 */
void Matrix::preRotateZ(const float angle)
{
	const float sa = (float)sin( angle );
	const float ca = (float)cos( angle );

	const Vector3 v0 = (*this)[0];

	m[0][0]= ca * v0[0] + sa * m[1][0];
	m[0][1]= ca * v0[1] + sa * m[1][1];
	m[0][2]= ca * v0[2] + sa * m[1][2];

	m[1][0]= -sa * v0[0] + ca * m[1][0];
	m[1][1]= -sa * v0[1] + ca * m[1][1];
	m[1][2]= -sa * v0[2] + ca * m[1][2];
}

#endif // EXT_MATH

BW_END_NAMESPACE

// matrix.cpp
