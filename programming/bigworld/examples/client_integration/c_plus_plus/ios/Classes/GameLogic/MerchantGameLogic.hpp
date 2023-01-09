#ifndef MerchantGameLogic_HPP
#define MerchantGameLogic_HPP

#include "EntityExtensions/MerchantExtension.hpp"


/**
 *	This class implements the app-specific logic for the Merchant entity.
 */
class MerchantGameLogic :
		public MerchantExtension
{
public:
	MerchantGameLogic( const Merchant * pEntity ) :
			MerchantExtension( pEntity )
	{}

	// Client methods

	virtual void chat(
			const BW::string & message );



	// Client property callbacks (optional)


	virtual void set_modeTarget( const BW::int32 & oldValue );

	virtual void set_cancelTradeRadius( const float & oldValue );
};

#endif // MerchantGameLogic_HPP
