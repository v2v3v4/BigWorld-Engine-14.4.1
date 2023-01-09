#include "pch.hpp"

#include "event_poller.hpp"

#include "event_dispatcher.hpp"

#include "interfaces.hpp"

#include "network_utils.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/profile.hpp"
#include "cstdmf/timestamp.hpp"



#if defined( __linux__ ) && !defined( EMSCRIPTEN )

# define HAS_EPOLL
# include <sys/epoll.h>

#elif defined( EMSCRIPTEN )

# define HAS_POLL
# include <poll.h>
# include <sys/ioctl.h> // for FIONREAD in PollPoller::processPendingEvents()

#endif

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_vector.hpp"

#include <string.h>
#include <errno.h>

#if defined( __linux__ ) || defined( __APPLE__ )
#include <sys/select.h>
#endif // defined( __linux__ ) || defined( __APPLE__ )

#include <sstream>

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: EventPoller
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EventPoller::EventPoller() : 
	fdReadHandlers_(), 
	fdWriteHandlers_(), 
	spareTime_( 0 )
{
}


/**
 *	Destructor.
 */
EventPoller::~EventPoller()
{
}


/**
 *	True if ready for shutdown, false otherwise
 */
bool EventPoller::isReadyForShutdown() const
{
	FDHandlers::const_iterator handler = fdReadHandlers_.begin();

	while (handler != fdReadHandlers_.end())
	{
		if (!handler->second.isReadyForShutdown())
		{
			return false;
		}
		++handler;
	}

	handler = fdWriteHandlers_.begin();

	while (handler != fdWriteHandlers_.end())
	{
		if (!handler->second.isReadyForShutdown())
		{
			return false;
		}
		++handler;
	}

	return true;
}


/**
 *	This method registers a file descriptor for reading.
 */
bool EventPoller::registerForRead( int fd,
		InputNotificationHandler * handler, const char * name )
{
	if (!this->doRegisterForRead( fd, name ))
	{
		return false;
	}

	fdReadHandlers_[ fd ] = InputHandlerEntry( handler, name );

	return true;
}


/**
 *	This method registers a file descriptor for writing.
 */
bool EventPoller::registerForWrite( int fd,
		InputNotificationHandler * handler, const char * name )
{
	if (!this->doRegisterForWrite( fd, name ))
	{
		return false;
	}

	fdWriteHandlers_[ fd ] = InputHandlerEntry( handler, name );

	return true;
}


/**
 *	This method deregisters a file previously registered for reading.
 */
bool EventPoller::deregisterForRead( int fd )
{
	fdReadHandlers_.erase( fd );

	return this->doDeregisterForRead( fd );
}


/**
 *	This method deregisters a file previously registered for writing.
 */
bool EventPoller::deregisterForWrite( int fd )
{
	fdWriteHandlers_.erase( fd );

	return this->doDeregisterForWrite( fd );
}


/**
 *	This method is called when there is data to be read or written on the
 *	input file descriptor.
 */
bool EventPoller::trigger( int fd, FDHandlers & handlers )
{
	FDHandlers::iterator iter = handlers.find( fd );

	if (iter == handlers.end())
	{
		return false;
	}

	iter->second.handleInputNotification( fd );

	return true;
}


/**
 *	This method is called when there is an error on the input file descriptor.
 */
bool EventPoller::triggerError( int fd )
{
	// Try read handlers first.

	FDHandlers::iterator iHandler = fdReadHandlers_.find( fd );
	if (iHandler != fdReadHandlers_.end())
	{
		if (!iHandler->second.handleErrorNotification( fd ))
		{
			// Error notification did not handle error. Fallback to input 
			// notification.
			iHandler->second.handleInputNotification( fd );
		}
		return true;
	}

	// No read handler found to claim responsibility. Check write handlers.
	iHandler = fdWriteHandlers_.find( fd );
	if (iHandler != fdWriteHandlers_.end())
	{
		if (!iHandler->second.handleErrorNotification( fd ))
		{
			// Error notification did not handle error. Fallback to input 
			// notification.
			iHandler->second.handleInputNotification( fd );
		}
		return true;
	}

	// No handler found for this file descriptor.

	return false;
}


