#ifndef CHANNEL_MAP_HPP
#define CHANNEL_MAP_HPP

#include "basictypes.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;
class NetworkInterface;

/**
 *	This class is used to maintain a collection of channels. They are keyed by
 *	their address.
 */
class ChannelMap
{
public:
	bool add( UDPChannel & channel );
	bool remove( UDPChannel & channel );

	UDPChannel * find( const Address & addr, 
		NetworkInterface * pInterface ) const;

	void delAnonymous( const Address & addr );
	void destroyOwnedChannels();

	bool hasUnackedPackets() const;

private:
	typedef BW::map< Address, UDPChannel * > Map;
	Map map_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // CHANNEL_MAP_HPP
