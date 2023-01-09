#include "pch.hpp"

#include "channel.hpp"

#include "bundle.hpp"
#include "channel_listener.hpp"
#include "event_dispatcher.hpp"
#include "interfaces.hpp"
#include "network_interface.hpp"

#include "cstdmf/timestamp.hpp"
#if ENABLE_WATCHERS
#include "cstdmf/watcher.hpp"
#endif // ENABLE_WATCHERS

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace // (anonymous)
{

/**
 *	This class is responsible for checking for inactivity on a channel, and
 *	informing the channel.
 */
class InactivityTimeoutChecker : public TimerHandler
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param channel 						The channel to monitor.
	 *	@param inactivityExceptionPeriod	How long since the last received
	 *										time before a channel is declared
	 *										to have timed out.
	 */
	InactivityTimeoutChecker( Channel & channel, 
			uint64 inactivityExceptionPeriod ) :
		TimerHandler(),
		channel_( channel ),
		inactivityExceptionPeriod_( inactivityExceptionPeriod )
	{}

	/**  Destructor. */
	virtual ~InactivityTimeoutChecker() {}

	/*
 	 *	Override from TimerHandler.
	 */
	virtual void handleTimeout( TimerHandle handle, void * pUser )
	{
		if ((timestamp() - channel_.lastReceivedTime()) > 
				inactivityExceptionPeriod_)
		{
			channel_.onChannelInactivityTimeout();
		}
	}

protected:
	/*
	 *	Override from TimerHandler.
	 */
	virtual void onRelease( TimerHandle handle, void * pUser )
	{
		delete this;
	}


private:
	Channel & channel_;
	uint64 inactivityExceptionPeriod_;

};

} // end namespace (anonymous)


/**
 *	Constructor.
 *
 *	@param networkInterface 	The network interface.
 *	@param addr 				The address of the peer this channel connects
 *								to.
 */
Channel::Channel( NetworkInterface & networkInterface, 
			const Address & addr ):
#if ENABLE_WATCHERS
		WatcherProvider(),
#endif // ENABLE_WATCHERS
		ReferenceCount(),
		addr_( addr ),
		pBundle_( NULL ),
		pBundlePrimer_( NULL ),
		pMessageFilter_( NULL ),
		pNetworkInterface_( &networkInterface ),
		isDestroyed_( false ),
		inactivityTimerHandle_(),
		lastReceivedTime_( 0 ),
		pListener_( NULL ),
		userData_( NULL ),
		numDataUnitsSent_( 0 ),
		numDataUnitsReceived_( 0 ),
		numBytesSent_( 0 ),
		numBytesReceived_( 0 )
{
	// This corresponds to the decRef in Channel::destroy.
	this->incRef();
}


/**
 *	Destructor.
 */
Channel::~Channel()
{
	MF_ASSERT_DEV( isDestroyed_ );

	bw_safe_delete( pBundle_ );
}


/**
 *  This method sets the BundlePrimer object for this channel.  If the
 *  channel's bundle is empty, it will be primed.
 *
 *	@param pPrimer 	The bundle primer, or NULL to remove the bundle primer.
 */
void Channel::bundlePrimer( BundlePrimer * pPrimer )
{
	pBundlePrimer_ = pPrimer;

	if (pBundlePrimer_ && pBundle_->numMessages() == 0)
	{
		pBundlePrimer_->primeBundle( *pBundle_ );
	}
}


/**
 *	This method sends the given bundle on this channel. If no bundle is
 *	supplied, the channel's own bundle will be sent.
 *
 *	@param pBundle 	The bundle to send, or NULL to send the channel's own
 *					bundle.
 */
void Channel::send( Bundle * pBundle /* = NULL */ )
{
	ChannelPtr pChannel( this );

	if (!this->isConnected())
	{
		ERROR_MSG( "Channel::send( %s ): Channel is not connected\n",
			this->c_str() );
		return;
	}

	if (pBundle == NULL)
	{
		pBundle = pBundle_;
	}

	this->doPreFinaliseBundle( *pBundle );

	pBundle->finalise();

	this->networkInterface().addReplyOrdersTo( *pBundle, this );

	this->doSend( *pBundle );

	// Clear the bundle
	if (pBundle == pBundle_)
	{
		this->clearBundle();
	}
	else
	{
		pBundle->clear();
	}

	if (pListener_)
	{
		pListener_->onChannelSend( *this );
	}
}


