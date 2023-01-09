#ifndef POLYGON_HPP
#define POLYGON_HPP

#include "cstdmf/bw_namespace.hpp"

#include <limits>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class Vector2;

namespace Math
{

/**
 *  This templated class implements a simple polygon container. At the moment,
 *  it's useful for Vector2 polygon intersection.
 */
template <class PointType> class Polygon
{
public:
	Polygon() {}

	// Removes all points from the polygon.
	void clear() { return points_.clear(); }

	// Returns the number of points in the polygon.
	size_t size() const { return points_.size(); }

	// Returns a point from the polygon.
	PointType point( unsigned int i ) const { return points_[i]; }

	// Adds a point to the polygon.
	void addPoint( const PointType& point ) { points_.push_back( point ); }

	bool intersects( const Polygon<PointType>& other ) const;

private:
	BW::vector<PointType> points_;

	/**
	 *  This helper method projects all points from the poly onto the separation
	 *  line, and returns the minimum and maximum position after projection. Used
	 *  when testing intersection in 'intersects', using the separation of axes
	 *  theorem.
	 *
	 *  @param separationLine		Normal of the separation line.
	 *  @return						Pair containing the minimum and maximum
	 *                              projected position, in that order.
	 *  @see intersects
	 */
	std::pair<float,float> calcMinMax( const PointType& separationLine ) const
	{
		std::pair<float,float> minMax( std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() );
		for (int i = 0; i < (int)this->size(); i++)
		{
			float projection = separationLine.dotProduct( this->point(i) );
			if ( minMax.first > projection ) minMax.first = projection;
			if ( minMax.second < projection ) minMax.second = projection;
		}
		return minMax;
	}
};


// At the moment, this class is only useful for 2D polygons.
typedef Polygon<Vector2> Polygon2D;


} // namespace Math

BW_END_NAMESPACE

#endif //POLYGON_HPP
