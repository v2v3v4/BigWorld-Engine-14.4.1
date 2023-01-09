#include "pch.hpp"

#include <memory>
#include "moo/visual.hpp"
#include "physics2/bsp.hpp"

BW_USE_NAMESPACE

TEST( BSPTreeHelper_testCreateVertexList )
{
	{
		// No triangles
		RealWTriangleSet empty;
		std::auto_ptr<BSPTree> emptyBsp( BSPTreeTool::buildBSP( empty ) );

		// Test Empty
		BW::vector<Moo::VertexXYZL> results;
		Moo::BSPTreeHelper::createVertexList( * emptyBsp, results );
		CHECK_EQUAL( 0, results.size() );
	}

	{
		// One triangle, no collision
		RealWTriangleSet singleNoCollide;
		singleNoCollide.push_back( WorldTriangle( Vector3::zero(),
			Vector3( 1.0f, 2.0f, 3.0f ),
			Vector3( 3.0f, 2.0f, 1.0f ),
			TRIANGLE_NOCOLLIDE ) );
		std::auto_ptr<BSPTree> singleNoCollideBsp(
			BSPTreeTool::buildBSP( singleNoCollide ) );

		// Test Single (no collision)
		BW::vector<Moo::VertexXYZL> results;
		Moo::BSPTreeHelper::createVertexList( * singleNoCollideBsp, results );
		CHECK_EQUAL( 0, results.size() );
	}

	{
		// One triangle, collision
		RealWTriangleSet singleCollide;
		singleCollide.push_back( WorldTriangle( Vector3::zero(),
			Vector3( 1.0f, 2.0f, 3.0f ),
			Vector3( 3.0f, 2.0f, 1.0f ),
			0 ) );
		std::auto_ptr<BSPTree> singleCollideBsp(
			BSPTreeTool::buildBSP( singleCollide ) );

		// Test Single (with collision)
		BW::vector<Moo::VertexXYZL> results;
		Moo::BSPTreeHelper::createVertexList( * singleCollideBsp, results );
		CHECK_EQUAL( 3, results.size() );

		CHECK_EQUAL( results[0].pos_ , singleCollideBsp->triangles()[0].v0() );
		CHECK_EQUAL( results[1].pos_ , singleCollideBsp->triangles()[0].v1() );
		CHECK_EQUAL( results[2].pos_ , singleCollideBsp->triangles()[0].v2() );
	}
};
