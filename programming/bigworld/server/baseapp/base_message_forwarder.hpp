#ifndef BASE_MESSAGE_FORWARDER_HPP
#define BASE_MESSAGE_FORWARDER_HPP

#include "network/basictypes.hpp"
#include "network/channel.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
namespace Mercury
{
	class NetworkInterface;
	class UnpackedMessageHeader;
}

/**
 *	This class is responsible for maintaining a mapping for bases that have
 *	been offloaded to their offloaded destination addresses, and forwarding
 *	messages destined for those bases to their new location.
 */
class BaseMessageForwarder
{
public:
	BaseMessageForwarder( Mercury::NetworkInterface & networkInterface );

	void addForwardingMapping( EntityID entityID, 
			const Mercury::Address & destAddr );

	bool forwardIfNecessary( EntityID entityID, 
			const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	Mercury::ChannelPtr getForwardingChannel( EntityID entityID );

private:
	typedef BW::map< EntityID, Mercury::Address > Map;

	Map 							map_;
	Mercury::NetworkInterface & 	networkInterface_;
};

BW_END_NAMESPACE

#endif // BASE_MESSAGE_FORWARDER_HPP

