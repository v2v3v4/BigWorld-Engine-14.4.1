#ifndef PASSENGER_EXTRA_HPP
#define PASSENGER_EXTRA_HPP

#include "entity_extra.hpp"


BW_BEGIN_NAMESPACE

class PassengerController;


/// This macro defines the attribute function for a method

/**
 *	This class manages the functionality related to an entity being a passenger.
 */
class PassengerExtra : public EntityExtra
{
	Py_EntityExtraHeader( PassengerExtra )

public:
	PassengerExtra( Entity & e );
	~PassengerExtra();

	void setController( PassengerController * pController );

	void onVehicleGone();

	static const Instance<PassengerExtra> instance;

	bool boardVehicle( EntityID vehicleID, bool shouldUpdateClient = true );
	bool alightVehicle( bool shouldUpdateClient = true );

	static bool isInLimbo( Entity & e );

	virtual void relocated();

protected:
	PY_AUTO_METHOD_DECLARE( RETOK, boardVehicle, ARG( EntityID, END ) );
	PY_AUTO_METHOD_DECLARE( RETOK, alightVehicle, END );

private:
	PassengerExtra( const PassengerExtra& );
	PassengerExtra& operator=( const PassengerExtra& );

	PassengerController * pController_;
};

BW_END_NAMESPACE


#endif // PASSENGER_EXTRA_HPP
