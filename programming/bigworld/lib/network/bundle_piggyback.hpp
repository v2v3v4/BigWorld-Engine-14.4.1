#ifndef BUNDLE_PIGGYBACK_HPP
#define BUNDLE_PIGGYBACK_HPP

#include "network/packet.hpp"
#include "network/reliable_order.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *  @internal
 *  A Piggyback is the data structure used to represent a piggyback packet
 *  between the call to Bundle::piggyback() and the data actually being
 *  streamed onto the packet footers during NetworkInterface::send().
 */
class BundlePiggyback
{
public:
	BundlePiggyback( Packet * pPacket,
			Packet::Flags flags,
			SeqNum seq,
			int16 len ) :
		pPacket_( pPacket ),
		flags_( flags ),
		seq_( seq ),
		len_( len )
	{}

	PacketPtr		pPacket_; 	///< Original packet messages come from
	Packet::Flags	flags_;     ///< Header for the piggyback packet
	SeqNum			seq_;       ///< Sequence number of the piggyback packet
	int16			len_;       ///< Length of the piggyback packet
	ReliableVector	rvec_;      ///< Reliable messages to go onto the packet
};


typedef BW::vector< BundlePiggyback* > BundlePiggybacks;

} // end namespace Mercury

BW_END_NAMESPACE

#endif // BUNDLE_PIGGYBACK_HPP