/**
 *	This method returns whether or not a file descriptor is already registered.
 *	
 *	@param fd The file descriptor to test.
 *	@param isForRead Indicates whether checking for read or write registration.
 *
 *	@return True if registered, otherwise false.
 */
bool EventPoller::isRegistered( int fd, bool isForRead ) const
{
	const FDHandlers & handlers =
		isForRead ? fdReadHandlers_ : fdWriteHandlers_;

	return handlers.find( fd ) != handlers.end();
}


/**
 *	This method overrides the InputNotificationHandler. It is called when there
 *	are events to process.
 */
int EventPoller::handleInputNotification( int fd )
{

	this->processPendingEvents( 0.0 );

	return 0;
}


/**
 *	This method returns the file descriptor for this EventPoller or -1 if it
 *	does not have one. For epoll, this is used to register with the parent
 *	EventDispatcher's poller.
 */
int EventPoller::getFileDescriptor() const
{
	return -1;
}


/**
 *	Returns the largest file descriptor present in both the read or write 
 *	handler maps. If neither have entries, -1 is returned.
 */
int EventPoller::maxFD() const
{
	int readMaxFD = EventPoller::maxFD( fdReadHandlers_ );
	int writeMaxFD = EventPoller::maxFD( fdWriteHandlers_ );
	return std::max( readMaxFD, writeMaxFD );
}


/**
 *	Return the largest file descriptor present in the given map. If the given 
 *	map is empty, -1 is returned.
 */
int EventPoller::maxFD( const FDHandlers & handlerMap )
{
	int maxFD = -1;
	FDHandlers::const_iterator iFDHandler = handlerMap.begin();
	while (iFDHandler != handlerMap.end())
	{
		if (iFDHandler->first > maxFD)
		{
			maxFD = iFDHandler->first;
		}
		++iFDHandler;
	}
	return maxFD;
}


// -----------------------------------------------------------------------------
// Section: SelectPoller
// -----------------------------------------------------------------------------

/**
 *	This class is an EventPoller that uses select.
 */
class SelectPoller : public EventPoller
{
public:
	SelectPoller();

	/** Destructor. */
	virtual ~SelectPoller() {}

protected:
	virtual bool doRegisterForRead( int fd, const char * name );
	virtual bool doRegisterForWrite( int fd, const char * name );

	virtual bool doDeregisterForRead( int fd );
	virtual bool doDeregisterForWrite( int fd );

	virtual int processPendingEvents( double maxWait );

private:
	void handleInputNotifications( int & countReady,
		fd_set & readFDs, fd_set & writeFDs, fd_set & exceptionFDs );

	void updateLargestFileDescriptor();

	// The file descriptor sets used in the select call.
	fd_set						fdReadSet_;
	fd_set						fdWriteSet_;
	fd_set						fdExceptionSet_;

	// This is the largest registered file descriptor (read or write). It's used
	// in the call to select.
	int							fdLargest_;

	// This is the number of file descriptors registered for the write event.
	int							fdWriteCount_;
};


/**
 *	Constructor.
 */
SelectPoller::SelectPoller() :
	EventPoller(),
	fdReadSet_(),
	fdWriteSet_(),
	fdExceptionSet_(),
	fdLargest_( -1 ),
	fdWriteCount_( 0 )
{
	// set up our list of file descriptors
	FD_ZERO( &fdReadSet_ );
	FD_ZERO( &fdWriteSet_ );
	FD_ZERO( &fdExceptionSet_);
}


/**
 *  This method finds the highest file descriptor in the read and write sets
 *  and writes it to fdLargest_.
 */
void SelectPoller::updateLargestFileDescriptor()
{
	fdLargest_ = this->maxFD();
}


/**
 *  Trigger input notification handlers for ready file descriptors.
 */
