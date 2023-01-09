#include "pch.hpp"

#include "avatar_filter_callback.hpp"

#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
AvatarFilterCallback::AvatarFilterCallback( double targetTime ) :
	targetTime_( targetTime ),
	callbackListNode_()
{
	BW_GUARD;

	callbackListNode_.setAsRoot();
}


/**
 *	Destructor
 */
AvatarFilterCallback::~AvatarFilterCallback()
{
	BW_GUARD;

	// Assert as a "Don't rely on auto-remove" warning.
	MF_ASSERT_DEV( callbackListNode_.getNext() == &callbackListNode_ );
	MF_ASSERT_DEV( callbackListNode_.getPrev() == &callbackListNode_ );

	this->removeFromList();
}


/**
 *	This method inserts us into the given list, sorted by targetTime()
 */
void AvatarFilterCallback::insertIntoList( ListNode * pListHead )
{
	BW_GUARD;

	// Work backwards, assuming new callbacks will be newer
	ListNode * pPrevNode = pListHead->getPrev();

	while (pPrevNode != pListHead)
	{
		AvatarFilterCallback * pPrevCallback =
			AvatarFilterCallback::getFromListNode( pPrevNode );

		if (pPrevCallback->targetTime() < this->targetTime())
		{
			callbackListNode_.addThisAfter( pPrevNode );
			return;
		}

		pPrevNode = pPrevNode->getPrev();
	}

	// Turns out we're the newest element
	callbackListNode_.addThisAfter( pListHead );
}


/**
 *	This method triggers the subclass callback and removes us from the list
 *	we are currently part of.
 */
bool AvatarFilterCallback::triggerCallback( double outputTime )
{
	BW_GUARD;

	MF_ASSERT( outputTime >= targetTime_ );

	return this->onCallback( outputTime - targetTime_ );
}


/**
 *	This method removes us from the list we are currently part of.
 */
void AvatarFilterCallback::removeFromList()
{
	BW_GUARD;

	callbackListNode_.remove();
	callbackListNode_.setAsRoot();
}


BW_END_NAMESPACE

// avatar_filter_callback.cpp
