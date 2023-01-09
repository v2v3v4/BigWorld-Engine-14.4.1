#ifndef EVENT_POLLER_HPP
#define EVENT_POLLER_HPP

#include "cstdmf/stdmf.hpp"
#include "network/interfaces.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/profiler.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class InputNotificationHandler;

class InputHandlerEntry
{
	static const uint32 INFORM_COUNT = 100000;
public:
	InputHandlerEntry() : pHandler_( NULL ), name_( ), numTriggers_( 0 ),
		numErrors_( 0 )
	{
	}

	InputHandlerEntry( InputNotificationHandler * pHandler,
			const char * name ) :
		pHandler_( pHandler ),
		name_( name ),
		numTriggers_( 0 ),
		numErrors_( 0 )
	{}

	int handleInputNotification( int fd )
	{
		PROFILER_SCOPED_DYNAMIC_STRING( name_.c_str() );
		++numTriggers_;

		if ((numTriggers_ % INFORM_COUNT) == 0)
		{
			INFO_MSG( "InputHandlerEntry::handleInputNotification: "
				"The fd %d (%s) has triggered %d times\n", fd, name_.c_str(), 
				numTriggers_ );
		}

		return pHandler_->handleInputNotification( fd );
	}

	bool handleErrorNotification( int fd )
	{
		PROFILER_SCOPED_DYNAMIC_STRING( name_.c_str() );
		++numErrors_;

		if ((numErrors_ % INFORM_COUNT) == 0)
		{
			INFO_MSG( "InputHandlerEntry::handleErrorNotification: "
				"The fd %d (%s) has triggered %d times\n", fd, name_.c_str(),
				numErrors_ );
		}
		
		return pHandler_->handleErrorNotification( fd );
	}

	bool isReadyForShutdown() const
	{
		return pHandler_->isReadyForShutdown();
	}

	const char * name() const
	{
		return name_.c_str();
	}

	int numTriggers() const
	{
		return numTriggers_;
	}

	int numErrors() const
	{
		return numErrors_;
	}
private:
	InputNotificationHandler * pHandler_;
	BW::string name_;
	uint32 numTriggers_;
	uint32 numErrors_;
};

typedef BW::map< int, InputHandlerEntry > FDHandlers;


class EventPoller : public InputNotificationHandler
{
public:
	EventPoller();
	virtual ~EventPoller();

	bool isReadyForShutdown() const;

	bool registerForRead( int fd, InputNotificationHandler * handler,
				const char * name );
	bool registerForWrite( int fd, InputNotificationHandler * handler,
				const char * name );

	bool deregisterForRead( int fd );
	bool deregisterForWrite( int fd );

	/**
	 *	This method polls the registered file descriptors for events.
	 *
	 *	@param maxWait The maximum number of seconds to wait for events.
	 */
	virtual int processPendingEvents( double maxWait ) = 0;

	virtual int getFileDescriptor() const;

	// Override from InputNotificationHandler.
	virtual int handleInputNotification( int fd );

	void clearSpareTime()		{ spareTime_ = 0; }
	uint64 spareTime() const	{ return spareTime_; }

	static EventPoller * create();

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

protected:
	virtual bool doRegisterForRead( int fd, const char * name ) = 0;
	virtual bool doRegisterForWrite( int fd, const char * name ) = 0;

	virtual bool doDeregisterForRead( int fd ) = 0;
	virtual bool doDeregisterForWrite( int fd ) = 0;

	bool triggerRead( int fd )	{ return this->trigger( fd, fdReadHandlers_ ); }
	bool triggerWrite( int fd )	{ return this->trigger( fd, fdWriteHandlers_ ); }
	bool triggerError( int fd );

	bool trigger( int fd, FDHandlers & handlers );

	bool isRegistered( int fd, bool isForRead ) const;

	int maxFD() const;

private:
	static int maxFD( const FDHandlers & handlerMap );

	// Maps from file descriptor to their callbacks
	FDHandlers fdReadHandlers_;
	FDHandlers fdWriteHandlers_;

protected:
	uint64 spareTime_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // EVENT_POLLER_HPP