void SelectPoller::handleInputNotifications( int & countReady,
	fd_set & readFDs, fd_set & writeFDs, fd_set & exceptionFDs )
{
	BW::set< int > handledFDs;

#if defined( _WIN32 )
	// X360 fd_sets don't look like POSIX ones, we know exactly what they are
	// and can just iterate over the provided FD arrays

	for (unsigned i = 0; i < exceptionFDs.fd_count; ++i)
	{
		SOCKET fd = exceptionFDs.fd_array[ i ];
		--countReady;
		MF_ASSERT( fd <= INT_MAX );
		this->triggerError( ( int ) fd );
		handledFDs.insert( ( int ) fd );
	}

	for (unsigned i = 0; i < readFDs.fd_count; ++i)
	{
		SOCKET fd = readFDs.fd_array[ i ];
		MF_ASSERT( fd <= INT_MAX );
		--countReady;

		if (handledFDs.count( ( int ) fd ) == 0)
		{
			// Check we haven't already triggered this file descriptor with an
			// error condition above.
			this->triggerRead( ( int ) fd );
		}
	}

	for (unsigned i = 0; i < writeFDs.fd_count; ++i)
	{
		SOCKET fd = writeFDs.fd_array[ i ];
		MF_ASSERT( fd <= INT_MAX );
		--countReady;

		if (handledFDs.count( ( int ) fd ) == 0)
		{
			// Check we haven't already triggered this file descriptor with an
			// error condition above.
			this->triggerWrite( (int ) fd );
		}
	}

#else // !defined( _WIN32 )
	// POSIX fd_sets are more opaque and we just have to count up blindly until
	// we hit valid FD's with FD_ISSET

	for (int fd = 0; fd <= fdLargest_ && countReady > 0; ++fd)
	{
		if (FD_ISSET( fd, &exceptionFDs ))
		{
			--countReady;
			this->triggerError( fd );
			handledFDs.insert( fd );
		}

		if (FD_ISSET( fd, &readFDs ))
		{
			--countReady;

			if (handledFDs.count( fd ) == 0)
			{
				// Check we haven't already triggered this file descriptor with
				// an error condition above.
				this->triggerRead( fd );
			}
		}

		if (FD_ISSET( fd, &writeFDs ))
		{
			--countReady;

			if (handledFDs.count( fd ) == 0)
			{
				// Check we haven't already triggered this file descriptor with
				// an error condition above.
				this->triggerWrite( fd );
			}
		}
	}
#endif // defined( _WIN32 )
}


/*
 *	Override from EventPoller.
 */
int SelectPoller::processPendingEvents( double maxWait )
{
	fd_set	readFDs;
	fd_set	writeFDs;
	fd_set	exceptionFDs;
	struct timeval		nextTimeout;
	// Is this needed?
	FD_ZERO( &readFDs );
	FD_ZERO( &writeFDs );
	FD_ZERO( &exceptionFDs );

	readFDs = fdReadSet_;
	writeFDs = fdWriteSet_;
	exceptionFDs = fdExceptionSet_;

	nextTimeout.tv_sec = (int)maxWait;
	nextTimeout.tv_usec =
		(int)( (maxWait - (double)nextTimeout.tv_sec) * 1000000.0 );

#if ENABLE_WATCHERS
	static ProfileVal g_idleProfile( "Idle" );
	g_idleProfile.start();
#else // !ENABLE_WATCHERS
	uint64 startTime = BW_NAMESPACE timestamp();
#endif // ENABLE_WATCHERS

	int countReady = 0;
	
	BWConcurrency::startMainThreadIdling();
	{
		PROFILER_IDLE_SCOPED( Idle );

	#if defined( _WIN32 )
		if (fdLargest_ == -1)
		{
			// Windows can't handle it if we don't have any FDs to select on, but
			// we have a non-NULL timeout.
			Sleep( int( maxWait * 1000.0 ) );
		}
		else
	#endif // defined( _WIN32 )
		{
			countReady = select( fdLargest_+1, &readFDs,
					fdWriteCount_ ? &writeFDs : NULL, &exceptionFDs, &nextTimeout );
		}
	}
	BWConcurrency::endMainThreadIdling();

#if ENABLE_WATCHERS
	g_idleProfile.stop();
	spareTime_ += g_idleProfile.lastTime_;
#else // !ENABLE_WATCHERS
	spareTime_ += BW_NAMESPACE timestamp() - startTime;
#endif // ENABLE_WATCHERS

	if (countReady > 0)
	{
		this->handleInputNotifications( countReady, readFDs, writeFDs,
			exceptionFDs );
	}
	else if (countReady == -1)
	{
		// TODO: Clean this up on shutdown
		// if (!breakProcessing_)
		{
			WARNING_MSG( "SelectPoller::processPendingEvents: "
				"error in select(): %s\n", lastNetworkError() );
		}
	}

	return countReady;
}


