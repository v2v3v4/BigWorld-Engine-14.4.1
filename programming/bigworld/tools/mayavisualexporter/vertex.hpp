#ifndef __vertex_hpp__
#define __vertex_hpp__

#include "bonevertex.hpp"

BW_BEGIN_NAMESPACE

struct Point2
{
	Point2( float u = 0, float v = 0 );
		
	float u, v;

	inline bool operator==( const Point2& p ) const { return u == p.u && v == p.v; }
	inline bool operator!=( const Point2& p ) const { return !( *this == p ); }
};

BW_END_NAMESPACE

#endif // __vertex_hpp__