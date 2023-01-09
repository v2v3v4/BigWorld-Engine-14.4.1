#ifndef HISTORY_EVENT_HPP
#define HISTORY_EVENT_HPP

#include "entitydef/data_description.hpp"

#include "network/basictypes.hpp"
#include "network/interface_element.hpp"
#include "network/misc.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{
	class Bundle;
};


/**
 * 	This class is used to store an event in the event history.
 */
class HistoryEvent
{
public:
	/**
	 *	This class is a bit of a hack. For state change events, we want to use
	 *	a detail level, while for messages (that is, events with no state
	 *	change), we want to store a priority.
	 *
	 */
	class Level
	{
	public:
		Level() {}

		Level( int i ) : detail_( i ), isDetail_( true ) {}
		Level( float f ) : priority_( f ), isDetail_( false ) {};

		bool shouldSend( float threshold, int detailLevel ) const
		{
			return isDetail_ ?
				(detailLevel <= detail_) :
				(threshold < priority_);

		}

	private:
		union
		{
			float priority_;
			int detail_;
		};

		bool isDetail_;
	};

	void recreate( Mercury::MessageID msgID, EventNumber number,
		void * msg, int msgLen, Level level,
		const MemberDescription * pDescription, int16 msgStreamSize );

	void recreate( Mercury::MessageID msgID, EventNumber number,
		void * msg, int msgLen, Level level,
		int latestEventIndex, int16 msgStreamSize );

	EventNumber number() const;

	void addToBundle( Mercury::Bundle & bundle );

	INLINE void addToStream( BinaryOStream & stream );
	INLINE static void extractFromStream( BinaryIStream & stream,
		EventNumber & eventNumber, uint8 & type, HistoryEvent::Level & level,
		int & latestEventIndex, bool & isReliable, int16 & msgStreamSize,
		void * & msg, int & msgLen );

	INLINE bool shouldSend( float threshold, int detailLevel ) const;

	const BW::string * pName() const
			{ return pDescription_ ? &pDescription_->name() : NULL; }
	bool isReliable() const					{ return isReliable_; }
	int msgLen() const						{ return msgLen_; }
	int16 msgStreamSize() const				{ return msgStreamSize_; }

	int latestEventIndex() const	{ return latestEventIndex_; }

private:
	friend class EventHistory;

	HistoryEvent( Mercury::MessageID msgID, EventNumber number,
		void * msg, int msgLen, Level level,
		const MemberDescription * pDescription,
		int latestEventIndex,
		bool isReliable,
		int16 msgStreamSize );

	~HistoryEvent();

	HistoryEvent( const HistoryEvent & );
	HistoryEvent & operator=( const HistoryEvent & );

	Level level_;

	EventNumber number_;

	Mercury::MessageID msgID_;
	bool isReliable_;
	int16 msgStreamSize_;

	char * msg_;
	/// This member stores the length of the message (in bytes).
	int msgLen_;

	const MemberDescription * pDescription_;
	int latestEventIndex_;
};


/**
 *	This class is used to store a queue of history events.
 */
class EventHistory
{
private:
	typedef BW::list< HistoryEvent * > Container;

public:
	typedef Container::const_reverse_iterator	const_reverse_iterator;

	EventHistory( unsigned int numLatestChangeOnlyMembers );
	~EventHistory();

	EventNumber addFromStream( BinaryIStream & stream );

	HistoryEvent * add( EventNumber eventNumber,
		uint8 type, MemoryOStream & stream,
		const MemberDescription & description,
		HistoryEvent::Level level, int16 msgStreamSize );

	void trim();
	void clear();

	const_reverse_iterator rbegin() const	{ return container_.rbegin(); }
	const_reverse_iterator rend() const		{ return container_.rend(); }

	EventNumber lastTrimmedEventNumber() const
		{ return lastTrimmedEventNumber_; }
	void lastTrimmedEventNumber( EventNumber v )
		{ lastTrimmedEventNumber_ = v; }

private:
	void add( HistoryEvent * pEvent );
	void deleteEvent( HistoryEvent * pEvent );

	Container container_;
	EventNumber trimToEvent_;

	// The last EventNumber that we cannot supply
	EventNumber lastTrimmedEventNumber_;

	// Events that have LatestChangeOnly have (at most) a single instance.
	typedef BW::vector< Container::iterator > LatestChangePointers;
	LatestChangePointers latestEventPointers_;
};

#ifdef CODE_INLINE
#include "history_event.ipp"
#endif

BW_END_NAMESPACE

#endif // HISTORY_EVENT_HPP
