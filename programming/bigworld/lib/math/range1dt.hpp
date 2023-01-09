#ifndef RANGE1DT_HPP
#define RANGE1DT_HPP

#include <limits>
#include "cstdmf/debug.hpp"

namespace BW
{


/**
 *	This class is used to represent a 1 dimensional range.
 */
template<typename COORDINATE>
class Range1DT
{
public:
	typedef COORDINATE		Coordinate;

	union
	{
		Coordinate	data_[2];
		struct { Coordinate min_, max_; };
	};

	void			set( Coordinate minValue, Coordinate maxValue );

	Coordinate &	operator[]( int i );
	Coordinate		operator[]( int i ) const;

	Coordinate		length() const;
	Coordinate		midPoint() const;

	void			inflateBy( Coordinate value );
	void			safeInflateBy( Coordinate value );

	bool			intersects( const Range1DT & range ) const;

	bool			contains( Coordinate pt ) const;
	bool			contains( const Range1DT & range ) const;

	Coordinate		distTo( Coordinate pt ) const;
};


typedef Range1DT< float >	Range1D;

#include "range1dt.ipp"

} // namespace BW

#endif // RANGE1DT_HPP
