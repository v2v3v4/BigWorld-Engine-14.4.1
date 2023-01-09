#include "history_event.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "history_event.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: HistoryEvent
// -----------------------------------------------------------------------------

/**
 *	This constructor for HistoryEvent. The level parameter should use the detail
 *	entry for events that are state changes and the priority entry for messages.
 *	Messages use a priority to decide whether or not to be sent while state
 *	changes use a Detail Level.
 *
 *	@param msgID		The msgID of the event.
 *	@param number	The event number of the event.
 *	@param msg		The data describing the event.
 *	@param msgLen	The length of the data (in bytes).
 *	@param level	The level associated with the event.
 *  @param pName	Human readable name for the HistoryEvent.
 */
HistoryEvent::HistoryEvent( Mercury::MessageID msgID,
		EventNumber number,
		void * msg,
		int msgLen,
		Level level,
		const MemberDescription * pDescription,
		int latestEventIndex,
		bool isReliable,
		int16 msgStreamSize ) :
	level_( level ),
	number_( number ),
	msgID_( msgID ),
	isReliable_( isReliable ),
	msgStreamSize_( msgStreamSize ),
	msg_( (char*)msg ),
	msgLen_( msgLen ),
	pDescription_( pDescription ),
	latestEventIndex_( latestEventIndex )
{
	MF_ASSERT( msgStreamSize_ < 0 || msgStreamSize_ == msgLen_ );
}


/**
 *	This method is used to reuse an existing HistoryEvent.
 */
void HistoryEvent::recreate( Mercury::MessageID msgID, EventNumber number,
	void * msg, int msgLen, Level level,
	const MemberDescription * pDescription, int16 msgStreamSize )
{
	MF_ASSERT( msgID == msgID_ );
	MF_ASSERT( (pDescription_ == NULL) || (pDescription_ == pDescription) );

	MF_ASSERT( number_ < number );

	MF_ASSERT( pDescription->shouldSendLatestOnly() );
	MF_ASSERT( pDescription->latestEventIndex() == latestEventIndex_ );

	pDescription_ = pDescription;
	number_ = number;
	msgStreamSize_ = msgStreamSize;

	delete [] msg_;
	msg_ = (char *)msg;
	msgLen_ = msgLen;
	MF_ASSERT( msgStreamSize_ < 0 || msgStreamSize_ == msgLen_ );
}


/**
 *	This method is used to reuse an existing HistoryEvent.
 */
void HistoryEvent::recreate( Mercury::MessageID msgID, EventNumber number,
	void * msg, int msgLen, Level level,
	int latestEventIndex, int16 msgStreamSize )
{
	MF_ASSERT( msgID == msgID_ );

	MF_ASSERT( number_ < number );

	MF_ASSERT( latestEventIndex == latestEventIndex_ );

	number_ = number;
	msgStreamSize_ = msgStreamSize;

	delete [] msg_;
	msg_ = (char *)msg;
	msgLen_ = msgLen;
	MF_ASSERT( msgStreamSize_ < 0 || msgStreamSize_ == msgLen_ );
}


/**
 *	This method adds this event to the input bundle.
 *
 *	@param bundle	The bundle to add the event to.
 */
void HistoryEvent::addToBundle( Mercury::Bundle & bundle )
{
	if (pDescription_)
	{
		pDescription_->stats().countSentToOtherClients( msgLen_ );
	}

	// Script method calls and property changes
	bundle.startMessage(
			isReliable_ ?
				BaseAppIntInterface::sendMessageToClient :
				BaseAppIntInterface::sendMessageToClientUnreliable );
	bundle << msgID_;
	bundle << (int16) msgStreamSize_;

	bundle.addBlob( msg_, msgLen_ );
}


// -----------------------------------------------------------------------------
// Section: EventHistory
// -----------------------------------------------------------------------------

/**
 *	Destructor
 */
EventHistory::~EventHistory()
{
	// have to call our clear function so all the pointers in our
	// container get properly deleted and not just discarded
	this->clear();
}


/**
 *	This method adds a history event that has been streamed over the network.
 *	This is done from the real entity to its ghost entities.
 */
