#ifndef TIMER_HANDLER_HPP
#define TIMER_HANDLER_HPP

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This type is a handle to a timer event.
 */
class TimeQueueNode;

/**
 *	This class is a handle to a timer added to TimeQueue.
 */
class TimerHandle
{
public:
	CSTDMF_DLL explicit TimerHandle( TimeQueueNode * pNode = NULL );
	CSTDMF_DLL TimerHandle( const TimerHandle & other );
	CSTDMF_DLL ~TimerHandle();

	CSTDMF_DLL void cancel();
	void clearWithoutCancel()	{ pNode_ = NULL; }

	bool isSet() const		{ return pNode_ != NULL; }

	CSTDMF_DLL TimerHandle & operator=( const TimerHandle & other );
	friend bool operator==( const TimerHandle & h1, const TimerHandle & h2 );

	TimeQueueNode * pNode() const	{ return pNode_; }

private:
	TimeQueueNode * pNode_;
};

inline bool operator==( const TimerHandle & h1, const TimerHandle & h2 )
{
	return h1.pNode_ == h2.pNode_;
}


/**
 *	This is an interface which must be derived from in order to
 *	receive time queue events.
 */
class TimerHandler
{
public:
	TimerHandler() : numTimesRegistered_( 0 ) {}
	virtual ~TimerHandler()
	{
		MF_ASSERT( numTimesRegistered_ == 0 );
	};

	/**
	 * 	This method is called when a timeout expires.
	 *
	 * 	@param	handle	The handle returned when the event was added.
	 * 	@param	pUser	The user data passed in when the event was added.
	 */
	virtual void handleTimeout( TimerHandle handle, void * pUser ) = 0;

protected:
	virtual void onRelease( TimerHandle /* handle */, void * /* pUser */ ) {}

private:
	friend class TimeQueueNode;
	void incTimerRegisterCount() { ++numTimesRegistered_; }
	void decTimerRegisterCount() { --numTimesRegistered_; }
	void release( TimerHandle handle, void * pUser )
	{
		this->decTimerRegisterCount();
		this->onRelease( handle, pUser );
	}

	int numTimesRegistered_;
};

BW_END_NAMESPACE

#endif // TIMER_HANDLER_HPP
