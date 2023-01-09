#ifndef SeatGameLogic_HPP
#define SeatGameLogic_HPP

#include "EntityExtensions/SeatExtension.hpp"


/**
 *	This class implements the app-specific logic for the Seat entity.
 */
class SeatGameLogic : public SeatExtension
{
public:
	SeatGameLogic( const Seat * pEntity ) :
			SeatExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_seatType( const BW::int8 & oldValue );

	virtual void set_ownerID( const BW::int32 & oldValue );
};

#endif // SeatGameLogic_HPP
