#ifndef BLEND_TRANSFORM_HPP
#define BLEND_TRANSFORM_HPP

#include "cstdmf/debug.hpp"
#include "matrix.hpp"
#include "quat.hpp"
#include "vector3.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to linearly interpolate between two blends. The BlendTransform
 *	is primarly used for blending from one animation to another smoothly. 
 */
class BlendTransform
{
private:
	Quaternion	rotate_;
	Vector3		scale_;
	Vector3		translate_;

public:
	/**
	 *	Gets the rotation in the form of a Quaternion.
	 *	@return Returns the rotation as a Quaternion.
	 */
	const Quaternion& rotation() const 
	{
		return rotate_;
	};

	/**
	 *	Gets the scale.
	 *	@return Returns the scale as a Vector3.
	 */
	const Vector3& scaling() const
	{
		return scale_;
	};

	/**
	 *	Gets the translation.
	 *	@return Returns the translation as a Vector3.
	 */
	const Vector3& translation() const
	{
		return translate_;
	};

	/**
	 *	Set the translation.
	 *	@param translation The translation as a Vector3.
	 */
	void translation( const Vector3 & translation )
	{
		translate_ = translation;
	};

	void init( const Matrix& ma );

	explicit BlendTransform( const Matrix&	m );

	/**
	 *	Default Constructor.
	 */
	inline BlendTransform() :
		rotate_( 0, 0, 0, 1 ),
		scale_( 1, 1, 1 ),
		translate_( 0, 0, 0 )
	{
	}

	BlendTransform(
			const Quaternion &	irotate,
			const Vector3 &		iscale,
			const Vector3 &		itranslate );

	~BlendTransform()
	{
	}

	bool operator ==( const BlendTransform & rhs ) const;

	/**
	 *  This function normalises the rotation quaternion.
	 */
	inline void normaliseRotation()
	{
		rotate_.normalise();
	}

	/**
	 *  This function blends between this BlendTransform and the given BlendTransform by 
	 *	the supplied amount
	 *	
	 *	@param ts The amount to blend by.
	 *	@param bt The BlendTransform to blend with.
	 */
	inline void blend( float ts, const BlendTransform& bt )
	{
		if ( ts > 0 && ts < 1 )
		{
			XPQuaternionSlerp( &rotate_, &rotate_, &bt.rotate_, ts );
			XPVec3Lerp( &translate_, &translate_, &bt.translate_, ts );
			XPVec3Lerp( &scale_, &scale_, &bt.scale_, ts );
		}
		else if ( ts >= 1 )
		{
			*this = bt;
		}
		/*
		else
		{
			// do nothing
		}
		*/
	}

	/**
	 *	This function outputs the rotation, scale and translation information into the input matrix.
	 *
	 *	@param mOut The matrix to insert the rotation, scale and translation information into.
	 */
	inline void output( Matrix & mOut ) const
	{
		Matrix m(mOut);

		XPMatrixRotationQuaternion( &m, &rotate_ );
		
		MF_ASSERT(m.applyToOrigin().lengthSquared() < 20000.0f * 20000.0f);

		mOut = m;

		mOut[0] *= scale_.x;
		mOut[1] *= scale_.y;
		mOut[2] *= scale_.z;
		mOut[3] = translate_;
	}

	bool valid( BW::string* reason = NULL ) const;
};

BW_END_NAMESPACE

#endif // BLEND_TRANSFORM_HPP
