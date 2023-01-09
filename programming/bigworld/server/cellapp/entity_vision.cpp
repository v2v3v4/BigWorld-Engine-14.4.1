#include "script/first_include.hpp"

#include "entity_vision.hpp"

#include "entity.hpp"
#include "real_entity.hpp"
#include "cell.hpp"
#include "cellapp.hpp"
#include "space.hpp"

#include "vision_controller.hpp"
#include "scan_vision_controller.hpp"
#include "visibility_controller.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "physics2/worldtri.hpp"


DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityVision
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( EntityVision )

PY_BEGIN_METHODS( EntityVision )
	PY_METHOD( addVision )
	PY_METHOD( addScanVision )
	PY_METHOD( setVisionRange )
	PY_METHOD( entitiesInView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( EntityVision )
	/*~ attribute Entity.seeingHeight
	*  @components{ cell }
	*
	*  This attribute stores the height on the Entity to use as the source of
	*  the current field of view, eg, eye-level of current Model.  This should
	*  be changed as the point of vision changes, eg, if the Model crouches,
	*  but not when jumping because the height from the base of the Model is
	*  the same.
	*
 	*  @see Entity.addVision
 	*
	*  @type float
	*/
	PY_ATTRIBUTE( seeingHeight )

	/*~ attribute Entity.visibleHeight
	*  @components{ cell }
	*
	*  This attribute stores the height that the vision controllers will use to
	*  determine whether this Entity can be seen.  It should correspond to the
	*  height of the current Model and be updated if the Model changes height,
	*  eg, when crouched, but not when jumping because the height from the base
	*  of the Model is the same.
 	*
 	*  @see Entity.addVision
	*
	*  @type float
	*/
	PY_ATTRIBUTE( visibleHeight )

	/*~ attribute Entity.canBeSeen
	*  @components{ cell }
	*
	*  If set to True, this attribute indicates that this Entity should be
	*  included in vision updates.
	*
	*  If set to False, the Entity will be 'invisible' to any Entity's vision
	*  controller.
	*
 	*  @see Entity.addVision
 	*
	*  @type bool
	*/
	PY_ATTRIBUTE( canBeSeen )

	/*~ attribute Entity.shouldDropVision
	*  @components{ cell }
	*
	*  If set to True, this attribute indicates that this Entity's seeingHeight
	*  and visibleHeight should be dropped by 2 metres or to the terrain
	*  surface, if within that 2m range.  This is because on the Cell, the
	*  Entity moves on the BigWorld navPoly system.  No part of any navPoly
	*  will be beneath the terrain or more than 2 metres higher, so by dropping
	*  these seeingHeight and visibleHeight by 2m, we ensure that the heights
	*  are relative to the terrain.
	*
	*  If set to False, the heights will remain relative to the navPoly, rather
	*  than the terrain.
	*
 	*  @see Entity.addVision
 	*
	*  @type bool
	*/
	PY_ATTRIBUTE( shouldDropVision )

	/*~ attribute Entity.fieldOfView
	*  @components{ cell }
	*
	*  This attribute stores the angle of the vision cone in radians.  Entities
	*  that are greater than this angle, in radians, from this entity's
	*  direction will not be seen. The field of view is double this number. For
	*  example, a field of view of 180 degrees (pi radians) would be equivalent
	*  to a vision cone angle of 90 degrees (pi/2 radians).
	*
	*  This attribute is deprecated due to the confusion it can cause. It is
	*  identical to the visionAngle attribute.
	*
 	*  @see Entity.addVision
 	*
	*  @type float
	*/
	PY_ATTRIBUTE( fieldOfView )

	/*~ attribute Entity.visionAngle
	*  @components{ cell }
	*
	*  This attribute stores the angle of the vision cone in radians.  Entities
	*  that are greater than this angle, in radians, from this entity's
	*  direction will not be seen. The field of view is double this number. For
	*  example, a field of view of 180 degrees (pi radians) would be equivalent
	*  to a vision cone angle of 90 degrees (pi/2 radians).
	*
 	*  @see Entity.addVision
 	*
	*  @type float
	*/
	PY_ATTRIBUTE( visionAngle )

	/*~ attribute Entity.visionRange
	*  @components{ cell }
	*
	*  This attribute stores the range, in metres, of the entity's
	*  vision. Entities further away than this distance will not be
	*  seen by this entity.
	*
 	*  @see Entity.addVision
 	*
	*  @type float
	*/
	PY_ATTRIBUTE( visionRange )
PY_END_ATTRIBUTES()

// Callbacks
/*~ callback Entity.onStartSeeing
 *  @components{ cell }
 *	This method is called when a new entity enters this entity's vision. See
 *	Entity.addVision for how to add vision to an entity.
 *
 *	@see Entity.addVision
 *
 *	@param entity	The entity that has just entered this entity's vision.
 *	@param userData
 */
/*~ callback Entity.onStopSeeing
 *  @components{ cell }
 *	This method is called when an entity leaves this entity's vision.
 *
 *	@see Entity.addVision
 *	@see Entity.onStopSeeingID
 *
 *	@param 	entity	The entity that has just left this entity's vision.
 *	@param 	userData
 */
/*~ callback Entity.onStopSeeingID
 *  @components{ cell }
 *	This method is called when an entity leaves this entity's vision and the
 *	entity is not on this cell. Getting this method called should be rare. It
 *	can only occur if the entity is onloaded to a new cell that does not have
 *	a currently seen entity.
 *
 *	@see Entity.addVision
 *	@see Entity.onStopSeeing
 *
 *	@param 	entityID	The id of the entity that has just left this entity's
 *						vision.
 *	@param 	userData
 */


/// static initialiser
const EntityVision::Instance<EntityVision>
	EntityVision::instance( EntityVision::s_getMethodDefs(),
			EntityVision::s_getAttributeDefs() );

/**
 *	Constructor
 */
EntityVision::EntityVision( Entity & e ) :
	EntityExtra( e ),
	lastDropPosition_( 100000.f, 0.f, 100000.f ),
	entitiesInVisionRange_(),
	visibleEntities_(),
	visibility_( NULL ),
	vision_( NULL ),
	shouldDropVision_( false ),
	iterating_( false ),
	iterationCancelled_( false )
#ifdef DEBUG_VISION
	, seenByEntities_()
#endif
{
}

/**
 *	Destructor
 */
EntityVision::~EntityVision()
{
}



/*~ function Entity.entitiesInView
 *  @components{ cell }
 *
 *	This function returns a list of all the Entities that can be seen with
 *	the current field of view and vision range.
 *
 *	@see Entity.addVision
 *
 *	@return					(list)	List of Entities currently in view
 */
/**
 *	This method returns a list of entities that can be seen by the associated
 *	entity. This returns a new reference to a Python object.
 */
PyObject* EntityVision::entitiesInView() const
{
	// TODO: May be able to use our stl-to-python wrappers here.
	PyObject *pList= PyList_New( 0 );

	for (EntitySet::const_iterator iter = visibleEntities_.begin();
			iter != visibleEntities_.end();
			++iter)
	{
		Entity* entity = (*iter).get();
		PyList_Append( pList, entity );
	}

	return pList;
}


/**
 *	This method updates the set of visible entities. It tells the script about
 *	any changes.
 *
 *	@return The set of visible entities.
 */
const EntitySet & EntityVision::updateVisibleEntities( float seeingHeight,
		float yawOffset )
{
	EntitySet newVisible;
	EntitySet oldVisible;
	oldVisible.swap( visibleEntities_ );

	const ChunkSpace * cs = entity_.pChunkSpace();
	// TODO: remove the following when collide() query
	// is supported by all Physical Space types. Then
	// will need to use PhysicalSpace instead of ChunkSpace
	if (!cs) {
		ERROR_MSG( "EntityVision::updateVisibleEntities(): "
				"Delegate space type is not supported\n" );
		return visibleEntities_;
	}

	Vector3 headPos = this->getDroppedPosition();
	headPos.y += seeingHeight;

	float yaw = entity_.direction().yaw + yawOffset;

	float visionAngle = (vision_ != NULL) ? vision_->visionAngle() : 1.f;

	Vector3 direction;
	direction.setPitchYaw( entity_.direction().pitch, yaw );

	for (EntitySet::iterator iter = entitiesInVisionRange_.begin();
			iter != entitiesInVisionRange_.end();
			iter++)
	{
		Entity* pEntity = (*iter).get();
		MF_ASSERT( pEntity );
		MF_ASSERT( !pEntity->isDestroyed() );

		bool canSee = false;
		if (EntityVision::canBeSeen( pEntity ))
		{
			EntityVision & ev = EntityVision::instance( *pEntity );

			Vector3 targetPos =
				ev.getDroppedPosition() + Vector3( 0, ev.visibleHeight(), 0 );

			for (int i = 0; i < 2; i++)
			{
				float dist = cs->collide( headPos, targetPos );
				// collide above does not require closest obstacle,
				// any obstacle will do (i.e. dist may be too big)

				//if sight is blocked, move on to next entity
				if (dist > 0.f)
				{
					continue;
				}

				Vector3 entityDirection = targetPos - headPos;
				entityDirection.normalise();

				float dotProduct = direction.dotProduct( entityDirection );
				float angle      = acosf( dotProduct );

				if (angle < visionAngle)
				{
					canSee = true;
					break;
				}

				// second time through, try a point
				targetPos = ev.getDroppedPosition() +
					Vector3(0, 3.f / 4.f *  ev.visibleHeight(), 0);
			}
		}

		if (canSee)
		{
			MF_ASSERT( pEntity );
			newVisible.insert( pEntity );
		}
	}

	// Find the difference between the old and the new visible entities.
	EntitySet::const_iterator oldIter = oldVisible.begin();
	EntitySet::const_iterator newIter = newVisible.begin();

	EntitySet::const_iterator oldEnd = oldVisible.end();
	EntitySet::const_iterator newEnd = newVisible.end();

	iterating_ = true;
	while (oldIter != oldEnd && newIter != newEnd)
	{
		if ((*oldIter) < (*newIter))
		{
			this->callback( "onStopSeeing", oldIter->get() );
			++oldIter;
		}
		else if ((*oldIter) > (*newIter))
		{
			this->callback( "onStartSeeing", newIter->get() );
			++newIter;
		}
		else
		{
			// Still visible
			++oldIter;
			++newIter;
		}
	}

	// process the tails if we have not stopped iterating
	while (oldIter != oldEnd)
	{
		this->callback( "onStopSeeing", oldIter->get() );
		++oldIter;
	}

	while (newIter != newEnd)
	{
		this->callback( "onStartSeeing", newIter->get() );
		++newIter;
	}
	iterating_ = false;

	if (iterationCancelled_)
	{
		// otherwise we are no longer being controlled by the same vision
		// controller, so delete ourselves unless a new controller has
		// since come along
		iterationCancelled_ = false;

		if (!this->isNeeded())
			// TODO: Note that the onStopSeeing method is not called for the
			// remaining entities in the visible list.
			instance.clear( entity_ );
		else if (vision_)
			visibleEntities_.swap( newVisible );
	}
	else
	{
		// if we're still iterating, then good, swap to the new visible set
		visibleEntities_.swap( newVisible );
	}

	return visibleEntities_;
}


/**
 *	This helper method is used to call a script method on the associated entity.
 */
void EntityVision::callback( const char * methodName, Entity * pEntity ) const
{
	SCOPED_PROFILE( SCRIPT_CALL_PROFILE );
	entity_.callback( methodName, Py_BuildValue( "(O)", pEntity ),
		methodName, true );
}


/**
 *	This method is called by VisionController::startReal when it has entities
 *	from an onload.
 */
void EntityVision::setVisibleEntities(
		const BW::vector< EntityID > & visibleEntities )
{
	// This should be called before we have any visible entities.
	MF_ASSERT( visibleEntities_.empty() );

	BW::vector< EntityID >::const_iterator iter = visibleEntities.begin();
	BW::vector< EntityID >::const_iterator endIter = visibleEntities.end();

	while (iter != endIter)
	{
		Entity * pEntity = CellApp::instance().findEntity( *iter );

		if (pEntity != NULL)
		{
			visibleEntities_.insert( pEntity );
		}
		else
		{
			// TODO: Need to document this well.
			WARNING_MSG( "EntityVision::setVisibleEntities: "
					"Could not find entity %d\n", (int)*iter );

			entity_.callback( "onStopSeeingID",
				Py_BuildValue( "(i)", *iter ), "onStopSeeingID: ", true );
		}

		++iter;
	}
}



/**
 *	This method is called when an entity has entered the Vision Range.
 *
 *	@param who The entity that entered vision range.
 */
void EntityVision::triggerVisionEnter( Entity * who )
{
	if (!entity_.isReal()) return;

#ifdef DEBUG_VISION
	MF_ASSERT(entitiesInVisionRange_.find(who) == entitiesInVisionRange_.end())
	if(entitiesInVisionRange_.find(who) != entitiesInVisionRange_.end())
	{
		who->py_debug(NULL);
		who->debugDump();
		entity_.py_debug(NULL);
		entity_.debugDump();
		EntitySet::iterator iter;
		DEBUG_MSG("Entities in Vision Range\n");
		for (iter = entitiesInVisionRange_.begin();
				iter != entitiesInVisionRange_.end();
					iter++)
		{
			Entity* entity = (*iter).get();
			DEBUG_MSG("Entity %d %d\n",
					(int)entity->id(), (int)entity->isDestroyed());
		}
		DEBUG_MSG("Visible Entities\n");
		for (iter = visibleEntities_.begin();
				iter != visibleEntities_.end();
					iter++)
		{
			Entity* entity = (*iter).get();
			DEBUG_MSG("Entity %d %d\n",
					(int)entity->id(), (int)entity->isDestroyed());
		}
		DEBUG_MSG("Seen byEntities\n");
		for (iter = seenByEntities_.begin();
				iter != seenByEntities_.end();
					iter++)
		{
			Entity* entity = (*iter).get();
			DEBUG_MSG("Entity %d %d\n",
					(int)entity->id(), (int)entity->isDestroyed());
		}

		entity_.space().debugRangeList();
	}

	/*
	DEBUG_MSG("Entity %d entered vision range of %d\n",
		(int)who->id(), (int)entity_.id());

	EntityVision whoEV = EntityVision::instance( *who );
	if (entitiesInVisionRange_.find(who) != entitiesInVisionRange_.end())
	{
		DEBUG_MSG("%d already in  %d's vision range!!!!\n",
			(int)who->id(),(int)entity_.id());
	}
	if (whoEV.seenByEntities().find(&entity_) != whoEV.seenByEntities().end())
	{
		DEBUG_MSG("%d already in %d's seen bby list!!!!\n",
			(int)entity_.id(),(int)who->id());
	}
	*/
	EntityVision::instance( *who ).seenByEntities().insert( &entity_ );

#endif // DEBUG_VISION

	MF_ASSERT( who );
	entitiesInVisionRange_.insert( who );
}


/**
 *	This method is called when an entity has left the Vision Range.
 *
 *	@param who The entity that left vision range.
 */
void EntityVision::triggerVisionLeave( Entity * who )
{
	if (!entity_.isReal()) return;

#ifdef DEBUG_VISION
	if(entitiesInVisionRange_.find(who) == entitiesInVisionRange_.end())
	{
		who->py_debug(NULL);
		who->debugDump();
		entity_.py_debug(NULL);
		entity_.debugDump();
		entity_.space().debugRangeList();
	}
	MF_ASSERT(entitiesInVisionRange_.find(who) != entitiesInVisionRange_.end())

#endif // DEBUG_VISION

	// TODO: May want to consider what happens when an entity teleports. We do
	// not want to call script here because we may be iterating over
	// visibleEntities_.
	entitiesInVisionRange_.erase( who );
#ifdef DEBUG_VISION
	EntityVision::instance( *who ).seenByEntities().erase( &entity_ );

	EntityVision whoEv = EntityVision::instance( *who );
	if(entitiesInVisionRange_.find(who) != entitiesInVisionRange_.end())
	{
		DEBUG_MSG("%d didn't delete %d from vision range!!!!\n",
				(int)entity_.id(),(int)who->id());
	}
	if(whoEv.seenByEntities().find(&entity_) != whoEv.seenByEntities().end())
	{
		DEBUG_MSG("%d didn't delete %d from seen by list!!!!\n",
				(int)who->id(),(int)entity_.id());
	}
#endif // DEBUG_VISION
}


/*~ function Entity.setVisionRange
 *  @components{ cell }
 *
 *	This function alters the range and field of view of currently
 *	active vision controller.
 *
 *	A TypeError is raised if it is not called on a real entity and if the
 *	entity does not have a vision controller.
 *
 *	@see Entity.addVision
 *
 *	@param	visionAngle	(float)	The angle of the vision cone in radians. The
 * 								field of view is double this value.
 *	@param	range		(float)	The new range of the vision
 */
/**
 *	This method is exposed to scripting. It is used by an entity to setup
 *	a vision trigger at a specific range from the entity.
 *
 *	@param visionAngle 	The angle of the vision cone in radians.
 * 						The field of view is double this value.
 *	@param range Vision range in metres.
 *
 *	@return true on success and false if this is a ghost or on failure
 *
 *	@note  The Python error state is set on failure
 */
bool EntityVision::setVisionRange( float visionAngle, float range )
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return false;
	}

	if (vision_ == NULL)
	{
		PyErr_SetString( PyExc_TypeError,
			"This method requires an entity with a vision controller." );
		return false;
	}

	vision_->setVisionRange( visionAngle, range );
	return true;
}

