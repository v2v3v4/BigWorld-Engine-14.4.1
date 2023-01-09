#include "signal_processor.hpp"

#include "server_app.hpp"
#include "signal_set.hpp"

#include "network/event_dispatcher.hpp"


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( SignalProcessor );

namespace // (anonymous)
{



const int SIGMIN = 1;
const int SIGMAX = SIGSYS;

const char * SIGNAL_NAMES[] = 
{
	NULL,
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGABRT",
	"SIGBUS",
	"SIGFPE",
	"SIGKILL",
	"SIGUSR1",
	"SIGSEGV",
	"SIGUSR2",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGSTKFLT",
	"SIGCHLD",
	"SIGCONT",
	"SIGSTOP",
	"SIGTSTP",
	"SIGTTIN",
	"SIGTTOU",
	"SIGURG",
	"SIGXCPU",
	"SIGXFSZ",
	"SIGVTALRM",
	"SIGPROF",
	"SIGWINCH",
	"SIGIO",
	"SIGPWR",
	"SIGSYS"
};


/**
 *	Signal handling function that passes it to the SignalProcessor singleton.
 */
void signalHandler( int sigNum )
{
	// Don't allocate memory in signal handlers. e.g. calling *_MSG().

	if (SignalProcessor::pInstance() != NULL)
	{
		SignalProcessor::instance().handleSignal( sigNum );
	}
}


} // end namespace (anonymous)


/**
 *	Destructor.
 */
SignalHandler::~SignalHandler()
{
	if (SignalProcessor::pInstance() != NULL)
	{
		SignalProcessor::instance().clearSignalHandler( this );
	}
}


// ---------------------------------------------------------------------------
// Section: SignalProcessor
// ---------------------------------------------------------------------------

/**
 *	Constructor.
 */
SignalProcessor::SignalProcessor( Mercury::EventDispatcher & dispatcher ):
		Mercury::FrequentTask( "SignalProcessor" ),
	dispatcher_( dispatcher ),
	signalHandlers_(),
	pSigQuitHandler_( NULL ),
	signals_()
{
	dispatcher_.addFrequentTask( this );
}


/**
 *	Destructor.
 */
SignalProcessor::~SignalProcessor()
{
	dispatcher_.cancelFrequentTask( this );

	// Reset the signal disposition to be the default actions.
	for (SignalHandlers::iterator iSignalHandler = signalHandlers_.begin();
			iSignalHandler != signalHandlers_.end();
			++iSignalHandler)
	{
		::signal( iSignalHandler->first, SIG_DFL );
	}
}


/**
 *	Set the signal handling for the given signal to be to ignore the signal.
 */
void SignalProcessor::ignoreSignal( int sigNum )
{
	this->clearSignalHandlers( sigNum );
	::signal( sigNum, SIG_IGN );
}


/**
 *	Set the signal handling for the given signal to the default action for that
 *	signal.
 */
void SignalProcessor::setDefaultSignalHandler( int sigNum )
{
	this->clearSignalHandlers( sigNum );
	::signal( sigNum, SIG_DFL );
}


/**
 *	Add a signal handler for the given signal.
 *
 *	@param sigNum			The signal number.
 *	@param pSignalHandler	The signal handler.
 *	@param flags			The signal action flags (the sa_flags member of the
 *							sigaction struct - see man page for the sigaction
 *							function()).
 */
void SignalProcessor::addSignalHandler( int sigNum, 
		SignalHandler * pSignalHandler, int flags )
{
	if (sigNum == SIGQUIT)
	{
		// Can only have one handler for SIGQUIT for now.
		MF_ASSERT( pSigQuitHandler_ == NULL );
		pSigQuitHandler_ = pSignalHandler;
	}
	else
	{
		signalHandlers_.insert( 
			SignalHandlers::value_type( sigNum, pSignalHandler ) );
	}

	this->enableDetectSignal( sigNum, flags );
}


/**
 *	Enable signal detection for the given signal. All signals are blocked while
 *	the handler is executing. 
 *
 *	@param sigNum 			The signal number.
 *	@param flags			The signal action flags (the sa_flags member of the
 *							sigaction struct - see man page for the sigaction
 *							function()).
 *
 */
void SignalProcessor::enableDetectSignal( int sigNum, int flags )
{
	struct sigaction action;
	action.sa_handler = &signalHandler;
	sigfillset( &(action.sa_mask) );

	if (flags & SA_SIGINFO)
	{
		ERROR_MSG( "SignalProcessor::enableDetectSignal: "
				"SA_SIGINFO is not supported, ignoring\n" );
		flags &= ~SA_SIGINFO;
	}

	action.sa_flags = flags;

	::sigaction( sigNum, &action, NULL );
}


/**
 *	Clear all signal handlers that handle the given signal.
 *
 *	@param sigNum		The signal.
 */
void SignalProcessor::clearSignalHandlers( int sigNum )
{
	signalHandlers_.erase( sigNum );
}


