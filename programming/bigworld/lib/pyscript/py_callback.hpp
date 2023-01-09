#ifndef __PYSCRIPT_PY_CALLBACK_HPP__
#define __PYSCRIPT_PY_CALLBACK_HPP__

#ifndef MF_SERVER
// Not available on server yet, pending refactoring.

#include "Python.h"

#include "cstdmf/timer_handler.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

namespace Script
{
	// Timer handles
	typedef uint32 TimerHandle;
	const TimerHandle INVALID_TIMER_HANDLE = 0;
	const TimerHandle MAX_TIMER_HANDLE = 0xffffffff;
	TimerHandle getHandle();

	// Game time
	typedef double (*TotalGameTimeFn)();
	void	setTotalGameTimeFn( TotalGameTimeFn fn );
	double	getTotalGameTime();

	// Callbacks
	void clearTimers();
	void tick( double timeNow );
	void callNextFrame( PyObject * fn, PyObject * args, const char * reason,
		double age = 0.0 );
} // namespace Script 

BW_END_NAMESPACE

#endif

#endif // __PYSCRIPT_PY_CALLBACK_HPP__
