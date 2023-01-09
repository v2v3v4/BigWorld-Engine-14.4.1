#include "script/first_include.hpp"

#include "passenger_controller.hpp"

#include "cellapp.hpp"
#include "entity.hpp"
#include "passenger_extra.hpp"
#include "passengers.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PassengerController
// -----------------------------------------------------------------------------

IMPLEMENT_CONTROLLER_TYPE( PassengerController, DOMAIN_GHOST )


/**
 *	Constructor.
 */
PassengerController::PassengerController( EntityID vehicleID ) :
	vehicleID_( vehicleID ),
	initialLocalPosition_( Entity::INVALID_POSITION ),
	initialGlobalPosition_( Entity::INVALID_POSITION )
{
}


/**
 *	Destructor.
 */
PassengerController::~PassengerController()
{
}


/**
 *	This method streams on ghosted data.
 */
void PassengerController::writeGhostToStream( BinaryOStream & stream )
{
	this->Controller::writeGhostToStream( stream );

	Entity & entity = this->entity();

	stream << vehicleID_ <<
		entity.localPosition() << entity.localDirection() <<
		entity.position() << entity.direction();
}


/**
 *	This method streams off ghosted data.
 */
bool PassengerController::readGhostFromStream( BinaryIStream & stream )
{
	this->Controller::readGhostFromStream( stream );

	stream >> vehicleID_ >>
		initialLocalPosition_ >> initialLocalDirection_ >>
		initialGlobalPosition_ >> initialGlobalDirection_;

	return true;
}


/**
 *	Ghost startup method.
 *	We either got on a vehicle or we are just learning about this entity.
 */
void PassengerController::startGhost()
{
	Entity * pVehicle = CellApp::instance().findEntity( vehicleID_ );
	PassengerExtra::instance( this->entity() ).setController( this );

	if (initialLocalPosition_ != Entity::INVALID_POSITION)
	{
		this->entity().setLocalPositionAndDirection( initialLocalPosition_,
				initialLocalDirection_ );
	}

	if (pVehicle)
	{
		MF_VERIFY( Passengers::instance( *pVehicle ).add( this->entity() ) );

		this->entity().setVehicle( pVehicle, Entity::KEEP_LOCAL_POSITION );
	}
	else
	{
		WARNING_MSG( "PassengerController::startGhost(%u): "
			"Could not find vehicle %u\n", this->entity().id(), vehicleID_ );
		MF_ASSERT( !this->entity().isReal() );

		if ((initialGlobalPosition_ != Entity::INVALID_POSITION) &&
				(this->entity().removalHandle() == NO_SPACE_REMOVAL_HANDLE))
		{
			this->entity().setGlobalPositionAndDirection(
				initialGlobalPosition_, initialGlobalDirection_ );
		}

		this->onVehicleGone();
	}
}


/**
 *	Ghost shut down method.
 *	We either got off the vehicle or the (passenger) entity is going away.
 */
void PassengerController::stopGhost()
{
	Entity & entity = this->entity();
	MF_ASSERT( PassengerExtra::instance.exists( entity ) );

	PassengerExtra::instance( entity ).setController( NULL );

	Entity * pVehicle = entity.pVehicle();

	if (pVehicle)
	{
		if (Passengers::instance.exists( *pVehicle ))
		{
			MF_VERIFY( Passengers::instance( *pVehicle ).remove( entity ) );
		}

		entity.setVehicle( NULL, Entity::KEEP_GLOBAL_POSITION );
	}
	else
	{
		DEBUG_MSG( "PassengerController::stopGhost(%u): "
			"Passenger was in limbo for vehicle id %u\n",
			this->entity().id(), vehicleID_ );
		MF_VERIFY( Entity::population().removeObserver( vehicleID_, this ) );
		// note: position will be wrong if we get onloaded to right after
		// this, but hey... that should be pretty rare :)
	}

	// We no longer need the passenger extra.
	PassengerExtra::instance.clear( entity );
}


/**
 *	This method is called when the vehicle that we are on is destructed,
 *	disappears in some other way, or is not there when we start.
 */
void PassengerController::onVehicleGone()
{
	Entity * pVehicle = this->entity().pVehicle();

	if (this->entity().isReal())
	{
		ERROR_MSG( "PassengerController::onVehicleGone( %u ): "
				"Real entity alighting since it has lost "
				"its vehicle id %u (was %s on this cell)\n",
			this->entity().id(), vehicleID_, pVehicle ? "present" :"missing" );

		bool ok = PassengerExtra::instance( this->entity() ).alightVehicle();

		// not much that we can do about it if it fails 'tho
		if (!ok)
		{
			PyErr_Print();
			PyErr_Clear();
		}
	}
	else
	{
		if (pVehicle)
		{
			MF_ASSERT( Passengers::instance.exists( *pVehicle ) );
			MF_VERIFY( Passengers::instance( *pVehicle ).
				remove( this->entity() ) );
		}

		// The passenger is now in limbo. If we are being called from
		// startReal, then we do have some idea of our position, passed
		// in as pBackupPosition. Otherwise we just leave it where it lies.
		this->entity().setVehicle( NULL, Entity::IN_LIMBO );

		Entity::population().addObserver( vehicleID_, this );
	}
}


/**
 *	This method is called when the vehicle enters the population.
 */
void PassengerController::onEntityAdded( Entity & vehicle )
{
	MF_VERIFY( Passengers::instance( vehicle ).add( this->entity() ) );
	this->entity().setVehicle( &vehicle, Entity::KEEP_LOCAL_POSITION );
}

BW_END_NAMESPACE

// passenger_controller.cpp
