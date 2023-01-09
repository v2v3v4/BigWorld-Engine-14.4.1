#include "script/first_include.hpp"

#include "entity.hpp"
#include "navmesh_navigation_system.hpp"

#include "move_controller.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

int NavmeshEntityNavigationSystem_token = 0;


namespace Util
{

/**
 *	This helper function returns a list of points to navigate between the given
 *	source location to the given destination, for the given ChunkSpace. This is
 *	called by Entity.navigatePathPoints() and BigWorld.navigatePathPoints().
 */ 
PyObject * navigatePathPoints( const Navigator & navigator, 
		ChunkSpace * pChunkSpace, const Vector3 & src, const Vector3 & dst, 
		float maxSearchDistance, float girth, bool checkFirstPosition )
{
	Vector3Path pathPoints;

	if (!navigator.findFullPath( pChunkSpace, src, dst, maxSearchDistance, 
		/* blockNonPermissive: */ true, girth, pathPoints ))
	{
		PyErr_SetString( PyExc_ValueError, 
			"Full navigation path could not be found" );
		return NULL;
	}

	if (checkFirstPosition && !almostEqual( src, pathPoints.front() ))
	{
		pathPoints.insert( pathPoints.begin(), src );
	}

	PyObject * pList = PyList_New( pathPoints.size() );

	Vector3Path::const_iterator iPoint = pathPoints.begin();
	while (iPoint != pathPoints.end())
	{
		PyList_SetItem( pList, iPoint - pathPoints.begin(), 
			Script::getData( *iPoint ) );
		++iPoint;
	}

	return pList;
}

} // namespace Util


/**
 *	
 */
bool NavmeshNavigationSystem::findRandomNeighbourPointWithRange(
	ChunkSpace * pSpace, Vector3 position, float minRadius, float maxRadius,
	float girth, Vector3 & result )
{
	static Navigator navigator;

	return navigator.findRandomNeighbourPointWithRange( pSpace, position,
		minRadius, maxRadius, girth, result );
}


/**
 *	
 */
PyObject * NavmeshNavigationSystem::navigatePathPoints( ChunkSpace * pSpace,
	const Vector3 & src, const Vector3 & dst, float maxSearchDistance,
	float girth )
{
	static Navigator navigator;

	return Util::navigatePathPoints( navigator, pSpace, 
		src, dst, maxSearchDistance, girth, true );
}


/**
 *	
 */
NavmeshEntityNavigationSystem::NavmeshEntityNavigationSystem(
		Entity & entity ) :
	entity_( entity )
{
}


/**
 *	
 */
Controller * NavmeshEntityNavigationSystem::createController(
	const Vector3 & dstPosition, float velocity, bool faceMovement,
	float maxSearchDistance, float girth, float closeEnough ) const
{
	return new NavigationController( dstPosition, velocity,
		faceMovement, maxSearchDistance, girth, closeEnough );
}


/**
 *	
 */