/**
 *  This method clears the bundle on this channel and gets it ready to have a
 *  new set of messages added to it.
 */
void Channel::clearBundle()
{
	if (!pBundle_)
	{
		pBundle_ = this->newBundle();
	}
	else
	{
		pBundle_->clear();
	}

	// If we have a bundle primer, now's the time to call it!
	if (pBundlePrimer_)
	{
		pBundlePrimer_->primeBundle( *pBundle_ );
	}
}


/**
 *	This method "destroys" this channel. It should be considered similar to
 *	delete pChannel except that there may be other references remaining.
 */
void Channel::destroy()
{
	IF_NOT_MF_ASSERT_DEV( !isDestroyed_ )
	{
		return;
	}

	inactivityTimerHandle_.cancel();
	this->doDestroy();

	isDestroyed_ = true;

	pNetworkInterface_->onChannelGone( this );

	if (this->pChannelListener())
	{
		this->pChannelListener()->onChannelGone( *this );
	}

	this->decRef();
}


/**
 *	This method returns this channel's network interface event dispatcher.
 *
 *	@return 	The network interface event dispatcher.
 */
EventDispatcher & Channel::dispatcher()
{
	return pNetworkInterface_->dispatcher();
}


/**
 *	This method starts detection of inactivity on this channel. If nothing is
 *	received for the input period amount of time, an INACTIVITY error is
 *	generated.
 *
 *	@param period		The number of seconds without receiving a packet before
 *						the channel is considered inactive.
 *	@param checkPeriod 	The number of seconds between checking for inactivity.
 */
void Channel::startInactivityDetection( float period, float checkPeriod )
{
	inactivityTimerHandle_.cancel();

	uint64 inactivityExceptionPeriod = uint64( period * stampsPerSecond() );
	lastReceivedTime_ = timestamp();

	inactivityTimerHandle_ = this->dispatcher().addTimer( 
			int( checkPeriod * 1000000 ),
			new InactivityTimeoutChecker( *this, inactivityExceptionPeriod ), 
			0, "ChannelInactivity" );
}


/**
 *	This method returns the locally bound address.
 */
const Address & Channel::localAddress() const
{
	return pNetworkInterface_->address();
}


#if ENABLE_WATCHERS
/**
 *	This method creates a new directory watcher with the standard Channel-level
 *	watchers.
 */
WatcherPtr Channel::createBaseWatcher()
{
	WatcherPtr pWatcher( new DirectoryWatcher() );

#define ADD_WATCHER( PATH, MEMBER )		\
		pWatcher->addChild( #PATH, makeWatcher( &Channel::MEMBER ) );
	ADD_WATCHER( addressRemote,			addr );
	ADD_WATCHER( addressLocal,			localAddress );
	ADD_WATCHER( description,			c_str );
	ADD_WATCHER( isTCP, 				isTCP );
	ADD_WATCHER( isExternal, 			isExternal );
	ADD_WATCHER( isConnected, 			isConnected );
	ADD_WATCHER( roundTripTime, 		roundTripTimeInSeconds );
	ADD_WATCHER( timeSinceLastRecieved, timeSinceLastReceived );
	ADD_WATCHER( numDataUnitsSent,		numDataUnitsSent_ );
	ADD_WATCHER( numDataUnitsReceived,	numDataUnitsReceived_ );
	ADD_WATCHER( numBytesSent,			numBytesSent_ );
	ADD_WATCHER( numBytesReceived,		numBytesReceived_ );
#undef ADD_WATCHER

	return pWatcher;
}


#endif // ENABLE_WATCHERS

} // end namespace Mercury


BW_END_NAMESPACE

// channel.cpp