/*~ function Entity.addVision
 *  @components{ cell }
 *
 *	This function creates a controller that allows the Entity to (virtually)
 *	visualise other nearby Entities that have their Entity.canBeSeen attribute
 *	set to True, invoking a notification method when an Entity becomes seen and
 *	not seen.  For a given Entity, only one vision controller should be active
 *	at any one time.  Returns a ControllerID that can be used to cancel() the
 *	'vision'.
 *
 *	The notification methods are not called when 'vision' is cancelled...
 *	The notification methods are defined as follows;
 *
 *	@{
 *		def onStartSeeing( self, entityNowSeen ):
 *		def onStopSeeing( self, entityNoLongerSeen ):
 *		def onStopSeeingID( self, entityIDNoLongerSeen ):
 *	@}
 *
 *	@see Entity.canBeSeen
 *	@see Entity.addScanVision
 *	@see Entity.cancel
 *
 *	@param	angle			(float)				The angle of the vision cone in
 * 												radians. The field of view is
 * 												double this value.
 *	@param	range			(float)				The range of the vision
 *	@param	seeingHeight	(float)				The height on the Entity to use
 *												as the source of the field of
 *												view, eg, eye-level of current
 *												Model
 *	@param	period			(optional int)		How often the vision is updated
 *												in ticks (defaults to 10 == 1
 *												sec)
 *
 *	@return					(integer)			The ID of the newly created
 *												controller
 */
