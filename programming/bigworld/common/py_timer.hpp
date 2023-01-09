#ifndef PY_TIMER_HPP
#define PY_TIMER_HPP

#include "cstdmf/timer_handler.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "server/script_timers.hpp"

BW_BEGIN_NAMESPACE

class PyTimer;

/**
 *	This method handles timer events associated with the PyTimer.
 */
class PyTimerHandler : public TimerHandler
{
public:
	PyTimerHandler( PyTimer & pyTimer ) :
		pyTimer_( pyTimer ) {}

private:
	// Overrides
	virtual void handleTimeout( TimerHandle handle, void * pUser );
	virtual void onRelease( TimerHandle handle, void  * pUser );

	PyTimer & pyTimer_;
};


/**
 *	This class is used by PyObjects to add, remove and invoke expired
 *	timers.
 */
class PyTimer
{
public:
	PyTimer( PyObject * pyObject, int ownerID );
	~PyTimer();

	PyObject * addTimer( PyObject * args );
	PyObject * delTimer( PyObject * args );

	void handleTimeout( TimerHandle handle, void * pUser );
	void onTimerReleased( TimerHandle timerHandle );

	void backUpTimers( BinaryOStream & stream );
	void restoreTimers( BinaryIStream & stream );

	void cancelAll();

	void ownerID( int ownerID ) { ownerID_ = ownerID; }

private:
	friend class PyTimerHandler;
	PyTimerHandler timerHandler_;

	int ownerID_;

	bool isCancelled_;

	PyObject * pyObject_;

	ScriptTimers * pTimers_;
};

BW_END_NAMESPACE

#endif // PY_TIMER_HPP
