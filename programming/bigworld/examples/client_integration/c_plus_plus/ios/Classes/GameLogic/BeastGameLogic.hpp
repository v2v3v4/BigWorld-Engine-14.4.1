#ifndef BeastGameLogic_HPP
#define BeastGameLogic_HPP

#include "EntityExtensions/BeastExtension.hpp"


/**
 *	This class implements the app-specific logic for the Beast entity.
 */
class BeastGameLogic : public BeastExtension
{
public:
	BeastGameLogic( const Beast * pEntity ) :
			BeastExtension( pEntity )
	{}

	// Client methods

	virtual void initiateRage( );



	// Client property callbacks (optional)


	virtual void set_lookAtTarget( const BW::int32 & oldValue );

	virtual void set_effectRadius( const float & oldValue );

	virtual void set_dustTriggerRadius( const float & oldValue );
};

#endif // BeastGameLogic_HPP