/**
 *	This method is exposed to scripting. It is used by an entity to register
 *	that it can see things
 *
 *	@param angle		The angle of the vision cone in radians.
 * 						The field of view is double this value.
 *	@param range		Range of vision
 *	@param seeingHeight	The distance above the entity position that vision
 *						should be from.
 *	@param period		The period in ticks that vision should be recalculated.
 *	@param userArg		User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject* EntityVision::addVision( float angle, float range, float seeingHeight,
		int period, int userArg )
{
	ControllerID controllerID;
	//DEBUG_MSG("EntityVision::addVision: id=%d, angle=%6.3f, range=%6.3f, period=%d\n",
	//	(int)entity_.id(), angle, range, period);
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	if (range <= 0.f)
	{
		PyErr_SetString( PyExc_AttributeError,
				"Can't start vision system with 0 or negative range" );
		return NULL;
	}
	else if (!vision_)
	{
		ControllerPtr pController =
			new VisionController( angle, range, seeingHeight, period );
		controllerID = entity_.addController( pController, userArg );

		return Script::getData( controllerID );
	}
	else
	{
		PyErr_SetString( PyExc_AttributeError,
				"Already have a vision controller" );
		return NULL;
	}
}

PyObject* addScanVision( float angle, int updatePeriod = 1, int userArg = 0 );


/*~ function Entity.addScanVision
 *  @components{ cell }
 *
 *	This function creates a controller that allows the Entity to scan left and
 *	right to (virtually) visualise other nearby Entities, invoking a
 *	notification method when an Entity becomes seen and not seen.  For a given
 *	Entity, only one vision controller should be active at any one time.
 *	Returns a ControllerID that can be used to cancel() the 'vision'.  The
 *	notification methods are not called when 'vision' is cancelled...  The
 *	notification methods are defined as follows;
 *
 *	@{
 *		def onStartSeeing( self, entityNowSeen ):
 *		def onStopSeeing( self, entityNoLongerSeen ):
 *		def onStopSeeingID( self, entityIDNoLongerSeen ):
 *	@}
 *
 *	@see Entity.addVision
 *	@see Entity.cancel
 *
 *	@param	angle		(float)				The angle of the vision cone in
 * 											radians. The field of view is
 * 											double this value.
 *	@param	range		 (float)			The range of the vision
 *	@param	seeingHeight (float)			The height on the Entity to use as
 *											the source of the field of view, eg,
 *											eye-level of current Model
 *	@param	amplitude	 (float)			The maximum yaw offset, in radians,
 *								that the entity scans. That is, the scan range
 *								is [yaw - amplitude, yaw + amplitude].
 *	@param	scanPeriod	 (float)			The time it takes to make each scan
 *											in seconds
 *	@param	phaseOffset	 (float)			Offset (phase) of the scan as a
 *											fraction.
 *	@param	updatePeriod (optional int)		How often the vision is updated in
 *											ticks (defaults to 1 == 1/10 sec)
 *	@param	userData	 (optional object)	Data that is passed to notification
 *											method
 *
 *	@return				 (integer)			The ID of the newly created
 *											controller
 */
