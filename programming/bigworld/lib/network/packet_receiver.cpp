#include "pch.hpp"

#include "packet_receiver.hpp"

#include "basictypes.hpp"
#include "endpoint.hpp"
#include "error_reporter.hpp"
#include "event_dispatcher.hpp"
#include "network_interface.hpp"
#include "packet_monitor.hpp"
#include "process_socket_stats_helper.hpp"
#include "udp_bundle.hpp"
#include "udp_bundle_processor.hpp"
#include "udp_channel.hpp"

#include "connection/server_connection.hpp"

#include "cstdmf/debug.hpp"

#include <limits>

#if defined( PLAYSTATION3 )
#include "ps3_compatibility.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Network", 0 )

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "packet_receiver.ipp"
#endif


namespace // (anonymous)
{

const float STATS_UPDATE_INTERVAL_SECONDS = 1.f;

} // end namespace (anonymous)
namespace Mercury
{

/**
 *	Constructor.
 */
PacketReceiver::PacketReceiver( Endpoint & socket,
	   NetworkInterface & networkInterface	) :
	InputNotificationHandler(),
	TimerHandler(),
	socket_( socket ),
	networkInterface_( networkInterface ),
	pNextPacket_( new Packet() ),
	stats_(),
	statsUpdateTimer_(),
	onceOffReceiver_(),
	pNoSuchPortCallback_( NULL ),
	pPacketMonitor_( NULL ),
	maxSocketProcessingTimeStamps_( 0 )
{
	onceOffReceiver_.init( this->dispatcher() );

	statsUpdateTimer_ = this->dispatcher().addTimer(
		int64( STATS_UPDATE_INTERVAL_SECONDS * 1000000 ), this, /* pUser */ 0,
		"PacketReceiverStatsUpdate" );
}


/**
 *	Destructor.
 */
PacketReceiver::~PacketReceiver()
{
	onceOffReceiver_.fini();

	statsUpdateTimer_.cancel();
}


/*
 *	This method is called when there is data on the socket.
 */
int PacketReceiver::handleInputNotification( int fd )
{
	uint64 processingStartStamps = BW_NAMESPACE timestamp();

	int numPacketsProcessed = 0;

	bool expectingPacket = true; // only true for first call to processSocket()
	bool shouldProcess = true;

	while (shouldProcess)
	{
		Address sourceAddress;
		shouldProcess = this->processSocket( sourceAddress, expectingPacket );
		expectingPacket = false;

		uint64 processingElapsedStamps = 
			BW_NAMESPACE timestamp() - processingStartStamps;

		++numPacketsProcessed;

		if (shouldProcess && 
				(maxSocketProcessingTimeStamps_ != 0) && 
				(processingElapsedStamps > maxSocketProcessingTimeStamps_))
		{
			WARNING_MSG( "PacketReceiver::handleInputNotification: "
					"last received from %s, "
					"stopping socket processing after %.03fms "
					"(processed %d packets), receive queue at %d bytes",
				sourceAddress.c_str(),
				processingElapsedStamps * 1000.f / 
					BW_NAMESPACE stampsPerSecondD(),
				numPacketsProcessed,
				networkInterface_.socket().receiveQueueSize() );

			shouldProcess = false;
		}

	}

	return 0;
}

/*
 *	Override from InputNotificationhandler.
 */
bool PacketReceiver::handleErrorNotification( int fd )
{
	int queueErrNo = 0;
	Address offender;
	uint32 info = 0;

	if (socket_.readFromErrorQueue( queueErrNo, offender, info ))
	{
		if (queueErrNo == ECONNREFUSED)
		{
			this->onConnectionRefusedTo( offender );
		}
		else if (queueErrNo == EMSGSIZE)
		{
			this->onMTUExceeded( offender, info );
		}
		else if (networkInterface_.isVerbose())
		{
			UDPChannel * pChannel = 
				networkInterface_.findChannel( offender );
			if (pChannel != NULL)
			{
				if (networkInterface_.isExternal())
				{
					TRACE_MSG( "PacketReceiver::handleErrorNotification( %s ):"
						" Ignoring errno %d: %s (from %s to %s).\n",
						pChannel->c_str(), queueErrNo, strerror( queueErrNo ),
						socket_.c_str(), offender.c_str() );
				}
				else
				{
					ERROR_MSG( "PacketReceiver::handleErrorNotification( %s ):"
						" Ignoring errno %d: %s (from %s to %s).\n",
						pChannel->c_str(), queueErrNo, strerror( queueErrNo ),
						socket_.c_str(), offender.c_str() );
				}
			}
		}
		return true;
	}
	else
	{
		// Failed to clear the error state.
		return false;
	}
}


/*
 *	Override from TimerHandler.
 */
void PacketReceiver::handleTimeout( TimerHandle handle, void * pUser )
{
	// Update this to ensure the averages are updated even if we have not
	// received input - for external sockets, it's possible that there are no
	// active connections to trigger the stats update.
	stats_.updateSocketStats( socket_ );
}


/**
 *	This method will read and process any pending data on this object's socket.
 *
 *	@param srcAddr 			This will be filled with the source address of any
 *							packets received.
 *	@param expectingPacket 	If true, a packet was expected to be read, 
 *							otherwise false.
 *
 *	@return 				True if a packet was read, otherwise false.
 */
bool PacketReceiver::processSocket( Address & srcAddr, 
		bool expectingPacket )
{
	stats_.updateSocketStats( socket_ );

	// Used to collect stats
	ProcessSocketStatsHelper statsHelper( stats_ );

	// try a recvfrom
	int len = pNextPacket_->recvFromEndpoint( socket_, srcAddr );

	statsHelper.socketReadFinished( len );

	if (len <= 0)
	{
		this->checkSocketErrors( len, expectingPacket );
		return false;
	}

	// process it if it succeeded
	PacketPtr curPacket = pNextPacket_;
	pNextPacket_ = new Packet();

	Reason ret = this->processPacket( srcAddr, curPacket.get(),
			&statsHelper );

	if ((ret != REASON_SUCCESS) &&
			networkInterface_.isVerbose())
	{
		this->dispatcher().errorReporter().reportException( ret, srcAddr );
	}

	return true;
}


/**
 *	This method processes any error that was received from a call to
 *	Endpoint::recvFromEndpoint().
 *
 *	@param len 				The return value from recvFromEndpoint(), which is
 *							the return value from the recvfrom() system call.
 *	@param expectingPacket 	Whether or not a packet was expected, that is, we
 *							have received an input notification for the
 *							receive.
 *
 *	@return 				True if a reportable error was found, false
 *							otherwise. If we are not expecting a packet, and we
 *							receive EAGAIN or equivalent, then this is not
 *							considered a reportable error, and false is
 *							returned in this case.
 */
bool PacketReceiver::checkSocketErrors( int len, bool expectingPacket )
{
	// is len weird?
	if (len == 0)
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::checkSocketErrors: "
				"Throwing REASON_GENERAL_NETWORK (1)- %s\n",
				strerror( errno ) );
		}

		this->dispatcher().errorReporter().reportException(
				REASON_GENERAL_NETWORK );

 		return true;
 	}

	// I'm not quite sure what it means if len is 0
	// (0 => 'end of file', but with dgram sockets?)


