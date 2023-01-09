#include "pch.hpp"

#include "network_interface.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE
#include "network_interface.ipp"
BW_END_NAMESPACE

#endif

#include "channel_finder.hpp"
#include "channel_map.hpp"
#include "condemned_channels.hpp"
#include "delayed_channels.hpp"
#include "event_dispatcher.hpp"
#include "error_reporter.hpp"
#include "interface_table.hpp"
#include "irregular_channels.hpp"
#include "keepalive_channels.hpp"
#include "machined_utils.hpp"
#include "network_utils.hpp"
#include "packet_receiver.hpp"
#include "packet_sender.hpp"
#include "packet_loss_parameters.hpp"
#include "recently_dead_channels.hpp"
#include "request_manager.hpp"
#include "rescheduled_sender.hpp"
#include "udp_bundle.hpp"


#ifdef PLAYSTATION3
#include "ps3_compatibility.hpp"
#endif

#if defined( __linux__ ) && !defined( EMSCRIPTEN )
#include <sys/syscall.h>
#include <linux/sysctl.h>
#endif


BW_BEGIN_NAMESPACE

namespace Mercury
{



// -----------------------------------------------------------------------------
// Section: Constants
// -----------------------------------------------------------------------------

const char * NetworkInterface::USE_BWMACHINED = "bwmachined";

// -----------------------------------------------------------------------------
// Section: Statics
// -----------------------------------------------------------------------------


/**
 *  How much buffer we want for sockets.
 */
int NetworkInterface::recvBufferSize_ = getMaxBufferSize( 
											/*isReadBuffer:*/ true );
int NetworkInterface::sendBufferSize_ = getMaxBufferSize( 
											/*isReadBuffer:*/ false );


// -----------------------------------------------------------------------------
// Section: Construction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
NetworkInterface::NetworkInterface( Mercury::EventDispatcher * pMainDispatcher,
		NetworkInterfaceType networkInterfaceType,
		uint16 listeningPort, const char * listeningInterface ) :
	udpSocket_(),
	address_( Address::NONE ),
	pPacketSender_( NULL ),
	pPacketReceiver_( NULL ),
	pPacketLossParameters_( new PacketLossParameters() ),
	rateLimitPeriod_( 0.f ),
	rateLimitPerIPAddress_( 0 ),
	rateLimitedIPAddresses_(),
	ipAddressRateLimitAverage_( EMA::calculateBiasFromNumSamples( 5 ) ),
	rateLimitPerIPAddressPort_( 0 ),
	rateLimitedIPAddressPorts_(),
	ipAddressPortRateLimitAverage_( EMA::calculateBiasFromNumSamples( 5 ) ),
	pRecentlyDeadChannels_( NULL ),
	pChannelMap_( new ChannelMap() ),
	pIrregularChannels_( new IrregularChannels() ),
	pCondemnedChannels_( new CondemnedChannels() ),
	pKeepAliveChannels_( new KeepAliveChannels() ),
	pDelayedChannels_( new DelayedChannels() ),
	pChannelFinder_( NULL ),
	isExternal_( networkInterfaceType == NETWORK_INTERFACE_EXTERNAL ),
	verbosityLevel_( VERBOSITY_LEVEL_DEBUG ),
	pDispatcher_( new EventDispatcher ),
	pMainDispatcher_( NULL ),
	pInterfaceTable_( new InterfaceTable( *pDispatcher_ ) ),
	pOffChannelFilter_(),
	pRequestManager_( new RequestManager( isExternal_, 
		*pDispatcher_ ) ),
	pExtensionData_( NULL ),
	pOnceOffSender_( new OnceOffSender() ),
	sendingStats_()
{
	pPacketSender_ =
		new PacketSender( udpSocket_, *pRequestManager_, *pDispatcher_,
				*pOnceOffSender_, sendingStats_, *pPacketLossParameters_ );
	pPacketReceiver_ = new PacketReceiver( udpSocket_, *this );

	// This registers the file descriptor and so needs to be done after
	// initialising fdReadSet_ etc.

#if !defined( EMSCRIPTEN )
	this->recreateListeningSocket( listeningPort, listeningInterface );
#endif
	if (pMainDispatcher != NULL)
	{
		this->attach( *pMainDispatcher );
	}

	// and put our request manager in as the reply handler
	this->interfaceTable().serve( InterfaceElement::REPLY, pRequestManager_ );
}


/**
 *	This method initialises this object.
 */
void NetworkInterface::attach( EventDispatcher & mainDispatcher )
{
	MF_ASSERT( pMainDispatcher_ == NULL );

	pMainDispatcher_ = &mainDispatcher;

	mainDispatcher.attach( this->dispatcher() );

	pDelayedChannels_->init( this->mainDispatcher() );
	sendingStats_.init( this->mainDispatcher() );

	if (isExternal_)
	{
		MF_ASSERT( !pRecentlyDeadChannels_ );
		pRecentlyDeadChannels_ = new RecentlyDeadChannels( mainDispatcher );
	}
}


/**
 *	This method prepares this NetworkInterface for being shut down.
 */
void NetworkInterface::prepareForShutdown()
{
	if (pMainDispatcher_)
	{
		pMainDispatcher_->prepareForShutdown();
	}

	this->finaliseRequestManager();
}


/**
 *	This method destroys the RequestManager. It should be called when shutting
 *	down to ensure that no ReplyMessageHandler instances are cancelled when the
 *	server is in a bad state (e.g. after the App has been destroyed.)
 */
void NetworkInterface::finaliseRequestManager()
{
	this->interfaceTable().serve( InterfaceElement::REPLY, NULL );

	RequestManager * pRequestManager = pRequestManager_;
	pRequestManager_ = NULL;
	bw_safe_delete( pRequestManager );
}


/**
 *	Destructor.
 */
NetworkInterface::~NetworkInterface()
{
	this->interfaceTable().deregisterWithMachined( this->address() );

	// This cancels outstanding requests. Need to make sure no more are added.
	this->finaliseRequestManager();

	pChannelMap_->destroyOwnedChannels();

	this->detach();

	this->closeSocket();

	bw_safe_delete( pDelayedChannels_ );
	bw_safe_delete( pIrregularChannels_ );
	bw_safe_delete( pKeepAliveChannels_ );
	bw_safe_delete( pCondemnedChannels_ );
	bw_safe_delete( pOnceOffSender_ );
	bw_safe_delete( pInterfaceTable_ );
	bw_safe_delete( pPacketSender_ );
	bw_safe_delete( pPacketReceiver_ );
	bw_safe_delete( pPacketLossParameters_ );
	bw_safe_delete( pDispatcher_ );
	bw_safe_delete( pChannelMap_ );
	bw_safe_delete( pRecentlyDeadChannels_ );
}


/**
 *	This method disassociates this interface from a dispatcher.
 */
void NetworkInterface::detach()
{
	if (pMainDispatcher_ != NULL )
	{
		sendingStats_.fini();
		pDelayedChannels_->fini( this->mainDispatcher() );

		pMainDispatcher_->detach( this->dispatcher() );

		// May be NULL already
		bw_safe_delete( pRecentlyDeadChannels_ );

		pMainDispatcher_ = NULL;
	}
}


/**
 *	This method prepares this network interface for shutting down. It will stop
 *	pinging anonymous channels.
 */
void NetworkInterface::stopPingingAnonymous()
{
	this->keepAliveChannels().stopMonitoring( this->dispatcher() );
}


/**
 *  This method processes events until all registered channels have no unacked
 *  packets, and there are no more unacked once-off packets, or the given
 *  timeout period expires.
 *
 *	@param timeout 	The timeout period.
 */
void NetworkInterface::processUntilChannelsEmpty( float timeout )
{
	PROFILER_SCOPED( processUntilChannelsEmpty );
	bool done = false;
	const uint64 startTime = timestamp();
	uint64 timeoutInStamps = TimeStamp::fromSeconds( timeout );

	while (!done &&
			((timestamp() - startTime) < timeoutInStamps))
	{
		this->mainDispatcher().processOnce();
		this->dispatcher().processOnce();

		done = !this->hasUnackedPackets();

		if (!this->deleteFinishedChannels())
		{
			done = false;
		}

		// Wait 100ms
#if defined( PLAYSTATION3 )
		sys_timer_usleep( 100000 );
#elif !defined( _WIN32 )
		usleep( 100000 );
#else
		Sleep( 100 );
#endif
	}

	this->mainDispatcher().errorReporter().reportPendingExceptions(
			true /*reportBelowThreshold*/ );

	if (!done)
	{
		WARNING_MSG( "NetworkInterface::processUntilChannelsEmpty: "
			"Timed out after %.1fs, unacked packets may have been lost\n",
			timeout );
	}
}


/**
 *	This method closes the socket associated with this interface. It will also
 *	deregister the interface if necessary.
 */
void NetworkInterface::closeSocket()
{
	// first unregister any existing interfaces.
	if (udpSocket_.good())
	{
		this->interfaceTable().deregisterWithMachined( this->address() );

		this->dispatcher().deregisterFileDescriptor( udpSocket_.fileno() );
		udpSocket_.close();
		udpSocket_.detach();	// in case close failed
	}
}


/**
 *	This method discards the existing socket and attempts to create a new one
 *	with the given parameters. The interfaces served by a nub are re-registered
 *	on the new socket if successful.
 *
 *	@param listeningPort		The port to listen on.
 *	@param listeningInterface	The network interface to listen on.
 *	@return	true if successful, otherwise false.
 */
bool NetworkInterface::recreateListeningSocket( uint16 listeningPort,
	const char * listeningInterface )
{
	this->closeSocket();

	// clear this unless it gets set otherwise
	address_.ip = 0;
	address_.port = 0;
	address_.salt = 0;

	// make the socket
	udpSocket_.socket( SOCK_DGRAM );

	if (!udpSocket_.good())
	{
		ERROR_MSG( "NetworkInterface::recreateListeningSocket: "
				"couldn't create a socket\n" );
		return false;
	}

	this->dispatcher().registerFileDescriptor( udpSocket_.fileno(),
			pPacketReceiver_, "PacketReceiver" );

	// ask endpoint to parse the interface specification into a name
	char ifname[IFNAMSIZ];
	u_int32_t ifaddr = INADDR_ANY;
	bool listeningInterfaceEmpty =
		(listeningInterface == NULL || listeningInterface[0] == 0);

	// Query bwmachined over the local interface (dev: lo) for what it
	// believes the internal interface is.
	if (listeningInterface &&
		(strcmp( listeningInterface, USE_BWMACHINED ) == 0))
	{
		INFO_MSG( "NetworkInterface::recreateListeningSocket: "
				"Querying BWMachined for interface\n" );

		if (MachineDaemon::queryForInternalInterface( ifaddr ))
		{
			WARNING_MSG( "NetworkInterface::recreateListeningSocket: "
				"No address received from machined so binding to all interfaces.\n" );
		}
	}
	else if (udpSocket_.findIndicatedInterface( listeningInterface, ifname ) == 0)
	{
		INFO_MSG( "NetworkInterface::recreateListeningSocket: "
				"Creating on interface '%s' (= %s)\n",
			listeningInterface, ifname );
		if (udpSocket_.getInterfaceAddress( ifname, ifaddr ) != 0)
		{
			WARNING_MSG( "NetworkInterface::recreateListeningSocket: "
				"Couldn't get addr of interface %s so using all interfaces\n",
				ifname );
		}
	}
	else if (!listeningInterfaceEmpty)
	{
		WARNING_MSG( "NetworkInterface::recreateListeningSocket: "
				"Couldn't parse interface spec '%s' so using all interfaces\n",
			listeningInterface );
	}

	// now we know where to bind, so do so
	if (udpSocket_.bind( listeningPort, ifaddr ) != 0)
	{
		ERROR_MSG( "NetworkInterface::recreateListeningSocket: "
				"Couldn't bind the socket to %s (%s)\n",
			Address( ifaddr, listeningPort ).c_str(), strerror( errno ) );
		udpSocket_.close();
		udpSocket_.detach();
		return false;
	}

	// but for advertising it ask the socket for where it thinks it's bound
	udpSocket_.getlocaladdress( (u_int16_t*)&address_.port,
		(u_int32_t*)&address_.ip );

	if (address_.ip == 0)
	{
		// we're on INADDR_ANY, report the address of the
		//  interface used by the default route then
		if (udpSocket_.findDefaultInterface( ifname ) != 0 ||
			udpSocket_.getInterfaceAddress( ifname,
				(u_int32_t&)address_.ip ) != 0)
		{
			ERROR_MSG( "NetworkInterface::recreateListeningSocket: "
				"Couldn't determine ip addr of default interface\n" );

			udpSocket_.close();
			udpSocket_.detach();
			return false;
		}

		INFO_MSG( "NetworkInterface::recreateListeningSocket: "
				"bound to all interfaces with default route "
				"interface on %s ( %s )\n",
			ifname, address_.c_str() );
	}

	INFO_MSG( "NetworkInterface::recreateListeningSocket: address %s\n",
		address_.c_str() );

	udpSocket_.setnonblocking( true );

	udpSocket_.enableErrorQueue( true );

#ifdef __APPLE__
	udpSocket_.setSocketOption( SOL_SOCKET, SO_NOSIGPIPE, true );
#endif
	
#ifdef MF_SERVER
	if (!udpSocket_.setBufferSize( SO_RCVBUF, recvBufferSize_ ))
	{
		WARNING_MSG( "NetworkInterface::recreateListeningSocket: "
			"Operating with a receive buffer of only %d bytes (instead of %d)\n",
			udpSocket_.getBufferSize( SO_RCVBUF ), recvBufferSize_ );
	}
	if (!udpSocket_.setBufferSize( SO_SNDBUF, sendBufferSize_ ))
	{
		WARNING_MSG( "NetworkInterface::recreateListeningSocket: "
			"Operating with a send buffer of only %d bytes (instead of %d)\n",
			udpSocket_.getBufferSize( SO_SNDBUF ), sendBufferSize_ );
	}
#endif

	this->interfaceTable().registerWithMachined( this->address() );

	return true;
}


/**
 *	This method is used to register or deregister an interface with the machine
 *	guard (a.k.a. machined).
 */
Reason NetworkInterface::registerWithMachined(
		const BW::string & name, int id )
{
	return this->interfaceTable().registerWithMachined( this->address(),
					name, id );
}


/**
 *	This method registers a channel with this interface.
 */
bool NetworkInterface::registerChannel( UDPChannel & channel )
{
	MF_ASSERT( &channel.networkInterface() == this );

	return pChannelMap_->add( channel );
}


/**
 *	This method deregisters a channel that has been previously registered with
 *	this interface.
 */
bool NetworkInterface::deregisterChannel( UDPChannel & channel )
{
	if (channel.isExternal() && pRecentlyDeadChannels_)
	{
		pRecentlyDeadChannels_->add( channel.addr() );
	}

	pChannelMap_->remove( channel );

	return true;
}


/**
 *  This method cleans up all internal data structures and timers related to the
 *  specified address.
 */
void NetworkInterface::onAddressDead( const Address & addr )
{
	this->onceOffSender().onAddressDead( addr );

	// Clean up the anonymous channel to the dead address, if there was one.
	UDPChannel * pDeadChannel = this->findChannel( addr );

	if (pDeadChannel && pDeadChannel->isAnonymous())
	{
		pDeadChannel->setRemoteFailed();
	}
}


/**
 *	This method returns whether or not an address has recently died.
 */
bool NetworkInterface::isDead( const Address & addr ) const
{
	return pRecentlyDeadChannels_ && pRecentlyDeadChannels_->isDead( addr );
}


/**
 *	This method handles timer events.
 */
void NetworkInterface::handleTimeout( TimerHandle handle, void * arg )
{
	ipAddressRateLimitAverage_.sample();
	ipAddressPortRateLimitAverage_.sample();

	this->clearRateLimitCounts();
}


/**
 *	This method finds the channel associated with a given id.
 *
 *	@param channelID The id of the channel to find.
 *	@param srcAddr The address that the packet came from.
 *	@param pPacket The packet that's requesting to find the channel. It is
 *		possible that the registered finder will handle the packet immediately.
 *	@param rpChannel A reference to a channel pointer. This will be set to the
 *		found channel.
 *
 *	@return An enumeration indicating whether the channel was found (anything
 *		but INDEXED_CHANNEL_CORRUPTED) and whether the packet has already been
 *		handled (either INDEXED_CHANNEL_HANDLED or INDEXED_CHANNEL_NOT_HANDLED).
 */
NetworkInterface::IndexedChannelFinderResult
	NetworkInterface::findIndexedChannel( ChannelID channelID,
			const Mercury::Address & srcAddr,
			Packet * pPacket, UDPChannelPtr & rpChannel ) const
{
	if (pChannelFinder_ == NULL)
	{
		return INDEXED_CHANNEL_CORRUPTED;
	}

	bool hasBeenHandled = false;
	rpChannel = pChannelFinder_->find( channelID, srcAddr,
			pPacket, hasBeenHandled );

	return hasBeenHandled ?
		INDEXED_CHANNEL_HANDLED : INDEXED_CHANNEL_NOT_HANDLED;
}


/**
 *	This method finds the condemned channel with the indexed ChannelID.
 */
UDPChannelPtr NetworkInterface::findCondemnedChannel( 
		ChannelID channelID ) const
{
	return pCondemnedChannels_->find( channelID );
}


/**
 *  This method sets the ChannelFinder to be used for resolving channel ids.
 */
void NetworkInterface::registerChannelFinder( ChannelFinder * pFinder )
{
	MF_ASSERT( pChannelFinder_ == NULL );
	pChannelFinder_ = pFinder;
}


/**
 *	This method returns whether there are unacked once-off packets and whether
 *	any channel has any unacked packets.
 */
bool NetworkInterface::hasUnackedPackets() const
{
	return pOnceOffSender_->hasUnackedPackets() ||
		pChannelMap_->hasUnackedPackets() ||
		pCondemnedChannels_->hasUnackedPackets();
}


/**
 *	This method cleans up condemned channels that have finished.
 */
bool NetworkInterface::deleteFinishedChannels()
{
	return pCondemnedChannels_->deleteFinishedChannels();
}


/**
 *	This method returns a channel to the input address.
 *
 *	@param addr	The address of the channel to find.
 *	@param createAnonymous If set and the channel is not found, a new channel
 *		will be created and returned.
 *
 *	@return A pointer to a channel to the given address. If none is found and
 *		createAnonymous is false, NULL is returned.
 */
UDPChannel * NetworkInterface::findChannel( const Address & addr,
		bool createAnonymous )
{
	MF_ASSERT( !createAnonymous || !isExternal_ );

	return pChannelMap_->find( addr, createAnonymous ? this : NULL );
}


/**
 *	This method is called when a channel has been condemned. Any outstanding
 *	requests are told of this failure.
 */
void NetworkInterface::onChannelGone( Channel * pChannel )
{
	this->cancelRequestsFor( pChannel );
}


/**
 *	This method cancels the requests for a given channel.
 */
void NetworkInterface::cancelRequestsFor( Channel * pChannel )
{
	if (pRequestManager_)
	{
		pRequestManager_->cancelRequestsFor( pChannel );
	}
}


/**
 *	This method cancels the requests for a given handler.
 */
void NetworkInterface::cancelRequestsFor( ReplyMessageHandler * pHandler,
	 	 Reason reason )
{
	if (pRequestManager_)
	{
		pRequestManager_->cancelRequestsFor( pHandler, reason );
	}
}


/**
 *	This method cancels the requests for a given bundle.
 */
void NetworkInterface::cancelRequestsFor( Bundle & bundle, Reason reason )
{
	if (pRequestManager_)
	{
		bundle.cancelRequests( pRequestManager_, reason );
	}
}


/**
 *	Register a channel for delayed sending.
 */
void NetworkInterface::delayedSend( UDPChannel & channel )
{
	pDelayedChannels_->add( channel );
}


/**
 *	Sends on a channel immediately if it has been registered previously for
 *	delayed sending.
 */
void NetworkInterface::sendIfDelayed( UDPChannel & channel )
{
	pDelayedChannels_->sendIfDelayed( channel );
}


/**
 *  This method condemns the anonymous channel to the specified address.
 */
void NetworkInterface::delAnonymousChannel( const Address & addr )
{
	pChannelMap_->delAnonymous( addr );
}


/**
 *	This method sets the period that irregular channels are resent.
 */
void NetworkInterface::setIrregularChannelsResendPeriod( float seconds )
{
	pIrregularChannels_->setPeriod( seconds, this->dispatcher() );
}


/**
 *	This method adds requests from the given bundle to the request manager.
 */
void NetworkInterface::addReplyOrdersTo( Bundle & bundle, Channel * pChannel )
{
	bundle.addReplyOrdersTo( pRequestManager_, pChannel );
}


// -----------------------------------------------------------------------------
// Section: Interface table
// -----------------------------------------------------------------------------

/**
 *	This method returns the interface table associated with this network
 *	interface.
 */
InterfaceTable & NetworkInterface::interfaceTable()
{
	return *pInterfaceTable_;
}


/**
 *	This method returns the interface table associated with this network
 *	interface.
 */
const InterfaceTable & NetworkInterface::interfaceTable() const
{
	return *pInterfaceTable_;
}


// -----------------------------------------------------------------------------
// Section: Processing
// -----------------------------------------------------------------------------

/**
 *  This method reads data from the stream into a packet and then processes it.
 */
Reason NetworkInterface::processPacketFromStream( const Address & addr,
	BinaryIStream & data )
{
	// Set up a fresh packet from the message and feed it to the nub
	PacketPtr pPacket = new Packet();
	int len = data.remainingLength();

	memcpy( pPacket->data(), data.retrieve( len ), len );
	pPacket->msgEndOffset( len );

	return pPacketReceiver_->processPacket( addr, pPacket.get(), NULL );
}


// -----------------------------------------------------------------------------
// Section: Sending
// -----------------------------------------------------------------------------


/**
 *	This method returns a new bundle suitable for off-channel sending over UDP
 *	channels only.
 *
 *	@return 	A new off-channel network bundle.
 */
Bundle * NetworkInterface::createOffChannelBundle()
{
	return new UDPBundle();
}


/**
 * 	This method sends a bundle to the given address.
 *
 * 	Note: any pointers you have into the packet may become invalid after this
 * 	call (and whenever a channel calls this too).
 *
 * 	@param address	The address to send to.
 * 	@param bundle	The bundle to send
 *	@param pChannel	The Channel that is sending the bundle.
 *				(even if the bundle is not sent reliably, it is still passed
 *				through the filter associated with the channel).
 */
void NetworkInterface::send( const Address & address,
								UDPBundle & bundle, UDPChannel * pChannel )
{
	pPacketSender_->send( address, bundle, pChannel );
}


/**
 *	This method will attempt to send a bundle on an existing channel if one
 *	exists. If non exists, it is sent off channel.
 */
void NetworkInterface::sendOnExistingChannel( const Address & address,
		UDPBundle & bundle )
{
	UDPChannel * pChannel = this->findChannel( address );

	if (pChannel)
	{
		pChannel->send( &bundle );
	}
	else
	{
		this->send( address, bundle );
	}
}


/**
 *	This method sends a packet. No result is returned as it cannot be trusted.
 *	The packet may never get to the other end.
 */
void NetworkInterface::sendPacket( const Address & address,
						Packet * pPacket,
						UDPChannel * pChannel, bool isResend )
{
	pPacketSender_->sendPacket( address, pPacket, pChannel, isResend );
}


/**
 *	This method sets the artificial latency on this interface.
 *
 *	@param latencyMin	The minimum latency.
 *	@param latencyMax	The maximum latency.
 */
void NetworkInterface::setLatency( float latencyMin, float latencyMax )
{
	pPacketLossParameters_->minLatency( latencyMin );
	pPacketLossParameters_->maxLatency( latencyMax );
}


/**
 *	This method sets the artificial loss ratio on this interface.
 *
 *	@param lossRatio 	The artificial loss ratio.
 */
void NetworkInterface::setLossRatio( float lossRatio )
{
	pPacketLossParameters_->lossRatio( lossRatio );
}


/**
 *	This method drops the next network send.
 */
void NetworkInterface::dropNextSend()
{
	pPacketSender_->dropNextSend();
}


/**
 *	This method returns whether checksums are used.
 *
 *	@return 	True if checksums are used, false otherwise.
 */
bool NetworkInterface::shouldUseChecksums() const 
{
	return pPacketSender_->shouldUseChecksums();
}


/**
 *	This method sets whether checksums are used.
 *
 *	@param v 	True if checksums should be used, false if they should not.
 */
void NetworkInterface::shouldUseChecksums( bool v )
{
	pPacketSender_->shouldUseChecksums( v );
}


/**
 *	This method registers a handler to monitor sent/received packets. Only a
 *	single monitor is supported. To remove the monitor, set it to NULL.
 *
 *	@param pPacketMonitor	An object implementing the PacketMonitor interface.
 */
void NetworkInterface::setPacketMonitor( PacketMonitor * pPacketMonitor )
{
	pPacketReceiver_->setPacketMonitor( pPacketMonitor );
	pPacketSender_->setPacketMonitor( pPacketMonitor );
}


/**
 *	This method sets a callback that will be called when a UDP packet is sent
 *	to a port that is closed.
 */
void NetworkInterface::setNoSuchPortCallback( INoSuchPort * pCallback )
{
	if (pPacketReceiver_)
	{
		pPacketReceiver_->setNoSuchPortCallback( pCallback );
	}
}


/**
 *	This method sets the rate limiting period, over which rate-limiting takes
 *	effect if counts are exceeded within a single period. Counts are reset
 *	after the end of each period.
 *
 *	@param rateLimitPeriod 	The new rate-limiting period.
 */
void NetworkInterface::rateLimitPeriod( float rateLimitPeriod )
{
	rateLimitTimer_.cancel();

	this->clearRateLimitCounts();

	rateLimitPeriod_ = rateLimitPeriod;

	if (!isZero( rateLimitPeriod_ ) && (rateLimitPeriod_ > 0.f))
	{
		rateLimitTimer_ = this->dispatcher().addTimer(
			int64( rateLimitPeriod_ * 1000000 ),
			this, /* arg */ NULL,
			"NetworkInterface::rateLimitPeriod" );
	}
}


/**
 *	This method resets the rate-limit counts.
 */
void NetworkInterface::clearRateLimitCounts()
{
	rateLimitedIPAddresses_.clear();
	rateLimitedIPAddressPorts_.clear();

	ipAddressRateLimitAverage_.value() = 0;
	ipAddressPortRateLimitAverage_.value() = 0;
}


/**
 *	This method checks whether the given address should be rate-limited.
 */
bool NetworkInterface::incrementAndCheckRateLimit( const Address & addr )
{
	bool shouldRateLimit = false;

	if (isZero( rateLimitPeriod_ ))
	{
		return false;
	}

	// Check and update IP address.
	if (rateLimitPerIPAddress_ > 0)
	{
		Address zeroedPortAddr( addr.ip, 0 );
		uint & count = rateLimitedIPAddresses_[zeroedPortAddr];

		if (++count > rateLimitPerIPAddress_)
		{
			++(ipAddressRateLimitAverage_.value());
			shouldRateLimit = true;
		}
	}

	// Check and update IP address and port pair.
	if (rateLimitPerIPAddressPort_ > 0)
	{
		uint & count = rateLimitedIPAddressPorts_[addr];

		if (++count > rateLimitPerIPAddressPort_)
		{
			++(ipAddressPortRateLimitAverage_.value());
			shouldRateLimit = true;
		}
	}

	return shouldRateLimit;
}


/**
 *	This method returns a object containing the stats associated with receiving
 *	for this interface.
 */
const PacketReceiverStats & NetworkInterface::receivingStats() const
{
	return pPacketReceiver_->stats();
}


/**
 *	This method returns the maximum socket processing time limit per input
 *	notification.
 *
 *	@return the maximum socket processing time limit.
 */
float NetworkInterface::maxSocketProcessingTime() const 
{
	return pPacketReceiver_->maxSocketProcessingTime();
}


/**
 *	This method sets the maximum socket processing time limit per input
 *	notification.
 *
 *	@param maxTime 	The time limit to set.
 */
void NetworkInterface::maxSocketProcessingTime( float maxTime )
{
	pPacketReceiver_->maxSocketProcessingTime( maxTime );
}


/**
 *	This method returns whether artificial loss or latency is active on this
 *	interface.
 */
bool NetworkInterface::hasArtificialLossOrLatency() const
{
	return pPacketLossParameters_->hasArtificialLossOrLatency();
}


#if ENABLE_WATCHERS

namespace // anonymous
{

const char * VERBOSITY_LEVEL_NAMES[] =
{
	"quiet",
	"normal",
	"debug"
};


/**
 *	This method converts an verbosity level value to a string.
 */
const char * verbosityLevelToString( NetworkInterface::VerbosityLevel level )
{
	if ((level < NetworkInterface::VERBOSITY_LEVEL_QUIET) || 
			(level > NetworkInterface::VERBOSITY_LEVEL_MAX))
	{
		return NULL;
	}
	return VERBOSITY_LEVEL_NAMES[level];
}


/**
 *	This method converts an verbosity level string to its value.
 */
int verbosityLevelFromString( const char * levelName )
{
	for (size_t i = 0; i < ARRAY_SIZE( VERBOSITY_LEVEL_NAMES ); ++i)
	{
		if (0 == strncmp( levelName, VERBOSITY_LEVEL_NAMES[i],
				strlen( VERBOSITY_LEVEL_NAMES[i] ) ))
		{
			return int( i );
		}
	}

	return -1;
}


} // end namespace (anonymous)


/** Streaming operator for NetworkInterface::VerbosityLevel. */
std::ostream & operator<<( std::ostream & os,
		NetworkInterface::VerbosityLevel level )
{
	os << verbosityLevelToString( level );
	return os;
}


/** Streaming operator for NetworkInterface::VerbosityLevel. */
std::istream & operator>>( std::istream & is,
		NetworkInterface::VerbosityLevel & level )
{
	BW::string levelString;
	is >> levelString;

	int levelValue = verbosityLevelFromString( levelString.c_str() );
	if (levelValue != -1)
	{
		level = (NetworkInterface::VerbosityLevel) levelValue;
	}

	return is;
}


/** Streaming operator for NetworkInterface::VerbosityLevel. */
BinaryOStream & operator<<( BinaryOStream & os,
		NetworkInterface::VerbosityLevel level )
{
	os << verbosityLevelToString( level );
	return os;
}


/** Streaming operator for NetworkInterface::VerbosityLevel. */
BinaryIStream & operator>>( BinaryIStream & is,
		NetworkInterface::VerbosityLevel & level )
{
	BW::string levelString;
	is >> levelString;

	int levelValue = verbosityLevelFromString( levelString.c_str() );
	if (levelValue != -1)
	{
		level = (NetworkInterface::VerbosityLevel) levelValue;
	}

	return is;
}


/**
 *	This method returns a watcher that can inspect a NetworkInterface.
 */
WatcherPtr NetworkInterface::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		NetworkInterface * pNull = NULL;