PyObject * NavmeshEntityNavigationSystem::getStopPoint(
	const Vector3 & destination, bool ignoreFirstStopPoint,
	float maxSearchDistance, float girth, float stopDist, float nearPortalDist )
{
	Vector3 source = entity_.position();

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
				"NavmeshEntityNavigationSystem::getStopPoint(): "
				"This method can only be called on a real entity." );
		return NULL;
	}

	// Determine the source NavLoc.
	NavLoc srcLoc = navigator_.createPathNavLoc(entity_.pChunkSpace(),
		entity_.position(), girth );
	if (!srcLoc.valid()) // complain if invalid
	{
		PyErr_Format( PyExc_ValueError,
				"NavmeshEntityNavigationSystem::getStopPoint(): "
				"Could not resolve source position" );
		return NULL;
	}

	//ToDo: change when navigation system supports non-grid spaces
	if (!entity_.pChunkSpace())
	{
		PyErr_Format( PyExc_ValueError,
				"NavmeshEntityNavigationSystem::getStopPoint(): "
				"Physical space of entity does not support navigation queries" );
		return NULL;
	}

	// Determine the dest NavLoc.
	NavLoc dstLoc( entity_.pChunkSpace(), destination, girth );
	if (!dstLoc.valid()) // complain if invalid
	{
		PyErr_Format( PyExc_ValueError,
				"NavmeshEntityNavigationSystem::getStopPoint(): "
				"Could not resolve destination position." );
		return NULL;
	}

	dstLoc.clip();

	// Prepare jump over waypoint path.
	// entity_.pReal()->navigator().clearCache();
	Vector3 position = source; // entity_.position();
	BW::vector< Vector3 > stepPoints;
	stepPoints.push_back( position );
	int safetyCount = 500;
	bool skippedFirstPoint = false;
	while ((fabs( position.x - destination.x ) > 0.01f ||
			    fabs( position.z - destination.z ) > 0.01f) &&
			(--safetyCount > 0))
	{

		bool passedActivatedPortal;

		if (!this->getNavigatePosition( srcLoc, dstLoc, maxSearchDistance, 
				position, passedActivatedPortal, girth ))
		{
			Py_RETURN_NONE;
		}

		srcLoc = navigator_.createPathNavLoc(entity_.pChunkSpace(),
				position, girth );
		if (!srcLoc.valid())	// complain if srcLoc invalid
		{
			PyErr_Format( PyExc_ValueError, "Source location not valid." );
			return NULL;
		}

		stepPoints.push_back( position );

		if (passedActivatedPortal)
		{
			// We've changed chunks. Calculate distance traveled from start.

			float dist = 0.0;

			for (uint i = 1; i < stepPoints.size(); ++i) // HERE
			{
				dist += sqrtf( 
					(stepPoints[i].x - stepPoints[i-1].x) * 
						(stepPoints[i].x - stepPoints[i-1].x) +
					(stepPoints[i].z - stepPoints[i-1].z) * 
						(stepPoints[i].z - stepPoints[i-1].z) );
			}

			// If starting point is a long way away, then we definitely want
			// to stop at this portal.
			if (dist > nearPortalDist)
			{
				// But if we're told to skip the first point, and we haven't
				// yet, then this is a problem.
				if (!skippedFirstPoint && ignoreFirstStopPoint)
				{
					PyErr_Format( PyExc_ValueError, "Entity.getStopPoint: "
							"told to ignore first portal, but further than "
							"nearPortalDist away from it." );

					return NULL;
				}
				// The following will never happen, but put check in just for
				// good measure.
				if (skippedFirstPoint && !ignoreFirstStopPoint)
				{
					PyErr_Format( PyExc_AssertionError, "Entity.getStopPoint: "
						"Programmer logic error." );

					return NULL;
				}

				// This is the portal we want to stop at. Find the position
				// stopDist back from the portal.
				float backDist = 0.0;
				float prevBackDist = 0.0;
				float lastLength = 0.0;
				int i = (int)stepPoints.size() - 1; // HERE
				for ( ; i > 0; --i )
				{
					lastLength = sqrtf( 
						(stepPoints[i].x - stepPoints[i-1].x) *
							(stepPoints[i].x - stepPoints[i-1].x) +
						(stepPoints[i].z - stepPoints[i-1].z) *
							(stepPoints[i].z - stepPoints[i-1].z) );

					backDist += lastLength;

					if (backDist > stopDist)
					{
						break;
					}

					prevBackDist = backDist;
				}

				// Will never happen but check anyway.
				if (backDist < stopDist)
				{
					PyErr_Format( PyExc_AssertionError, "Entity.getStopPoint: "
						"Programmer logic error (1)." );
					return NULL;
				}

				float prop = (stopDist - prevBackDist) / lastLength;

				Vector3 dir( (stepPoints[i-1].x - stepPoints[i].x), 0.0,
					(stepPoints[i-1].z - stepPoints[i].z) );

				dir.normalise();

				dir.x *= prop;
				dir.z *= prop;

				Vector3 stopPosition = stepPoints[i]; // HERE
				stopPosition += dir;

				ScriptTuple result = ScriptTuple::create( 2 );
				MF_VERIFY( result.setItem( 0, ScriptObject::createFrom( stopPosition ) ) );
				MF_VERIFY( result.setItem( 1, ScriptObject::createFrom( false ) ) );
				return result.newRef();
			}

			// Otherwise starting point is close to this portal.
			else
			{
				// If we're not ignoring the first portal, then return the
				// start point.
				if (!ignoreFirstStopPoint)
				{
					ScriptTuple result = ScriptTuple::create( 2 );
					MF_VERIFY( result.setItem( 0, ScriptObject::createFrom( source ) ) );
					MF_VERIFY( result.setItem( 1, ScriptObject::createFrom( false ) ) );
					return result.newRef();
				}
				else
				{
					// sanity check.
					if (skippedFirstPoint)
					{
						PyErr_Format( PyExc_AssertionError,
							"Entity.getStopPoint: "
								"Programmer logic error (2)." );
						return NULL;
					}

					// Continue around loop, but note that we've skipped first
					// point.
					skippedFirstPoint = true;
				}
			}

		}

	}

	if (safetyCount <= 0)
	{
		PyErr_Format( PyExc_ValueError, "Safety count exceeded." );
		return NULL;
	}

	// No doors encountered.
	ScriptTuple result = ScriptTuple::create( 2 );
	MF_VERIFY( result.setItem( 0, ScriptObject::createFrom( destination ) ) );
	MF_VERIFY( result.setItem( 1, ScriptObject::createFrom( true ) ) );
	return result.newRef();
}


