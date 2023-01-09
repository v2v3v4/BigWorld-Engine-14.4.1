#include "script/first_include.hpp"

#include "entity_navigate.hpp"

#include "accelerate_along_path_controller.hpp"
#include "accelerate_to_entity_controller.hpp"
#include "accelerate_to_point_controller.hpp"
#include "cell.hpp"
#include "cell_chunk.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"
#include "move_controller.hpp"
#include "navmesh_navigation_system.hpp"
#include "profile.hpp"
#include "real_entity.hpp"
#include "space.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

#include "pyscript/script_math.hpp"

#include "waypoint/chunk_waypoint_set.hpp"
#include "waypoint/navigator.hpp"
#include "waypoint/navigator_cache.hpp"
#include "waypoint/waypoint_neighbour_iterator.hpp"

DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityNavigate
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( EntityNavigate )

PY_BEGIN_METHODS( EntityNavigate )
	PY_METHOD( moveToPoint )
	PY_METHOD( moveToEntity )
	PY_METHOD( accelerateToPoint )
	PY_METHOD( accelerateAlongPath )
	PY_METHOD( accelerateToEntity )
	PY_METHOD( navigate )
	PY_METHOD( navigateStep )
	PY_METHOD( canNavigateTo )
	PY_METHOD( navigateFollow )
	PY_METHOD( waySetPathLength )
	PY_METHOD( getStopPoint )
	PY_METHOD( navigatePathPoints )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( EntityNavigate )
PY_END_ATTRIBUTES()


/// static initialiser
const EntityNavigate::Instance<EntityNavigate>
	EntityNavigate::instance( EntityNavigate::s_getMethodDefs(),
			EntityNavigate::s_getAttributeDefs() );



/**
 *	Constructor
 */
EntityNavigate::EntityNavigate( Entity & e ) :
	EntityExtra( e ),
	pNavigationSystem_( NULL ),
	pNavigateStepController_( NULL )
{
}


/**
 *	Destructor
 */
EntityNavigate::~EntityNavigate()
{
	bw_safe_delete( pNavigationSystem_ );
}


void EntityNavigate::registerNavigateStepController(
	NavigateStepController* pController )
{
	pNavigateStepController_ = pController;
}


void EntityNavigate::deregisterNavigateStepController(
	NavigateStepController* pController )
{
	if (pNavigateStepController_ == pController)
	{
		pNavigateStepController_ = NULL;
	}
}


/*~ function Entity.moveToPoint
 *  @components{ cell }
 *
 *	This function moves the Entity in a straight line towards a given point,
 *	invoking a notification method on success or failure.  For a given Entity,
 *	only one navigation/movement controller is active at any one time.
 *	Returns a ControllerID that can be used to cancel the movement. For example,
 *	Entity.cancel( movementID ).  Movement can also be cancelled with
 *	Entity.cancel( "Movement" ). The notification methods are not called when
 *	movement is cancelled.
 *
 *	The notification methods are defined as follows:
 *	@{
 *		def onMove( self, controllerId, userData ):
 *	@}
 *
 * 	Note that the destination point is specified relative to the entity's local
 * 	space, which may differ from global space if the entity is currently
 * 	aboard a vehicle. If the entity is aboard a vehicle, and a destination in
 * 	global coordinates is desired, the entity should alight from the vehicle
 * 	first before calling moveToPoint().
 *
 *	@see cancel
 *
 *	@param	destination		(Vector3) The destination point for the Entity, in
 *							local space.
 *	@param	velocity		(float)	The speed to move the Entity in	
 *							metres / second.
 *	@param	userData		(optional int) Data that can be passed to
 *							notification method.
 *	@param	faceMovement	(optional bool) True if entity is to face in
 *							direction of movement (default).
 *							Should be False if another mechanism is to provide
 *							direction.
 *	@param	moveVertically	(optional bool) True if Entity is allowed to move
 *							vertically, that is, to fly. Set to false if Entity
 *							is to remain on ground (default).
 *
 *	@return					(int) The ID of the newly created controller.
 */
/**
 *	This method is exposed to scripting. It creates a controller that
 *	will move an entity to the given point. The arguments below are passed
 *	via a python tuple.
 *
 *	@param destination		3D destination vector in local space.
 *	@param velocity			Movement velocity in metres / second.
 *	@param userArg			User data to be passed to the callback (optional).
 *	@param faceMovement		Should turn to face movement direction (optional).
 *	@param moveVertically	Should allow vertical movement (optional).
 *
 *	@return						The integer ID of the newly created controller.
 */
PyObject * EntityNavigate::moveToPoint( Vector3 destination,
	float velocity, int userArg, bool faceMovement, bool moveVertically )
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	ControllerPtr pController = new MoveToPointController(
			destination, "", 0, velocity, faceMovement, moveVertically );

	ControllerID controllerID;
	controllerID = entity_.addController( pController, userArg );
	return Script::getData( controllerID );
}


