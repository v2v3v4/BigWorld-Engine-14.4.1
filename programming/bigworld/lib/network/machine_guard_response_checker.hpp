#ifndef MACHINE_GUARD_RESPONSE_CHECKER_HPP
#define MACHINE_GUARD_RESPONSE_CHECKER_HPP

#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class MGMPacket;

/**
 *	This class is used to detect when all response from a broadcast message have
 *	been received from all bwmachined instances.
 */
class MachineGuardResponseChecker
{
public:
	void receivedPacket( uint32 srcAddr, const MGMPacket & packet );

	bool isDone() const;

private:
	BW::set< uint32 > waitingFor_;
	BW::set< uint32 > receivedFrom_;
};

BW_END_NAMESPACE

#endif // MACHINE_GUARD_RESPONSE_CHECKER_HPP