/**
 *	This method implements a virtual method from EventPoller to do the real
 *	work of registering a file descriptor for reading.
 */
bool SelectPoller::doRegisterForRead( int fd, const char * name )
{
#if !defined( _WIN32 )
	if ((fd < 0) || (FD_SETSIZE <= fd))
	{
		ERROR_MSG( "SelectPoller::doRegisterForRead: "
			"Tried to register invalid fd %d (%s). FD_SETSIZE (%d)\n",
			fd, name, FD_SETSIZE );

		return false;
	}
#endif // !defined( _WIN32 )

	// Bail early if it's already in the read set
	if (FD_ISSET( fd, &fdReadSet_ ))
	{
		return false;
	}

	FD_SET( fd, &fdReadSet_ );
	if (!FD_ISSET( fd, &fdExceptionSet_ ))
	{
		FD_SET( fd, &fdExceptionSet_ );
	}

	if (fd > fdLargest_)
	{
		fdLargest_ = fd;
	}

	return true;
}


/**
 *	This method implements a virtual method from EventPoller to do the real
 *	work of registering a file descriptor for writing.
 */
bool SelectPoller::doRegisterForWrite( int fd, const char * name )
{
#if !defined( _WIN32 )
	if ((fd < 0) || (FD_SETSIZE <= fd))
	{
		ERROR_MSG( "SelectPoller::doRegisterForWrite: "
			"Tried to register invalid fd %d (%s). FD_SETSIZE (%d)\n",
			fd, name, FD_SETSIZE );

		return false;
	}
#endif // !defined( _WIN32 )

	if (FD_ISSET( fd, &fdWriteSet_ ))
	{
		return false;
	}

	FD_SET( fd, &fdWriteSet_ );
	if (!FD_ISSET( fd, &fdExceptionSet_ ))
	{
		FD_SET( fd, &fdExceptionSet_ );
	}

	if (fd > fdLargest_)
	{
		fdLargest_ = fd;
	}

	++fdWriteCount_;
	return true;
}


/**
 *	This method implements a virtual method from EventPoller to do the real
 *	work of deregistering a file descriptor for reading.
 */
bool SelectPoller::doDeregisterForRead( int fd )
{
#if !defined( _WIN32 )
	if ((fd < 0) || (FD_SETSIZE <= fd))
	{
		return false;
	}
#endif // !defined( _WIN32 )

	if (!FD_ISSET( fd, &fdReadSet_ ))
	{
		return false;
	}

	FD_CLR( fd, &fdReadSet_ );

	if (!FD_ISSET( fd, &fdWriteSet_ ))
	{
		FD_CLR( fd, &fdExceptionSet_ );
	}

	if (fd == fdLargest_)
	{
		this->updateLargestFileDescriptor();
	}

	return true;
}


/**
 *	This method implements a virtual method from EventPoller to do the real
 *	work of deregistering a file descriptor for reading.
 */
bool SelectPoller::doDeregisterForWrite( int fd )
{
#if !defined( _WIN32 )
	if ((fd < 0) || (FD_SETSIZE <= fd))
	{
		return false;
	}
#endif // !defined( _WIN32 )

	if (!FD_ISSET( fd, &fdWriteSet_ ))
	{
		return false;
	}

	FD_CLR( fd, &fdWriteSet_ );


	if (!FD_ISSET( fd, &fdReadSet_ ))
	{
		FD_CLR( fd, &fdExceptionSet_ );
	}

	if (fd == fdLargest_)
	{
		this->updateLargestFileDescriptor();
	}

	--fdWriteCount_;
	return true;
}


#if defined( HAS_POLL )

// -----------------------------------------------------------------------------
// Section: PollPoller
// -----------------------------------------------------------------------------


/**
 *	This class is an event poller based on the poll() system call available on 
 *	Linux (though in this case we use EPoller below), BSD, and Emscripten.
 */
class PollPoller : public EventPoller
{
public:
	/**
	 *	Constructor.
	 */
	PollPoller() : 
			EventPoller(),
			container_( 16 ),
			fdMap_()
	{}

	/**
	 *	Destructor.
	 */
	virtual ~PollPoller()
	{}


