#ifndef PASSENGER_CONTROLLER_HPP
#define PASSENGER_CONTROLLER_HPP

#include "controller.hpp"

#include "entity_population.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to place an entity on a vehicle.
 */
class PassengerController : public Controller, public EntityPopulation::Observer
{
	DECLARE_CONTROLLER_TYPE( PassengerController )
public:
	PassengerController( EntityID vehicleID = 0 );
	~PassengerController();

	virtual void writeGhostToStream( BinaryOStream & stream );
	virtual bool readGhostFromStream( BinaryIStream & stream );

	virtual void startGhost();
	virtual void stopGhost();

	void onVehicleGone();

	// Override from PopulationWatcher
	virtual void onEntityAdded( Entity & entity );

private:
	PassengerController( const PassengerController & );
	PassengerController& operator=( const PassengerController & );

	EntityID	vehicleID_;
	Position3D	initialLocalPosition_;
	Direction3D initialLocalDirection_;

	Position3D	initialGlobalPosition_;
	Direction3D initialGlobalDirection_;
};

BW_END_NAMESPACE

#endif // PASSENGER_CONTROLLER_HPP
