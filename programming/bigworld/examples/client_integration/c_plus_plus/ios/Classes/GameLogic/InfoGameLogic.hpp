#ifndef InfoGameLogic_HPP
#define InfoGameLogic_HPP

#include "EntityExtensions/InfoExtension.hpp"


/**
 *	This class implements the app-specific logic for the Info entity.
 */
class InfoGameLogic : public InfoExtension
{
public:
	InfoGameLogic( const Info * pEntity ) :
			InfoExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_text( const BW::string & oldValue );

	virtual void set_showWhenNear( const BW::uint8 & oldValue );

	virtual void set_modelType( const BW::int8 & oldValue );
};

#endif // InfoGameLogic_HPP
