#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "cstdmf/polymorphic_watcher.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/timer_handler.hpp"

#include "basictypes.hpp"
#include "message_filter.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/timestamp.hpp"

BW_BEGIN_NAMESPACE

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;


namespace Mercury
{

class BlockCipher;
typedef SmartPointer< BlockCipher > BlockCipherPtr;
class Bundle;
class BundlePrimer;
class ChannelListener;
class EventDispatcher;
class NetworkInterface;


/**
 *	This abstract class represents a Mercury Channel between two processes.
 */
class Channel : 
#if ENABLE_WATCHERS
	public WatcherProvider, 
#endif // ENABLE_WATCHERS
	public ReferenceCount
{
public:

	// Override from WatcherProvider.
#if ENABLE_WATCHERS
	virtual WatcherPtr getWatcher() = 0;
#endif // ENABLE_WATCHERS

	/**
	 *	This method returns a new on-channel bundle for this channel.
	 */
	virtual Bundle * newBundle() = 0;


	/**
	 *	This method returns a string representation of this channel.
	 */
	virtual const char * c_str() const = 0;

	/**
	 *	This method returns if there is any data pending to be sent on this 
	 *	channel.
	 */
	virtual bool hasUnsentData() const = 0;

	/**
	 *	This method returns true if this is a server-client channel.
	 */
	virtual bool isExternal() const = 0;

	/**
	 *	This method returns whether this channel is usable for sending.
	 *	This method may return false when the channel is destroyed, or is being
	 *	shut down.
	 */
	virtual bool isConnected() const
	{
		return !this->isDestroyed();
	}

	/**
	 *	This method starts a clean shutdown of this channel.
	 *	Subclasses can use this to implement clean shutdown procedures.
	 */
	virtual void shutDown()
	{
		this->destroy();
	}


	/**
	 *	This method is called when the inactivity detection is enabled no
	 *	activity on the channel has been detected for the inactivity period.
	 */
	virtual void onChannelInactivityTimeout()
	{
		this->destroy();
	}


	/**
	 *	This method returns whether this is a TCP-based Mercury channel.
	 *	TODO: remove the need for this.
	 */
	virtual bool isTCP() const = 0;


	/**
	 *	This method sets the block cipher to use for encryption on this
	 *	channel.
	 *
	 *	@param pBlockCipher 	The block cipher to use for encryption.
	 */
	virtual void setEncryption( Mercury::BlockCipherPtr pBlockCipher ) = 0;


	/**
	 *	This method returns the round trip time in seconds for this channel.
	 */
	virtual double roundTripTimeInSeconds() const = 0;


	/**
	 *	This method returns the address of the connected peer.
	 */
	const Address & addr() const 		{ return addr_; }
	const Address & localAddress() const;

	/**
	 *	This method returns the channel's bundle.
	 */
	Bundle & bundle() 					{ return *pBundle_; }

	/**
	 *	This method returns the channel's bundle.
	 */
	const Bundle & bundle() const		{ return *pBundle_; }

	/**
	 *	This method returns the channel's associated network interface.
	 */
	NetworkInterface & networkInterface() { return *pNetworkInterface_; }

	void send( Bundle * pBundle = NULL ); 

	void clearBundle();
	void bundlePrimer( BundlePrimer * pPrimer );

	void destroy();

	/**
	 *	This method returns whether this channel has been destroyed.
	 */
	bool isDestroyed() const { return isDestroyed_; }

	/**
	 *	Set the channel's message filter to be a new reference to the given
	 *	message filter, releasing any reference to any previous message filter.
	 *
	 *	@param pMessageFilter 	The new message filter.
	 */
	void pMessageFilter( MessageFilter * pMessageFilter )
	{
		pMessageFilter_ = pMessageFilter;
	}

	/**
	 *	Return a new reference to the message filter for this channel.
	 *
	 *	@return 	The message filter.
	 */
	MessageFilterPtr pMessageFilter()
	{
		return pMessageFilter_;
	}

	/**
	 *	This method returns the timestamp of the last time data was received on
	 *	this channel. 
	 *
	 *	@return The last received time, in stamps.
	 */
	uint64 lastReceivedTime() const { return lastReceivedTime_; }

	/**
	 *	This method returns the number of seconds since the last receive.
	 */
	double timeSinceLastReceived() const 
	{
		return (timestamp() - lastReceivedTime_) / stampsPerSecondD();
	}

	virtual void startInactivityDetection( float period,
		float checkPeriod = 1.f );

	/**
	 *	This method sets the channel listener for this channel.
	 *
	 *	@param pListener 	The channel listener, or NULL to clear the
	 *						listener.
	 */
	void pChannelListener( ChannelListener * pListener ) 
	{ 
		pListener_ = pListener; 
	}

	/**
	 *	This method returns the channel listener for this channel.
	 */
	ChannelListener * pChannelListener()
	{ 
		return pListener_ ;
	}

	/**
	 *	Set the application user data for this channel.
	 *
	 *	@param userData 	The user data to set.
	 */
	void userData( void * userData ) { userData_ = userData; }
	
	/**
	 *	Return the application user data for this channel.
	 *
	 *	@return 	The user data.
	 */
	void * userData() { return userData_; }

	/**
	 *	This method returns the number of packets sent on this channel. It does
	 *	not include resends.
	 */
	uint32	numDataUnitsSent() const		{ return numDataUnitsSent_; }

	/**
	 *	This method returns the number of packets received on this channel.
	 */
	uint32	numDataUnitsReceived() const	{ return numDataUnitsReceived_; }

	/**
	 *	This method returns the number of bytes sent on this channel. It does
	 *	not include bytes sent by resends.
	 */
	uint32	numBytesSent() const		{ return numBytesSent_; }

	/**
	 *	This method returns the number of bytes received by this channel.
	 */
	uint32	numBytesReceived() const	{ return numBytesReceived_; }

protected:
	Channel( NetworkInterface & networkInterface, const Address & addr );
	virtual ~Channel();

	/** 
	 *	This method is called when the given bundle is about to be finalised.
	 *	Subclasses can use this to manipulate the bundle before it is finalised
	 *	for sending.
	 *
	 *	@param bundle 	The bundle that is about to be finalised.
	 */
	virtual void doPreFinaliseBundle( Bundle & bundle ) {}

	/**
	 *	This method implements the sending of the given bundle on this channel.
	 *
	 *	@param bundle 	The bundle to send.
	 */
	virtual void doSend( Bundle & bundle ) = 0;

	/**
	 *	This method is called to destroy the channel. Subclasses should use
	 *	this to clean up their state.
	 */
	virtual void doDestroy() {}

	EventDispatcher & dispatcher();

#if ENABLE_WATCHERS
	WatcherPtr createBaseWatcher();
#endif // ENABLE_WATCHERS

// Member data
	Address 			addr_;

	Bundle * 			pBundle_;

	BundlePrimer * 		pBundlePrimer_;
	MessageFilterPtr 	pMessageFilter_;
	
	NetworkInterface * 	pNetworkInterface_;

	bool 				isDestroyed_;

	TimerHandle 		inactivityTimerHandle_;
	uint64 				lastReceivedTime_;

	ChannelListener *	pListener_;
	void * 				userData_;

	// Statistics
	uint32				numDataUnitsSent_;
	uint32				numDataUnitsReceived_;
	uint32				numBytesSent_;
	uint32				numBytesReceived_;
};

typedef SmartPointer< Channel > ChannelPtr;

} // end namespace Mercury


BW_END_NAMESPACE

#endif // CHANNEL_HPP

