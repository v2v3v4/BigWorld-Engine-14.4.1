#ifndef GRID_COORD_HPP
#define GRID_COORD_HPP

#include "math/Vector2.hpp"

BW_BEGIN_NAMESPACE

/** Basically an int vector */
class GridCoord
{
public:
	GridCoord( int xc, int yc );
	GridCoord( Vector2 v );

	int x;
	int y;

	GridCoord operator+ (const GridCoord& rhs );

	static GridCoord zero();
};

class GridRect
{
public:
	GridRect( GridCoord bl, GridCoord tr );

	GridCoord bottomLeft;
	GridCoord topRight;

	bool valid();

	GridRect operator+ (const GridCoord& rhs );

	static GridRect zero();
	/** Create a GridRect from any two points */
	static GridRect fromCoords( GridCoord a, GridCoord b);
};

BW_END_NAMESPACE

#endif // GRID_COORD_HPP