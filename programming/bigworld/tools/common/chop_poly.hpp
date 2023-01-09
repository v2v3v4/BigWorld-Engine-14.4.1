/**
 *	@file
 */

#ifndef CHOP_POLYGON_HPP
#define CHOP_POLYGON_HPP

#include "math/vector3.hpp"
#include "math/planeeq.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent a polygon in NavGen.
 *  In NavGen we will never need any polygon with size larger than 7
 */
class ChopPolygon
{
	static const int MAX_WORLD_POLYGON_VECTOR_SIZE = 8;
public:
	ChopPolygon() : size_( 0 )
	{}

	ChopPolygon( size_t size ) : size_( size )
	{
		MF_ASSERT_DEBUG( size <= MAX_WORLD_POLYGON_VECTOR_SIZE );
	}

	void push_back( const Vector3& v )
	{
		v_[ size_ ] = v;
		++size_;
		MF_ASSERT_DEBUG( size_ <= MAX_WORLD_POLYGON_VECTOR_SIZE );
	}

	Vector3& front()
	{
		MF_ASSERT_DEBUG( size_ > 0 );
		return *v_;
	}

	const Vector3& front() const
	{
		MF_ASSERT_DEBUG( size_ > 0 );
		return *v_;
	}

	Vector3& back()
	{
		MF_ASSERT_DEBUG( size_ > 0 );
		return v_[ size_ - 1 ];
	}

	const Vector3& back() const
	{
		MF_ASSERT_DEBUG( size_ > 0 );
		return v_[ size_ - 1 ];
	}

	Vector3& operator[]( int idx )
	{
		MF_ASSERT_DEBUG( idx >= 0 && idx < MAX_WORLD_POLYGON_VECTOR_SIZE );
		return v_[ idx ];
	}

	const Vector3& operator[]( int idx ) const
	{
		MF_ASSERT_DEBUG( idx >= 0 && idx < MAX_WORLD_POLYGON_VECTOR_SIZE );
		return v_[ idx ];
	}

	int size() const
	{
		MF_ASSERT( size_ <= INT_MAX );
		return ( int ) size_;
	}

	void* empty() const
	{
		return (void*)!size_;
	}

	void clear()
	{
		size_ = 0;
	}

	void pop_back()
	{
		MF_ASSERT_DEBUG( size_ > 0 );
		--size_;
	}

	void chop( const PlaneEq & planeEq );

private:
	Vector3 v_[ MAX_WORLD_POLYGON_VECTOR_SIZE ];
	size_t size_;

	void split( const PlaneEq & planeEq,
		ChopPolygon & frontPoly, float* dists, BOOL* sides ) const;
};

BW_END_NAMESPACE

#endif // CHOP_POLYGON_HPP