/**
 *	This method is exposed to scripting. It is a scanning version of
 *	Entity.addVision.
 *
 *	@param angle		The angle of the vision cone in radians.
 *	@param range		Range of vision
 *	@param seeingHeight	The distance above the entity position that vision
 *						should be from.
 *  @param amplitude	Amplitude of scan in radians
 *  @param scanPeriod	Scanning Period
 *  @param timeOffset   Offset (phase) for scanning, in seconds
 *  @param updatePeriod		Update period
 *	@param userArg		User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject* EntityVision::addScanVision( float angle,
							 float range,
							 float seeingHeight,
							 float amplitude,
							 float scanPeriod,
							 float timeOffset,
							 int updatePeriod,
							 int userArg )
{
	ControllerID controllerID;

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	if( range <= 0.f )
	{
		PyErr_SetString( PyExc_AttributeError,
			"Can't start vision system with 0 or negative range" );
		return NULL;
	}
	else if( amplitude <= 0.f || scanPeriod <= 0.f )
	{
		PyErr_SetString( PyExc_AttributeError,
			"Can't start vision system with 0 or negative amplitude or scan period" );
		return NULL;
	}
	else if( !vision_ )
	{
		ControllerPtr pController = new ScanVisionController( angle, range,
			seeingHeight, amplitude, scanPeriod, timeOffset, updatePeriod );
		controllerID = entity_.addController(pController, userArg);
		return Script::getData( controllerID );
	}
	else
	{
		PyErr_SetString( PyExc_AttributeError, "Already have a vision controller" );
		return NULL;
	}
}


/**
 *	This method sets the vision controller connected with this EntityExtra.
 */
