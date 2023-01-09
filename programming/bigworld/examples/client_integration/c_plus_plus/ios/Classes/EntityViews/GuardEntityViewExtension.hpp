#ifndef GUARD_ENTITY_VIEW_EXTENSION_HPP
#define GUARD_ENTITY_VIEW_EXTENSION_HPP

#include "EntityExtensions/GuardExtension.hpp"
#include "GenericEntityViewExtension.hpp"


class GuardEntityViewExtension :
		public GenericEntityViewExtension< GuardExtension >
{
public:
	GuardEntityViewExtension( const Guard * pEntity ) :
			GenericEntityViewExtension( pEntity )
	{}
	
	virtual ~GuardEntityViewExtension() {}
	
	// Overrides from EntityViewDataProvider
	virtual BW::string spriteName() const;
	virtual BW::string popoverString() const;
};


#endif // GUARD_ENTITY_VIEW_EXTENSION_HPP