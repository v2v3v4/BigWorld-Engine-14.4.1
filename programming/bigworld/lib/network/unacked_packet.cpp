#include "pch.hpp"

#include "unacked_packet.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: UnackedPacket
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
UDPChannel::UnackedPacket::UnackedPacket( Packet * pPacket ) :
	pPacket_( pPacket ),
	lastSentAtOutSeq_(),
	lastSentTime_(),
	wasResent_()
{
}


/**
 *	This method reads this object from the input stream.
 */
UDPChannel::UnackedPacket * UDPChannel::UnackedPacket::initFromStream(
	BinaryIStream & data, uint64 timeNow )
{
	PacketPtr pPacket = Packet::createFromStream( data, Packet::UNACKED_SEND );

	if (pPacket)
	{
		UnackedPacket * pInstance = new UnackedPacket( pPacket.get() );

		data >> pInstance->lastSentAtOutSeq_;

		pInstance->lastSentTime_ = timeNow;
		pInstance->wasResent_ = false;

		return pInstance;
	}
	else
	{
		return NULL;
	}
}


/**
 *	This method adds this object to the input stream.
 */
void UDPChannel::UnackedPacket::addToStream(
	UnackedPacket * pInstance, BinaryOStream & data )
{
	if (pInstance)
	{
		Packet::addToStream( data, pInstance->pPacket_.get(),
			Packet::UNACKED_SEND );

		data << pInstance->lastSentAtOutSeq_;
	}
	else
	{
		Packet::addToStream( data, (Packet*)NULL, Packet::UNACKED_SEND );
	}
}

} // namespace Mercury

BW_END_NAMESPACE

// unacked_packet.cpp
