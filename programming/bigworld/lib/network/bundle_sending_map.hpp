#ifndef BUNDLE_SENDING_MAP_HPP
#define BUNDLE_SENDING_MAP_HPP

#include "basictypes.hpp"

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Bundle;
class NetworkInterface;
class UDPChannel;

/**
 *  This class is useful when you have a lot of data you want to send to a
 *  collection of other apps, but want to group the sends to each app together.
 */
class BundleSendingMap
{
public:
	BundleSendingMap( NetworkInterface & networkInterface ) :
		networkInterface_( networkInterface ) {}
	Bundle & operator[]( const Address & addr );
	void sendAll();

private:
	NetworkInterface & networkInterface_;

	typedef BW::map< Address, UDPChannel* > Channels;
	Channels channels_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // BUNDLE_SENDING_MAP_HPP
