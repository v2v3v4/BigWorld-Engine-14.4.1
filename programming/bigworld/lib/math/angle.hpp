#ifndef ANGLE_HPP
#define ANGLE_HPP

// Standard Library Headers.
#include <iostream>
#include <math.h>

// Application Specific Headers.
#include "mathdef.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a floating-point implementation of an angle. It lies between
 *	the values plus and minus MATH_PI. The operations defined for angle include
 *	addition, subtraction, multiplication and division by scalars. Angles can be
 *	cast as a float, and floats can be casted as angles.
 *
 *	@ingroup Math
 */
class Angle
{
public:
	///	@name Constructors and Destructor.
	//@{
	Angle();
	Angle( float valueInRadians );
   ~Angle();
	//@}

	/// @name Assignment Operators.
	//@{
	const Angle &operator = ( const Angle &rhs );
	const Angle &operator = ( float valueInRadians );
	//@}

	///	@name Unary Arithmetic Operators.
	//@{
	const Angle &operator += ( float rhs );
	const Angle &operator -= ( float rhs );
	//@}

	///	@name Casting Operators.
	//@{
	operator float () const;
	//@}

	/// @name Static Methods.
	//@{
	inline static float normalise( float value );
	inline static float decay( float src, float dst, float halfLife, float dTime );
	inline static float sameSignAngle( float angle, float closest );
	inline static float turnRange( Angle from, Angle to );
	//@}


	///	@name Extractors and Insertors.
	//@{
    friend std::ostream& operator << ( std::ostream&, const Angle & );
    friend std::istream& operator >> ( std::istream&, Angle & );
	//@}

	///	@name Miscellaneous.
	//@{
	bool clampBetween( Angle min, Angle max );
	bool isBetween( Angle min, Angle max ) const;
	void lerp( Angle a, Angle b, float alpha );
	//@}

private:

	///	@name Private Data Members.
	//@{
	///	Internal representation of the Angle.
	float angle_;
	//@}
};


#include "angle.ipp"

BW_END_NAMESPACE

#endif

// angle.hpp
