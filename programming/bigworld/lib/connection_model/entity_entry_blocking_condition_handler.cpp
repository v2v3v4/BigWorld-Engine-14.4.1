#include "pch.hpp"

#include "entity_entry_blocking_condition_handler.hpp"

#include "entity_entry_blocking_condition_impl.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: EntityEntryBlockingConditionHandler
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
EntityEntryBlockingConditionHandler::EntityEntryBlockingConditionHandler(
	EntityEntryBlockingConditionImpl * pImpl ) :
	pImpl_( pImpl ),
	hasTriggered_( false )
{
	pImpl_->setHandler( this );
}


/**
 *	Destructor
 */
EntityEntryBlockingConditionHandler::~EntityEntryBlockingConditionHandler()
{
	// If we are destroyed before our condition is cleared, treat it as an
	// abort call.
	if (!hasTriggered_)
	{
		this->abort();
	}
}


/**
 *	This method is called to tell by our owner to notify us that it no longer
 *	cares about our condition being cleared.
 */
void EntityEntryBlockingConditionHandler::abort()
{
	hasTriggered_ = true;
	pImpl_->setHandler( NULL );
	pImpl_ = NULL;
	this->onAborted();
}


/**
 *	This method is called by our EntityEntryBlockingConditionImpl to notify us
 *	that its condition has been cleared.
 */
void EntityEntryBlockingConditionHandler::onBlockingConditionCleared()
{
	hasTriggered_ = true;
	pImpl_->setHandler( NULL );
	pImpl_ = NULL;
	this->onConditionCleared();
}

BW_END_NAMESPACE

// entity_entry_blocking_condition_impl.cpp