#if defined( _WIN32 )
	DWORD wsaErr = WSAGetLastError();
	if (wsaErr == WSAEWOULDBLOCK)
	{
		return false;
	}

	if (wsaErr == WSAECONNRESET)
	{
		if (pNoSuchPortCallback_)
		{
			// Not currently retrieving address
			Mercury::Address addr( 0, 0 );
			pNoSuchPortCallback_->onNoSuchPort( addr );
		}

		return true;
	}

#elif defined( __unix__ ) || defined( __APPLE__ ) || defined( __ANDROID__ )

	// is the buffer empty?
	if ((errno == EAGAIN) && !expectingPacket)
	{
		return false;
	}

	// is it telling us there's an error?
	if ((errno == EAGAIN) ||
			(errno == ECONNREFUSED) ||
			(errno == EHOSTUNREACH))
	{

#if defined( PLAYSTATION3 )

		this->dispatcher().errorReporter().reportException(
				REASON_NO_SUCH_PORT );
		return true;

#else // defined( PLAYSTATION3 )

		int queueErrNo = 0;
		Address offender;
		uint32 info = 0;

		if (socket_.readFromErrorQueue( queueErrNo, offender, info ))
		{
			// If we got a NO_SUCH_PORT error and there is an internal
			// channel to this address, mark it as remote failed. For external
			// channels, just let it be handled as a channel timeout.
			if (queueErrNo == ECONNREFUSED)
			{
				this->onConnectionRefusedTo( offender );

				if (pNoSuchPortCallback_)
				{
					pNoSuchPortCallback_->onNoSuchPort( offender );
				}

				this->dispatcher().errorReporter().reportException(
						REASON_NO_SUCH_PORT, offender );
			}
			else
			{
				WARNING_MSG( "PacketReceiver::checkSocketErrors: "
						"got errno %d: %s (info: %u)\n",
					queueErrNo, strerror( queueErrNo ), info );

				this->dispatcher().errorReporter().reportException(
						REASON_GENERAL_NETWORK, offender );
			}

			return true;
		}
#if defined( __linux__ ) && !defined( EMSCRIPTEN )
// must match the check of Endpoint::readFromErrorQueue()
		else
		{
			WARNING_MSG( "PacketReceiver::checkSocketErrors: "
					"Endpoint::readFromErrorQueue() failed: %s\n",
				strerror( errno ) );
 		}
#endif // defined( __linux__ ) && !defined( EMSCRIPTEN )
#endif // defined( PLAYSTATION3 )

	}