/**
 *	
 */
PyObject * NavmeshEntityNavigationSystem::navigateStep(
	NavigateStepController * pNSController,
	const Vector3 & dstPosition, float velocity, float maxMoveDistance,
	float maxSearchDistance, bool faceMovement, float girth, int userArg )
{
	//ToDo: change when navigation system supports non-grid spaces
	if (!entity_.pChunkSpace())
	{
		PyErr_Format( PyExc_ValueError,
				"NavmeshEntityNavigationSystem::navigateStep(): "
				"Physical space of entity does not support navigation queries" );
		return NULL;
	}

	// figure out the source navloc
	NavLoc srcLoc = navigator_.createPathNavLoc(
		entity_.pChunkSpace(), entity_.position(), girth );

	// complain if invalid
	if (!srcLoc.valid())
	{
		PyErr_Format( PyExc_ValueError,
			"Could not resolve source position." );
		return NULL;
	}

	// figure out the dest navloc
	NavLoc dstLoc( entity_.pChunkSpace(), dstPosition, girth );
	// complain if invalid
	if (!dstLoc.valid())
	{
		PyErr_Format( PyExc_ValueError, 
			"Could not resolve destination position." );
		return NULL;
	}

	dstLoc.clip();

	Vector3 nextPosition;
	bool passedActivatedPortal;

	bool foundPath = this->getNavigatePosition( srcLoc, dstLoc,
		maxSearchDistance, nextPosition, passedActivatedPortal, girth );

	if (!foundPath)
	{
		PyErr_SetString( PyExc_ValueError,
				"No path found." );
		return NULL;
	}

	Vector3 disp = nextPosition - entity_.position();

	disp.y = 0.f;
	float wayDistance = disp.length();
	float dstY = nextPosition.y;

	Vector3 wsv( nextPosition );
	srcLoc.makeMaxHeight( wsv );
	float srcY = wsv.y;

	if (wayDistance > maxMoveDistance)
	{
		// shorten it
		nextPosition = entity_.position() +
			(disp * (maxMoveDistance / wayDistance));

		if (srcLoc.pSet()->waypoint( srcLoc.waypoint() ).
				containsProjection( *(srcLoc.pSet()),
					MappedVector3( nextPosition, srcLoc.pSet()->chunk(),
						MappedVector3::WORLD_SPACE ) ))
		{
			dstY = srcY;
		}
	}

	// set our y position now
	Position3D position = entity_.position();
	nextPosition.y = std::max( dstY, srcY );
	position.y = nextPosition.y;

	//since not on a vehicle, so it's ok to pass global position/direction.
	entity_.setAndDropGlobalPositionAndDirection( position,
				entity_.direction() );

	if (!pNSController)
	{
		pNSController = new NavigateStepController(
			nextPosition, velocity, faceMovement );
		entity_.addController( pNSController, userArg );
	}
	else
	{
		pNSController->setDestination( nextPosition );
	}

	return Script::getData( pNSController->id() );
}


