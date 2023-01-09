#include "pch.hpp"

#include "packet_sender.hpp"
#include "packet_loss_parameters.hpp"

#include "basictypes.hpp"
#include "endpoint.hpp"
#include "once_off_packet.hpp"
#include "packet_monitor.hpp"
#include "rescheduled_sender.hpp"
#include "sending_stats.hpp"
#include "udp_bundle.hpp"
#include "udp_channel.hpp"


BW_BEGIN_NAMESPACE


namespace Mercury
{

/**
 *	Constructor.
 *
 *	@param socket 			The socket to send on.
 *	@param requestManager 	The request manager to use.
 *	@param eventDispatcher 	The event dispatcher.
 *	@param onceOffSender 	The once-off packet sender.
 *	@param sendingStats 	The statistics object.
 */
PacketSender::PacketSender( Endpoint & socket,
		RequestManager & requestManager, EventDispatcher & eventDispatcher,
		OnceOffSender & onceOffSender, SendingStats & sendingStats, 
		PacketLossParameters & packetLossParameters ) :
	socket_( socket ),
	requestManager_( requestManager ),
	eventDispatcher_( eventDispatcher ),
	onceOffSender_( onceOffSender ),
	sendingStats_( sendingStats ),
	seqNumAllocator_( 1 ),
	pPacketMonitor_( NULL ),
	packetLossParameters_( packetLossParameters ),
	isDroppingNextSend_( false ),
	shouldUseChecksums_( false )
{
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
void PacketSender::send( const Address & address,
		UDPBundle & bundle, UDPChannel * pChannel )
{
	MF_ASSERT( address != Address::NONE );
	MF_ASSERT( !pChannel || pChannel->addr() == address );

	MF_ASSERT( !bundle.pChannel() || (bundle.pChannel() == pChannel) );

#if ENABLE_WATCHERS
	sendingStats_.mercuryTimer().start();
#endif // ENABLE_WATCHERS

	if (!bundle.isFinalised())
	{
		// Handle bundles sent off-channel that won't have been finalised by
		// their channels yet.
		bundle.finalise();
		bundle.addReplyOrdersTo( &requestManager_, pChannel );
	}

	// fill in all the footers that are left to us
	Packet * pFirstOverflowPacket = bundle.preparePackets( pChannel,
				seqNumAllocator_, sendingStats_, shouldUseChecksums_ );

	// Finally actually send the darned thing. Do not send overflow packets.
	for (Packet * pPacket = bundle.pFirstPacket();
			pPacket != pFirstOverflowPacket;
			pPacket = pPacket->next())
	{
		this->sendPacket( address, pPacket, pChannel, false );
	}

#if ENABLE_WATCHERS
	sendingStats_.mercuryTimer().stop( 1 );
#endif // ENABLE_WATCHERS

	sendingStats_.numBundlesSent_++;
	sendingStats_.numMessagesSent_ += bundle.numMessages();

	sendingStats_.numReliableMessagesSent_ += bundle.numReliableMessages();
}


/**
 *	This method sends a packet. No result is returned as it cannot be trusted.
 *	The packet may never get to the other end.
 *
 *	@param address 	The destination address.
 *	@param pPacket	The packet to send.
 *	@param pChannel The channel to be sent on, or NULL if off-channel.
 *	@param isResend If true, this is a resend, otherwise, it is the initial
 *					send.
 */
void PacketSender::sendPacket( const Address & address,
						Packet * pPacket,
						UDPChannel * pChannel, bool isResend )
{
	if (isResend)
	{
		sendingStats_.numBytesResent_ += pPacket->totalSize();
		++sendingStats_.numPacketsResent_;
	}
	else
	{
		if (!pChannel && pPacket->hasFlags( Packet::FLAG_IS_RELIABLE ))
		{
			onceOffSender_.addOnceOffResendTimer(
				address, pPacket->seq(), pPacket, *this, eventDispatcher_ );
		}

		if (!pPacket->hasFlags( Packet::FLAG_ON_CHANNEL ))
		{
			++sendingStats_.numPacketsSentOffChannel_;
		}
	}

	// Check if we want artificial loss or latency
	if (!this->rescheduleSend( address, pPacket, pChannel ))
	{
		this->sendRescheduledPacket( address, pPacket, pChannel );
	}
}


/**
 * This method reschedules a packet to be sent to the address provided
 * some short time in the future (or drop it) depending on the latency
 * settings on the nub.
 *
 *
 *	@param addr 		The destination address.
 *	@param pPacket 		The packet to send.
 *	@param pChannel 	The channel to send on, or NULL for off-channel
 *						packets.
 *	@return 			True if the packet was rescheduled for sending, false
 *						if the packet should be sent now.
 */
bool PacketSender::rescheduleSend( const Address & addr,
		Packet * pPacket, UDPChannel * pChannel )
{
	// see if we drop it
	int artificialDropPerMillion = packetLossParameters_.dropPerMillion();
	if (this->isDroppingNextSend() ||
		((artificialDropPerMillion != 0) &&
		rand() < int64( artificialDropPerMillion ) * RAND_MAX / 1000000))
	{
		if (pPacket->seq() != SEQ_NULL)
		{
			if (pPacket->channelID() != CHANNEL_ID_NULL)
			{
				DEBUG_MSG( "PacketSender::rescheduleSend( %s ): "
						"dropped packet #%u to %s/%d due to artificial loss\n",
					socket_.c_str(), pPacket->seq(), addr.c_str(), 
					pPacket->channelID() );
			}
			else
			{
				DEBUG_MSG( "PacketSender::rescheduleSend( %s ): "
						"dropped packet #%u to %s due to artificial loss\n",
					socket_.c_str(), pPacket->seq(), addr.c_str() );
			}
		}
		else
		{
			// All internal messages should be reliable
			// TODO: Removing this as client currently sets isExternal_ to
			// false temporarily while logging in.
#if 0
			MF_ASSERT( isExternal_ ||
				pPacket->msgEndOffset() == Packet::HEADER_SIZE );
#endif

			DEBUG_MSG( "PacketSender::rescheduleSend( %s ): "
					"Dropped packet with %d ACKs to %s due to artificial "
					"loss\n",
				socket_.c_str(), pPacket->nAcks(), addr.c_str() );
		}

		isDroppingNextSend_ = false;

		sendingStats_.numBytesSent_ += pPacket->totalSize() + UDP_OVERHEAD;
		sendingStats_.numPacketsSent_++;
		return true;
	}

	int artificialLatencyMax = packetLossParameters_.maxLatencyMillion();
	// now see if we delay it
	if (artificialLatencyMax == 0)
	{
		return false;
	}

	int artificialLatencyMin = packetLossParameters_.minLatencyMillion();
	int latency = artificialLatencyMin;

	if (artificialLatencyMax > artificialLatencyMin)
	{
		latency += rand() % (artificialLatencyMax - artificialLatencyMin);
	}

	if (latency < 2)
	{
		return false;	// 2ms => send now
	}

	// ok, we'll delay this packet then
	new RescheduledSender( eventDispatcher_, *this,
			addr, pPacket, pChannel, latency );

	return true;
}


/**
 *	This method sends the packet after rescheduling has occurred.
 *
 *	@param address 	The destination address.
 *	@param pPacket 	The packet.
 *	@param pChannel The channel, or NULL if off-channel send.
 */
void PacketSender::sendRescheduledPacket( const Address & address,
						Packet * pPacket,
						UDPChannel * pChannel )
{
	PacketFilterPtr pFilter = pChannel ? pChannel->pFilter() : NULL;

	if (pPacketMonitor_)
	{
		pPacketMonitor_->packetOut( address, *pPacket );
	}

	// Otherwise send as appropriate
	if (pFilter)
	{
		pFilter->send( *this, address, pPacket );
	}
	else
	{
		this->basicSendWithRetries( address, pPacket );
	}
}


/**
 *	Basic packet sending functionality that retries a few times
 *	if there are transitory errors.
 *
 *	@param addr 	The destination address.
 *	@param pPacket 	The packet to send.
 *
 *	@return 		REASON_SUCCESS on success, otherwise an appropriate
 *					Mercury::Reason.
 */
Reason PacketSender::basicSendWithRetries( const Address & addr,
		Packet * pPacket )
{
	// try sending a few times
	int retries = 0;
	Reason reason;

	while (retries <= 3)
	{
		++retries;
#if ENABLE_WATCHERS
		sendingStats_.systemTimer().start();
#endif // ENABLE_WATCHERS

		reason = this->basicSendSingleTry( addr, pPacket );

#if ENABLE_WATCHERS
		sendingStats_.systemTimer().stop( 1 );
#endif // ENABLE_WATCHERS

		if (reason == REASON_SUCCESS)
			return reason;

		// If we've got an error in the queue simply send it again;
		// we'll pick up the error later.
		if (reason == REASON_NO_SUCH_PORT)
		{
			continue;
		}

		// If the transmit queue is full wait 10ms for it to empty.
		if ((reason == REASON_RESOURCE_UNAVAILABLE) ||
				(reason == REASON_TRANSMIT_QUEUE_FULL))
		{
			fd_set	fds;
			struct timeval tv = { 0, 10000 };
			FD_ZERO( &fds );
			FD_SET( socket_.fileno(), &fds );

			WARNING_MSG( "PacketSender::basicSendWithRetries( %s ): "
					"Transmit queue full, waiting for space... (%d)\n",
				socket_.c_str(), retries );

#if ENABLE_WATCHERS
			sendingStats_.systemTimer().start();
#endif

			select( socket_.fileno()+1, NULL, &fds, NULL, &tv );

#if ENABLE_WATCHERS
			sendingStats_.systemTimer_.stop( 1 );
#endif

			continue;
		}

		// some other error, so don't bother retrying
		break;
	}

	return reason;
}


/**
 *	Basic packet sending function that just tries to send once.
 *
 *	@param addr 	The destination address.
 *	@param pPacket 	The packet to send.
 *
 *	@return 		REASON_SUCCESS on success otherwise an appropriate
 *					Mercury::Reason.
 */
Reason PacketSender::basicSendSingleTry( const Address & addr, 
		Packet * pPacket )
{
	int len = socket_.sendto( pPacket->data(), pPacket->totalSize(), 
		addr.port, addr.ip );

	if (len == pPacket->totalSize())
	{
		sendingStats_.numBytesSent_ += len + UDP_OVERHEAD;
		sendingStats_.numPacketsSent_++;

		return REASON_SUCCESS;
	}
	else
	{
		int err;
		Reason reason;

		sendingStats_.numFailedPacketSend_++;

		// We need to store the error number because it may be changed by later
		// calls (such as OutputDebugString) before we use it.
		#ifndef _WIN32
			err = errno;

			switch (err)
			{
				case ECONNREFUSED:	reason = REASON_NO_SUCH_PORT; break;
				case EAGAIN:		reason = REASON_RESOURCE_UNAVAILABLE; break;
				case ENOBUFS:		reason = REASON_TRANSMIT_QUEUE_FULL; break;
				case EMSGSIZE:		reason = REASON_MESSAGE_TOO_LONG; break;
				default:			reason = REASON_GENERAL_NETWORK; break;
			}
		#else
			err = WSAGetLastError();

			switch (err)
			{
				case WSAECONNREFUSED:
					reason = REASON_NO_SUCH_PORT;
					break;
				case WSAEWOULDBLOCK:
				case WSAEINTR:
					reason = REASON_RESOURCE_UNAVAILABLE;
					break;
				case WSAENOBUFS:
					reason = REASON_TRANSMIT_QUEUE_FULL;
					break;
				case WSAEMSGSIZE:
					reason = REASON_MESSAGE_TOO_LONG;
					break;
				default:
					reason = REASON_GENERAL_NETWORK;
					break;
			}
		#endif

		if (len == -1)
		{
			if (reason == REASON_MESSAGE_TOO_LONG)
			{
				ERROR_MSG( "PacketSender::basicSendSingleTry( %s ): "
						"Packet to %s: Could not send: %s "
						"(packet size %d bytes)\n",
					socket_.c_str(), addr.c_str(), strerror( err ),
					pPacket->totalSize() );
			}
			else if (reason != REASON_NO_SUCH_PORT)
			{
				ERROR_MSG( "PacketSender::basicSendSingleTry( %s ): "
						"Packet to %s: Could not send: %s\n",
					socket_.c_str(), addr.c_str(), strerror( err ) );
			}
		}
		else
		{
			WARNING_MSG( "PacketSender::basicSendSingleTry( %s ): "
					"Packet to %s: length %d does not match sent length %d "
					"(err = %s)\n",
				socket_.c_str(), addr.c_str(), pPacket->totalSize(), len,
				strerror( err ) );
		}

		return reason;
	}
}

} // namespace Mercury


BW_END_NAMESPACE


// packet_sender.cpp