	// Overrides from EventPoller
	virtual bool doRegisterForRead( int fd, const char * name );
	virtual bool doRegisterForWrite( int fd, const char * name );
	virtual bool doDeregisterForRead( int fd );
	virtual bool doDeregisterForWrite( int fd );
	virtual int processPendingEvents( double maxWait );


private:
	bool doRegister( int fd, bool isRegister, int event );
	void resize();

	typedef BW::vector< pollfd > Container;
	Container container_;

	typedef BW::map< int, size_t > FDMap;
	FDMap fdMap_;
};


/*
 *	Override from EventPoller.
 */
bool PollPoller::doRegisterForRead( int fd, const char * name )
{
	return this->doRegister( fd, true, POLLIN );
}


/*
 *	Override from EventPoller.
 */
bool PollPoller::doRegisterForWrite( int fd, const char * name )
{
	return this->doRegister( fd, true, POLLOUT );
}


/*
 *	Override from EventPoller.
 */
bool PollPoller::doDeregisterForRead( int fd )
{
	return this->doRegister( fd, false, POLLIN );
}


/*
 *	Override from EventPoller.
 */
bool PollPoller::doDeregisterForWrite( int fd )
{
	return this->doRegister( fd, false, POLLOUT );
}


namespace // anonymous
{

/**
 *	Convert poll() event codes to human-readable strings.
 *
 *	@param event 	The event code.
 *
 *	@return 		A human-readable string describing the given code.
 */
const char * eventToCString( int event )
{
	switch (event)
	{
	case POLLIN:
		return "POLLIN";
	case POLLOUT:
		return "POLLOUT";
	case POLLERR:
		return "POLLERR";
	case POLLHUP:
		return "POLLHUP";
	case POLLNVAL:
		return "POLLNVAL";
	default:
		return "(unknown event)";
	}
}


/**
 *	Return a descriptive string of all the event codes present in the given 
 *	event descriptor.
 *
 *	@param event 	The given event descriptor.
 *
 *	@return 		A string describing each set flag in the event descriptor.
 */
std::string eventsToCString( int event )
{
	std::ostringstream oss;
	int pollEvents[] = {POLLIN, POLLOUT, POLLERR, POLLHUP, POLLNVAL, 0};
	int * pPollEvent = pollEvents;

	while (*pPollEvent != 0)
	{
		if (event & *pPollEvent)
		{
			if (oss.str().size() > 0)
			{
				oss << " | ";
			}
			oss << eventToCString( *pPollEvent );
		}
		++pPollEvent;
	}

	return oss.str();
}

} // end namespace (anonymous)


/**
 *	This method registers / deregisters the given file descriptor for the given
 *	event.
 *
 *	@param fd 			The file descriptor to poll.
 *	@param isRegister 	Whether this is a registration or a deregistration.
 *	@param event 		The event code(s) to register/unregister for.
 *
 *	@return 			True on success, false otherwise.
 */ 
bool PollPoller::doRegister( int fd, bool isRegister, int event )
{
	FDMap::iterator iMapping = fdMap_.find( fd );

	if (iMapping == fdMap_.end())
	{
		if (!isRegister)
		{
			return false;
		}

		fdMap_[fd] = container_.size();

		pollfd pollData = { fd, event, 0 };
		container_.push_back( pollData );
	}
	else
	{
		pollfd & pollData = container_[iMapping->second];
		if (isRegister)
		{
			pollData.events |= event;
		}
		else
		{
			pollData.events &= ~event;
		}

		if (pollData.events == 0)
		{
			fdMap_.erase( iMapping );

			// If there are 50% empty container slots, then resize.

			if (container_.size() > 2 * fdMap_.size())
			{
				this->resize();
			}
		}
	}

	return true;
}


/**
 *	This method resizes the poll container.
 */
void PollPoller::resize()
{
	Container newContainer( fdMap_.size() );
	
	for (FDMap::iterator iMapping = fdMap_.begin();
			iMapping != fdMap_.end();
			++iMapping)
	{
		size_t newIndex = newContainer.size();
		newContainer.push_back( container_[iMapping->second] );
		iMapping->second = newIndex;
	}

	container_.swap( newContainer );
}


/*
 *	Override from EventPoller.
 */