/*~ function Entity.moveToEntity
 *  @components{ cell }
 *
 *	This function moves the Entity in a straight line towards another Entity,
 *	invoking a notification method on success or failure.  For a given Entity,
 *	only one navigation/movement controller is active at any one time.
 *	Returns a ControllerID that can be used to cancel the movement. For example,
 *	Entity.cancel( movementID ).  Movement can also be cancelled with
 *	Entity.cancel( "Movement" ). The notification methods are not called when
 *	movement is cancelled.
 *
 * 	@{
 *		def onMove( self, controllerId, userData ):
 *		def onMoveFailure( self, controllerId, userData ):
 *	@}
 *
 *	@see cancel
 *
 *	@param	destEntityID	(int) The ID of the target Entity.
 *	@param	velocity		(float)	The speed to move the Entity in metres /
 *							second.
 *	@param	range			(float)	Maxmimum range from destination to stop.
 *	@param	userData		(optional int) Data that can be passed to
 *							the notification method.
 *	@param	faceMovement	(optional bool) True if entity is to face in
 *							direction of movement (default).
 *							Should be False if another mechanism is to provide
 *							direction.
 *	@param	moveVertically	(optional bool) True if Entity is allowed to move
 *							vertically, that is, to fly.
 *							Set to false if Entity is to remain on ground
 *							(default).
 *
 *	@return					(int) The ID of the newly created controller.
 */
/**
 *	This method is exposed to scripting. It creates a controller that
 *	will move an entity towards another entity. The arguments below are passed
 *	via a python tuple.
 *
 *	@param destEntityID		Entity ID of destination
 *	@param velocity			Movement velocity in metres / second.
 *	@param range			Range within destination to stop
 *	@param userArg			User data to be passed to the callback
 *	@param faceMovement		True if entity is to face in direction of movement
 *							(default). Should be False if another mechanism is
 *							to provide direction.
 *	@param moveVertically	True if Entity is allowed to move vertically, that
 *							is, to fly. Set to false if Entity is to remain on
 *							ground (default).
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject * EntityNavigate::moveToEntity( int destEntityID,
										float velocity,
										float range,
										int userArg,
										bool faceMovement,
										bool moveVertically )
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	ControllerPtr pController = new MoveToEntityController( destEntityID,
			velocity, range, faceMovement, moveVertically );

	ControllerID controllerID;
	controllerID = entity_.addController(pController, userArg);
	return Script::getData( controllerID );
}


/*~ function Entity.accelerateToPoint
 *  @components{ cell }
 *
 *	This function is equivalent to calling:
 *	@{
 *		entity.accelerateAlongPath( [destination], ... )
 *	@}
 *
 * 	Similar to how accelerateAlongPath() treats the points inside the
 * 	waypoints parameter, the destination parameter for accelerateToPoint() is
 * 	specified relative to the entity's local space, which may differ from
 * 	global space if the entity is currently aboard a vehicle. If the entity is
 * 	aboard a vehicle, and a destination in global coordinates is desired, the
 * 	entity should alight from the vehicle first before calling
 * 	accelerateToPoint().
 *
 *	@see accelerateAlongPath
 *	@see cancel
 *
 *	@param	destination		(Vector3) The destination to move to in local
 *							space. 
 *	@param	acceleration	(float)	The rate at which the entity will
 *							accelerate, in metres / second / second.
 *	@param	maxSpeed		(float) The maximum speed the entity will
 *	accelerate to.
 *	@param	facing			(optional int) Defines direction the entity should
 *							face while it moves. Valid values are:
 *								0	Disabled
 *								1	Face velocity (default)
 *								2	Face acceleration
 *	@param	userData		(optional int) Data that will be passed to
 *							notification method.
 *
 *	@return					(int) The ID of the newly created controller.
 */
/**
 *	This function is equivalent to calling
 *	accelerateAlongPath( [destination], ... )
 *
 *	@see EntityNavigate::accelerateAlongPath()
 *
 *	@param	destination			The destination to move to in local space.
 *	@param	acceleration		The rate at which the entity will accelerate,
 *								in metres / second / second.
 *	@param	maxSpeed			The maximum speed the entity will maintain.
 *	@param	intFacing			Defines direction the entity should face
 *								while it moves.
 *									0	Disabled
 *									1	Face velocity (default)
 *									2	Face acceleration
 *	@param	stopAtDestination	Describes whether the speed of the entity
 *								should be zero upon reaching its destination.
 *	@param	userArg				Data that will be passed to notification
 *								methods
 *
 *	@return					The ID of the newly created controller
 */
PyObject * EntityNavigate::accelerateToPoint(	Position3D destination,
												float acceleration,
												float maxSpeed,
												int intFacing,
												bool stopAtDestination,
												int userArg)
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	BaseAccelerationController::Facing facing;
	if (intFacing >= 0 && intFacing < BaseAccelerationController::FACING_MAX)
	{
		facing = BaseAccelerationController::Facing( intFacing );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, 
			"Facing must one of the value 0, 1 or 2" );
		return NULL;
	}

	ControllerPtr pController = new AccelerateToPointController(
															destination,
															acceleration,
															maxSpeed,
															facing,
															stopAtDestination);

	ControllerID controllerID;
	controllerID = entity_.addController( pController, userArg );
	return Script::getData( controllerID );
}


/*~ function Entity.accelerateAlongPath
 *  @components{ cell }
 *
 *	This function moves the Entity through a series of given waypoints by
 *	applying an acceleration. For a given Entity only one movement controller
 *	can be active and so this function will replace any existing controller.
 *
 *	The notification methods are defined as follows:
 * 	@{
 *		def onMove( self, controllerId, userData ):
 *	@}
 *
 * 	The points inside the waypoints argument for accelerateAlongPath() are
 * 	specified relative to the entity's local space, which may differ from
 * 	global space if the entity is currently aboard a vehicle. If the entity is
 * 	aboard a vehicle, and waypoints in global coordinates are desired, the
 * 	entity should alight from the vehicle first before calling
 * 	accelerateAlongPath().
 *	@see cancel
 *
 *	@param	waypoints		([Vector3])	 The waypoints, in local space, to pass
 *							through.
 *	@param	acceleration	(float) The rate at which the entity will
 *							accelerate, in metres / second / second.
 *	@param	maxSpeed		(float)	The maximum speed the entity will
 *							accelerate to.
 *	@param	facing			(optional int) Defines direction the entity should
 *							face while it moves. Should be one of:
 *								0	Disabled
 *								1	Face velocity (default)
 *								2	Face acceleration
 *	@param	userData		(optional int) Data that will be passed to
 *							notification method.
 *
 *	@return					(int)	The ID of the newly created	controller.
 */
