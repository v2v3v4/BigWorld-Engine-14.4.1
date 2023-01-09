#include "pch.hpp"
#include "collision_scene_provider.hpp"
#include "physics2/bsp.hpp"

BW_BEGIN_NAMESPACE

BSPTreeCollisionSceneProvider::BSPTreeCollisionSceneProvider( const BSPTree* bsp, const Matrix& transform,
															 const RealWTriangleSet& portalTriangles )
	: transform_( transform )
{
	triangles_.assign( bsp->triangles(), bsp->triangles() + bsp->numTriangles() );
	portalTriangles_.assign( portalTriangles.begin(), portalTriangles.end() );
}


void BSPTreeCollisionSceneProvider::appendCollisionTriangleList(
	BW::vector<Vector3>& triangleList ) const
{
	for (RealWTriangleSet::const_iterator it = triangles_.begin();
		it != triangles_.end(); ++it)
	{
		const WorldTriangle & tri = *it;

		if (!( tri.flags() & TRIANGLE_NOCOLLIDE ))
		{
			triangleList.push_back( transform_.applyPoint( tri.v0() ) );
			triangleList.push_back( transform_.applyPoint( tri.v1() ) );
			triangleList.push_back( transform_.applyPoint( tri.v2() ) );
		}
	}
}


size_t BSPTreeCollisionSceneProvider::collisionTriangleCount() const
{
	return triangles_.size();
}


void BSPTreeCollisionSceneProvider::feed( CollisionSceneConsumer* consumer ) const
{
	static const int LOOP_TIME_BEFORE_CHECK = 1024;
	int loopTimes = 0;

	for (RealWTriangleSet::const_iterator it = triangles_.begin();
		it != triangles_.end(); ++it)
	{
		++loopTimes;

		if (loopTimes > LOOP_TIME_BEFORE_CHECK)
		{
			loopTimes = 0;

			if (consumer->stopped())
			{
				break;
			}
		}

		const WorldTriangle & tri = *it;

		if (!( tri.flags() & TRIANGLE_NOCOLLIDE ))
		{
			consumer->consume( transform_.applyPoint( tri.v0() ) );
			consumer->consume( transform_.applyPoint( tri.v1() ) );
			consumer->consume( transform_.applyPoint( tri.v2() ) );
		}
	}
}


void BSPTreeCollisionSceneProvider::feedPortals( CollisionSceneConsumer* consumer ) const
{
	static const int LOOP_TIME_BEFORE_CHECK = 1024;
	int loopTimes = 0;

	for (RealWTriangleSet::const_iterator it = portalTriangles_.begin();
		it != portalTriangles_.end(); ++it)
	{
		++loopTimes;

		if (loopTimes > LOOP_TIME_BEFORE_CHECK)
		{
			loopTimes = 0;

			if (consumer->stopped())
			{
				break;
			}
		}

		const WorldTriangle & tri = *it;

		if (!( tri.flags() & TRIANGLE_NOCOLLIDE ))
		{
			consumer->consumePortal( transform_.applyPoint( tri.v0() ) );
			consumer->consumePortal( transform_.applyPoint( tri.v1() ) );
			consumer->consumePortal( transform_.applyPoint( tri.v2() ) );
		}
	}
}


void CollisionSceneProviders::append( CollisionSceneProviderPtr provider )
{
	if (provider)
	{
		providers_.push_back( provider );
	}
}


void CollisionSceneProviders::appendCollisionTriangleList(
	BW::vector<Vector3>& triangleList ) const
{
	size_t count = 0;

	for (Providers::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter)
	{
		count += (*iter)->collisionTriangleCount();
	}

	triangleList.reserve( count * 3 );

	for (Providers::const_iterator iter = providers_.begin();
		iter != providers_.end(); ++iter)
	{
		(*iter)->appendCollisionTriangleList( triangleList );
	}
}


bool CollisionSceneProviders::feed( CollisionSceneConsumer* consumer ) const
{
	for (Providers::const_iterator iter = providers_.begin();
		iter != providers_.end() && !consumer->stopped(); ++iter)
	{
		(*iter)->feed( consumer );
	}

	consumer->flush();

	return !consumer->stopped();
}

bool CollisionSceneProviders::feedPortals( CollisionSceneConsumer* consumer ) const
{
	for (Providers::const_iterator iter = providers_.begin();
		iter != providers_.end() && !consumer->stopped(); ++iter)
	{
		(*iter)->feedPortals( consumer );
	}

	consumer->flush();

	return !consumer->stopped();
}



bool CollisionSceneProviders::dumpOBJ( const BW::string& filename ) const
{
	FILE* fp = bw_fopen( filename.c_str(), "wt" );
	if( !fp )
	{
		return false;
	}

	fprintf(fp, "# Collision scene dump\n");
	fprintf(fp, "o CollisionSceneProvider\n");
	fprintf(fp, "\n");

	BW::vector<Vector3> triList;

	appendCollisionTriangleList(triList);

	for (BW::vector<Vector3>::const_iterator it = triList.begin();
		it != triList.end(); ++it)
	{
		const Vector3 & tri = *it;

		// vertex
		fprintf(fp, "v %f %f %f\n", tri[0], tri[1], tri[2] );
	}

	int totalTris = static_cast<int>( triList.size() );

	for(int i = 1; i <= totalTris; i+=3)
	{
		// face
		fprintf(fp, "f %i %i %i\n", i, i+1, i+2);
	}


	fclose(fp);

	return true;
}

BW_END_NAMESPACE

// collision_scene_provider.cpp
