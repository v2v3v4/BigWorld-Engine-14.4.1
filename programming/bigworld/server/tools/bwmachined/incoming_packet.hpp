#ifndef INCOMING_PACKET_HPP
#define INCOMING_PACKET_HPP

#include "bwmachined.hpp"

#include "network/machine_guard.hpp"

BW_BEGIN_NAMESPACE

//class BWMachined;

/**
 *	This class represents an incoming bwmachined packet to be handled.
 */
class IncomingPacket
{
public:
	IncomingPacket( BWMachined & machined, MGMPacket * pPacket,
		sockaddr_in & sin );
	~IncomingPacket();

	void handle();

private:
	BWMachined &machined_;
	MGMPacket *pPacket_;
	sockaddr_in sin_;
};

BW_END_NAMESPACE

#endif // INCOMING_PACKET_HPP