#endif // defined( _WIN32 )

	// Something's wrong, but no other details are available.
	// Only warn on internal interfaces. External interfaces can be noisy
	if (networkInterface_.isVerbose())
	{
#if defined( _WIN32 )

	WARNING_MSG( "PacketReceiver::checkSocketErrors: "
					"Throwing REASON_GENERAL_NETWORK - %lu\n",
				wsaErr );
#else // !defined( _WIN32 )

		WARNING_MSG( "PacketReceiver::checkSocketErrors: "
				"Throwing REASON_GENERAL_NETWORK - %s\n",
			strerror( errno ) );
#endif // defined( _WIN32 )
	}

	this->dispatcher().errorReporter().reportException(
			REASON_GENERAL_NETWORK );

	return true;
}


/**
 *	This method handles a connection refusal error to the given offending
 *	address.
 *
 *	@param offender 	The address that caused a connection refusal error.
 */
void PacketReceiver::onConnectionRefusedTo( const Address & offender )
{
	UDPChannel * pDeadChannel = 
		networkInterface_.findChannel( offender );

	if (pDeadChannel &&
			pDeadChannel->isInternal() &&
			!pDeadChannel->hasRemoteFailed())
	{
		INFO_MSG( "PacketReceiver::onConnectionRefusedTo: "
				"Marking channel to %s as dead (%s)\n",
			pDeadChannel->c_str(),
			reasonToString( REASON_NO_SUCH_PORT ) );

		pDeadChannel->setRemoteFailed();
	}
}


/**
 *	This method handles a message too long (EMSGSIZE) error  
 *    from the given offending address.
 *
 *	@param offender	The address that caused a message too long error.
 *	@param info		The MTU information.
 */
void PacketReceiver::onMTUExceeded( const Address & offender,
	 const uint32 & info )
{
	UDPChannel * pChannel = 
		networkInterface_.findChannel( offender );
	if (pChannel)
	{
		INFO_MSG( "PacketReceiver::onMTUExceeded( %s ): "
				"MTU now %u bytes from %s to %s.\n",
			pChannel->c_str(), info, socket_.c_str(), offender.c_str() );
	}
}


/**
 *	This is the entrypoint for new packets, which just gives it to the filter.
 */
Reason PacketReceiver::processPacket( const Address & addr, Packet * p,
	   ProcessSocketStatsHelper * pStatsHelper )
{
	if (networkInterface_.incrementAndCheckRateLimit( addr ))
	{
		if (networkInterface_.isDebugVerbose())
		{
			DEBUG_MSG( "PacketReceiver::processPacket( %s ): "
					"Ignoring packet from rate-limited address\n",
				addr.c_str() );
		}

		return Mercury::REASON_GENERAL_NETWORK;
	}

	// Packets arriving on external interface will probably be encrypted, so
	// there's no point examining their header flags right now.
	UDPChannel * pChannel = networkInterface_.findChannel( addr,
			!networkInterface_.isExternal() && p->shouldCreateAnonymous() );

	if (pChannel != NULL)
	{
		// We update received times for addressed channels here.  Indexed
		// channels are done in processFilteredPacket().
		pChannel->onPacketReceived( p->totalSize() );

		if (pChannel->pFilter() && !pChannel->hasRemoteFailed())
		{
			// let the filter decide what to do with it
			return pChannel->pFilter()->recv( *this, addr, p, pStatsHelper );
		}
	}
	// If we're monitoring recent channel deaths, check now if this packet
	// should be dropped.
	else if (networkInterface_.isExternal() &&
		networkInterface_.isDead( addr ))
	{
		if (networkInterface_.isDebugVerbose())
		{
			DEBUG_MSG( "PacketReceiver::processPacket( %s ): "
				"Ignoring incoming packet on recently dead channel\n",
				addr.c_str() );
		}

		return REASON_SUCCESS;
	}
	//use the network interface's default filter to filter out 
	//off channel malicious/noise packets
	else if (networkInterface_.pOffChannelFilter())
	{
		return networkInterface_.pOffChannelFilter()->recv( *this, 
											addr, p, pStatsHelper );
	}

	//parse raw.
	return this->processFilteredPacket( addr, p, pStatsHelper );
}


/**
 *  This macro is used by PacketReceiver::processFilteredPacket() and
 *  PacketReceiver::processOrderedPacket() whenever they need to return early
 *  due to a corrupted incoming packet.
 */
#define RETURN_FOR_CORRUPTED_PACKET()										\
	++stats_.numCorruptedPacketsReceived_;									\
	return REASON_CORRUPTED_PACKET;											\