void EntityVision::setVision( VisionController * vc )
{
	if (!iterating_)
	{
		// if we're not iterating then it's easy
		vision_ = vc;

		if (!this->isNeeded())
		{
			// TODO: Note that the onStopSeeing method is not called for the
			// remaining entities in the visible list.
			instance.clear( entity_ );
			return;
		}
	}
	else
	{
		iterationCancelled_ = true;
		vision_ = vc;
	}

	// Reset vision stuff
	entitiesInVisionRange_.clear();
	shouldDropVision_ = false;
	if (!vc)
	{
		// TODO: Note that the onStopSeeing method is not called for the
		// remaining entities in the visible list.
		visibleEntities_.clear();
		lastDropPosition_ = Position3D( 100000.f, 0.f, 100000.f );
	}
}

/**
 *	This method sets the visibility controller connected with this EntityExtra.
 */
void EntityVision::setVisibility( VisibilityController * vc )
{
	visibility_ = vc;
	// If iteration was cancelled then we will check isNeeded() at the end
	// of updateVisibleEntities().
	if (!iterationCancelled_ && !this->isNeeded())
		instance.clear( entity_ );
}

/**
 *	This method returns the visible height.
 */
float EntityVision::visibleHeight() const
{
	if (visibility_ != NULL)
		return visibility_->visibleHeight();
	return 0.f;
}


