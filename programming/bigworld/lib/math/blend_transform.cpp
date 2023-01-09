#include "pch.hpp"
#include "blend_transform.hpp"

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

/**
*	Constructor. Caller must check BlendTransform::valid()
*
*	@param	m	The Matrix to initialise the BlendTransform with.
*/
BlendTransform::BlendTransform( const Matrix& m )
{
	// Initialise BlendTransform, caller must check BlendTransform::valid()
	init( m );
}

/**
*	Constructor that takes in rotation, scale and translation.
*   Caller must check BlendTransform::valid().
*	@param irotate The Quaternion to initialise the rotation with.
*	@param iscale The Vector3 to initialise the scale with.
*	@param itranslate The Vector3 to initialise the translation with.
*/
BlendTransform::BlendTransform(
					  const Quaternion & irotate,
					  const Vector3 & iscale,
					  const Vector3 & itranslate ) :
		rotate_( irotate ),
		scale_( iscale ),
		translate_( itranslate )
{
	// Caller must check BlendTransform::valid();
}

bool BlendTransform::operator ==( const BlendTransform & rhs ) const
{
	return
		(rotate_ == rhs.rotate_)
		&&
		(scale_ == rhs.scale_)
		&&
		(translate_ == rhs.translate_);
}

/**
*	This function initialises the rotation, scale and translation of the 
*	blend transform with the given matrix.
*	Caller must check BlendTransform::valid().
*   
*	@param	ma	The matrix to extract rotation, scale and translation data
*				from. The matrix must be invertible i.e. have a non-zero
*				determinant.
*/
void BlendTransform::init( const Matrix& ma )
{
	Matrix m = ma;

	Vector3& row0 = m[0];
	Vector3& row1 = m[1];
	Vector3& row2 = m[2];
	Vector3& row3 = m[3];

	scale_.x = XPVec3Length( &row0 );
	scale_.y = XPVec3Length( &row1 );
	scale_.z = XPVec3Length( &row2 );

	row0 *= 1.f / scale_.x;
	row1 *= 1.f / scale_.y;
	row2 *= 1.f / scale_.z;

	Vector3 in;

	XPVec3Cross( &in, &row0, &row1 );
	if( XPVec3Dot( &in, &row2 ) < 0 )
	{
		row2 *= -1;
		scale_.z *= -1;
	}

	translate_ = row3;
	XPQuaternionRotationMatrix( &rotate_, &m );
}

/**
 * Returns true if this transform has a unit length rotation, a reasonable scale
 * and a reasonable translation, otherwise returns false and writes reason to given
 * string.
 */
bool BlendTransform::valid( BW::string* reason ) const
{
	float rotLength = rotate_.length();
	if ( !almostEqual( rotLength, 1.0f ) )
	{	
		if ( reason )
		{
			*reason = "BlendTransform::valid() - Rotation quaternion not normalised, "
						"possibly generated from a skewed matrix.\n";
		}
		return false;
	}

	if ( fabsf(scale_.x) >= 1000.0f ||
		fabsf(scale_.y) >= 1000.0f || 
		fabsf(scale_.z) >= 1000.0f )
	{
		if ( reason )
		{
			*reason = "BlendTransform::valid() - Scale is too large.\n"; 
		}
		return false;
	}

	if ( fabsf(translate_.x) >= 200.0f ||
		fabsf(translate_.y) >= 200.0f || 
		fabsf(translate_.z) >= 200.0f )
	{
		if ( reason )
		{
			*reason = "BlendTransform::valid() - Translation is too large.\n"; 
		}
		return false;
	}

	return true;
}

BW_END_NAMESPACE

// blend_transform.cpp
