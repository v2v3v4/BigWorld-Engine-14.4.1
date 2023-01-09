#ifndef SoundEmitterGameLogic_HPP
#define SoundEmitterGameLogic_HPP

#include "EntityExtensions/SoundEmitterExtension.hpp"


/**
 *	This class implements the app-specific logic for the SoundEmitter entity.
 */
class SoundEmitterGameLogic :
		public SoundEmitterExtension
{
public:
	SoundEmitterGameLogic( const SoundEmitter * pEntity ) :
			SoundEmitterExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_eventName( const BW::string & oldValue );

	virtual void set_initialDelay( const float & oldValue );

	virtual void set_replayDelay( const float & oldValue );

	virtual void set_delayVariation( const float & oldValue );
};

#endif // SoundEmitterGameLogic_HPP
