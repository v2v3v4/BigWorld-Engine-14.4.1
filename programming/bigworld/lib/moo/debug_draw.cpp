#include "pch.hpp"
#include "debug_draw.hpp"
#include "math/vector3.hpp"
#include "physics2/worldtri.hpp"
#include "physics2/worldpoly.hpp"
#include "moo/geometrics.hpp"

BW_BEGIN_NAMESPACE

namespace DebugDraw
{

bool s_enabled = false;
BW::vector< std::pair<WorldTriangle, Moo::Colour> > s_dtris;
BW::vector< std::pair<WorldPolygon, Moo::Colour> > s_dpolys;

void enabled( bool enable )
{
	if (s_enabled != enable)
	{
		s_dtris.clear();
		s_dpolys.clear();
		s_enabled = enable;
	}
}

bool enabled()
{
	return s_enabled;
}

void triAdd( const WorldTriangle & wt, uint32 col )
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}
	s_dtris.push_back( std::make_pair( wt, Moo::Colour( col ) ) );
}

void arrowAdd( const Vector3 & start, const Vector3 & end, uint32 col )
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}
	if (Vector3(end - start).length() > 0.001f)
	{
		Matrix lookat;
		Vector3 dir(end-start);
		float l = dir.length();
		dir.normalise();

		Vector3 up;
		if (fabsf(dir.y) < 0.95f)
		{
			up = Vector3(0.f,1.f,0.f);
		}
		else
		{
			up = Vector3(0.f,0.f,-1.f);
		}

		Moo::Colour colour(col);

		{
			lookat.lookAt(end, dir, up );				
			lookat.invert();
			float s = 0.03f;
			Vector3 v1( -s, 0.f, -s);
			Vector3 v2(  s, 0.f, -s);
			Vector3 v3(0.f, 0.f,  s);
			triAdd( WorldTriangle( lookat.applyPoint(v1), lookat.applyPoint(v2), lookat.applyPoint(v3) ), colour );
		}

		{
			lookat.lookAt(start, dir, up);
			lookat.invert();
			float s = 0.001f;
			Vector3 v1( -s,  0.f, 0.f);
			Vector3 v2( s,   0.f, 0.f);
			Vector3 v3( 0.f, 0.f,   l);
			triAdd( WorldTriangle( lookat.applyPoint(v1), lookat.applyPoint(v2), lookat.applyPoint(v3) ), colour );
		}
	}
}

void polyAdd( const WorldPolygon & wp, uint32 col )
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}
	s_dpolys.push_back( std::make_pair( wp, Moo::Colour( col ) ) );
}

void lineAdd( const Vector3 & p1, const Vector3 & p2, uint32 col )
{
	if (!s_enabled)
	{
		return;
	}
	WorldPolygon line(2);
	line[0] = p1;
	line[1] = p2;
	polyAdd( line, col );
}

void bboxAdd( const AABB & bbox, uint32 col )
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}
	const Vector3 & minV = bbox.minBounds();
	const Vector3 & maxV = bbox.maxBounds();

	const Vector3 & v0 = minV;
	const Vector3 v1(maxV.x, minV.y, minV.z);
	const Vector3 v2(maxV.x, maxV.y, minV.z);
	const Vector3 v3(minV.x, maxV.y, minV.z);

	const Vector3 v4(minV.x, minV.y, maxV.z);
	const Vector3 v5(maxV.x, minV.y, maxV.z);
	const Vector3 & v6 = maxV;
	const Vector3 v7(minV.x, maxV.y, maxV.z);

	WorldPolygon bboxFace(4);
	// Front face
	{
		bboxFace[0] = v0;
		bboxFace[1] = v1;
		bboxFace[2] = v2;
		bboxFace[3] = v3;
		polyAdd( bboxFace, col );
	}
	// Back face
	{
		bboxFace[0] = v4;
		bboxFace[1] = v5;
		bboxFace[2] = v6;
		bboxFace[3] = v7;
		polyAdd( bboxFace, col );
	}
	// Top face
	{
		bboxFace[0] = v3;
		bboxFace[1] = v2;
		bboxFace[2] = v6;
		bboxFace[3] = v7;
		polyAdd( bboxFace, col );
	}
	// Bottom face
	{
		bboxFace[0] = v0;
		bboxFace[1] = v1;
		bboxFace[2] = v5;
		bboxFace[3] = v4;
		polyAdd( bboxFace, col );
	}
}

void draw()
{
	BW_GUARD;
	if (!s_enabled)
	{
		return;
	}

	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	for (uint i = 0; i < s_dtris.size(); i++)
	{
		WorldTriangle & triangle = s_dtris[i].first;
		Moo::Colour & col = s_dtris[i].second;
		Geometrics::drawLine( triangle.v0(), triangle.v1(), col );
		Geometrics::drawLine( triangle.v1(), triangle.v2(), col );
		Geometrics::drawLine( triangle.v2(), triangle.v0(), col );
	}

	for (uint i = 0; i < s_dpolys.size(); i++)
	{
		WorldPolygon & poly = s_dpolys[i].first;
		Moo::Colour & col = s_dpolys[i].second;

		for (int j = 0; j < int(poly.size()) - 1; j++)
		{
			Geometrics::drawLine( poly[j], poly[j+1], col );
		}

		if (!poly.empty() && poly.size() > 2)
		{
			Geometrics::drawLine( poly.back(), poly.front(), col );
		}
	}

	Moo::rc().pop();

	s_dtris.clear();
	s_dpolys.clear();
}

} // namespace DebugDraw

BW_END_NAMESPACE