/**
 *	This is a Python exposed function that creates a controller to moves the
 *	Entity through a series of waypoints by applying acceleration.
 *
 *	@param	waypoints		The waypoints to pass through, in local space.
 *	@param	acceleration	The rate at which the entity will accelerate, in 
 *							metres / second / second.
 *	@param	maxSpeed		The maximum speed the entity will maintain.
 *	@param	intFacing		Defines direction the entity should face while it
 *							moves. Should be one of:
 *								0	Disabled
 *								1	Face velocity (default)
 *								2	Face acceleration
 *	@param	userArg			Data that will be passed to notification methods.
 *
 *	@return					The ID of the newly created controller
 */
PyObject * EntityNavigate::accelerateAlongPath(	
											BW::vector<Position3D> waypoints,
											float acceleration,
											float maxSpeed,
											int intFacing,
											int userArg)
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	if (waypoints.empty())
	{
		PyErr_SetString( PyExc_TypeError,
			"Waypoints must not be empty." );
		return NULL;
	}

	BaseAccelerationController::Facing facing;
	if (intFacing >= 0 && intFacing < BaseAccelerationController::FACING_MAX)
	{
		facing = BaseAccelerationController::Facing( intFacing );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, 
			"Facing must one of the value 0, 1 or 2" );
		return NULL;
	}

	ControllerPtr pController = new AccelerateAlongPathController(	
																waypoints,
																acceleration,
																maxSpeed,
																facing );

	ControllerID controllerID;
	controllerID = entity_.addController( pController, userArg );
	return Script::getData( controllerID );
}


/*~ function Entity.accelerateToEntity
 *  @components{ cell }
 *
 *	This function moves the entity towards the given target entity by applying
 *	an acceleration. For a given Entity only one movement controller can be
 *	active and so this function will replace any existing controller.
 *
 *	The notification methods are defined as follows;
 * 	@{
 *		def onMove( self, controllerId, userData ):
 *		def onMoveFailure( self, controllerId, userData ):
 *	@}
 *
 *	@see cancel
 *
 *	@param	destinationEntity 	(int) The ID of the entity to accelerate
 *								towards.
 *	@param	acceleration		(float)	The rate at which the entity will
 *								accelerate, in metres / second / second.
 *	@param	maxSpeed			(float) The maximum speed the entity will
 *								accelerate to.
 *	@param	range				(float)	Range within destination to stop.
 *	@param	facing				(optional int) Defines direction the entity
 *								should face while it moves. Should be one of:
 *									0	Disabled
 *									1	Face velocity (default)
 *									2	Face acceleration
 *	@param	userData			(optional int) Data that will be passed to
 *								notification method.
 *
 *	@return						(int)	The ID of the newly created	controller.
 */
/**
 *	This is a python exposed function that creates a controller to moves the
 *	Entity through a series of waypoints by applying acceleration.

 *	@param	destinationEntity	The ID of the entity to accelerate towards.
 *	@param	acceleration		The rate at which the entity will accelerate,
 *								in metres / second / second.
 *	@param	maxSpeed			The maximum speed the entity will accelerate
 *								to.
 *	@param	range				Range within destination to stop.
 *	@param	intFacing			Defines direction the entity should face while
 *								it moves. Should be one of:
 *									0	Disabled
 *									1	Face velocity (default)
 *									2	Face acceleration
 *	@param	userArg				Data that will be passed to notification
 *								method.
 *
 *	@return	The ID of the newly created controller
 */
PyObject * EntityNavigate::accelerateToEntity(	EntityID destinationEntity,
												float acceleration,
												float maxSpeed,
												float range,
												int intFacing,
												int userArg)
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	BaseAccelerationController::Facing facing;
	if (intFacing >= 0 && intFacing < BaseAccelerationController::FACING_MAX)
	{
		facing = BaseAccelerationController::Facing( intFacing );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError,
			"Facing must one of the value 0, 1 or 2" );
		return NULL;
	}

	ControllerPtr pController = new AccelerateToEntityController(	
															destinationEntity,
															acceleration,
															maxSpeed,
															range,
															facing );

	ControllerID controllerID;
	controllerID = entity_.addController( pController, userArg );
	return Script::getData( controllerID );
}


