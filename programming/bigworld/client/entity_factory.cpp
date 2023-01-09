#include "pch.hpp"

#include "entity_factory.hpp"

#include "entity.hpp"
#include "entity_type.hpp"

#include "cstdmf/debug.hpp"

#include "connection_model/bw_entity.hpp"
#include "connection_model/bw_entity_factory.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 );

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EntityFactory
// -----------------------------------------------------------------------------

/*
 *	This method overrides BWEntityFactory::doCreate
 */
BWEntity * EntityFactory::doCreate( EntityTypeID entityTypeID,
	BWConnection * pConnection )
{
	BW_GUARD;

	EntityType * pEntityType = EntityType::find( entityTypeID );
	if (!pEntityType)
	{
		ERROR_MSG( "EntityFactory::doCreate: Bad type %d\n", entityTypeID );
		return NULL;
	}

	return new Entity( *pEntityType, pConnection );
}


BW_END_NAMESPACE

// entity_factory.cpp
