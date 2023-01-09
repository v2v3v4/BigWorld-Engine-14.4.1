#include "pch.hpp"

#include "entity_extension_factory_manager.hpp"

#include "entity_extension_factory_base.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This method adds an extension factory to this collection.
 */
void EntityExtensionFactoryManager::add( EntityExtensionFactoryBase * pFactory )
{
	pFactory->setSlot( static_cast<int>(factories_.size()) );
	factories_.push_back( pFactory );
}

BW_END_NAMESPACE

// entity_extension_factory_manager.cpp
