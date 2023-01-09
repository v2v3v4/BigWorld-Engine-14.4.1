#ifndef MONITORED_CHANNELS_HPP
#define MONITORED_CHANNELS_HPP

#include "interfaces.hpp"

#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class UDPChannel;
class EventDispatcher;

/**
 *	This class is used to store monitored channels.  It provides base
 *	functionality used by IrregularChannels and KeepAliveChannels.
 */
class MonitoredChannels : public TimerHandler
{
public:
	typedef BW::list< UDPChannel * > Channels;
	typedef Channels::iterator iterator;

	MonitoredChannels() : period_( 0.f ), timerHandle_() {}
	virtual ~MonitoredChannels();

	void addIfNecessary( UDPChannel & channel );
	void delIfNecessary( UDPChannel & channel );
	void setPeriod( float seconds, EventDispatcher & dispatcher );

	void stopMonitoring( EventDispatcher & dispatcher );

	iterator end()	{ return channels_.end(); }

protected:
	/**
	 *  This method must return a reference to the Channel's iterator that
	 *  stores its location in this collection.
	 */
	virtual iterator & channelIter( UDPChannel & channel ) const = 0;

	/**
	 *  This method must return the default check period for this collection,
	 *  and will be used in the first call to addIfNecessary if no period has
	 *  been set previously.
	 */
	virtual float defaultPeriod() const = 0;

	Channels channels_;
	float period_;
	TimerHandle timerHandle_;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // MONITORED_CHANNELS_HPP