/**
 *	
 */
PyObject * NavmeshEntityNavigationSystem::canNavigateTo(
	const Vector3 & dstPosition, float maxSearchDistance, float girth ) const
{
	//ToDo: change when navigation system supports non-grid spaces
	if (!entity_.pChunkSpace())
	{
		PyErr_Format( PyExc_ValueError,
				"NavmeshEntityNavigationSystem::canNavigateTo(): "
				"Physical space of entity does not support navigation queries" );
		return NULL;
	}

	// figure out the source navloc
	NavLoc srcLoc = navigator_.createPathNavLoc(
		entity_.pChunkSpace(), entity_.position(), girth );


	// complain if invalid
	if (!srcLoc.valid())
	{
		/*
		DEBUG_MSG( "Entity.canNavigateTo: "
			"Could not resolve source position (%f,%f,%f)+%f",
			entity_.position().x, entity_.position().y, entity_.position().z, 
			girth );
		*/
		Py_RETURN_NONE;
	}

	// figure out the dest navloc
	NavLoc dstLoc( entity_.pChunkSpace(), dstPosition, girth );

	// complain if invalid
	if (!dstLoc.valid())
	{
		/*
		DEBUG_MSG( "Entity.canNavigateTo: "
			"Could not resolve destination position (%f,%f,%f)+%f\n",
			dstPosition.x, dstPosition.y, dstPosition.z, girth );
		*/
		Py_RETURN_NONE;
	}

	dstLoc.clip();

	// if they are the same waypoint it is easy
	if (srcLoc.waypointsEqual( dstLoc ))
	{
		Vector3 wp = dstLoc.point();
		// set our y position now
		srcLoc.makeMaxHeight( wp );
		return Script::getData( wp );
	}

	// Need to clear navigation cache. It is possible that when a door was
	// open and a search from srcLoc -> dstLoc was stored, which is still
	// there. The door may now be closed.
	//entity_.pReal()->navigator().clearCache();
	NavLoc wayLoc;
	bool passedActivatedPortal;
	bool foundPath;

	{
		AUTO_SCOPED_PROFILE( "findPath" );
		foundPath = navigator_.findPath( srcLoc, dstLoc, 
				maxSearchDistance, /* blockNonPermissive: */ true, 
				wayLoc, passedActivatedPortal,
				Navigator::READ_CACHE );
	}

	if (!foundPath)
	{
		Py_RETURN_NONE;
	}

	Vector3 wp = dstLoc.point();
	// set our y position now
	dstLoc.makeMaxHeight( wp );

	//DEBUG_MSG( "Entity.canNavigateTo: destPosition is: "
	//		"(%6.3f, %6.3f, %6.3f)\n",
	//	wp.x, wp.y, wp.z );
	return Script::getData( wp );
}


/**
 *	This method returns a Python list contains the navigation path points to
 *	the given destination position.
 */
PyObject * NavmeshEntityNavigationSystem::navigatePathPoints(
	const Vector3 & dstPosition, float maxSearchDistance, float girth ) const
{
	NavLoc srcLoc = navigator_.createPathNavLoc(
		entity_.pChunkSpace(), entity_.position(), girth );

	// Don't clip the source point, as then the entity's actual position will
	// be out of sync.
	return Util::navigatePathPoints( navigator_, entity_.pChunkSpace(), srcLoc.point(), 
		dstPosition, maxSearchDistance, girth, false );
}


/**
 *	This method returns the length of the most recently determined navigation
 *	path in chunks.
 */
int NavmeshEntityNavigationSystem::waySetPathLength()
{
	return navigator_.getWaySetPathSize();
}


/**
 *	This method gets the next straight-line step position for navigation
 *	between the given NavLocs. Note: both NavLocs must be valid.
 *
 *	@returns true if a navigation position could be found, false otherwise.
 */
