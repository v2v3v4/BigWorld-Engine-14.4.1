#include "pch.hpp"
#include "grid_coord.hpp"


BW_BEGIN_NAMESPACE

GridCoord::GridCoord( int xc, int yc ) : x( xc ), y( yc )
{
}

GridCoord::GridCoord( Vector2 v ) : x( (int) floorf( v.x )), y( (int) floorf( v.y ) )
{
}

GridCoord GridCoord::operator+ (const GridCoord& rhs ) const
{
	return GridCoord( x + rhs.x, y + rhs.y );
}

bool GridCoord::operator!=( const GridCoord& other ) const
{
	return ( other.x != this->x || other.y != this->y );
}

GridCoord GridCoord::zero()
{
	return GridCoord( 0, 0 );
}

GridCoord GridCoord::invalid()
{
	return GridCoord( 0x7fffffff, 0x7fffffff );
}

GridRect::GridRect( GridCoord bl, GridCoord tr ) : bottomLeft( bl ), topRight( tr )
{
}

bool GridRect::valid() const
{
	return bottomLeft.x < topRight.x && bottomLeft.y < topRight.y;
}

GridRect GridRect::operator+ (const GridCoord& rhs ) const
{
	return GridRect( bottomLeft + rhs, topRight + rhs );
}

GridRect GridRect::zero()
{
	return GridRect( GridCoord::zero(), GridCoord::zero() );
}

GridRect GridRect::fromCoords( GridCoord a, GridCoord b)
{
	return GridRect(
		GridCoord( min( a.x, b.x ), min( a.y, b.y )),
		GridCoord( max( a.x, b.x ), max( a.y, b.y ))
		);
}


void GridRect::clamp( int minX, int minY, int maxX, int maxY )
{
	bottomLeft.x = max( bottomLeft.x, minX );
	bottomLeft.y = max( bottomLeft.y, minY );
	topRight.x = min( topRight.x, maxX );
	topRight.y = min( topRight.y, maxY );
}

BW_END_NAMESPACE