int PollPoller::processPendingEvents( double maxWait )
{
#if ENABLE_WATCHERS
	static ProfileVal g_idleProfile( "Idle" );
	g_idleProfile.start();
#else // !ENABLE_WATCHERS
	uint64 startTime = timestamp();
#endif // ENABLE_WATCHERS

#if defined( EMSCRIPTEN )
	maxWait = 0.0;
#endif

	for (Container::iterator iter = container_.begin();
			iter != container_.end();
			++iter)
	{
		iter->revents = 0;
	}

	int pollResult = poll( &(container_.front()), container_.size(),
		int( maxWait * 1000 ) );

#if ENABLE_WATCHERS
	g_idleProfile.stop();
	spareTime_ += g_idleProfile.lastTime_;
#else // !ENABLE_WATCHERS
	spareTime_ += timestamp() - startTime;
#endif // ENABLE_WATCHERS

	if (pollResult < 0 && errno != EINTR)
	{
		ERROR_MSG( "poll() returned -1: %s\n", strerror( errno ) );
		return -1;
	}

	if (pollResult == 0)
	{
		return pollResult;
	}

	// fdMap_ and container_ may be modified while we trigger notifications.
	FDMap copy( fdMap_ );
	Container container( container_ );

	int numSocketsLeft = pollResult;
	for (Container::iterator iter = container.begin();
			(iter != container.end()) && (numSocketsLeft > 0);
			++iter)
	{
		pollfd & poll = *iter;

		if (!poll.revents)
		{
			continue;
		}

		--numSocketsLeft;

		if (poll.revents & (POLLERR | POLLHUP))
		{
			this->triggerError( poll.fd );
		}
		else
		{
			if (poll.revents & POLLIN)
			{
				this->triggerRead( poll.fd );
			}

			if (poll.revents & POLLOUT)
			{
				this->triggerWrite( poll.fd );
			}
		}

		poll.revents = 0;
	}

	return pollResult;
}


#endif  // defined( HAS_POLL )


// -----------------------------------------------------------------------------
// Section: EPoller
// -----------------------------------------------------------------------------


#if defined( HAS_EPOLL )


/**
 *	This class is an EventPoller that uses epoll.
 */
class EPoller : public EventPoller
{
public:
	EPoller( int expectedSize = 10 );
	virtual ~EPoller();

	int getFileDescriptor() const { return epfd_; }

protected:
	virtual bool doRegisterForRead( int fd, const char * name )
		{ return this->doRegister( fd, true, true, name ); }

	virtual bool doRegisterForWrite( int fd, const char * name )
		{ return this->doRegister( fd, false, true, name ); }

	virtual bool doDeregisterForRead( int fd )
		{ return this->doRegister( fd, true, false, NULL ); }

	virtual bool doDeregisterForWrite( int fd )
		{ return this->doRegister( fd, false, false, NULL ); }

	virtual int processPendingEvents( double maxWait );

	bool doRegister( int fd, bool isRead, bool isRegister, const char * name );

private:
	// epoll file descriptor
	int epfd_;
};


/**
 *	Constructor.
 */
EPoller::EPoller( int expectedSize ) :
	epfd_( epoll_create( expectedSize ) )
{
	if (epfd_ == -1)
	{
		CRITICAL_MSG( "EPoller::EPoller: epoll_create failed: %s\n",
				strerror( errno ) );
	}
};


/**
 *	Destructor.
 */
EPoller::~EPoller()
{
	if (epfd_ != -1)
	{
		close( epfd_ );
	}
}


/**
 *	This method does the work registering and deregistering file descriptors
 *	from the epoll descriptor.
 */
bool EPoller::doRegister( int fd, bool isRead, bool isRegister,
		const char * name )
{
	struct epoll_event ev;
	memset( &ev, 0, sizeof( ev ) ); // stop valgrind warning
	int op;

	ev.data.fd = fd;

	// Handle the case where the file is already registered for the opposite
	// action.
	if (this->isRegistered( fd, !isRead ))
	{
		op = EPOLL_CTL_MOD;

		ev.events = isRegister ? EPOLLIN | EPOLLOUT :
					isRead ? EPOLLOUT : EPOLLIN;
	}
	else
	{
		// TODO: Could be good to use EPOLLET (leave like select for now).
		ev.events = isRead ? EPOLLIN : EPOLLOUT;
		op = isRegister ? EPOLL_CTL_ADD : EPOLL_CTL_DEL;
	}

	if (epoll_ctl( epfd_, op, fd, &ev ) < 0)
	{
		char message[ 512 ];

		bw_snprintf( message, sizeof( message ),
				"EPoller::doRegister: Failed to %s %s file "
				"descriptor %s %d (%s)\n",
			isRegister ? "add" : "remove",
			isRead ? "read" : "write",
			name == NULL ? "" : name, fd,
			strerror( errno ) );

		if (errno == ENOENT)
		{
			WARNING_MSG( "%s", message );
		}
		else
		{
			ERROR_MSG( "%s", message );
		}

		return false;
	}

	return true;
}


