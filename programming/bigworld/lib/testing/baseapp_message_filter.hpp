#ifndef BASEAPP_MESSAGE_FILTER_HPP
#define BASEAPP_MESSAGE_FILTER_HPP

#include "message_filter.hpp"

namespace Testing
{

/**
 * A MessageFilter for BaseAppExtInterface messages, which currently filters
 * messages on the receive side (i.e. on the baseapp before recv(), not on the
 * client after send()).
 */
class BaseAppMessageFilter : public MessageFilter
{
public:
	// Construction stuff
	BaseAppMessageFilter( ChannelData &data );
	static PacketFilter *create( ChannelData &data );
	virtual PacketFilterCreator creator()
		{ return &BaseAppMessageFilter::create; }

	// The function table that understands the BaseAppExtInterface
	static FilterFunctionTable *funcTable;
};

};

#endif // BASEAPP_MESSAGE_FILTER_HPP
