#include "pch.hpp"

#include "deferred_passengers.hpp"

#include "bw_entity.hpp"
#include "bw_entities.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: DeferredPassengers
// ----------------------------------------------------------------------------


/**
 *	Constructor.
 */
DeferredPassengers::DeferredPassengers( BWEntities & entities ) :
	entities_( entities ),
	passengers_(),
	passengersIndex_()
{
	entities_.addListener( this );
}


/**
 *	Destructor.
 */
DeferredPassengers::~DeferredPassengers()
{
	MF_ASSERT( passengers_.empty() );
	MF_ASSERT( passengersIndex_.empty() );
	entities_.removeListener( this );
}


/**
 *	This method searches our list of pending passengers for a given EntityID
 *	and returns the BWEntity * if we know of it or NULL otherwise.
 *
 *	@param passengerID	The EntityID to search for.
 *	@return The BWEntity * of the found entity, or NULL if not known
 */
BWEntity * DeferredPassengers::findPassenger( EntityID passengerID ) const
{
	PassengersAndVehiclesByEntityID::const_iterator iIndex =
		passengersIndex_.find( passengerID );
	if (iIndex == passengersIndex_.end())
	{
		return NULL;
	}

	return iIndex->second.first;
}


/**
 *	This method adds a Passenger waiting for a given vehicle to our list of
 *	pending passengers. When the passenger's vehicle is available, the
 *	BWEntities::onEntityConsistent callback will be triggered.
 *	This may happen immediately.
 *
 *	@param pPassenger The BWEntity to store.
 *	@param vehicleID The EntityID of the vehicle the passenger is upon
 */
void DeferredPassengers::addPassenger( BWEntity * pPassenger,
	EntityID vehicleID )
{
	// To avoid early-deletion accidents...
	BWEntityPtr pPassengerHolder( pPassenger );

	// Is it already consistent?
	if (vehicleID == NULL_ENTITY_ID)
	{
		entities_.onEntityConsistent( pPassenger );
		return;
	}

	BWEntity * pVehicle = entities_.find( vehicleID );

	if (pVehicle != NULL && pVehicle->isInWorld())
	{
		entities_.onEntityConsistent( pPassenger );
		return;
	}

	EntityID passengerID = pPassenger->entityID();

	MF_ASSERT( passengersIndex_.find( passengerID ) ==
		passengersIndex_.end() );

	passengers_.insert( std::make_pair( vehicleID, pPassengerHolder ) );
	passengersIndex_[ passengerID ] = std::make_pair( pPassenger, vehicleID );
}


/**
 *	This method changes the vehicle a passenger is waiting for.
 *	When the passenger's vehicle is available, the
 *	BWEntities::onEntityConsistent callback will be triggered.
 *	This may happen immediately.
 *
 *	@param passengerID The ID of the passenger entity to update.
 *	@param newVehicleID The EntityID of the vehicle the passenger is now upon
 *
 *	@return true if the entity was in our collection.
 */
bool DeferredPassengers::changeVehicleForPassenger( EntityID passengerID,
		EntityID newVehicleID )
{
	PassengersAndVehiclesByEntityID::iterator iIndex =
		passengersIndex_.find( passengerID );

	if( iIndex == passengersIndex_.end() )
	{
		return false;
	}

	// To avoid early-deletion accidents...
	BWEntityPtr pPassenger = iIndex->second.first;

	// Is it already consistent?
	if ((newVehicleID == NULL_ENTITY_ID) ||
			(entities_.find( newVehicleID ) != NULL))
	{
		this->erasePassenger( passengerID );
		entities_.onEntityConsistent( pPassenger.get() );
		return true;
	}

	EntityID oldVehicleID = iIndex->second.second;

	IF_NOT_MF_ASSERT_DEV( oldVehicleID != newVehicleID )
	{
		// Nothing to do here
		return true;
	}

	iIndex->second.second = newVehicleID;

	std::pair< PassengersByVehicle::iterator, PassengersByVehicle::iterator >
		iPassengers = passengers_.equal_range( oldVehicleID );

	PassengersByVehicle::iterator iPassenger;

	for (iPassenger = iPassengers.first; iPassenger != iPassengers.second;
		++iPassenger )
	{
		if ( iPassenger->second.get() == pPassenger )
		{
			break;
		}
	}

	// Ensure we found the passenger in question.
	MF_ASSERT( iPassenger != iPassengers.second );

	passengers_.erase( iPassenger );
	passengers_.insert( std::make_pair( newVehicleID, pPassenger ) );

	return true;
}


