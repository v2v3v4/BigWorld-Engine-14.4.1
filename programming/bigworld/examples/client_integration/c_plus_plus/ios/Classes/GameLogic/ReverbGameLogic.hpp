#ifndef ReverbGameLogic_HPP
#define ReverbGameLogic_HPP

#include "EntityExtensions/ReverbExtension.hpp"


/**
 *	This class implements the app-specific logic for the Reverb entity.
 */
class ReverbGameLogic : public ReverbExtension
{
public:
	ReverbGameLogic( const Reverb * pEntity ) :
			ReverbExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_presetName( const BW::string & oldValue );

	virtual void set_innerRadius( const float & oldValue );

	virtual void set_outerRadius( const float & oldValue );
};

#endif // ReverbGameLogic_HPP
