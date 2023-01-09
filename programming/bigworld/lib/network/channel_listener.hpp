#ifndef CHANNEL_LISTENER_HPP
#define CHANNEL_LISTENER_HPP


#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{

class Channel;


/**
 *	This class is used to inform interested parties about events on a Channel.
 */
class ChannelListener
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~ChannelListener() {}

	/**
	 *	This method is called whenever the given channel has finished a send.
	 */
	virtual void onChannelSend( Channel & channel ) {}

	/**
	 *	This method is called when the given channel has become unusable, and 
	 *	any owned references should be removed.
	 */
	virtual void onChannelGone( Channel & channel ) {}

};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // CHANNEL_LISTENER_HPP
