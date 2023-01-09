#ifndef AlertBeastTriggerGameLogic_HPP
#define AlertBeastTriggerGameLogic_HPP

#include "EntityExtensions/AlertBeastTriggerExtension.hpp"


/**
 *	This class implements the app-specific logic for the AlertBeastTrigger entity.
 */
class AlertBeastTriggerGameLogic :
		public AlertBeastTriggerExtension
{
public:
	AlertBeastTriggerGameLogic( const AlertBeastTrigger * pEntity ) :
			AlertBeastTriggerExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_trapRadius( const float & oldValue );

	virtual void set_findBeastRadius( const float & oldValue );
};

#endif // AlertBeastTriggerGameLogic_HPP