/**
 *	This method sets the visible height. It should only be called on real
 *	entities.
 */
void EntityVision::visibleHeight( float h )
{
	// scripts never get this far
	MF_ASSERT( entity_.isReal() );

	if (visibility_ != NULL)
	{
		if (!isEqual( visibility_->visibleHeight(), h ))
		{
			visibility_->visibleHeight( h );
		}
	}
	else
	{
		entity_.addController( new VisibilityController( h ), 0 );
		MF_ASSERT( visibility_ != NULL)
	}
}


/**
 *	This method returns whether or not this entity can be seen.
 */
bool EntityVision::canBeSeen() const
{
	return visibility_ != NULL;
}


/**
 *	This method sets whether or not this entity can be seen.
 */
void EntityVision::canBeSeen( bool v )
{
	// scripts never get this far
	MF_ASSERT( entity_.isReal() );

	if ((visibility_ != NULL) == v) return;

	if (visibility_ == NULL)
	{
		entity_.addController( new VisibilityController(), 0 );
		MF_ASSERT( visibility_ != NULL)
	}
	else
	{
		entity_.delController( visibility_->id() );
		// 'this' may have been deleted at this stage.
		// MF_ASSERT( visibility_ == NULL );
	}
}

/**
 *	Static method to determine whether or not the given entity can be seen.
 */
