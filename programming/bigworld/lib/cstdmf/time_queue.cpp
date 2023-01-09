#include "pch.hpp"

#include "time_queue.hpp"

BW_BEGIN_NAMESPACE


/**
 * Constructor.
 */
TimerHandle::TimerHandle( TimeQueueNode * pNode ) :
	pNode_( pNode )
{
	if (pNode_ != NULL)
	{
		pNode_->incRef();
	}
}


/**
 * Constructor.
 */
TimerHandle::TimerHandle( const TimerHandle & other ) :
	pNode_( other.pNode() )
{
	if (pNode_ != NULL)
	{
		pNode_->incRef();
	}
}


/**
 * Destructor.
 */
TimerHandle::~TimerHandle()
{
	if (pNode_ != NULL)
	{
		pNode_->decRef();
		pNode_ = NULL;
	}
}


/**
 * Assignment operator.
 */
TimerHandle & TimerHandle::operator=( const TimerHandle & other )
{
	if (pNode_ != other.pNode())
	{
		if (pNode_ != NULL)
		{
			pNode_->decRef();
		}

		pNode_ = other.pNode();

		if (pNode_ != NULL)
		{
			pNode_->incRef();
		}
	}
	return *this;
}


/**
 *	This method cancels the timer associated with this handle. It is safe to
 *	call when the TimerHandle has not been set.
 */
void TimerHandle::cancel()
{
	if (pNode_ != NULL)
	{
		TimeQueueNode* pNode = pNode_;

		pNode_ = NULL;
		pNode->cancel();
		pNode->decRef();
	}
}

BW_END_NAMESPACE

// time_queue.cpp
