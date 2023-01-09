#ifndef VISUAL_PORTAL_HPP
#define VISUAL_PORTAL_HPP

#include "cstdmf/bw_vector.hpp"
#include "math/planeeq.hpp"
#include "math/boundbox.hpp"
#include "resmgr/datasection.hpp"
#include "mesh_piece.hpp"
#include "node.hpp"


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{

/**
 *	This class is a portal in a visual
 */
class Portal : public ReferenceCount
{
public:
	Portal();
	~Portal();

	void init( MeshPiecePtr pMesh, NodePtr pNode );

	void addPoint( const Vector3 & pt );
	void type( const BW::string& type );
//	void label( const BW::string& label );
	void save( DataSectionPtr pInSect );
	void planeEquation( PlaneEq& result );
	bool cull( const BoundingBox& bb, bool& culled );
	bool cullHull( const BW::vector<PlaneEq>& boundaries, bool& culled );
	void reverse();

	//This helper method takes all the points in the pts_
	//container, and sorts them into a convex hull, based
	//on the plane equation gleened from the first 3 points.
	void createConvexHull();

	static bool isPortal( const BW::string& name );
	static BW::string portalType( const BW::string& name );
//	static BW::string portalName( const BW::string& name );

private:
	Portal( const Portal& );
	Portal& operator=( const Portal& );

	BW::vector<Vector3>	pts_;
	BW::string				type_;
};


typedef SmartPointer<Portal>	PortalPtr;

};

BW_END_NAMESPACE

#endif // VISUAL_PORTAL_HPP
