#ifndef MERCHANT_ENTITY_VIEW_EXTENSION_HPP
#define MERCHANT_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/MerchantExtension.hpp"
#include "GenericEntityViewExtension.hpp"


class MerchantEntityViewExtension :
		public GenericEntityViewExtension< MerchantExtension >
{
public:
	MerchantEntityViewExtension( const Merchant * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~MerchantEntityViewExtension() {}
	
	
	// Override from EntityViewDataProvider
	virtual BW::string spriteName() const;
};


#endif // MERCHANT_ENTITY_VIEW_EXTENSION_HPP