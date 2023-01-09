#ifndef ButtonGameLogic_HPP
#define ButtonGameLogic_HPP

#include "EntityExtensions/ButtonExtension.hpp"


/**
 *	This class implements the app-specific logic for the Button entity.
 */
class ButtonGameLogic : public ButtonExtension
{
public:
	ButtonGameLogic( const Button * pEntity ) :
		ButtonExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_modelType( const BW::uint8 & oldValue );

	virtual void set_useData( const BW::string & oldValue );
};

#endif // ButtonGameLogic_HPP
