#ifndef RECTT_HPP
#define RECTT_HPP


#include "range1dt.hpp"


namespace BW
{

template<typename COORDINATE> class RectT;
template<typename COORDINATE>
bool operator==( const RectT< COORDINATE > & l, const RectT< COORDINATE > & r );

/**
 *	This class represents a rectangle in 2 dimensions.
 */
template<typename COORDINATE>
class RectT
{
	friend bool operator==<>( const RectT & l, const RectT & r );

public:
	typedef COORDINATE				Coordinate;
	typedef Range1DT< Coordinate >	Range;

	RectT();
	RectT( Coordinate xMin, Coordinate yMin, Coordinate xMax, Coordinate yMax );

	Coordinate		area() const;

	void			inflateBy( Coordinate value );
	void			safeInflateBy( Coordinate value );

	bool			intersects( const RectT & rect ) const;
	bool			contains( Coordinate x, Coordinate y ) const;
	bool			contains( const RectT & rect ) const;

	float			distTo( Coordinate x, Coordinate y ) const;

	void			setToIntersection( const RectT & r1, const RectT & r2 );
	void			setToUnion( const RectT & r1, const RectT & r2 );

	Range &			range1D( bool getY );
	const Range &	range1D( bool getY ) const;

	Coordinate 		xMin() const;
	Coordinate 		xMax() const;
	Coordinate 		yMin() const;
	Coordinate 		yMax() const;

	void 			xMin( Coordinate value );
	void 			xMax( Coordinate value );
	void 			yMin( Coordinate value );
	void 			yMax( Coordinate value );

	const Range &	xRange() const;
	const Range &	yRange() const;

	union
	{
		struct { Coordinate xMin_, xMax_; };
		Range x_;
	};

	union
	{
		struct { Coordinate yMin_, yMax_; };
		Range y_;
	};
};


typedef RectT< int >	RectInt;
typedef RectT< float >	Rect;


#include "rectt.ipp"

} // namespace BW

#endif // RECTT_HPP
