#include "pch.hpp"

#include "entity_entry_blocker.hpp"

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: EntityEntryBlocker
// ----------------------------------------------------------------------------


/**
 *	Constructor.
 */
EntityEntryBlocker::EntityEntryBlocker() :
	pImpl_( NULL )
{
}


/**
 *	This method provides a blocking condition for the caller to
 *	hold until blocking is complete.
 *
 *	@return An EntityEntryBlockingCondition blocking this EntityEntryBlocker
 */
EntityEntryBlockingCondition EntityEntryBlocker::blockEntry() const
{
	if (pImpl_ == NULL)
	{
		pImpl_ = new EntityEntryBlockingConditionImpl();
	}

	return EntityEntryBlockingCondition( pImpl_ );
}


/**
 *	This method returns the blocking condition impl if we are currently
 *	blocked, or NULL otherwise.
 *
 *	Note that the returned SmartPointer will keep the blocking condition
 *	blocked as long as it is around, so do not store it!
 *
 *	@return An EntityEntryBlockingConditionImplPtr if we are currently blocked,
 *			or NULL if our blocks have cleared.
 */
EntityEntryBlockingConditionImplPtr EntityEntryBlocker::getImpl()
{
	// _We_ hold a reference for our lifetime.
	if (pImpl_ != NULL && pImpl_->refCount() > 1)
	{
		return pImpl_;
	}

	return NULL;
}

BW_END_NAMESPACE

// entity_entry_blocker.cpp