/*~ function Entity.navigate
 *  @components{ cell }
 *	This function has now been deprecated. Use Entity.navigateStep instead.
 *
 *	This function uses the BigWorld navigation system to move the Entity to a
 *	destination point, invoking a notification method on success or failure.
 *	BigWorld can have several pre-generated navigation systems, each with a
 *	different girth (resulting in different navigation paths). 
 *	Currently we don't support an entity which is on a vehicle to navigate,
 *	if you want to do so, you have to alight the vehicle first,
 *	or do navigate on the vehicle entity. For a given
 *	Entity, only one navigation/movement controller can be active at any one
 *	time.
 *	Returns a ControllerID that can be used to cancel the movement. For example,
 *	Entity.cancel( movementID ).  Movement can also be cancelled with
 *	Entity.cancel( "Movement" ). The notification methods are not called when
 *	movement is cancelled.
 *
 *	The notification methods are defined as follows;
 *	@{
 *		def onNavigate( self, controllerId, userData ):
 *		def onNavigateFailed( self, controllerId, userData ):
 *	@}
 *
 *	@see cancel
 *
 *	@param	destination			(Vector3) The destination point for the entity.
 *	@param	velocity			(float) The speed to move the Entity in 
 *								metres / second.
 *	@param	faceMovement		(optional bool) True if entity is to face in
 *								direction of movement (default).
 *								Should be False if another mechanism is to
 *								provide direction.
 *	@param	maxSearchDistance 	(float) Maximum distance to search.
 *	@param	girth				(optional float) Which navigation girth to use
 *								(defaults to 0.5).
 *	@param	closeEnough			(optional float) Maximum distance from
 *								destination to stop.
 *								Note:  A value of 0 should never be used.
 *	@param	userData			(optional int) Data that is passed to
 *								notification method.
 *
 *	@return						(int) The ID of the newly created controller
*/
/**
 *	This method is exposed to scripting. It creates a controller that
 *	will move an entity towards a destination point, following waypoints.
 *	The entity will continue moving until it reaches its destination,
 *	or an error occurs (or the controller is cancelled).
 *
 *	@param dstPosition			Destination point
 *	@param velocity				Movement velocity in metres / second.
 *	@param faceMovement         Whether to face the direction of movement or
 *								not ie strafe.
 *	@param maxSearchDistance	Max distance to search
 *	@param girth				The girth of this entity
 *  @param closeEnough			How close to end point we need to get.
 *	@param userArg				User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject * EntityNavigate::navigate( const Vector3 & dstPosition,
		float velocity, bool faceMovement, float maxSearchDistance,
		float girth, float closeEnough, int userArg )
{
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	if (entity_.pVehicle())
	{
		PyErr_SetString( PyExc_ValueError,
			"Cannot call Entity.navigate() for an entity on a vehicle, either "
			"alight from the vehicle first, or call Entity.navigate() for the "
			"vehicle." );
		return NULL;
	}

	ControllerPtr pController = navigationSystem().createController(
		dstPosition, velocity, faceMovement, maxSearchDistance, girth,
		closeEnough );

	if (pController)
	{
		return Script::getData( entity_.addController( pController, userArg ) );
	}

	PyErr_SetString( PyExc_EnvironmentError,
		"Entity.navigate(): Failed to create a controller" );

	return NULL;
}


/*~ function Entity.getStopPoint
 *  @components{ cell }
 *	This function should be called if we want to navigate an entity from its
 *	current position to destination. It is intended to be used as a replacement
 *	to canNavigateTo() that knows about activated portals (see
 *	ChunkPortal.activate()).
 *
 *	@param	destination				(Vector3) The final destination point for
 *									the Entity.
 *	@param	ignoreFirstStopPoint 	(bool) Set to True to determine next
 *									stopping point past current stopping point,
 *									otherwise the current stopping point would
 *									get triggered again.
 *	@param	maxSearchDistance 		(optional float) Set the max distance of
 *									search (default to maxAoIRadius metres).
 *	@param	girth 					(optional float) Which navigation girth to
 *									use (defaults to 0.5).
 *	@param	stopDist 				(optional float) Distance before portal to
 *									stop (default 1.5 metres).
 *	@param	nearPortalDist 			(optional float) Distance back from portal
 *									that the entity should be to be considered
 *									next to model blocking portal (eg, door)
 *									(default 1.8 metres).
 *	@return 						(Vector3, bool) Returns None if the entity
 *									can't navigate to destination. 
 *									Otherwise, returns the point stopDist back
 *									along the navigation path from the
 *									activated portal and True if it reached the
 *									destination, False otherwise.
 */
/*
 * This function should be called if we want to navigate an entity from its
 * current position to destination. It is intended to be used as a replacement
 * to canNavigateTo that knows about doors.
 *
 * returns:
 *		None - can't navigate to destination.
 *		else: ( Vector3 stopPoint, bool reachedDestination )
 *
 *  where stopPoint is a point stopDist back along the navigation path from the
 *  activated portal (portal with a door). nearPortalDist is the distance along
 *  path back from activated portal to consider that entity is next to door.
 */
PyObject * EntityNavigate::getStopPoint( const Vector3 & destination,
		bool ignoreFirstStopPoint, float maxSearchDistance,
		float girth, float stopDist, float nearPortalDist )
{
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	return navigationSystem().getStopPoint( destination, ignoreFirstStopPoint,
		maxSearchDistance, girth, stopDist, nearPortalDist );
}


