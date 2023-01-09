#ifndef FlockGameLogic_HPP
#define FlockGameLogic_HPP

#include "EntityExtensions/FlockExtension.hpp"


/**
 *	This class implements the app-specific logic for the Flock entity.
 */
class FlockGameLogic : public FlockExtension
{
public:
	FlockGameLogic( const Flock * pEntity ) :
			FlockExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_state( const BW::int8 & oldValue );
};

#endif // FlockGameLogic_HPP