/**
 *	This function has to be very robust, if we intend to use this transport over
 *	the big bad internet. We basically have to assume it'll be complete garbage.
 */
Reason PacketReceiver::processFilteredPacket( const Address & addr,
		Packet * p, ProcessSocketStatsHelper * pStatsHelper )
{
	if (p->totalSize() < int( sizeof( Packet::Flags ) ))
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
					"received undersized packet\n",
				addr.c_str() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	if (pPacketMonitor_)
	{
		pPacketMonitor_->packetIn( addr, *p );
	}

	// p->debugDump();

	// Make sure we understand all the flags
	if (p->flags() & ~Packet::KNOWN_FLAGS)
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
				"received packet with bad flags %x\n",
				addr.c_str(), p->flags() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	if (!p->hasFlags( Packet::FLAG_ON_CHANNEL ))
	{
		++stats_.numPacketsReceivedOffChannel_;
	}

	// Don't allow FLAG_CREATE_CHANNEL on external interfaces
	if (networkInterface_.isExternal() &&
			p->hasFlags( Packet::FLAG_CREATE_CHANNEL ))
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
				"Got FLAG_CREATE_CHANNEL on external interface\n",
				addr.c_str() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	// make sure there's something in the packet
	if (p->totalSize() <= Packet::HEADER_SIZE)
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
				"received undersize packet (%d bytes)\n",
				addr.c_str(), p->totalSize() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	// Start stripping footers
	if (!p->validateChecksum())
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
					"Invalid checksum\n", 
				addr.c_str() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	// Strip off piggybacks and process them immediately (i.e. before the
	// messages on this packet) as the piggybacks must be older than this packet
	if (!this->processPiggybacks( addr, p, pStatsHelper ))
	{
		if (networkInterface_.isVerbose())
		{
			WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
					"Corrupted piggyback packets\n",
				addr.c_str() );
		}

		RETURN_FOR_CORRUPTED_PACKET();
	}

	UDPChannelPtr pChannel = NULL;	// don't bother getting channel twice
	bool shouldSendChannel = false;

	// Strip off indexed channel ID if present
	if (p->hasFlags( Packet::FLAG_INDEXED_CHANNEL ))
	{
		if (!p->stripFooter( p->channelID() ) ||
			!p->stripFooter( p->channelVersion() ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Not enough data for indexed channel footer "
						"(%d bytes left)\n",
					addr.c_str(), p->bodySize() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		switch (networkInterface_.findIndexedChannel( p->channelID(), addr,
				p, pChannel ))
		{
		case NetworkInterface::INDEXED_CHANNEL_CORRUPTED:
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Got indexed channel packet with no finder "
						"registered\n",
					addr.c_str() );
			}

			RETURN_FOR_CORRUPTED_PACKET();

		case NetworkInterface::INDEXED_CHANNEL_HANDLED:
			// If the packet has already been handled, we're done!
			// It may have been forwarded.
			return REASON_SUCCESS;

		case NetworkInterface::INDEXED_CHANNEL_NOT_HANDLED:
			// Common case, handled below
			break;

		default:
			MF_ASSERT( !"Invalid return value" );
			break;
		}

		// If we couldn't find the channel, check if it was recently condemned.
		if (!pChannel)
		{
			pChannel = networkInterface_.findCondemnedChannel( p->channelID() );
		}

		if (pChannel)
		{
			// We update received times for indexed channels here.  Addressed
			// channels are done in processPacket().
			pChannel->onPacketReceived( p->totalSize() );
		}
		else
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Could not get indexed channel for id %d\n",
					addr.c_str(), p->channelID() );
			}

			return REASON_SUCCESS;
		}
	}

	if (!pChannel && p->hasFlags( Packet::FLAG_ON_CHANNEL ))
	{
		pChannel = networkInterface_.findChannel( addr );

		if (!pChannel)
		{
			MF_ASSERT_DEV( !p->shouldCreateAnonymous() );

			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Dropping packet due to absence of local channel\n",
					addr.c_str() );
			}

			return REASON_GENERAL_NETWORK;
		}
	}

	if (pChannel)
	{
		if (pChannel->hasRemoteFailed())
		{
			// __glenc__ We could consider resetting the channel here if it has
			// FLAG_CREATE_CHANNEL.  This would help cope with fast restarts on
			// the same address.  Haven't particularly thought this through
			// though.  Could be issues with app code if you do this
			// underneath.  In fact I'm guessing you would need an onReset()
			// callback to inform the app code that this has happened.
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
					"Dropping packet due to remote process failure\n",
					pChannel->c_str() );
			}

			return REASON_GENERAL_NETWORK;
		}
		// If the packet is out of date, drop it.
		else if (seqLessThan( p->channelVersion(),
					pChannel->creationVersion() ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Dropping packet from old channel %s (v%u < v%u)\n",
					pChannel->c_str(), addr.c_str(),
					p->channelVersion(), pChannel->creationVersion() );
			}

			return REASON_SUCCESS;
		}
		else if (pChannel->wantsFirstPacket())
		{
			if (p->hasFlags( Packet::FLAG_CREATE_CHANNEL ))
			{
				pChannel->gotFirstPacket();
			}
			else
			{
				if (networkInterface_.isVerbose())
				{
					WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Dropping packet on "
						"channel wanting FLAG_CREATE_CHANNEL (flags: %x)\n",
						pChannel->c_str(), p->flags() );
				}

				return REASON_GENERAL_NETWORK;
			}
		}
	}

	if (p->hasFlags( Packet::FLAG_HAS_CUMULATIVE_ACK ))
	{
		if (!pChannel)
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Cumulative ack was not on a channel.\n",
					addr.c_str() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		SeqNum endSeq;

		if (!p->stripFooter( endSeq ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Not enough data for cumulative.\n",
					pChannel->c_str() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		if (!pChannel->handleCumulativeAck( endSeq ))
		{
			RETURN_FOR_CORRUPTED_PACKET();
		}
	}

	// Strip and handle ACKs
	if (p->hasFlags( Packet::FLAG_HAS_ACKS ))
	{
		if (!p->stripFooter( p->nAcks() ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Not enough data for ack count footer "
						"(%d bytes left)\n",
					addr.c_str(), p->bodySize() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		if (p->nAcks() == 0)
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Packet with FLAG_HAS_ACKS had 0 acks\n",
					addr.c_str() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		// The total size of all the ACKs on this packet
		int ackSize = p->nAcks() * sizeof( SeqNum );

		// check that we have enough footers to account for all of the
		// acks the packet claims to have (thanks go to netease)
		if (p->bodySize() < ackSize)
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Not enough footers for %d acks "
						"(have %d bytes but need %d)\n",
					addr.c_str(), p->nAcks(), p->bodySize(), ackSize );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		// For each ACK that we receive, we no longer need to store the
		// corresponding packet.
		if (pChannel)
		{
			for (uint i=0; i < p->nAcks(); i++)
			{
				SeqNum seq;

				if (!p->stripFooter( seq ))
				{
					if (networkInterface_.isVerbose())
					{
						WARNING_MSG(
							"PacketReceiver::processFilteredPacket( %s ): "
								"Not enough data for acknowledgement footer "
								"(%d bytes left)\n",
							addr.c_str(), p->bodySize() );
					}

					RETURN_FOR_CORRUPTED_PACKET();
				}

				if (!pChannel->handleAck( seq ))
				{
					if (networkInterface_.isVerbose())
					{
						WARNING_MSG( "PacketReceiver::"
								"processFilteredPacket( %s ): "
								"handleAck() failed for #%u\n",
							addr.c_str(), seq );
					}

					RETURN_FOR_CORRUPTED_PACKET();
				}
			}
		}
		else if (!p->hasFlags( Packet::FLAG_ON_CHANNEL ))
		{
			for (uint i=0; i < p->nAcks(); i++)
			{
				SeqNum seq;

				if (!p->stripFooter( seq ))
				{
					if (networkInterface_.isVerbose())
					{
						WARNING_MSG(
							"PacketReceiver::processFilteredPacket( %s ): "
								"Not enough data for acknowledgement footer "
								"(%d bytes left)\n",
							addr.c_str(), p->bodySize() );
					}

					RETURN_FOR_CORRUPTED_PACKET();
				}
				networkInterface_.onceOffSender().delOnceOffResendTimer( addr,
						seq );
			}
		}
		else
		{
			p->shrink( ackSize );

			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Got %d acks without a channel\n",
					addr.c_str(), p->nAcks() );
			}
		}
	}

	// Strip sequence number
	if (p->hasFlags( Packet::FLAG_HAS_SEQUENCE_NUMBER ))
	{
		if (!p->stripFooter( p->seq() ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Not enough data for sequence number footer "
						"(%d bytes left)\n",
					addr.c_str(), p->bodySize() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}
	}

	// now do something if it's reliable
	if (p->hasFlags( Packet::FLAG_IS_RELIABLE ))
	{
		// first make sure it has a sequence number, so we can address it
		if (p->seq() == SEQ_NULL)
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Dropping packet due to illegal request for "
						"reliability without related sequence number\n", 
					addr.c_str() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		// should we be looking in a channel
		if (pChannel)
		{
			UDPChannel::AddToReceiveWindowResult result =
				pChannel->addToReceiveWindow( p, addr, stats_ );

			if (!pChannel->isLocalRegular())
			{
				shouldSendChannel = true;
			}

#if 0 // DISABLED for BWT-28650, to be re-enabled as part of BWT-22007
			if (result == UDPChannel::PACKET_IS_BUFFERED_IN_WINDOW)
			{
				this->processEarlyPacket(
					addr, p, pChannel.get(), pStatsHelper );
			}
#endif // DISABLED for BWT-28650, to be re-enabled as part of BWT-22007

			if (result != UDPChannel::PACKET_IS_NEXT_IN_WINDOW)
			{
				// The packet is not corrupted, and has either already been
				// received, or is too early and has been buffered. In either
				// case, we send the ACK immediately, as long as the channel is
				// established and is irregular.
				if (result != UDPChannel::PACKET_IS_CORRUPT)
				{
					if (pChannel->isEstablished() && shouldSendChannel)
					{
						UDPBundle emptyBundle;
						pChannel->send( &emptyBundle );
					}

					return REASON_SUCCESS;
				}

				// The packet has an invalid sequence number.
				else
				{
					RETURN_FOR_CORRUPTED_PACKET();
				}
			}
		}

		// If the packet is not on a channel (i.e. FLAG_ON_CHANNEL is not
		// set), it must be once-off reliable.
		else
		{
			// Don't allow incoming once-off-reliable traffic on external
			// interfaces since this is a potential DOS vulnerability.
			if (networkInterface_.isExternal())
			{
				if (networkInterface_.isVerbose())
				{
					NOTICE_MSG( "PacketReceiver::processFilteredPacket( %s ): "
						"Dropping deprecated once-off-reliable packet\n",
						addr.c_str() );
				}

				return REASON_SUCCESS;
			}

			// send back the ack for this packet
			UDPBundle backBundle;
			backBundle.addAck( p->seq() );
			networkInterface_.send( addr, backBundle );

			if (onceOffReceiver_.onReliableReceived(
						this->dispatcher(), addr, p->seq() ))
			{
				return REASON_SUCCESS;
			}
		}
	}
	else // if !RELIABLE
	{
		// If the packet is unreliable, confirm that the sequence number
		// is incrementing as a sanity check against malicious parties.
		if ((pChannel != NULL) && (pChannel->isExternal()))
		{
			if (!pChannel->validateUnreliableSeqNum( p->seq() ))
			{
				RETURN_FOR_CORRUPTED_PACKET();
			}
		}

	}

	Reason oret = REASON_SUCCESS;
	PacketPtr pCurrPacket = p;
	PacketPtr pNextPacket = NULL;

	// push this packet chain (frequently one) through processOrderedPacket

	// NOTE: We check isCondemned on the channel and not isDead. If a channel
	// has isDestroyed set to true but isCondemned false, we still want to
	// process remaining messages. This can occur if there is a message that
	// causes the entity to teleport. Any remaining messages are still
	// processed and will likely be forwarded from the ghost entity to the
	// recently teleported real entity.

	// TODO: It would be nice to display a message if the channel is condemned
	// but there are messages on it.

	while (pCurrPacket &&
		((pChannel == NULL) || !pChannel->isCondemned()))
	{
		// processOrderedPacket expects packets not to be chained, since
		// chaining is used for grouping fragments into bundles.  The packet
		// chain we've set up doesn't have anything to do with bundles, so we
		// break the packet linkage before passing the packets into
		// processOrderedPacket.  This can mean that packets that aren't the one
		// just received drop their last reference, hence the need for
		// pCurrPacket and pNextPacket.
		pNextPacket = pCurrPacket->next();
		pCurrPacket->chain( NULL );

		// Make sure they are actually packets with consecutive sequences.
		MF_ASSERT( pNextPacket.get() == NULL || 
			seqMask( pCurrPacket->seq() + 1 ) == 
				pNextPacket->seq() );

		// At this point, the only footers left on the packet should be the
		// request and fragment footers.
		Reason ret = this->processOrderedPacket( addr, pCurrPacket.get(),
				pChannel.get(), pStatsHelper );

		if (oret == REASON_SUCCESS)
		{
			oret = ret;
		}

		pCurrPacket = pNextPacket;
	}

	// If this bundle was delivered to a channel and there are still ACKs to
	// send, do it now.
	if (pChannel &&
		!pChannel->isDestroyed() &&
		shouldSendChannel &&
		pChannel->isEstablished() &&
		pChannel->hasAcks())
	{
		UDPBundle emptyBundle;
		pChannel->send( &emptyBundle );
	}

	return oret;
}

namespace
{

/**
 *	This class is used by processPiggybacks to visit and process the piggyback
 *	packets on a packet.
 */
class PiggybackProcessor : public PacketVisitor
{
public:
	PiggybackProcessor( PacketReceiver & receiver,
			const Address & addr,
			ProcessSocketStatsHelper * pStatsHelper ) :
		receiver_( receiver ),
		addr_( addr ),
		pStatsHelper_( pStatsHelper )
	{
	}

private:
	bool onPacket( PacketPtr pPacket )
	{
		Reason reason = receiver_.processFilteredPacket( addr_,
				pPacket.get(), pStatsHelper_ );
		return reason != REASON_CORRUPTED_PACKET;
	}

	PacketReceiver & receiver_;
	const Address & addr_;
	ProcessSocketStatsHelper * pStatsHelper_;
};

} // anonymous namespace


/**
 *	This method processes any piggyback packets that may be on the packet
 *	passed in.
 */
bool PacketReceiver::processPiggybacks( const Address & addr,
		Packet * pPacket, ProcessSocketStatsHelper * pStatsHelper )
{
	PiggybackProcessor processor( *this, addr, pStatsHelper );

	return pPacket->processPiggybackPackets( processor );
}


/**
 * Process an out of order packet (there are some packets before this one
 * that haven't come yet)
 */
void PacketReceiver::processEarlyPacket( const Address & addr, Packet * p,
	UDPChannel * pChannel, ProcessSocketStatsHelper * pStatsHelper )
{
	// only single packet bundles are processed in the early mode #BWT-28245
	if (!p->hasFlags( Packet::FLAG_IS_FRAGMENT ))
	{
		UDPBundleProcessor outputBundle( p, /* isEarly */ true );

		outputBundle.dispatchMessages(
			networkInterface_.interfaceTable(),
			addr,
			pChannel,
			networkInterface_,
			NULL // we don't gather stats for early packets
		);
	}
}


/**
 * Process a packet after any ordering guaranteed by reliable channels
 * has been imposed (further ordering guaranteed by fragmented bundles
 * is still to be imposed)
 */
Reason PacketReceiver::processOrderedPacket( const Address & addr, Packet * p,
	UDPChannel * pChannel, ProcessSocketStatsHelper * pStatsHelper )
{
	// Label to use in debug output
#	define SOURCE_STR (pChannel ? pChannel->c_str() : addr.c_str())

	// Strip first request offset.
	if (p->hasFlags( Packet::FLAG_HAS_REQUESTS ))
	{
		if (!p->stripFooter( p->firstRequestOffset() ))
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processOrderedPacket( %s ): "
					"Not enough data for first request offset (%d bytes left)\n",
					SOURCE_STR, p->bodySize() );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}
		else if (p->firstRequestOffset() > p->msgEndOffset())
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processOrderedPacket( %s ): "
						"First request offset out of bounds "
						"(got offset of %hu bytes, %d bytes left)\n",
					SOURCE_STR, p->firstRequestOffset(), p->msgEndOffset() );
			}
			RETURN_FOR_CORRUPTED_PACKET();
		}
	}

	// Smartpointer required to keep the packet chain alive until the
	// outputBundle is constructed in the event a fragment chain completes.
	PacketPtr pChain = NULL;
	bool isOnChannel = pChannel && p->hasFlags( Packet::FLAG_IS_RELIABLE );

	// Strip fragment footers
	if (!p->hasFlags( Packet::FLAG_IS_FRAGMENT ) &&
			p->hasFlags( Packet::FLAG_HAS_SEQUENCE_NUMBER ) &&
			(isOnChannel && pChannel->pFragments() != NULL))
	{
		// Make sure that for incoming non-fragment packets with sequence
		// numbers, we are not expecting them to be fragments. 
		if (networkInterface_.isExternal())
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "Nub::processOrderedPacket( %s ): "
						"got non-fragment packet, when expecting fragment "
						"(#%u)\n",
					pChannel->c_str(), p->seq() );
			}
		}
		else
		{
			CRITICAL_MSG( "Nub::processOrderedPacket( %s ): "
					"got non-fragment packet, when expecting fragment "
					"(#%u)\n",
				pChannel->c_str(), p->seq() );
		}
		RETURN_FOR_CORRUPTED_PACKET();
	}


	if (p->hasFlags( Packet::FLAG_IS_FRAGMENT ))
	{
		if (!p->stripFragInfo())
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processOrderedPacket( %s ): "
						"Invalid fragment footers\n", SOURCE_STR );
			}

			RETURN_FOR_CORRUPTED_PACKET();
		}

		FragmentedBundle::Key key( addr, p->fragBegin() );

		// Find the existing packet chain for this bundle, if any.
		FragmentedBundlePtr pFragments = NULL;
		FragmentedBundles & fragmentedBundles =
			onceOffReceiver_.fragmentedBundles();
		FragmentedBundles::iterator fragIter = fragmentedBundles.end();

		// Channels are easy, they just maintain their own fragment chain, which
		// makes lookup really cheap.  Note that only reliable packets use the
		// channel's sequence numbers.
		if (isOnChannel)
		{
			pFragments = pChannel->pFragments();
		}

		// We need to look up off-channel fragments in fragmentedBundles
		else
		{
			fragIter = fragmentedBundles.find( key );
			pFragments = (fragIter != fragmentedBundles.end()) ?
				fragIter->second : NULL;
		}

		// If the previous fragment is really old, then this must be some
		// failed bundle that has not been resent and has been given up on, so
		// we get rid of it now.
		if (pFragments && pFragments->isOld() && !isOnChannel)
		{
			if (networkInterface_.isVerbose())
			{
				WARNING_MSG( "PacketReceiver::processOrderedPacket( %s ): "
						"Discarding abandoned stale overlapping fragmented "
						"bundle from seq %u to %u\n",
					SOURCE_STR, p->fragBegin(), pFragments->lastFragment() );
			}

			pFragments = NULL;

			fragmentedBundles.erase( fragIter );
		}

		if (pFragments == NULL)
		{
			// If this is on a channel, it must be the first packet in the
			// bundle, since channel traffic is ordered.
			if (pChannel && p->seq() != p->fragBegin())
			{
				if (networkInterface_.isVerbose())
				{
					ERROR_MSG( "PacketReceiver::processOrderedPacket( %s ): "
							"Bundle (#%u,#%u) is missing packets before #%u\n",
						SOURCE_STR, p->fragBegin(), p->fragEnd(), p->seq() );
				}

				RETURN_FOR_CORRUPTED_PACKET();
			}

			// This is the first fragment from this bundle we've seen, so make
			// a new element and bail out.
			pFragments = new FragmentedBundle( p );

			if (isOnChannel)
			{
				pChannel->pFragments( pFragments );
			}
			else
			{
				fragmentedBundles[ key ] = pFragments;
			}

			return REASON_SUCCESS;
		}

		if (!pFragments->addPacket( p,
					networkInterface_.isExternal(), SOURCE_STR ))
		{
			RETURN_FOR_CORRUPTED_PACKET();
		}

		// If the bundle is still incomplete, stop processing now.
		if (!pFragments->isComplete())
		{
			return REASON_SUCCESS;
		}

		// The bundle is now complete, so set p to the start of the chain and
		// we'll process the whole bundle below.
		else
		{
			// We need to acquire a reference to the fragment chain here to keep
			// it alive until we construct the outputBundle below.
			pChain = pFragments->pChain();
			p = pChain.get();

			if (isOnChannel)
			{
				pChannel->pFragments( NULL );
			}
			else
			{
				fragmentedBundles.erase( fragIter );
			}
		}
	}

	// We have a complete packet chain.  We can drop the reference in pChain now
	// since the Bundle owns it.
	UDPBundleProcessor outputBundle( p );
	pChain = NULL;

	Reason reason = outputBundle.dispatchMessages(
			networkInterface_.interfaceTable(),
			addr,
			pChannel,
			networkInterface_,
			pStatsHelper );

	if (reason == REASON_CORRUPTED_PACKET)
	{
		RETURN_FOR_CORRUPTED_PACKET();
	}

	return reason;