/*
 *	Override from EventPoller.
 */
int EPoller::processPendingEvents( double maxWait )
{
	static const int MAX_EVENTS = 10;

	struct epoll_event events[ MAX_EVENTS ];

	int maxWaitInMilliseconds = int( ceil( maxWait * 1000 ) );

#if ENABLE_WATCHERS
	static ProfileVal g_idleProfile( "Idle" );
	g_idleProfile.start();
#else // !ENABLE_WATCHERS
	uint64 startTime = timestamp();
#endif // ENABLE_WATCHERS

	BWConcurrency::startMainThreadIdling();
	int nfds = 0;
	{
		PROFILER_IDLE_SCOPED( Idle );
		nfds = epoll_wait( epfd_, events, MAX_EVENTS, maxWaitInMilliseconds );
	}
	BWConcurrency::endMainThreadIdling();

#if ENABLE_WATCHERS
	g_idleProfile.stop();
	spareTime_ += g_idleProfile.lastTime_;
#else // !ENABLE_WATCHERS
	spareTime_ += timestamp() - startTime;
#endif // ENABLE_WATCHERS

	for (int i = 0; i < nfds; ++i)
	{
		if (events[i].events & (EPOLLERR | EPOLLHUP))
		{
			this->triggerError( events[i].data.fd );
		}
		else
		{
			if (events[i].events & EPOLLIN)
			{
				this->triggerRead( events[i].data.fd );
			}

			if (events[i].events & EPOLLOUT)
			{
				this->triggerWrite( events[i].data.fd );
			}
		}
	}

	return nfds;
}

#endif // defined( HAS_EPOLL )


// -----------------------------------------------------------------------------
// Section: EventPoller::create
// -----------------------------------------------------------------------------

/**
 *	This static method creates an appropriate EventPoller. It may use select or
 *	epoll.
 */
EventPoller * EventPoller::create()
{
#if defined( HAS_POLL )
	return new PollPoller();

#elif defined( HAS_EPOLL )
	return new EPoller();

#else // !defined( HAS_EPOLL )
	return new SelectPoller();

#endif // defined( HAS_EPOLL )
}

#if ENABLE_WATCHERS
/**
 * 	This method returns the generic watcher for the EventPoller.
 */
WatcherPtr EventPoller::pWatcher()
{
	static DirectoryWatcherPtr watchMe = NULL;
	typedef SmartPointer< MapWatcher< FDHandlers > > MapWatcherPtr;

	if (watchMe == NULL)
	{
		EventPoller * pNull = NULL;

		watchMe = new DirectoryWatcher();

		DirectoryWatcherPtr pFDWatcher = new DirectoryWatcher();
		pFDWatcher->addChild( "name", makeWatcher( &InputHandlerEntry::name ) );
		pFDWatcher->addChild( "triggers", makeWatcher( &InputHandlerEntry::numTriggers ) );
		pFDWatcher->addChild( "errors", makeWatcher( &InputHandlerEntry::numErrors ) );

		watchMe->addChild( "fd",
			makeWatcher( &EventPoller::getFileDescriptor ) );

		MapWatcherPtr pReadFDs = 
			new MapWatcher<FDHandlers>( pNull->fdReadHandlers_ );
		pReadFDs->addChild( "*", pFDWatcher );
		watchMe->addChild( "readHandlers", pReadFDs );

		MapWatcherPtr pWriteFDs = 
			new MapWatcher<FDHandlers>( pNull->fdWriteHandlers_ );
		pWriteFDs->addChild( "*", pFDWatcher );
		watchMe->addChild( "writeHandlers", pWriteFDs );
	}

	return watchMe;
}
#endif


} // namespace Mercury


BW_END_NAMESPACE


// event_poller.cpp
