#ifndef LIB__SERVER__SIGNAL_PROCESSOR_HPP
#define LIB__SERVER__SIGNAL_PROCESSOR_HPP

#include "cstdmf/bw_map.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/singleton.hpp"

#include "network/frequent_tasks.hpp"

#include "server/signal_set.hpp"

#include <utility>

#include <signal.h>


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class EventDispatcher;
}

namespace Signal
{
	class Set;
}

/**
 *	Signal handler abstract class.
 */
class SignalHandler
{
public:
	virtual ~SignalHandler();

	virtual void handleSignal( int sigNum ) = 0;
};


/**
 *	Signal processing class.
 */
class SignalProcessor : private Mercury::FrequentTask,
		public Singleton< SignalProcessor >
{
public:
	SignalProcessor( Mercury::EventDispatcher & dispatcher );
	virtual ~SignalProcessor();

	void ignoreSignal( int sigNum );

	void setDefaultSignalHandler( int sigNum );

	void addSignalHandler( int sigNum, SignalHandler * pSignalHandler, 
		int flags = 0 );

	void clearSignalHandlers( int sigNum );
	void clearSignalHandler( int sigNum, SignalHandler * pSignalHandler );
	void clearSignalHandler( SignalHandler * pSignalHandler );

	int waitForSignals( const Signal::Set & signalSet );

	void handleSignal( int sigNum );

	static const char * signalNumberToString( int sigNum );
private:
	// Override from Mercury::FrequentTask
	virtual void doTask()
		{ this->dispatch(); }

	void dispatch();
	void dispatchSignal( int sigNum );

	void enableDetectSignal( int sigNum, int flags = 0 );

// Member data
	Mercury::EventDispatcher & dispatcher_;

	typedef BW::multimap< int, SignalHandler * > SignalHandlers;
	SignalHandlers signalHandlers_;

	SignalHandler * pSigQuitHandler_;

	Signal::Set signals_;
};

BW_END_NAMESPACE

#endif // LIB__SERVER__SIGNAL_PROCESSOR_HPP

