#include "pch.hpp"

#include "entity_entry_blocking_condition_impl.hpp"

#include "entity_entry_blocking_condition_handler.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: EntityEntryBlockingConditionImpl
// ----------------------------------------------------------------------------


/**
 *	Destructor
 */
EntityEntryBlockingConditionImpl::~EntityEntryBlockingConditionImpl()
{
	if (pHandler_ == NULL)
	{
		return;
	}

	pHandler_->onBlockingConditionCleared();
}

/**
 *	This method sets the handler for this EntityEntryBlockingConditionImpl
 *
 *	@param pHandler The handler for this EntityEntryBlockingConditionImpl
 */
void EntityEntryBlockingConditionImpl::setHandler(
	EntityEntryBlockingConditionHandler * pHandler )
{
	// Our previous Handler must have been aborted before we can set a new
	// Handler
	MF_ASSERT( pHandler_ == NULL || pHandler == NULL );
	pHandler_ = pHandler;
}

BW_END_NAMESPACE

// entity_entry_blocking_condition_impl.cpp