bool EntityVision::canBeSeen( Entity * pEntity )
{
	if (!EntityVision::instance.exists( *pEntity )) return false;
	return EntityVision::instance( *pEntity ).canBeSeen();
}


/**
 *	This method returns the dropped entity position for the
 *	purposes of vision.
 */
Position3D EntityVision::getDroppedPosition()
{
	if (!shouldDropVision_)
	{
		return entity_.position();
	}

	if (!isEqual( lastDropPosition_.x, entity_.position().x ) ||
		!isEqual( lastDropPosition_.z, entity_.position().z ))
	{
		lastDropPosition_ = entity_.getGroundPosition();
	}

	return lastDropPosition_;
}


/**
 *	This method returns the height from which you look from.
 */
float EntityVision::seeingHeight() const
{
	if (vision_ != NULL)
	{
		return vision_->seeingHeight();
	}
	return 0.f;
}


/**
 *	This method sets the height from which you look from.
 *	It should be called on real entities only.
 */
void EntityVision::seeingHeight( float h )
{
	// scripts never get this far
	MF_ASSERT( entity_.isReal() );

	if (vision_ != NULL)
	{
		if (!isEqual( vision_->seeingHeight(), h ))
		{
			vision_->seeingHeight( h );
		}
	}
}

/**
 *	This method returns the angle of the vision cone in radians.
 *
 * 	Deprecated. Use visionAngle() instead.
 */
float EntityVision::fieldOfView() const
{
	if (vision_ != NULL) return vision_->visionAngle();
	return 0.f;
}

/**
 *	This method sets the angle of the vision cone in radians.
 *
 * 	Deprecated. Use visionAngle() instead.
 */
void EntityVision::fieldOfView( float v )
{
	MF_ASSERT( entity_.isReal() );
	if (vision_ != NULL) vision_->setVisionRange( v, vision_->visionRange() );
}

/**
 *	This method returns the angle of the vision cone in radians.
 */
float EntityVision::visionAngle() const
{
	if (vision_ != NULL) return vision_->fieldOfView();
	return 0.f;
}

/**
 *	This method sets the angle of the vision cone in radians.
 */
void EntityVision::visionAngle( float v )
{
	MF_ASSERT( entity_.isReal() );
	if (vision_ != NULL) vision_->setVisionRange( v, vision_->visionRange() );
}

/**
 *	This method returns the vision range.
 */
float EntityVision::visionRange() const
{
	if (vision_ != NULL) return vision_->visionRange();
	return 0.f;
}

/**
 *	This method sets the vision range.
 */
void EntityVision::visionRange( float v )
{
	MF_ASSERT( entity_.isReal() );
	if (vision_ != NULL) vision_->setVisionRange( vision_->visionAngle(), v );
}

BW_END_NAMESPACE

// entity_vision.cpp