		pWatcher->addChild( "address", &Address::watcher(),
			&pNull->address_ );

		pWatcher->addChild( "interfaceByID",
				new BaseDereferenceWatcher( InterfaceTable::pWatcherByID() ),
				&pNull->pInterfaceTable_ );

		pWatcher->addChild( "interfaceByName",
				new BaseDereferenceWatcher( InterfaceTable::pWatcherByName() ),
				&pNull->pInterfaceTable_ );

		pWatcher->addChild( "artificialLoss",
				new BaseDereferenceWatcher( PacketLossParameters::pWatcher() ),
				&pNull->pPacketLossParameters_);

		pWatcher->addChild( "receiving",
			new BaseDereferenceWatcher( PacketReceiver::pWatcher() ),
			&pNull->pPacketReceiver_ );

		pWatcher->addChild( "sending/stats",
				SendingStats::pWatcher(), &pNull->sendingStats_ );

		pWatcher->addChild( "timing",
			new BaseDereferenceWatcher( EventDispatcher::pTimingWatcher() ),
			&pNull->pMainDispatcher_ );

		pWatcher->addChild( "dispatcher",
			new BaseDereferenceWatcher( EventDispatcher::pWatcher() ),
			&pNull->pDispatcher_ );

		pWatcher->addChild( "mainDispatcher",
			new BaseDereferenceWatcher( EventDispatcher::pWatcher() ),
			&pNull->pMainDispatcher_ );