/*~ function Entity.navigateStep
 *  @components{ cell }
 *
 *	This function uses the BigWorld navigation system to move the Entity
 *	towards a destination point, stopping at the next waypoint or given
 *	distance, invoking the onMove notification method on success.
 *
 *	When the Entity.onMove notification method is called, have it call 
 *	Entity.navigateStep again to allow the entity to continue to move toward the
 *	target position.
 *
 *	If the Entity.onMove notification method calls Entity.navigateStep more than
 *	100 times without moving the distance required for a single tick at the
 *	velocity specified, then the onMoveFailure notification method is invoked.
 *
 *	Currently BigWorld does not support using navigate methods on an entity
 *	which is on a vehicle. If you want to do so, you have to alight the vehicle
 *	first, or use the navigate methods on the vehicle entity.
 *
 *	For a given Entity, only one navigation/movement controller is active at
 *	any one time.
 *
 *	Returns a Controller ID that can be used to cancel the movement. For
 *	example:
 *	@{ 
 *		self.cancel( movementID )
 *	@}
 *	
 *	Movement can also be cancelled with:
 *	@{
 *		self.cancel( "Movement" )
 *	@}
 *	
 *	The notification method is not called when movement is cancelled.
 *
 *	The notification methods are defined as follows;
 *	@{
 *		def onMove( self, controllerId, userData ):
 *		def onMoveFailure( self, controllerId, userData ):
 *	@}
 *
 *	@see cancel
 *	@see onMove
 *	@see onMoveFailure
 *
 *	@param	destination			(Vector3) The destination point for the Entity
 *								to move towards.
 *	@param	velocity			(float) The speed to move the Entity in 
 *								metres / second.
 *	@param	maxMoveDistance		(float) Maximum distance to move, in metres.
 *	@param	maxSearchDistance 	(optional float) The maximum search distance
 *								(default maxAoIRadius metres)
 *	@param	faceMovement		(optional bool)	True if entity is to face in
 *								direction of movement (default).
 *								Should be False if another mechanism is to
 *								provide direction.
 *	@param	girth				(optional float) Which navigation girth to
 *								use (defaults to 0.5).
 *	@param	userData			(optional int) Data that can be passed to
 *								notification method.
 *
 *	@return						(int) The ID of the newly created controller.
 */
/**
 *	This method is exposed to scripting. It creates a controller that
 *	will move an entity towards a destination point, following waypoints.
 *	The entity will stop moving when it reaches a new waypoint, or when
 *	it exceeds a maximum distance.
 *
 *	@param dstPosition			Destination point
 *	@param velocity				Movement velocity in metres / second.
 *	@param maxMoveDistance		Maximum distance to move
 *	@param maxSearchDistance	Maximum distance to search
 *	@param faceMovement			Whether or not the entity should face the
 *								direction of movement.
 *	@param girth				The girth of this entity
 *	@param userArg				User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject * EntityNavigate::navigateStep( const Vector3 & dstPosition,
		float velocity, float maxMoveDistance, float maxSearchDistance, 
		bool faceMovement, float girth, int userArg )
{
	PROFILER_SCOPED( EntityNavigate_navigateStep );
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	if (entity_.pVehicle())
	{
		PyErr_SetString( PyExc_ValueError,
			"Cannot call Entity.navigateStep() for an entity on a vehicle, "
			"either alight from the vehicle first, or call "
			"Entity.navigateStep() for the vehicle." );
		return NULL;
	}

	return this->navigationSystem().navigateStep( pNavigateStepController_,
		dstPosition, velocity, maxMoveDistance, maxSearchDistance,
		faceMovement, girth, userArg );
}


/*~ function Entity.navigateFollow
 *  @components{ cell }
 *	This function has now been deprecated. Use Entity.navigateStep instead.
 *
 *	This function uses the BigWorld navigation system to move the Entity
 *	towards another Entity,	stopping at the next waypoint or given distance,
 *	invoking a notification method on success or failure.
 *
 *	BigWorld can have several pre-generated navigation systems, each with a
 *	different girth (resulting in different navigation paths).  For a given
 *	Entity, only one navigation / movement controller is active at any one
 *	time.
 *
 *	Returns a ControllerID that can be used to cancel the movement. For
 *	example; Entity.cancel( movementID ).  Movement can also be cancelled with
 *	Entity.cancel( "Movement" ).
 *
 *	The notification method is not called when movement is cancelled.
 *
 *	The notification method is defined as follows;
 * 	@{
 *		def onMove( self, controllerId, userData ):
 *	@}
 *
 *	@see cancel
 *
 *	@param	destEntity			(Entity) The target Entity
 *	@param	angle				(float)	The angle at which the Entity should
 *								attempt to follow (in radians)
 *	@param	distance			(float) The distance at which the Entity should
 *								attempt to follow
 *	@param	velocity			(float)	The speed to move the Entity in
 *								metres / second.
 *	@param	maxMoveDistance		(float)	Maximum distance to move
 *	@param	maxSearchDistance	(optional float) The maximum search distance
 *								(default maxAoIRadius metres)
 *	@param	faceMovement		(optional bool) True if entity is to face in
 *								direction of movement (default).
 *								Should be False if another mechanism is to
 *								provide direction.
 *	@param	girth				(optional float) Which navigation girth to use
 *								(defaults to 0.5)
 *	@param	userData			(optional int) Data that can be passed to
 *								notification method
 *
 *	@return (int) 				The ID of the newly created controller.
 */