#	undef SOURCE_STR
}


#if ENABLE_WATCHERS
/**
 *	This method returns a Watcher that can be used to inspect PacketReceiver
 *	instances.
 */
WatcherPtr PacketReceiver::pWatcher()
{
	WatcherPtr pWatcher = new DirectoryWatcher();
	PacketReceiver * pNull = NULL;

	// TODO:TN Needs to be named correctly.
	pWatcher->addChild( "stats",
			PacketReceiverStats::pWatcher(), &pNull->stats_ );

	pWatcher->addChild( "maxSocketProcessingTimeStamps",
			makeWatcher( pNull->maxSocketProcessingTimeStamps_,
				Watcher::WT_READ_WRITE ) );

	return pWatcher;
}
#endif


/**
*	This method returns the dispatcher that is used by this receiver.
*/
EventDispatcher & PacketReceiver::dispatcher()
{
	return networkInterface_.dispatcher();
}


/**
 *	This method sets the maximum socket processing time.
 *
 *	@param maxTime 	If non-zero, this sets the maximum time (in seconds)
 *					allowed for socket processing when notified of available
 *					input before relinquishing control.
 *
 *					If zero, the time limit for processing will be disabled,
 *					and socket processing will continue until the receive queue
 *					is empty.
 */
void PacketReceiver::maxSocketProcessingTime( float value )
{
	if ((value > std::numeric_limits< float >::max()) || (value <= 0.f))
	{
		// Disable time limit.
		maxSocketProcessingTimeStamps_ = 0;
	}
	else
	{
		maxSocketProcessingTimeStamps_ = uint64( value * 
			BW_NAMESPACE stampsPerSecondD() );
	}
}


} // namespace Mercury


BW_END_NAMESPACE


// packet_receiver.cpp
