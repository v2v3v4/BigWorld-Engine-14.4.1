#ifndef TriggeredDustGameLogic_HPP
#define TriggeredDustGameLogic_HPP

#include "EntityExtensions/TriggeredDustExtension.hpp"


/**
 *	This class implements the app-specific logic for the TriggeredDust entity.
 */
class TriggeredDustGameLogic :
		public TriggeredDustExtension
{
public:
	TriggeredDustGameLogic( const TriggeredDust * pEntity ) :
			TriggeredDustExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_effectName( const BW::string & oldValue );
};

#endif // TriggeredDustGameLogic_HPP
