#ifndef DELAYED_CHANNELS_HPP
#define DELAYED_CHANNELS_HPP

#include "frequent_tasks.hpp"

#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;
class EventDispatcher;

typedef SmartPointer< UDPChannel > UDPChannelPtr;

/**
 *	This class is responsible for sending delayed channels. These are channels
 *	that have had delayedSend called on them. This is so that multiple messages
 *	can be put on the outgoing bundle instead of sending a bundle for each.
 */
class DelayedChannels : public FrequentTask
{
public:
	DelayedChannels();

	void init( EventDispatcher & dispatcher );
	void fini( EventDispatcher & dispatcher );

	void add( UDPChannel & channel );

	void sendIfDelayed( UDPChannel & channel );

private:
	virtual void doTask();

	typedef BW::set< UDPChannelPtr > Channels;
	Channels channels_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // DELAYED_CHANNELS_HPP
