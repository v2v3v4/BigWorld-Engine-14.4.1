#include "base_message_forwarder.hpp"
#include "baseapp_int_interface.hpp"

#include "cstdmf/binary_stream.hpp"
#include "network/bundle.hpp"
#include "network/channel.hpp"
#include "network/network_interface.hpp"
#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
BaseMessageForwarder::BaseMessageForwarder( 
			Mercury::NetworkInterface & networkInterface ):
		map_(),
		networkInterface_( networkInterface )
{}


/**
 *	This method adds a forwarding mapping for the given entity ID to the given
 *	destination address.
 */
void BaseMessageForwarder::addForwardingMapping( EntityID entityID,
		const Mercury::Address & destAddr )
{
	map_[entityID] = destAddr;
}


/**
 *	This method forwards a message if a mapping exists for this entity ID.
 *	Returns true if a mapping exists, and forwarding occurred, otherwise it
 *	returns false.
 */
bool BaseMessageForwarder::forwardIfNecessary( EntityID entityID,
			const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
{
	Map::iterator iMapping = map_.find( entityID );

	if (iMapping != map_.end())
	{
		const Mercury::Address & destAddr = iMapping->second;
		Mercury::Channel & channel =
			networkInterface_.findOrCreateChannel( destAddr );

		Mercury::Bundle & bundle = channel.bundle();

		bundle.startMessage( BaseAppIntInterface::forwardedBaseMessage );
		bundle << srcAddr << header.identifier << header.replyID ;
		// A REAL handler would use this: bundle << entityID;
		bundle.transfer( data, data.remainingLength() );

		return true;
	}
	return false;
}


/**
 *	This method forwards a message if a mapping exists for this entity ID.
 *	Returns true if a mapping exists, and forwarding occurred, otherwise it
 *	returns false.
 */
Mercury::ChannelPtr BaseMessageForwarder::getForwardingChannel( 
		EntityID entityID )
{
	Map::iterator iMapping = map_.find( entityID );

	if (iMapping != map_.end())
	{
		const Mercury::Address & destAddr = iMapping->second;
		return networkInterface_.findChannel( destAddr, 
			/* createAnonymous: */ true );
	}
	return NULL;
}

BW_END_NAMESPACE

// base_message_forwarder.cpp