bool NavmeshEntityNavigationSystem::getNavigatePosition( const NavLoc & srcLoc, 
		const NavLoc & dstLoc, float maxSearchDistance, Vector3 & nextPosition, 
		bool & passedActivatedPortal, float girth )
{
	// TODO: Use the girth parameter.
	MF_ASSERT( srcLoc.valid() && dstLoc.valid() );

	// ok, we're ready to go
	/*
	DEBUG_MSG( "Entity.getNavigatePosition: %d "
			"srcLoc is (%f,%f,%f) in %s id %d [%s]\n",
		entity_.id(), srcLoc.point().x, srcLoc.point().y, srcLoc.point().z,
		srcLoc.pSet()->chunk()->identifier().c_str(), srcLoc.waypoint(),
		(srcLoc.isWithinWP() ? "Exact": "Closest"));

	DEBUG_MSG( "Entity.getNavigatePosition: "
			"dstLoc is (%f,%f,%f) in %s id %d [%s]\n",
		dstLoc.point().x, dstLoc.point().y, dstLoc.point().z,
		dstLoc.pSet()->chunk()->identifier().c_str(), dstLoc.waypoint(),
		(dstLoc.isWithinWP() ? "Exact": "Closest") );
	*/
	// if they are the same waypoint it is easy
	if (srcLoc.waypointsEqual( dstLoc ))
	{
		Vector3 wp = dstLoc.point();
		// set our y position now
		srcLoc.makeMaxHeight( wp );
		nextPosition = wp;
		return true;
	}

	NavLoc wayLoc;

	bool foundPath = navigator_.findPath(
		srcLoc, dstLoc, maxSearchDistance,
		/* blockNonPermissive */ true, 
		wayLoc, passedActivatedPortal );

	if (!foundPath)
	{
		/*
		Position3D dstPosition = srcLoc.pSet()->chunk()->transform().applyPoint(
			dstLoc.point() );
		DEBUG_MSG( "Entity.getNavigatePosition: No path "
			"from (%f,%f,%f) in %s "
			"to (%f,%f,%f) in %s with girth %f\n",
			entity_.position().x, entity_.position().y, entity_.position().z,
			srcLoc.pSet()->chunk()->identifier().c_str(),
			dstPosition.x, dstPosition.y, dstPosition.z,
			dstLoc.pSet()->chunk()->identifier().c_str(),
			girth );
		*/
		return false;
	}


	/*
	DEBUG_MSG( "Entity.getNavigatePosition: Path found: "
			"Next waypoint is (%f,%f,%f) in %s id %d [%s]\n",
		wayLoc.point().x, wayLoc.point().y, wayLoc.point().z,
		wayLoc.pSet()->chunk()->identifier().c_str(), wayLoc.waypoint(),
		(wayLoc.isWithinWP() ? "Exact": "Closest") );
	*/

	// calculate the way world position, and ensure it is not too far away
	Vector3 wp = wayLoc.point();

	// set our y position now
	float dstY = wp.y;
	const NavLoc & heightLoc = (wayLoc.waypoint() < 0) ? srcLoc : wayLoc;
	heightLoc.makeMaxHeight( wp );
	wp.y = std::max(wp.y, dstY);

	//DEBUG_MSG( "Entity.getNavigatePosition: "
	//		"nextPosition is: (%6.3f, %6.3f, %6.3f)\n",
	//	wp.x, wp.y, wp.z );

	nextPosition = wp;
	return true;
}

/**
 *	This method will dump the source, destination and the current waypoint cache
 *	for the entity and the target path.
 */
void NavmeshEntityNavigationSystem::navigationDump( const Vector3 & dstPosition,
	float girth )
{
	NavLoc srcLoc = navigator_.createPathNavLoc(
		entity_.pChunkSpace(), entity_.position(), girth );
	DEBUG_MSG("Source loc = %s\n", srcLoc.desc().c_str());

	NavLoc dstLoc( entity_.pChunkSpace(), dstPosition, girth );

	if (dstLoc.valid())
	{
		dstLoc.clip();
	}
	DEBUG_MSG("Dest loc = %s\n", dstLoc.desc().c_str());

	navigator_.dumpCache();
}

BW_END_NAMESPACE

// navmesh_navigation_system.cpp
