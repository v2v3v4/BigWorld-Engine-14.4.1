#ifndef DoorGameLogic_HPP
#define DoorGameLogic_HPP

#include "EntityExtensions/DoorExtension.hpp"


/**
 *	This class implements the app-specific logic for the Door entity.
 */
class DoorGameLogic : public DoorExtension
{
public:
	DoorGameLogic( const Door * pEntity ) :
			DoorExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_doorType( const BW::int8 & oldValue );

	virtual void set_state( const BW::int8 & oldValue );
};

#endif // DoorGameLogic_HPP