EventNumber EventHistory::addFromStream( BinaryIStream & stream )
{
	EventNumber eventNumber;
	uint8 type;
	HistoryEvent::Level level;
	int latestEventIndex;
	bool isReliable;
	int16 msgStreamSize;
	void * historyData;
	int length;

	HistoryEvent::extractFromStream( stream, eventNumber, type, level,
		latestEventIndex, isReliable, msgStreamSize, historyData, length );

	HistoryEvent * pNewEvent = NULL;

	if (latestEventIndex != -1)
	{
		Container::iterator latestEventIter =
			latestEventPointers_[ latestEventIndex ];

		if (latestEventIter != container_.end())
		{
			latestEventPointers_[ latestEventIndex ] = container_.end();

			pNewEvent = *latestEventIter;
			pNewEvent->recreate( type, eventNumber, historyData, length,
					level, latestEventIndex, msgStreamSize );

			container_.erase( latestEventIter );
		}
	}

	if (!pNewEvent)
	{
		pNewEvent = new HistoryEvent( type, eventNumber, historyData, length,
			level, NULL, latestEventIndex, isReliable, msgStreamSize );
	}

	this->add( pNewEvent );

	return pNewEvent->number();
}


/**
 *	This method adds a new event to the event history.
 */
HistoryEvent * EventHistory::add( EventNumber eventNumber,
	uint8 type, MemoryOStream & stream,
	const MemberDescription & description,
	HistoryEvent::Level level, int16 msgStreamSize )
{
	HistoryEvent * pNewEvent = NULL;

	if (description.shouldSendLatestOnly())
	{
		int latestEventIndex = description.latestEventIndex();

		MF_ASSERT( latestEventIndex != -1 );

		Container::iterator latestEventIter =
			latestEventPointers_[ latestEventIndex ];

		if (latestEventIter != container_.end())
		{
			latestEventPointers_[ latestEventIndex ] = container_.end();

			pNewEvent = *latestEventIter;
			pNewEvent->recreate( type, eventNumber,
					stream.data(), stream.size(),
					level, &description, msgStreamSize );

			container_.erase( latestEventIter );
		}
	}
	else
	{
		MF_ASSERT( description.latestEventIndex() == -1 );
	}

	if (!pNewEvent)
	{
		pNewEvent = new HistoryEvent( type, eventNumber,
			stream.data(), stream.size(), level, &description,
			description.latestEventIndex(),
			description.isReliable(), msgStreamSize );
	}

	stream.shouldDelete( false );

#if ENABLE_WATCHERS
	description.stats().countAddedToHistoryQueue( stream.size() );
#endif

	this->add( pNewEvent );

	return pNewEvent;
}


/**
 *	This method is used to trim the EventHistory. It deletes all of the events
 *	that were added before the last trim call (leaving only those events added
 *	since the last trim call).
 *
 *	This method should not be called more frequently than it takes any
 *	RealEntityWithWitnesses to go through all histories.
 */
void EventHistory::trim()
{
	// TODO: This is a bit dodgy because we do not know how often to go through
	// this.

	while (!container_.empty() &&
			container_.front()->number() <= trimToEvent_)
	{
		if (container_.front()->isReliable())
		{
			lastTrimmedEventNumber_ = container_.front()->number();
		}

		this->deleteEvent( container_.front() );
		container_.pop_front();
	}

	trimToEvent_ = container_.empty() ? 0 : container_.back()->number();
}


/**
 *	This method clears this event history.
 */
void EventHistory::clear()
{
	Container::iterator iter = container_.begin();

	while (iter != container_.end())
	{
		this->deleteEvent( *iter );

		++iter;
	}

	container_.clear();

	trimToEvent_ = 0;
	lastTrimmedEventNumber_ = 0;
}


/**
 *	This method deletes a HistoryEvent. It ensures that any other state is
 *	cleaned up correctly.
 */
void EventHistory::deleteEvent( HistoryEvent * pEvent )
{
	if (pEvent->latestEventIndex() != -1)
	{
		MF_ASSERT( pEvent ==
				*latestEventPointers_[ pEvent->latestEventIndex() ] );

		latestEventPointers_[ pEvent->latestEventIndex() ] = container_.end();
	}

	delete pEvent;
}

BW_END_NAMESPACE

// history_event.cpp
