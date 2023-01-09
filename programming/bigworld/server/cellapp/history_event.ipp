// history_event.ipp

#ifdef CODE_INLINE
#define INLINE	inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: HistoryEvent
// -----------------------------------------------------------------------------

/**
 *	Destructor.
 */
INLINE
HistoryEvent::~HistoryEvent()
{
	delete [] msg_;
}


/**
 *	This method returns the number associated with this event.
 */
INLINE
EventNumber HistoryEvent::number() const
{
	return number_;
}


/**
 *	This method adds this event to a stream.
 */
INLINE
void HistoryEvent::addToStream( BinaryOStream & stream )
{
	stream << number_ << msgID_ << level_ << latestEventIndex_ << isReliable_ << msgStreamSize_;
	stream.addBlob( msg_, msgLen_ );
}


/**
 *	This method creates an event from a stream.
 */
INLINE
void HistoryEvent::extractFromStream( BinaryIStream & stream,
	EventNumber & eventNumber, uint8 & type, HistoryEvent::Level & level,
	int & latestEventIndex, bool & isReliable, int16 & msgStreamSize,
	void * & msg, int & msgLen )
{
	stream >> eventNumber >> type >> level >> latestEventIndex >> isReliable >> msgStreamSize;

	msgLen = stream.remainingLength();
	msg = new char[ msgLen ];
	memcpy( msg, stream.retrieve( msgLen ), msgLen );
}


/**
 *	This method decides whether to send this event to a client based on the
 *	input priority threshold and detail level.
 */
INLINE bool HistoryEvent::shouldSend( float threshold, int detailLevel ) const
{
	return level_.shouldSend( threshold, detailLevel );
}


// -----------------------------------------------------------------------------
// Section: EventHistory
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
INLINE
EventHistory::EventHistory( unsigned int numLatestChangeOnlyMembers ) :
	container_(),
	trimToEvent_( 0 ),
	lastTrimmedEventNumber_( 0 ),
	latestEventPointers_( numLatestChangeOnlyMembers, container_.end() )
{
}


/**
 *	This method adds an event to the event history.
 *
 *	@param pEvent	The event to add.
 */
INLINE
void EventHistory::add( HistoryEvent * pEvent )
{
	container_.push_back( pEvent );

	int index = pEvent->latestEventIndex();

	if (index != -1)
	{
		Container::iterator iter = container_.end();
		--iter;

		MF_ASSERT( latestEventPointers_[ index ] == container_.end() );

		latestEventPointers_[ index ] = iter;
	}
}

// history_event.ipp
