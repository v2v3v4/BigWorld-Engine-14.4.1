#ifndef DEFERRED_PASSENGERS_HPP
#define DEFERRED_PASSENGERS_HPP


#include "network/basictypes.hpp"

#include "cstdmf/bw_map.hpp"

#include "bw_entities_listener.hpp"
#include "bw_entity.hpp"

BW_BEGIN_NAMESPACE

class BWEntities;

/**
 *	This container class tracks passengers deferred because their vehicle
 *	is not yet in-world.
 */
class DeferredPassengers : public BWEntitiesListener
{
public: /// C++ Housekeeping
	DeferredPassengers( BWEntities & entities );
	~DeferredPassengers();

public: /// Passenger list management
	BWEntity * findPassenger( EntityID passengerID ) const;

	/**
	 *	This method indicates that there are no passengers pending in our list
	 */
	bool empty() const { return passengersIndex_.empty(); }

	/**
	 *	This method returns the number of passengers pending in our list
	 */
	size_t size() const { return passengersIndex_.size(); }

	void addPassenger( BWEntity * pPassenger, EntityID vehicleID );

	bool changeVehicleForPassenger( EntityID passengerID,
		EntityID newVehicleID );

	bool erasePassenger( EntityID passengerID );

	void clear( bool keepLocalEntities = false );

private: /// BWEntitiesListener interface
	void onEntityEnter( BWEntity * pEntity ) /* override */;
	void onEntityLeave( BWEntity * pEntity ) /* override */;
	void onEntitiesReset() /* override */ {}


private:
	BWEntities & entities_;

	typedef BW::multimap< EntityID, BWEntityPtr > PassengersByVehicle;
	PassengersByVehicle passengers_;

	typedef BW::map< EntityID, std::pair< BWEntity *, EntityID > >
		PassengersAndVehiclesByEntityID;
	PassengersAndVehiclesByEntityID passengersIndex_;
};

BW_END_NAMESPACE

#endif // DEFERRED_PASSENGERS_HPP

