#ifndef LINEEQ3_HPP
#define LINEEQ3_HPP

#include "vector3.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the equation of a line in 3D space. It is not a line
 *	segment or a ray, but an infinite line (like a PlaneEq is an infinite
 *	plane).  Unlike LineEq, LineEq3 is defined in 3D space and not in a plane.
 */
class LineEq3
{
public:
	/// Constructors
	LineEq3();
	LineEq3( const Vector3& point1, const Vector3& point2 );

	bool isValid() const;
	void isValid( bool isValid );

	float perpDistance( const Vector3& point ) const;

private:
	bool		isValid_;
	Vector3		point_;
	Vector3		direction_;
};


#include "lineeq3.ipp"

BW_END_NAMESPACE

#endif // LINEEQ3_HPP
