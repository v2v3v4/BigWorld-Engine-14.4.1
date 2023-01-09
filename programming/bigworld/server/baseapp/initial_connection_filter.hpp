#ifndef INITIAL_CONNECTION_FILTER_HPP
#define INITIAL_CONNECTION_FILTER_HPP

#include "network/packet_filter.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
	class ProcessSocketStatsHelper;
}

class InitialConnectionFilter;
typedef SmartPointer< InitialConnectionFilter > InitialConnectionFilterPtr;


/**
 *  Default PacketFilter for external interface of a BaseApp
 *	filters unexpected off-channel packets
 */
class InitialConnectionFilter : public Mercury::PacketFilter
{
public:
	// Factory method
	static InitialConnectionFilterPtr create();

	// Overrides from PacketFilter 
	virtual Mercury::Reason recv( Mercury::PacketReceiver & receiver,
						const Mercury::Address & addr, Mercury::Packet * pPacket,
						Mercury::ProcessSocketStatsHelper * pStatsHelper );

	virtual int maxSpareSize();


protected:

	InitialConnectionFilter() {}
	// Only deletable via SmartPointer
	virtual ~InitialConnectionFilter();

private:
	// Internal methods

	// Prevent copy-construct or copy-assignment
	InitialConnectionFilter( const InitialConnectionFilter & other );
	InitialConnectionFilter & operator=( const InitialConnectionFilter & other );

};

BW_END_NAMESPACE


#endif // INITIAL_CONNECTION_FILTER_HPP
