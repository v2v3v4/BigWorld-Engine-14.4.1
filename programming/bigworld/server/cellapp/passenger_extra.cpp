#include "script/first_include.hpp"

#include "passenger_extra.hpp"

#include "cellapp.hpp"
#include "passenger_controller.hpp"
#include "passengers.hpp"
#include "real_entity.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PassengerExtra
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PassengerExtra )

PY_BEGIN_METHODS( PassengerExtra )
	PY_METHOD( boardVehicle )
	PY_METHOD( alightVehicle )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PassengerExtra )
PY_END_ATTRIBUTES()

/// static initialiser
const PassengerExtra::Instance<PassengerExtra>
	PassengerExtra::instance( PassengerExtra::s_getMethodDefs(),
			PassengerExtra::s_getAttributeDefs() );


/**
 *	Constructor.
 */
PassengerExtra::PassengerExtra( Entity & e ) :
	EntityExtra( e ),
	pController_( NULL )
{
}


/**
 *	Destructor.
 */
PassengerExtra::~PassengerExtra()
{
	MF_ASSERT( !pController_ );
}


/**
 *	This method sets the controller associated with this extra.
 */
void PassengerExtra::setController( PassengerController * pController )
{
	// We need to either be setting or clearing the controller.
	// So either pController must not exist or pController_ must not exist.
	MF_ASSERT( (!pController) ^ (!pController_) );

	pController_ = pController;

	if (pController == NULL)
	{
		PassengerExtra::instance.clear( entity_ );
	}
}


/*~ function Entity.boardVehicle
 *	@components{ cell }
 *
 *	This function adds an NPC Entity as a passenger of the entity with the
 *	given ID.  It should only be used for Entities controlled by a Cell, not
 *	Player Entities (see BigWorld.controlEntity() and
 *	Entity.physics.teleportVehicle() in the Client documentation).
 *
 *	Any Entity can be used as a vehicle, eg, car, bus, moving platform, animal.
 *	The position and movement of the Entities are linked so they move in
 *	unison.  The Model on the Client should support the animations required to
 *	board, alight from and ride the Entity, as necessary.
 *
 *	AI Control can be transferred to the vehicle in script if required, or left
 *	with the another Entity (eg, inside bus as passenger) or the vehicle itself
 *	(eg, moving platform).
 *
 *	An Entity may only use one vehicle at a time.  However, many Entities can
 *	be a passenger of the same vehicle. If the vehicle is destroyed, all
 *	passengers will be forced to alight.
 *
 *	This method raises a TypeError if the entity is not a real entity, or if 
 *	an entity is attempting to board its own passenger.
 *
 *	This method raises a ValueError if the entity is already on a vehicle, or
 *	if the vehicleID refers to a non-existent entity.
 *
 *	@param vehicleID (entityID) ID of Entity to be used as a vehicle
 *
 */
/**
 *	This method adds this entity as a passenger of the input entity.
 *
 *	@return True on success, otherwise false.
 *
 *	@note The Python error state is set on failure.
 */
bool PassengerExtra::boardVehicle( EntityID vehicleID, bool shouldUpdateClient )
{
	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method may only be called on a real" );
		PassengerExtra::instance.clear( entity_ );
		return false;
	}

	if (pController_)
	{
		PyErr_SetString( PyExc_ValueError, "Already on a vehicle" );
		return false;
	}

	Entity * pVehicle = CellApp::instance().findEntity( vehicleID );

	if (!pVehicle)
	{
		WARNING_MSG( "PassengerExtra::boardVehicle: "
				"%u is trying to board non-existent entity %u\n",
			entity_.id(), vehicleID );
		PyErr_Format( PyExc_ValueError, "Entity.boardVehicle: "
			"No entity id %d", vehicleID );
		PassengerExtra::instance.clear( entity_ );
		return false;
	}

	// make sure that the potential vehicle is not already one of our
	// passengers! .... yes someone did do this! this check is not perfect
	// as there may be passengers in limbo or a race condition, etc.
	for (Entity * pWalk = pVehicle; pWalk != NULL; pWalk = pWalk->pVehicle())
	{
		if (pWalk == &entity_)
		{
			WARNING_MSG( "PassengerExtra::boardVehicle: "
					"%u is trying to board its own passenger %d!\n",
				entity_.id(), vehicleID );
			PyErr_Format( PyExc_TypeError, "Entity.boardVehicle: "
				"Cannot board own passenger %d", vehicleID );
			PassengerExtra::instance.clear( entity_ );
			return false;
		}
	}

	// Make sure this vehicle is set up with Passengers extension
	Passengers::instance( *pVehicle );
	entity_.setVehicle( pVehicle, Entity::KEEP_GLOBAL_POSITION );

	entity_.addController( new PassengerController( vehicleID ), 0 );

	MF_ASSERT( pController_ );

	RealEntity * pReal = entity_.pReal();
	if (pReal->controlledByRef().id != 0 && shouldUpdateClient)
	{
		pReal->sendPhysicsCorrection();
	}

	return true;
}


