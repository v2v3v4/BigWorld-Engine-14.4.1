#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include "pch.hpp"

#include "portal.hpp"

#include "cstdmf/debug.hpp"
#include "math/graham_scan.hpp"
#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "VisualManipulator", 0 )


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;


// -----------------------------------------------------------------------------
// Section: Portal
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Portal::Portal()
{
}


/**
 *	Destructor.
 */
Portal::~Portal()
{
}


/**
 *	Add point
 */
void Portal::addPoint( const Vector3 & pt )
{
	pts_.push_back( pt );
}


/**
 *	Set type
 */
void Portal::type( const BW::string& type )
{
	type_ = type;
}

/*void Portal::label( const BW::string& label )
{
	label_ = label;
}*/


/**
 *	Save to the given data section.
 *
 *	The section must be a boundary section, with
 *	a plane normal and d-value already written out.
 */
void Portal::save( DataSectionPtr pInSect )
{
	MF_ASSERT( pts_.size() >= 3 )
	MF_ASSERT( &*pInSect->findChild( "normal" ) != NULL );
	MF_ASSERT( &*pInSect->findChild( "d" ) != NULL );

	//read in the boundary section, to create the plane basis
	//and generate the uaxis, and plane-local points.
	Vector3 normal = pInSect->readVector3( "normal" );
	float d = pInSect->readFloat( "d" );
	PlaneEq peq( normal, d );

	//create the basis matrix
	Vector3 uAxis = pts_[1] - pts_[0];
	uAxis.normalise();
	Vector3 vAxis = normal.crossProduct(uAxis);

	Matrix basis;
	basis[0] = uAxis;
	basis[1] = vAxis;
	basis[2] = normal;
	basis.translation( normal * d );
	Matrix invBasis; invBasis.invert( basis );

	DataSectionPtr pPortal = pInSect->newSection( "portal" );
//	pPortal->setString( label_ );
	if ( type_ != "" )
		pPortal->writeString( "chunk", type_ );
	pPortal->writeVector3( "uAxis", uAxis );
	for (uint32 i = 0; i < pts_.size(); i++)
	{
		pPortal->newSection( "point" )->setVector3(
			invBasis.applyPoint(pts_[i]) );
	}
}


// this is a quick implementation, not optimised
static bool cullByPlane( BW::vector<Vector3>& convex, const PlaneEq& plane )
{
	bool culled = false;
	BW::vector<Vector3> newConvex;
	for( BW::vector<Vector3>::iterator iter = convex.begin();
		convex.size() > 2 && iter != convex.end(); ++iter )
	{
		BW::vector<Vector3>::iterator iter2 = ( iter + 1 != convex.end() ) ? iter + 1 : convex.begin();
		if( !plane.isInFrontOfExact( *iter ) )
		{
			newConvex.push_back( *iter );
			if( plane.isInFrontOfExact( *iter2 ) )
			{
				newConvex.push_back( plane.intersectRay( *iter, *iter2 - *iter ) );
				culled = true;
			}
		}
		else
		{
			if( !plane.isInFrontOfExact( *iter2 ) )
			{
				newConvex.push_back( plane.intersectRay( *iter, *iter2 - *iter ) );
				culled = true;
			}
		}
	}
	convex = newConvex;
	if( convex.size() < 3 )
		convex.clear();

	return culled;
}


bool Portal::cull( const BoundingBox& cbb, bool& culled )
{
	//adjust bounding box to account for floating point imprecision.
	BoundingBox bb(cbb);	
	float epsilon = 0.0001f;
	bb.addBounds( bb.minBounds() + Vector3(-epsilon,-epsilon,-epsilon) );
	bb.addBounds( bb.maxBounds() + Vector3(epsilon, epsilon, epsilon) );

	culled = false;
	culled |= cullByPlane( pts_, PlaneEq( Vector3( bb.minBounds().x, 0.f, 0.f ), Vector3( -1.f, 0.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, bb.minBounds().y, 0.f ), Vector3( 0.f, -1.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, 0.f, bb.minBounds().z ), Vector3( 0.f, 0.f, -1.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( bb.maxBounds().x, 0.f, 0.f ), Vector3( 1.f, 0.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, bb.maxBounds().y, 0.f ), Vector3( 0.f, 1.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, 0.f, bb.maxBounds().z ), Vector3( 0.f, 0.f, 1.f ) ) );
	return !pts_.empty();
}


bool Portal::cullHull( const BW::vector<PlaneEq>& boundaries, bool& culled )
{
	culled = false;
	float epsilon = 0.0001f;

	// Iterate through the boundaries
	BW::vector<PlaneEq>::const_iterator cit;
	for(cit = boundaries.begin();
		cit != boundaries.end();
		++cit)
	{
		// Adjust plane to account for floating point inaccuracies
		PlaneEq testPlane(-cit->normal(), -cit->d() + epsilon);

		culled |= cullByPlane( pts_, testPlane );
	}

	return !pts_.empty();
}


void Portal::reverse()
{
	std::reverse( pts_.begin(), pts_.end() );
}


void Portal::planeEquation( PlaneEq& result )
{
	result.init( pts_[0], pts_[1], pts_[2], PlaneEq::SHOULD_NORMALISE );
}


void Portal::createConvexHull()
{
	GrahamScan gs(pts_);
}


namespace
{
	bool endsWith( const BW::string& s, const BW::string& token )
	{
		BW::string::size_type tlen = token.length();
		BW::string::size_type slen = s.length();

		return slen >= tlen && 0 == s.compare( slen - tlen, tlen, token );
	}
}


bool Portal::isPortal( const BW::string& name )
{
	return endsWith( name, "_portal" ) || endsWith( name, "_exitportal" ) ||
		endsWith( name, "_heavenportal" );
}


BW::string Portal::portalType( const BW::string& name )
{
	if (endsWith(name, "heaven_portal"))
	{
		return BW::string( "heaven" );
	}
	else if (endsWith(name, "exit_portal"))
	{
		return BW::string( "invasive" );
	}

	return BW::string();
}


void Portal::init( MeshPiecePtr pMesh, NodePtr pNode )
{
	Matrix world = pNode->worldTransform();

	VisualManipulator::TriangleVector& tris = pMesh->triangles();
	VisualManipulator::VertexVector& verts = pMesh->vertices();

	for (uint32 i = 0; i < tris.size(); i++)
	{
		Vector3 a = world.applyPoint( verts[tris[i].a].pos );
		Vector3 b = world.applyPoint( verts[tris[i].b].pos );
		Vector3 c = world.applyPoint( verts[tris[i].c].pos );

		addPoint( a );
		addPoint( b );
		addPoint( c );
	}
	
	type_ = portalType( pMesh->name() );

	createConvexHull();
}

BW_END_NAMESPACE