/**
 *	This method is exposed to scripting. It moves the entity to a position that
 *	is describe as an offset from another entity.
 *
 *	@param pEntity				The entity that the position is relative to.
 *	@param angle				The offset angle from the destination entity.
 *	@param distance				The offset distance (in metres) form the
 *								destination entity.
 *	@param velocity				Movement velocity in metres / second.
 *	@param maxMoveDistance		Maximum distance to move
 *	@param maxSearchDistance	The maximum search distance
 *	@param faceMovement			Whether or not the entity should face the
 *								direction of movement.
 *	@param girth				The girth of this entity
 *	@param userArg				User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject * EntityNavigate::navigateFollow( PyObjectPtr pEntityObj, float angle,
		float distance, float velocity, float maxMoveDistance, 
		float maxSearchDistance, bool faceMovement, float girth, int userArg )
{
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	// Check that the entity parameter actually has an entity
	if (!PyObject_IsInstance( pEntityObj.get(), (PyObject*)(&Entity::s_type_) ))
	{
		PyErr_SetString( PyExc_TypeError, 
			"parameter 1 must be an Entity instance" );
		return NULL;
	}

	Entity * pOtherEntity = static_cast< Entity * >( pEntityObj.get() );

	// A destroyed entity has no real location
	if (pOtherEntity->isDestroyed())
	{
		PyErr_SetString( PyExc_ValueError,
			"cannot follow a destroyed entity" );
		return NULL;
	}

	// Check that the entity is in the same space
	if (this->entity().space().id() != pOtherEntity->space().id())
	{
		PyErr_SetString( PyExc_ValueError,
			"followed entity is not in the same space" );
		return NULL;
	}

	if (almostZero( maxSearchDistance ))
	{
		PyErr_SetString( PyExc_ValueError,
			"maxSearchDistance cannot be 0.0" );
		return NULL;
	}

	float yaw = pOtherEntity->direction().yaw;
	float totalYaw = yaw + angle;
	Vector3 offset = Vector3( distance * sinf( totalYaw ),
			0, distance * cosf( totalYaw ) );
	Vector3 position = pOtherEntity->position() + offset;

	return this->navigateStep( position, velocity, maxMoveDistance, 
		maxSearchDistance, faceMovement, girth, userArg );
}


/*~ function Entity.canNavigateTo
 *  @components{ cell }
 *
 *	This function uses the BigWorld navigation system to determine whether the
 *	Entity can move to the given destination.  BigWorld can have several
 *	pre-generated navigation systems, each with a different girth (resulting in
 *	different navigation paths).
 *
 *	@see cancel
 *
 *	@param	destination 		(Vector3) The destination point for the
 *								navigation test.
 *	@param	maxSearchDistance	(optional float) Maximum distance to search
 *								(default maxAoIRadius).
 *	@param	girth				(optional float) Which navigation girth to use
 *								for the test (defaults to 0.5).
 *
 *	@return					(Vector3) The closest position that the entity can
 *							navigate to, or None if no path is found.
 */
/**
 *	This method is exposed to scripting. It checks to see if this entity
 *	can move towards a destination point, following waypoints.
 *
 *	@param dstPosition			The destination point.
 *	@param maxSearchDistance	Max distance to search (default maxAoIRadius
 *								metres.
 *	@param girth				The girth of this entity for the test.
 *
 *	@return		The closest position that the entity can navigate to, or None
 *				if no such path exists.
 */
PyObject * EntityNavigate::canNavigateTo( const Vector3 & dstPosition,
		float maxSearchDistance, float girth )
{
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	AUTO_SCOPED_PROFILE( "canNavigateTo" );

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	return this->navigationSystem().canNavigateTo( dstPosition,
					maxSearchDistance, girth );
}

/*~ function Entity.waySetPathLength
 *  @components{ cell }
 *
 *	Returns the length of the most recently determined navigation path, in
 *	chunks.
 *
 *	@return					(float)	The length of the most recently determined
 *							navigation path.
 */
/**
 *	This method is exposed to scripting. It returns the waypoint set path
 *	length of the last cached path.
 *
 *	@return		The length of last waypoint set path.
 */
int EntityNavigate::waySetPathLength()
{
	// TODO: Move this to RealEntity if it ever supports script extensions.
	if (!entity_.isRealToScript())
	{

		ERROR_MSG( "EntityNavigate::waySetPathLength: "
			"This method can only be called on a real entity." );
		return 0;
	}

	return this->navigationSystem().waySetPathLength();
}



/*~	function Entity.navigatePathPoints
 *	@components{ cell }
 *
 *	This function returns the points along a path from this entity position 
 *	to the given destination position.
 *
 *	@param 	destination 		(Vector3) 	The destination point.
 *	@param 	maxSearchDistance 	(float)		The maximum distance to search, in
 *											metres.
 *	@param 	girth				(float)		The navigation girth (defaults to 
											0.5).
 */
/**
 *	This method returns the points along a path from this entity position to
 *	the destination position.
 */
PyObject * EntityNavigate::navigatePathPoints( const Vector3 & dstPosition,
		float maxSearchDistance, float girth )
{
	if (!entity_.isRealToScript())
	{
		WARNING_MSG( "EntityNavigate::navigatePathPoints: "
				"This method was called on a ghost.\n" );
	}

	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	return this->navigationSystem().navigatePathPoints( dstPosition,
			maxSearchDistance, girth );
}


/**
 *	This method returns the navigation system to be used for the current
 *	space.
 *
 *	@returns A reference to the navigation system for the calling Entity.
 */
NavmeshEntityNavigationSystem & EntityNavigate::navigationSystem()
{
	if (!pNavigationSystem_)
	{
		// Note: Currently only supports NavmeshEntityNavigationSystem. In
		// theory, possible to support other systems.
		pNavigationSystem_ = new NavmeshEntityNavigationSystem( entity_ );
	}

	return *pNavigationSystem_;
}


// -----------------------------------------------------------------------------
// Section: Navigation script methods in the BigWorld module
// -----------------------------------------------------------------------------

