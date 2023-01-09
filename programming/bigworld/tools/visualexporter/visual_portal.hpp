#ifndef VISUAL_PORTAL_HPP
#define VISUAL_PORTAL_HPP

#include "cstdmf/bw_vector.hpp"
#include "math/planeeq.hpp"
#include "math/boundbox.hpp"
#include "resmgr/datasection.hpp"

// forward declaration from max sdk.
class INode;

BW_BEGIN_NAMESPACE

/**
 *	This class is a portal in a visual
 */
class VisualPortal : public ReferenceCount
{
public:
	VisualPortal();
	~VisualPortal();

	//this method adds a point to the portal,
	//swapping the y,z elements.
	void addPoint( const Vector3 & pt );
	//this method adds a point to the portal,
	//but does not swap the y,z elements.
	void addSwizzledPoint( const Vector3 & pt );
	void name( const BW::string & name );
	void save( DataSectionPtr pInSect );
	void planeEquation( PlaneEq& result );
	bool cull( const BoundingBox& bb, bool& culled );
	bool cullHull( const BW::vector<PlaneEq>& boundaries, bool& culled );
	void reverse();

	void nameFromProps( INode* n );
	static const char* propDataFromPropName( const char* propName );

	//This helper method takes all the points in the pts_
	//container, and sorts them into a convex hull, based
	//on the plane equation gleened from the first 3 points.
	void createConvexHull();

private:
	VisualPortal( const VisualPortal& );
	VisualPortal& operator=( const VisualPortal& );

	

	BW::vector<Vector3>	pts_;
	BW::string				name_;
	BW::string				label_;
};


typedef SmartPointer<VisualPortal>	VisualPortalPtr;

BW_END_NAMESPACE


#endif // VISUAL_PORTAL_HPP
