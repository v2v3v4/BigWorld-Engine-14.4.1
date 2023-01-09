#include "pch.hpp"

#include "pending_entities_blocking_condition_handler.hpp"

#include "pending_entities.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: PendingEntitiesBlockingConditionHandler
// ----------------------------------------------------------------------------

/**
 *	@see EntityEntryBlockingConditionHandler::onConditionCleared
 */
void PendingEntitiesBlockingConditionHandler::onConditionCleared()
{
	// Avoid deletion during this routine
	PendingEntitiesBlockingConditionHandlerPtr pHolder( this );

	pendingEntities_.releaseEntity( entityID_ );

	entityID_ = NULL_ENTITY_ID;
}

BW_END_NAMESPACE

// pending_entities_blocking_condition_handler.cpp