/**
 *	Clear the given signal handler for the given signal. This will remove all
 *	instances of the given signal handler that are registered for the given
 *	signal, and set the signal disposition to be the default.
 *
 *	@param sigNum			The signal number.
 *	@param pSignalHandler	The signal handler to remove.
 */
void SignalProcessor::clearSignalHandler( int sigNum, 
		SignalHandler * pSignalHandler )
{
	if (sigNum == SIGQUIT)
	{
		pSigQuitHandler_ = NULL;
	}

	std::pair< SignalHandlers::iterator, SignalHandlers::iterator > range = 
		signalHandlers_.equal_range( sigNum );

	SignalHandlers::iterator iSignalHandler = range.first;
	while (iSignalHandler != range.second)
	{
		int sigNum = iSignalHandler->first;
		if (pSignalHandler == iSignalHandler->second)
		{
			SignalHandlers::iterator toDelete = iSignalHandler++;
			signalHandlers_.erase( toDelete );

			if (signalHandlers_.count( sigNum ) == 0)
			{
				this->setDefaultSignalHandler( sigNum );
			}
		}
		else
		{
			++iSignalHandler;
		}
	}
}


/**
 *	Remove all instances of the given signal handler.
 *
 *	@param pSignalHandler 	The signal handler to remove.
 */
void SignalProcessor::clearSignalHandler( SignalHandler * pSignalHandler )
{
	SignalHandlers::iterator iSignalProcessor = signalHandlers_.begin();
	while (iSignalProcessor != signalHandlers_.end())
	{
		int sigNum = iSignalProcessor->first;
		if (iSignalProcessor->second == pSignalHandler)
		{
			SignalHandlers::iterator toDelete = iSignalProcessor++;
			signalHandlers_.erase( toDelete );

			if (signalHandlers_.count( sigNum ) == 0)
			{
				this->setDefaultSignalHandler( sigNum );
			}
		}
		else
		{
			++iSignalProcessor;
		}
	}
}


/**
 *	Handle the given signal. 
 *
 *	This is called from the signal handling function registered with
 *	sigaction(). It shouldn't allocate any memory, and should be fast.
 *
 *	@param sigNum 	The detected signal to handle.
 *
 */
void SignalProcessor::handleSignal( int sigNum )
{
	// In signal handler, be careful not to do anything like allocate memory
	// e.g. calling *_MSG(). We set the state on preallocated heap memory.

	signals_.set( sigNum );


	if (sigNum == SIGQUIT && pSigQuitHandler_)
	{
		pSigQuitHandler_->handleSignal( sigNum );
	}
}


/**
 *	Handle frequent task trigger.
 */
void SignalProcessor::dispatch()
{
	const Signal::Set allSignals( Signal::Set::FULL );
	Signal::Blocker blocker( allSignals );

	int sigNum = SIGMIN;
	while (sigNum <= SIGMAX)
	{
		if (signals_.isSet( sigNum ))
		{
			this->dispatchSignal( sigNum );
		}

		++sigNum;
	}

	signals_.clearAll();
}


/**
 *	Dispatch a signal.
 *
 *	This is called from the frequent tasks trigger, and so it is safe to
 *	allocate memory while dispatching. This method calls the
 *	SignalHandler::handleSignal() method, and so all SignalHandler subclasses
 *	are free to allocate memory as well.
 *
 *	@param sigNum 		The signal to dispatch to the registered SignalHandler
 *						instances.
 */
void SignalProcessor::dispatchSignal( int sigNum )
{
	DEBUG_MSG( "SignalProcessor::dispatchSignal: %s\n",
			this->signalNumberToString( sigNum ) );

	// Use a copy of the handler map in case signal handlers modify
	// signalHandlers_.
	SignalHandlers copy( signalHandlers_ );

	std::pair< SignalHandlers::iterator, SignalHandlers::iterator > range = 
		copy.equal_range( sigNum );

	SignalHandlers::iterator iSignalHandler = range.first;
	while (iSignalHandler != range.second)
	{
		iSignalHandler->second->handleSignal( sigNum );
		++iSignalHandler;
	}
}


/**
 *	Block until one of a given set of signals is received.
 *
 *	@param set		The set of signals to wait for.
 *
 *	@return 		The signal in the set that was triggered.
 */
int SignalProcessor::waitForSignals( const Signal::Set & set )
{
	int sigNum = ::sigwaitinfo( set, NULL );

	while (sigNum < 0 && errno == EINTR)
	{
		sigNum = ::sigwaitinfo( set, NULL );
	}

	this->dispatchSignal( sigNum );

	return sigNum;
}


/**
 *	Get a human-readable name for the given signal number.
 *
 *	@param sigNum 	The signal number.
 */
const char * SignalProcessor::signalNumberToString( int sigNum )
{
	if (sigNum >= SIGRTMIN && sigNum <= SIGRTMAX)
	{
		return "SIGRTxxx";
	}

	// Check sigNum is in valid signal range.
	MF_ASSERT( sigNum <= SIGUNUSED && sigNum != 0 );
	return SIGNAL_NAMES[sigNum];
}

BW_END_NAMESPACE

// signal_processor.cpp

