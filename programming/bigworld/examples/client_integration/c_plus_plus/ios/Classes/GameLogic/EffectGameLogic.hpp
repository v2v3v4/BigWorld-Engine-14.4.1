#ifndef EffectGameLogic_HPP
#define EffectGameLogic_HPP

#include "EntityExtensions/EffectExtension.hpp"


/**
 *	This class implements the app-specific logic for the Effect entity.
 */
class EffectGameLogic : public EffectExtension
{
public:
	EffectGameLogic( const Effect * pEntity ) :
			EffectExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_effectType( const BW::int8 & oldValue );
};

#endif // EffectGameLogic_HPP