/*~ function Entity.alightVehicle
 *  @components{ cell }
 *
 *	This function removes an NPC Entity as a passenger of its currently boarded
 *	vehicle. It should only be used for Entities controlled by a Cell, not
 *	Player Entities (see BigWorld.controlEntity() and
 *	Entity.physics.teleportVehicle() in the Client documentation). Any Entity
 *	can be used as a vehicle, eg, car, bus, moving platform, animal. The
 *	position and movement of the Entities are linked so they move in unison.
 *	The Model on the Client should support the animations required to board,
 *	alight from and ride the Entity, as necessary. AI Control can be transferred
 *	to the vehicle in script if required, or left with the another Entity (eg,
 *	inside bus as passenger) or the vehicle itself (eg, moving platform). An
 *	Entity may only use one vehicle at a time. However, many Entities can be a
 *	passenger of the same vehicle.  If the vehicle is destroyed, all passengers
 *	will be forced to alight.
 *
 */
/**
 *	This method removes this entity as a passenger of its current vehicle.
 *
 *	@return True on success, otherwise false.
 *
 *	@note The Python error state is set on failure.
 */
bool PassengerExtra::alightVehicle( bool shouldUpdateClient )
{
	if (!pController_)
	{
		PyErr_SetString( PyExc_TypeError, "Not on a vehicle" );
		return false;
	}

	if (!entity_.isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method may only be called on a real" );
		return false;
	}

	RealEntity * pReal = entity_.pReal();

	if (!entity_.delController( pController_->id() ))
	{
		PyErr_SetString( PyExc_RuntimeError,
			"Failed to delete the passenger controller" );
		return false;
	}

	// NOTE: The PassengerExtra might have been deleted by here

	if ((pReal->controlledByRef().id != 0) && shouldUpdateClient)
	{
		pReal->sendPhysicsCorrection();
	}

	return true;
}


/**
 *	This method is called to inform us that the vehicle entity has been destroyed.
 */
void PassengerExtra::onVehicleGone()
{
	MF_ASSERT( pController_ );

	if (pController_)
	{
		pController_->onVehicleGone();
	}
}


/**
 *	Static method to report whether or not the given entity is in limbo
 */
bool PassengerExtra::isInLimbo( Entity & e )
{
	return (e.pVehicle() == NULL) &&
		instance.exists( e ) && instance( e ).pController_;
}


/**
 *	Override of notification that our entity has moved around.
 */
void PassengerExtra::relocated()
{
	// Note: This call is called pretty much last in the process of an onload,
	// albeit fortunately before script calls are permitted again.
	// So we are pretty much safe to do anything here, even potentially
	// offloading the entity or some such ('tho that will not happen here)

	if (entity_.isReal() && isInLimbo( entity_ ))
	{
		INFO_MSG( "PassengerExtra::relocated(%u): "
			"Getting off our vehicle since we have just been onloaded "
			"into a cell where it does not exist\n", entity_.id() );

		this->onVehicleGone();
		// onVehicleGone will call through to our alightVehicle
		// (and if we are a controlled by a client then the physics correction
		// will be automatically sent to it by that method)
	}
}

BW_END_NAMESPACE

// passenger_extra.cpp
