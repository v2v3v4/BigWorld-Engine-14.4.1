#include "pch.hpp"

#include "physics2/bsp.hpp"

// Not compiled on server, due to unusual linkage issues to be fixed later.
#ifndef MF_SERVER


BW_BEGIN_NAMESPACE

TEST( BSPTree_Construction )
{
	// No triangles
	RealWTriangleSet empty;

	// One triangle
	RealWTriangleSet single;
	single.push_back( WorldTriangle( Vector3::zero(),
									 Vector3( 1.0f, 2.0f, 3.0f ),
									 Vector3( 3.0f, 2.0f, 1.0f ),
										0 ) );

	// Store number of nodes and triangles
	int nodes = 0, tris = 0;

	// Empty BSP is empty and has no root
	BSPTree* emptyBsp = BSPTreeTool::buildBSP( empty );
	CHECK_EQUAL( true ,		emptyBsp->numTriangles() == 0 );

	// Empty BSP has one node and zero triangles
	CHECK_EQUAL( 1,			emptyBsp->numNodes() );
	CHECK_EQUAL( 0,			emptyBsp->numTriangles() );

	bw_safe_delete( emptyBsp );

	// Single BSP is not empty
	BSPTree* singleBsp = BSPTreeTool::buildBSP( single );
	CHECK_EQUAL( false,		singleBsp->numTriangles() == 0 );

	// Single BSP has one node and one triangle
	CHECK_EQUAL( 1,			singleBsp->numNodes() );
	CHECK_EQUAL( 1,			singleBsp->numTriangles() );\

	bw_safe_delete( singleBsp );
}

TEST( BSP_Intersection )
{
	// create a simple tree with a single triangle on the x plane
	RealWTriangleSet		single;
	single.push_back( 
		WorldTriangle( 
			Vector3::zero(),
			Vector3( 0.0f, 0.0f, 5.0f ),
			Vector3( 0.0f, 5.0f, 0.0f ),
			0 ) 
		);
	BSPTree*				simpleBSP = BSPTreeTool::buildBSP( single );
	float					interval = 1.0f;
	const WorldTriangle*	pHitTriangle;

	// test intersection against a ray
	bool hit = simpleBSP->intersects(	Vector3(-5.0f, 0.0f, 2.0f ),
										Vector3( 5.0f, 2.0f, 0.0f ),
										interval,
										&pHitTriangle );
	CHECK_EQUAL( true, hit );
	CHECK_EQUAL( 0.5f, interval );
	bw_safe_delete( simpleBSP );
};

class TestVisitor: public CollisionVisitor
{
public:
	TestVisitor()
		: numHits_( 0 )
	{
	}

	virtual bool visit( const WorldTriangle & hitTriangle, float dist )
	{
		numHits_++;		
		return numHits_ == 2; // terminate after two hits
	}

	uint32 numHits_;
};

TEST( BSP_Multiple_Intersection )
{
	// create a simple tree with two coplanar triangles on the x plane
	RealWTriangleSet		multiple;
	multiple.push_back( 
		WorldTriangle( 
			Vector3::zero(),
			Vector3( 0.0f, 0.0f, 5.0f ),
			Vector3( 0.0f, 5.0f, 0.0f ),
			0 ) 
		);
	multiple.push_back( 
		WorldTriangle( 
			Vector3::zero(),
			Vector3( 0.0f, 0.0f, 6.0f ),
			Vector3( 0.0f, 6.0f, 0.0f ),
			0 ) 
		);

	BSPTree*				simpleBSP = BSPTreeTool::buildBSP(  multiple );
	float					interval = 1.0f;
	TestVisitor				visitor;

	// test intersection against a ray - visitor should return a hit for each
	// triangle (even though they are coplanar).
	bool hit = simpleBSP->intersects(	Vector3(-5.0f, 2.0f, 2.0f ),
										Vector3( 5.0f, 2.0f, 2.0f ),
										interval,
										NULL,
										&visitor );
	CHECK_EQUAL( true,	hit );
	CHECK_EQUAL( 0.5f,	interval );
	CHECK_EQUAL( 2,		visitor.numHits_ );
	bw_safe_delete( simpleBSP );
}

BW_END_NAMESPACE

#endif

