#include "buffered_ghost_message_queue.hpp"

#include "buffered_ghost_message.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BufferedGhostMessageQueue
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
BufferedGhostMessageQueue::BufferedGhostMessageQueue() :
	messages_(),
	pDelayedMessages_( NULL )
{
}


/**
 *  Destructor.
 */
BufferedGhostMessageQueue::~BufferedGhostMessageQueue()
{
	for (Messages::iterator iter = messages_.begin();
		 iter != messages_.end(); ++iter)
	{
		delete *iter;
	}
}


/**
 *	This method adds a message to this queue.
 */
void BufferedGhostMessageQueue::add( BufferedGhostMessage * pMessage )
{
	if (this->isDelaying())
	{
		pDelayedMessages_->push_back( pMessage );
	}
	else
	{
		messages_.push_back( pMessage );
		MF_ASSERT( this->isFrontSubsequenceStart() );
	}
}


/**
 *	This method removes and returns the first message in this queue.
 */
BufferedGhostMessage * BufferedGhostMessageQueue::popFront()
{
	// Unlink the front message from the queue and return it. The caller is
	// responsible for deleting the BufferedGhostMessage instance.
	BufferedGhostMessage * pMsg = messages_.front();
	messages_.pop_front();
	return pMsg;
}


/**
 *	This method returns whether the first message in this queue is valid.
 */
bool BufferedGhostMessageQueue::isFrontSubsequenceStart() const
{
	return !messages_.empty() && messages_.front()->isSubsequenceStart();
}


/**
 *	This method returns whether the first message in this queue is valid.
 */
bool BufferedGhostMessageQueue::isFrontSubsequenceEnd() const
{
	return !messages_.empty() && messages_.front()->isSubsequenceEnd();
}


/**
 *	This method returns whether the first message in this queue is createGhost.
 */
bool BufferedGhostMessageQueue::isFrontCreateGhost() const
{
	return !messages_.empty() && messages_.front()->isCreateGhostMessage();
}


/**
 *	This method plays and consumes the front message on the queue.
 *
 *	@return Whether this message caused the queue to become empty. Note that the
 *		queue may be empty but false still be returned. This is because playing
 *		messages may have caused to occur.
 */
bool BufferedGhostMessageQueue::playFront()
{
	if (this->isEmpty())
	{
		WARNING_MSG( "BufferedGhostMessageQueue::playFront: Queue is empty\n" );
		return false;
	}

	// The message is removed from the list. We are responsible for deleting it.
	BufferedGhostMessage * pMsg = this->popFront();

	// NOTE: This needs to be stored before play is called as that may modify
	// the queue.
	bool isNowEmpty = this->isEmpty() && (pDelayedMessages_ == NULL);

	// Make sure that pDelayedMessages_ is valid after play is called
	BufferedGhostMessageQueuePtr pThis = this;

	pMsg->play();
	delete pMsg;

	return isNowEmpty && (pDelayedMessages_ == NULL);
}


/**
 *	This method un-delays any delayed messages.
 */
void BufferedGhostMessageQueue::promoteDelayedSubsequences()
{
	if (pDelayedMessages_ != NULL)
	{
		WARNING_MSG( "BufferedGhostMessageQueue::promoteDelayedSubsequences:\n" );

		messages_.insert( messages_.begin(),
				pDelayedMessages_->begin(), pDelayedMessages_->end() );
		delete pDelayedMessages_;
		pDelayedMessages_ = NULL;
	}
}


/**
 *	This method plays the buffered messages up until the next sequence end.
 */
bool BufferedGhostMessageQueue::playSubsequence()
{
	BufferedGhostMessageQueuePtr pThis = this;

	this->promoteDelayedSubsequences();

	// Replay all the messages in the queue.  Note that if this queue ends with
	// a ghostSetNextReal message, this may cause a recursion back into this
	// method on this queue.

	bool hasFinishedSubsequence = false;

	while (!this->isEmpty() && !hasFinishedSubsequence)
	{
		hasFinishedSubsequence = this->isFrontSubsequenceEnd();

		if (this->playFront())
		{
			return true;
		}
	}

	return false;
}


/**
 *	This method delays the subsequence at the start of this queue.
 */
void BufferedGhostMessageQueue::delaySubsequence(
		BufferedGhostMessage * pFirstMessage )
{
	MF_ASSERT( pFirstMessage->isSubsequenceStart() );

	if (pDelayedMessages_ == NULL)
	{
		pDelayedMessages_ = new Messages();
	}

	pDelayedMessages_->push_back( pFirstMessage );

	// Delay this old (unexpected) subsequence.
	while (!messages_.empty() && this->isDelaying())
	{
		pDelayedMessages_->push_back( this->popFront() );
	}
}


/**
 *	This method returns true if this queue is in the middle of collecting a
 *	delayed subsequence.
 */
bool BufferedGhostMessageQueue::isDelaying() const
{
	return pDelayedMessages_ &&
		!pDelayedMessages_->back()->isSubsequenceEnd();
}

BW_END_NAMESPACE

// buffered_ghost_message_queue.cpp
