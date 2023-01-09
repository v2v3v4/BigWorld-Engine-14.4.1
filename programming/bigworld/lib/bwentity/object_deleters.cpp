#include "pch.hpp"

#include "object_deleters.hpp"

#include "bwentity_types.hpp"
#include "entity_description_map_factory.hpp"

BWENTITY_BEGIN_NAMESPACE

void BWENTITY_API destroyObject( EntityDescriptionMap * ptr )
{
	EntityDescriptionMapFactory::destroy( ptr );
}

BWENTITY_END_NAMESPACE



