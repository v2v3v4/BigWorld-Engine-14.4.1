#ifndef IRREGULAR_CHANNELS_HPP
#define IRREGULAR_CHANNELS_HPP

#include "monitored_channels.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 *	This class is used to store irregular channels and manages their resends.
 */
class IrregularChannels : public MonitoredChannels
{
public:
	void addIfNecessary( UDPChannel & channel );

protected:
	iterator & channelIter( UDPChannel & channel ) const;
	float defaultPeriod() const;

private:
	virtual void handleTimeout( TimerHandle, void * );
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // IRREGULAR_CHANNELS_HPP
