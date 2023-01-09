#include "pch.hpp"

#include "entity_extension_factory_base.hpp"

#include "bw_entity.hpp"

BW_BEGIN_NAMESPACE

EntityExtension *
	EntityExtensionFactoryBase::getFrom( const BWEntity & entity ) const
{
	return entity.extensionInSlot( slot_ );
}

BW_END_NAMESPACE

// entity_extension_factory_base.cpp