/**
 *	This method removes the given passenger from the list of passengers
 *	waiting. It does not trigger the BWEntities::onEntityConsistent callback
 *	as it is used when a passenger is no longer interesting to us.
 *
 *	@param passengerID The EntityID of the passenger to forget.
 *	@return true if the entity was in our collection.
 */
bool DeferredPassengers::erasePassenger( EntityID passengerID )
{
	PassengersAndVehiclesByEntityID::iterator iIndex =
		passengersIndex_.find( passengerID );

	if (iIndex == passengersIndex_.end())
	{
		return false;
	}

	BWEntity * pPassenger = iIndex->second.first;
	EntityID vehicleID = iIndex->second.second;

	passengersIndex_.erase( passengerID );

	std::pair< PassengersByVehicle::iterator, PassengersByVehicle::iterator >
		iPassengers = passengers_.equal_range( vehicleID );

	PassengersByVehicle::iterator iPassenger;

	for (iPassenger = iPassengers.first; iPassenger != iPassengers.second;
		++iPassenger )
	{
		if ( iPassenger->second.get() == pPassenger )
		{
			break;
		}
	}

	// Ensure we found the passenger in question.
	MF_ASSERT( iPassenger != iPassengers.second );

	passengers_.erase( iPassenger );

	return true;
}


/**
 *	This method forgets all pending passengers.
 *
 *	@param keepLocalEntities	If true, then locally created entities are not
 *								cleared.
 */
void DeferredPassengers::clear(
		bool keepLocalEntities /* = false */ )
{
	PassengersByVehicle::iterator iPassengers = passengers_.begin();
	while (iPassengers != passengers_.end())
	{
		BWEntityPtr pEntity = iPassengers->second;

		PassengersByVehicle::iterator iDelete = iPassengers;
		++iPassengers;
		
		if (!keepLocalEntities || !pEntity->isLocalEntity())
		{
			passengersIndex_.erase( pEntity->entityID() );
			passengers_.erase( iDelete );
			pEntity->destroyNonPlayer();
		}
	}
}


// Section: DeferredPassengers BWEntitiesListener interface

/*
 *	Override of BWEntitiesListener::onEntityEnter
 */
void DeferredPassengers::onEntityEnter( BWEntity * pEntity )
{
	EntityID vehicleID = pEntity->entityID();

	// Ensure we didn't just see an Entity enter the world which we believe
	// is still pending.
	MF_ASSERT( passengersIndex_.find( vehicleID ) ==
		passengersIndex_.end() );

	std::pair< PassengersByVehicle::iterator,
				PassengersByVehicle::iterator >
		iPassengers = passengers_.equal_range( vehicleID );

	if (iPassengers.first == iPassengers.second)
	{
		// No passengers waiting for this entity.
		return;
	}

	BW::vector< BWEntityPtr > vPassengersReady;
	vPassengersReady.reserve( passengers_.count( vehicleID ) );

	for (PassengersByVehicle::const_iterator iPassenger = iPassengers.first;
		iPassenger != iPassengers.second; ++iPassenger)
	{
		vPassengersReady.push_back( iPassenger->second );
		passengersIndex_.erase( iPassenger->second->entityID() );
	}

	passengers_.erase( iPassengers.first, iPassengers.second );

	for (BW::vector< BWEntityPtr >::const_iterator iEntity =
		vPassengersReady.begin(); iEntity != vPassengersReady.end();
		++iEntity )
	{
		entities_.onEntityConsistent( iEntity->get() );
	}
}


/*
 *	Override of BWEntitiesListener::onEntityLeave
 */
void DeferredPassengers::onEntityLeave( BWEntity * pEntity )
{
	// Do nothing. But check that we didn't think this entity
	// was already out-of-the-world
	MF_ASSERT( passengersIndex_.find( pEntity->entityID() ) ==
		passengersIndex_.end() );
}

BW_END_NAMESPACE

// deferred_passengers.cpp
