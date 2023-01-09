#ifndef MF_TRANSFER_DB_PROCESS
#define MF_TRANSFER_DB_PROCESS

#include "network/basictypes.hpp"
#include "network/machine_guard.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Class to handle transferring a secondary database from a remote machine.
 */
class TransferDBProcess : public MachineGuardMessage::ReplyHandler
{
public:
	TransferDBProcess( const Mercury::Address & listeningAddr );

	bool transfer( uint32 remoteIP, const BW::string & path );


private:
	// MachineGuardMessage::ReplyHandler interface
	bool onPidMessage( PidMessage & pm, uint32 addr );

// Member data
	bool shouldAbort_;
	Mercury::Address listeningAddr_;
};

BW_END_NAMESPACE

#endif // MF_TRANSFER_DB_PROCESS
