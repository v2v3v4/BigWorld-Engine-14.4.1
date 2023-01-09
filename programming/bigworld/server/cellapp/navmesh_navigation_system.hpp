#ifndef NAVMESH_NAVIGATION_SYSTEM_HPP
#define NAVMESH_NAVIGATION_SYSTEM_HPP

#include "Python.h"

#include "navigation_controller.hpp"

#include "waypoint/navigator.hpp"
#include "waypoint/navloc.hpp"


BW_BEGIN_NAMESPACE

class NavigateStepController;

/**
 *	This class implements a NavigationSystem that is based on BigWorld's
 *	navmesh.
 */
class NavmeshNavigationSystem
{
public:
	static bool findRandomNeighbourPointWithRange( ChunkSpace * pSpace,
		Vector3 position, float minRadius, float maxRadius, float girth,
		Vector3 & result );

	static PyObject * navigatePathPoints( ChunkSpace * pSpace,
		const Vector3 & src, const Vector3 & dst, float maxSearchDistance,
		float girth );
};


/**
 *	This class implements an EntityNavigationSystem that is based on BigWorld's
 *	navmesh.
 */
class NavmeshEntityNavigationSystem
{
public:
	NavmeshEntityNavigationSystem( Entity & entity );

	bool getNavigatePosition( const NavLoc & srcLoc, const NavLoc & dstLoc,
		float maxDistance, Vector3 & nextPosition, bool & passedActivatedPortal,
		float girth = 0.5f );

	Navigator & navigator() { return navigator_; }

	Controller * createController( const Vector3 & dstPosition,
		float velocity, bool faceMovement, float maxSearchDistance,
		float girth, float closeEnough ) const;

	PyObject * getStopPoint( const Vector3 & destination,
		bool ignoreFirstStopPoint, float maxSearchDistance,
		float girth, float stopDist, float nearPortalDist );

	PyObject * navigateStep( NavigateStepController* pNSController,
		const Vector3 & dstPosition,
		float velocity, float maxMoveDistance, float maxSearchDistance, 
		bool faceMovement, float girth, int userArg );

	PyObject * canNavigateTo( const Vector3 & dstPosition,
		float maxSearchDistance, float girth ) const;

	PyObject * navigatePathPoints( const Vector3 & dstPosition,
		float maxSearchDistance, float girth ) const;

	int waySetPathLength();

	void navigationDump( const Vector3 & dstPosition, float girth );

private:
	Navigator navigator_;
	Entity & entity_;
};

BW_END_NAMESPACE

#endif // NAVMESH_NAVIGATION_SYSTEM_HPP