		pWatcher->addChild( "isVerbose", makeNonRefWatcher( *pNull,
			&NetworkInterface::isVerbose,
			&NetworkInterface::isVerbose ) );

		pWatcher->addChild( "verbosityLevel", makeNonRefWatcher( *pNull,
			&NetworkInterface::verbosityLevel,
			&NetworkInterface::verbosityLevel) );

		pWatcher->addChild( "rateLimit/duration", makeNonRefWatcher(
			*pNull, &NetworkInterface::rateLimitPeriod,
			&NetworkInterface::rateLimitPeriod ) );

		pWatcher->addChild( "rateLimit/perIPAddressLimit",
			makeNonRefWatcher( *pNull, &NetworkInterface::perIPAddressRateLimit,
				&NetworkInterface::perIPAddressRateLimit ) );

		pWatcher->addChild( "rateLimit/perIPAddressPortLimit",
			makeNonRefWatcher( *pNull, 
				&NetworkInterface::perIPAddressPortRateLimit,
				&NetworkInterface::perIPAddressPortRateLimit ) );

		pWatcher->addChild( "rateLimit/stats/perIPAddressAverage",
			makeWatcher< float, AccumulatingEMA< uint > >( 
				pNull->ipAddressRateLimitAverage_,
				&AccumulatingEMA< uint >::average ) );

		pWatcher->addChild( "rateLimit/stats/perIPAddressPortAverage",
			makeWatcher< float, AccumulatingEMA< uint > >(
				pNull->ipAddressPortRateLimitAverage_,
				&AccumulatingEMA< uint >::average ) );
	}

	return pWatcher;
}
#endif // ENABLE_WATCHERS

} // namespace Mercury

BW_END_NAMESPACE

// network_interface.cpp
