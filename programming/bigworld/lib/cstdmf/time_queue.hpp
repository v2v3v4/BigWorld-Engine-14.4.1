#ifndef TIME_QUEUE_HEADER
#define TIME_QUEUE_HEADER

#include "timer_handler.hpp"

#include "debug.hpp"
#include "stdmf.hpp"
#include "bw_vector.hpp"
#include "smartpointer.hpp"
#include "cstdmf/profiler.hpp"

#include <functional>
#include <algorithm>
#include <climits>


BW_BEGIN_NAMESPACE

class TimeQueueBase;

/**
 *	This class is the base class for the nodes of the time queue.
 */
class TimeQueueNode : public ReferenceCount
{
public:
	TimeQueueNode( TimeQueueBase & owner,
			TimerHandler * pHandler,
			void * pUserData );

	void cancel( bool shouldCallOnRelease = true );

	void * pUserData() const	{ return pUserData_; }
	bool isCancelled() const	{ return state_ == STATE_CANCELLED; }

protected:
	bool isExecuting() const	{ return state_ == STATE_EXECUTING; }

	/// This enumeration is used to describe the current state of an element on
	/// the queue.
	enum State
	{
		STATE_PENDING,
		STATE_EXECUTING,
		STATE_CANCELLED
	};

	TimeQueueBase & owner_;
	TimerHandler * pHandler_;
	void * pUserData_;

	State state_;
};


/**
 *	This class is the base class for TimeQueueT. It allows TimeQueueNode to have
 *	a reference to its owner that is not a template.
 */
class TimeQueueBase
{
public:
	virtual ~TimeQueueBase() {};
	virtual void onCancel() = 0;
};


/**
 * 	This class implements a time queue, measured in game ticks. The logic is
 * 	basically stolen from Mercury, but it is intended to be used as a low
 * 	resolution timer.  Also, timestamps should be synchronised between servers.
 */
template< class TIME_STAMP >
class TimeQueueT : public TimeQueueBase
{
public:
	TimeQueueT();
	virtual ~TimeQueueT();

	void clear( bool shouldCallOnRelease = true );

	/// This is the unit of time used by the time queue
	typedef TIME_STAMP TimeStamp;

	/// Schedule an event
	TimerHandle	add( TimeStamp startTime, TimeStamp interval,
						TimerHandler* pHandler, void * pUser,
						const char * name = "UnnamedTimer" );

	/// Process all events older than or equal to now
	int		process( TimeStamp now );

	/// Determine whether or not the given handle is legal (slow)
	bool		legal( TimerHandle handle ) const;

	/// Return the number of timestamps until the first node expires.  This will
	/// return 0 if size() == 0, so you must check this first
	TIME_STAMP nextExp( TimeStamp now ) const;

	/// Returns the number of timers in the queue
	inline uint32 size() const
	{
		MF_ASSERT( timeQueue_.size() <= UINT_MAX );
		return ( uint32 ) timeQueue_.size();
	}

	/// Returns whether the time queue is empty.
	inline bool empty() const	{ return timeQueue_.empty(); }

	bool		getTimerInfo( TimerHandle handle,
					TimeStamp &			time,
					TimeStamp &			interval,
					void * &			pUser ) const;

	TIME_STAMP timerDeliveryTime( TimerHandle handle ) const;
	TIME_STAMP timerIntervalTime( TimerHandle handle ) const;
	TIME_STAMP & timerIntervalTime( TimerHandle handle );

	void adjustBy( TimeStamp adjustment )
		{ timeQueue_.adjustBy( adjustment ); }

private:
	void purgeCancelledNodes();
	void onCancel();

	/// This structure represents one event in the time queue.
	class Node : public TimeQueueNode
	{
	public:
		Node( TimeQueueBase & owner, TimeStamp startTime, TimeStamp interval,
			TimerHandler * pHandler, void * pUser, const char * name );

		TIME_STAMP time() const			{ return time_; }
		TIME_STAMP interval() const		{ return interval_; }
		TIME_STAMP & intervalRef()		{ return interval_; }

		TIME_STAMP deliveryTime() const;

		void triggerTimer();

		void adjustBy( TimeStamp adjustment )
		{
			time_ += adjustment;
		}

		const char * name()				{ return name_; }

	private:
		TimeStamp			time_;
		TimeStamp			interval_;
		const char *		name_;

		Node( const Node & );
		Node & operator=( const Node & );
	};

	/// Comparison object for the priority queue.
	class Comparator
	{
	public:
		bool operator()(const Node* a, const Node* b)
		{
			return a->time() > b->time();
		}
	};

	/**
	 *	This class implements a priority queue. std::priority_queue is not used
	 *	so that access to the underlying container can be gotten.
	 */
	class PriorityQueue
	{
	public:
		typedef BW::vector< Node * > Container;

		typedef typename Container::value_type value_type;
		typedef typename Container::size_type size_type;

		bool empty() const				{ return container_.empty(); }
		size_type size() const			{ return container_.size(); }

		const value_type & top() const	{ return container_.front(); }

		void push( const value_type & x )
		{
			container_.push_back( x );
			std::push_heap( container_.begin(), container_.end(),
					Comparator() );
		}

		void pop()
		{
			std::pop_heap( container_.begin(), container_.end(), Comparator() );
			container_.pop_back();
		}

		/**
		 *	Note: This leaves the queue in a bad state.
		 */
		Node * unsafePopBack()
		{
			Node * pNode = container_.back();
			container_.pop_back();
			return pNode;
		}

		/**
		 *	This method returns the underlying container. If this container is
		 *	modified, heapify should be called to return the PriorityQueue to
		 *	be a valid priority queue.
		 */
		Container & container()		{ return container_; }

		/**
		 *	This method enforces the underlying container to be in a valid heap
		 *	ordering.
		 */
		void heapify()
		{
			std::make_heap( container_.begin(), container_.end(),
					Comparator() );
		}

		void adjustBy( TimeStamp adjustment )
		{
			typename Container::iterator iter = container_.begin();

			while (iter != container_.end())
			{
				(*iter)->adjustBy( adjustment );

				++iter;
			}
		}

	private:
		Container container_;
	};

	PriorityQueue	timeQueue_;
	Node * 			pProcessingNode_;
	TimeStamp 		lastProcessTime_;
	int				numCancelled_;

	// Cannot be copied.
	TimeQueueT( const TimeQueueT & );
	TimeQueueT & operator=( const TimeQueueT & );
};

typedef TimeQueueT< uint32 > TimeQueue;
typedef TimeQueueT< uint64 > TimeQueue64;

#include "time_queue.ipp"

BW_END_NAMESPACE

#endif // TIME_QUEUE_HEADER
