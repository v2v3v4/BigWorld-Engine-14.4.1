#include "incoming_packet.hpp"

#include "network/machine_guard.hpp"

#include "cstdmf/memory_stream.hpp"


#include <syslog.h>

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
IncomingPacket::IncomingPacket( BWMachined & machined,
	MGMPacket * pPacket, sockaddr_in & sin ) :
	machined_( machined ),
	pPacket_( pPacket ),
	sin_( sin )
{
	if (!pPacket_->shouldStaggerReply())
	{
		syslog( LOG_ERR, "Broadcast packet didn't have flag: %x",
			pPacket_->flags_ );
	}
}


/**
 *	Destructor.
 */
IncomingPacket::~IncomingPacket()
{
	bw_safe_delete( pPacket_ );
}


/**
 *
 */
void IncomingPacket::handle()
{
	MemoryOStream os;
	machined_.handlePacket( machined_.endpoint(), sin_, *pPacket_ );
}

BW_END_NAMESPACE

// incoming_packet.cpp
