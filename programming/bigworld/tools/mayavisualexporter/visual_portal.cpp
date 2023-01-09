#include "pch.hpp"

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include "visual_portal.hpp"
//#include "max.h"
#include "cstdmf/debug.hpp"
#include "math/graham_scan.hpp"

#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "Exporter", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: VisualPortal
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualPortal::VisualPortal()
{
}


/**
 *	Destructor.
 */
VisualPortal::~VisualPortal()
{
}


/**
 *	Add point
 */
void VisualPortal::addPoint( const Vector3 & pt )
{
	pts_.push_back( Vector3( pt.x, pt.z, pt.y ) );
}


/**
 *	Add an already swizzled point ( do not swap pt.z, pt.y )
 */
void VisualPortal::addSwizzledPoint( const Vector3 & pt )
{
	pts_.push_back( pt );
}


/**
 *	Set name
 */
void VisualPortal::name( const BW::string & name )
{
	name_ = name;
}



const char * propStrs[] = { "heaven", "earth", "invasive", "exit" };
const char * dataStrs[] = { "heaven", "earth", "invasive", "invasive" };


/**
 *	This method converts the given property name into the appropriate
 *	data name, or returns NULL if the property name is not a portal name.
 */
const char* VisualPortal::propDataFromPropName( const char* propName )
{
	for (int i = 0; i < sizeof(propStrs)/sizeof(propStrs[0]); i++)
	{
		if (!::_stricmp(propName,propStrs[i]))
			return dataStrs[i];
	}

	return NULL;
}


/**
 *	This method sets the portal name from the user properties
 *	in the given node.
 */
/*
void VisualPortal::nameFromProps( INode* node )
{
	for (int i = 0; i < sizeof(propStrs)/sizeof(propStrs[0]); i++)
	{
		BOOL isProp = false;
		node->GetUserPropBool( propStrs[i], isProp );
		if (isProp)
		{
			const char * data = propDataFromPropName(propStrs[i]);
			if (data) this->name(data);
		}
	}
}
*/

/**
 *	Save to the given data section.
 *
 *	The section must be a boundary section, with
 *	a plane normal and d-value already written out.
 */
void VisualPortal::save( DataSectionPtr pInSect )
{
	MF_ASSERT( pts_.size() >= 3 )
	MF_ASSERT( pInSect->findChild( "normal" ) );
	MF_ASSERT( pInSect->findChild( "d" ) );

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
	pPortal->setString( label_ );
	if ( name_ != "" )
		pPortal->writeString( "chunk", name_ );
	pPortal->writeVector3( "uAxis", uAxis );
	for (uint32 i = 0; i < pts_.size(); i++)
	{
		pPortal->newSection( "point" )->setVector3(
			invBasis.applyPoint(pts_[i]) );
	}
}


void VisualPortal::planeEquation( PlaneEq& result )
{
	result.init( pts_[0], pts_[1], pts_[2], PlaneEq::SHOULD_NORMALISE );
}



void VisualPortal::reverseWinding()
{
	std::reverse( pts_.begin(), pts_.end() );
}

bool VisualPortal::init( Portal& portal)
{
	// copy flags
	name_ = portal.flags();
	label_ = portal.label();

	// copy points
	BW::vector<Point3>::iterator it = portal.positions().begin();
	for ( ; it != portal.positions().end(); ++it )
	{
		Point3 pt = (Point3)(*it);
		// TODO Point ordering.. swap Y & Z?
		// TODO check point positions, the Max code multiplies each point by the portal but we are getting the world coords straight off
		pts_.push_back( Vector3( pt.x, pt.y, pt.z ) * ExportSettings::instance().unitScale() );
	}

	// make sure all points are aligned on the same plane

	this->createConvexHull();	

	return true;
}


void VisualPortal::createConvexHull()
{
	// make a convex hull of the points
	GrahamScan gs(pts_);
}

BW_END_NAMESPACE

