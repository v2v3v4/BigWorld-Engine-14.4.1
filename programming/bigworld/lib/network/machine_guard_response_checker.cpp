#include "pch.hpp"

#include "machine_guard_response_checker.hpp"

#include "machine_guard.hpp"

BW_BEGIN_NAMESPACE

void MachineGuardResponseChecker::receivedPacket( uint32 srcAddr,
		const MGMPacket & packet )
{
	receivedFrom_.insert( srcAddr );
	waitingFor_.erase( srcAddr );

	uint32 buddyAddr = packet.buddy_;

	if ((receivedFrom_.count( buddyAddr ) == 0) && (buddyAddr != 0))
	{
		waitingFor_.insert( buddyAddr );
	}
}


bool MachineGuardResponseChecker::isDone() const
{
	return !receivedFrom_.empty() && waitingFor_.empty();
}

BW_END_NAMESPACE

// machine_guard_response_checker.cpp
