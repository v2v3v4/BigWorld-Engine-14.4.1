#ifndef ENTITY_CHANNEL_FINDER_HPP
#define ENTITY_CHANNEL_FINDER_HPP

#include "network/channel_finder.hpp"


BW_BEGIN_NAMESPACE

class EntityChannelFinder : public Mercury::ChannelFinder
{
public:
	virtual Mercury::UDPChannel * find( Mercury::ChannelID id,
		const Mercury::Address & srcAddr,
		const Mercury::Packet * pPacket,
		bool & rHasBeenHandled );
};

BW_END_NAMESPACE

#endif // ENTITY_CHANNEL_FINDER_HPP