namespace // (anonymous)
{


/**
 *	Find a point nearby random point in a connected navmesh
 *
 *	@param funcName 	The name of python function for error reporting.
 *	@param spaceID 		The id of the space in which to operate.
 *	@param position		The position of the point.
 *	@param minRadius	The minimum radius to search, in metres.
 *	@param maxRadius	The maximum radius to search.
 *	@param girth		Which navigation girth to use (optional and default to
 *						0.5).
 *	@return				The random point found, as a Vector3.
 */
PyObject * findRandomNeighbourPointWithRange( const char* funcName, 
		SpaceID spaceID, Vector3 position, 
		float minRadius, float maxRadius, float girth )
{
	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "%s: Invalid space ID %d",
			funcName, int(spaceID) );
		return NULL;
	}

	// if there is no underlying ChunkSpace, for now simply issue an error.
	// ToDo: implement it in other physical spaces?
	ChunkSpacePtr pChunkSpace = pSpace->pChunkSpace();
	if (!pChunkSpace)
	{
		PyErr_Format( PyExc_ValueError,
				"%s: Space ID %d does not support navigation-related queries",
				funcName, int(spaceID) );
		return NULL;
	}

	Vector3 result;

	if (NavmeshNavigationSystem::findRandomNeighbourPointWithRange(
			pChunkSpace.get(), position, minRadius, maxRadius, girth, result ))
	{
		return Script::getData( result );
	}

	PyErr_Format( PyExc_ValueError,
		"%s: Failed to find neighbour point", funcName );

	return NULL;
}


/*~ function BigWorld findRandomNeighbourPointWithRange
 *  @components{ cell }
 *	This function can be used to find a random point in a connected navmesh.
 *	The result point is guaranteed to be connectable to the point.
 *	Note that in some conditions the result point might be closer than
 *	minRadius.
 *
 *	@param spaceID 		(int) The ID of the space in which to operate.
 *	@param position		(Vector3) The position of the point to find a random
 *						point around.
 *	@param minRadius	(float) The minimum radius to search.
 *	@param maxRadius	(float) The maximum radius to search.
 *	@param girth		(optional float) Which navigation girth to use
 *						(defaults to 0.5).
 *	@return				(Vector3) The random point found.
 */
/**
 *	Find a point nearby random point in a connected navmesh.
 *
 *	@param spaceID 		The ID of the space in which to operate.
 *	@param position		The position of the point.
 *	@param minRadius	The minimum radius to search.
 *	@param maxRadius	The maximum radius to search.
 *	@param girth		Which navigation girth to use (optional and defaults to
 *						0.5).
 *	@return				The random point found, as a Vector3.
 */
PyObject * findRandomNeighbourPointWithRange( SpaceID spaceID,
		const Vector3 & position, float minRadius, float maxRadius,
		float girth = 0.5 )
{
	return findRandomNeighbourPointWithRange( 
		"BigWorld.findRandomNeighbourPointWithRange",
		spaceID, position, minRadius, maxRadius, girth );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findRandomNeighbourPointWithRange,
	ARG( SpaceID, ARG( Vector3, ARG( float, ARG( float, 
		OPTARG( float, 0.5f, END ) ) ) ) ), BigWorld )


/*~ function BigWorld findRandomNeighbourPoint
 *  @components{ cell }
 *	This function can be used to find a random point in a connected navmesh.
 *	The result point is guaranteed to be connectable to the point.
 *
 *	@param spaceID 	(int) The ID of the space in which to operate.
 *	@param position	(Vector3) The position of the point
 *	@param radius	(float) The radius to search, in metres.
 *	@param girth	(optional float)  Which navigation girth to use (optional
 *					and defaults to 0.5).
 *	@return			(Vector3) The random point found.
 */
/**
 *	Find a point nearby random point in a connected navmesh.
 *
 *	@param spaceID 	The id of the space in which to operate.
 *	@param position	The position of the point
 *	@param radius	The radius to search
 *	@param girth	Which navigation girth to use (optional and defaults to
 *					0.5).
 *	@return			The random point found, as a Vector3.
 */
PyObject * findRandomNeighbourPoint( SpaceID spaceID,
	Vector3 position, float radius, float girth = 0.5 )
{
	return findRandomNeighbourPointWithRange( 
		"BigWorld.findRandomNeighbourPoint",
		spaceID, position, 0.f, radius, girth );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, findRandomNeighbourPoint,
	ARG( SpaceID, ARG( Vector3, ARG( float, OPTARG( float, 0.5f, END ) ) ) ), 
	BigWorld )


/**
 *	NavInfo flags
 *	TODO: Rename this to something more appropriate
 */
enum ResolvePointFlags
{
	// NAVINFO_UNKNOWN	= 0x00,	// chunkID and waypointID are invalid
	NAVINFO_VALID	= 0x01, // chunkID and waypointID are valid
	// NAVINFO_GUESS	= 0x02,	// chunkID and waypointID are out of date
	NAVINFO_CLOSEST	= 0x04,	// no match. using closest chunk/waypoint.
};


/*~ function BigWorld configureConnection
 *  @components{ cell }
 *	This function has now been deprecated. Use Entity.addPortalConfig instead.
 *
 *	This function configures the connection between two demonstrative points
 *	that straddle a portal. The connection may be either open or closed.
 *
 *	@param spaceID 	The id of the space in which to operate.
 *	@param point1	The first demonstrative point.
 *	@param point2	The second demonstrative point.
 *	@param isOpen	A boolean value indicating whether or not the portal is
 *					open.
 */
/**
 *	Configure the connection between the demonstrative points pta and ptb
 *	in spaceID. The connection may be either open or closed.
 *	The demonstrative points must straddle a portal.
 */
PyObject * configureConnection( SpaceID spaceID,
		const Vector3 & pta, const Vector3 & ptb, bool connect )
{
	ERROR_MSG( "BigWorld.configureConnection: "
			"This function has been deprecated."
			"Use BigWorld.addPortalConfig instead\n" );
	// TODO: configureConnection should be implemented with a ghost controller
	// on the entity that is near the connection.

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.configureConnection(): "
				"No space ID %d", int(spaceID) );
		return NULL;
	}
	ChunkSpacePtr pChunkSpace = pSpace->pChunkSpace();
	if (!pChunkSpace)
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.configureConnection(): "
				"Space ID %d does not support navigation-related queries",
				int(spaceID) );
		return NULL;
	}

	NavLoc na( pChunkSpace.get(), pta, 0.5f );
	NavLoc nb( pChunkSpace.get(), ptb, 0.5f );
	// the same portals are used for all girths, so we don't need to
	// worry about what we pass here (as long as it is small enough)

	if (!na.valid() || !nb.valid())
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.configureConnection: "
			"Could not resolve demonstrative points" );
		return NULL;
	}

	if (na.pSet() == nb.pSet())
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.configureConnection: "
			"Both demonstrative points are in the same waypoint set" );
		return NULL;
	}

	// ok, find the connection. since nothing is loaded dynamically,
	// we do not need to worry about what happens when it goes away.
	// and this is still broken for multiple cells.
	ChunkWaypointConns::iterator iterA = na.pSet()->connectionsBegin();
	for (;iterA != na.pSet()->connectionsEnd(); ++iterA)
	{
		if (iterA->first == nb.pSet())
		{
			if (iterA->second != NULL)
			{
				iterA->second->permissive = connect;
/*
				DEBUG_MSG("set portal %s connection on chunk %s to %s\n",
							conn.portal_->label.c_str(),
							conn.portal_->pChunk->identifier().c_str(),
							connect ? "True" : "False");
*/
			}
			else
			{
				iterA = na.pSet()->connectionsEnd();
			}
			break;
		}
	}

	ChunkWaypointConns::iterator iterB = nb.pSet()->connectionsEnd();
	for (;iterB != nb.pSet()->connectionsEnd(); ++iterB)
	{
		if (iterB->first == na.pSet())
		{
			if (iterB->second != NULL)
			{
				iterB->second->permissive = connect;
/*
				DEBUG_MSG("set portal %s connection on chunk %s to %s\n",
							conn.portal_->label.c_str(),
							conn.portal_->pChunk->identifier().c_str(),
							connect ? "True" : "False");
*/
			}
			else
			{
				iterB = nb.pSet()->connectionsEnd();
			}
			break;
		}
	}

	// see if we succeeded
	if (iterA == na.pSet()->connectionsEnd() &&
			iterB == nb.pSet()->connectionsEnd())
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.configureConnection: "
			"Set 0x%p in chunk %s is not adjacent to set 0x%p in chunk %s "
			"in the waypoint graph",
			na.pSet().get(), na.pSet()->chunk()->identifier().c_str(),
			nb.pSet().get(), nb.pSet()->chunk()->identifier().c_str() );
		return NULL;
	}
	if (iterA == na.pSet()->connectionsEnd() ||
			iterB == nb.pSet()->connectionsEnd())
	{
		// this is a system problem, don't trouble Python with it.
		ERROR_MSG( "configureConnection: Waypoint sets not mutually adjacent - "
			"connection configured in one direction only. "
			"na 0x%p in %s, nb 0x%p in %s\n",
			na.pSet().get(), na.pSet()->chunk()->identifier().c_str(),
			nb.pSet().get(), nb.pSet()->chunk()->identifier().c_str() );
	}

	Py_RETURN_NONE;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, configureConnection,
	ARG( SpaceID, ARG( Vector3, ARG( Vector3, ARG( bool, END ) ) ) ), BigWorld )


/*~	function BigWorld.navigatePathPoints
 * 	@components{ cell }
 *
 * 	Return a path of points between the given source and destination points in
 * 	the space of the given space ID. The space must be loaded on this CellApp. 
 *
 * 	@param spaceID 	(int)		The space ID.
 * 	@param src	(Vector3)		The source point in the space.
 * 	@param dst	(Vector3)		The destination point in the space.
 * 	@param maxSearchDistance (float)
 * 								The maximum search distance, defaults to
 *								maxAoIRadius.
 * 	@param girth (float) 		The navigation girth grid to use, defaults to
 * 								0.5m.
 *
 * 	@return (list) 	A list of Vector3 points between the source point to the
 * 					destination point.
 *
 */
PyObject * navigatePathPoints( SpaceID spaceID, const Vector3 & src, 
		const Vector3 & dst, float maxSearchDistance, float girth )
{
	if (isEqual( maxSearchDistance, -1.f ))
	{
		maxSearchDistance = CellAppConfig::maxAoIRadius();
	}

	Space * pSpace = CellApp::instance().findSpace( spaceID );
	if (!pSpace)
	{
		PyErr_SetString( PyExc_ValueError, "Not a valid space ID." );
		return NULL;
	}

	ChunkSpacePtr pChunkSpace = pSpace->pChunkSpace();
	if (!pChunkSpace)
	{
		PyErr_Format( PyExc_ValueError,
				"BigWorld.navigatePathPoints(): "
				"Space ID %d does not support navigation-related queries",
				int(spaceID) );
		return NULL;
	}

	PyObject * pResult =
		NavmeshNavigationSystem::navigatePathPoints( pChunkSpace.get(),
							src, dst, maxSearchDistance, girth );

	return pResult;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, navigatePathPoints, 
	ARG( SpaceID, ARG( Vector3, ARG( Vector3, 
		OPTARG( float, -1.f, OPTARG( float, 0.5f, END ) ) ) ) ), 
	BigWorld )

} // end (anonymous) namespace

BW_END_NAMESPACE

// entity_navigate.cpp
