#ifndef MF_CONSOLIDATE_DBAPP_HPP
#define MF_CONSOLIDATE_DBAPP_HPP

#include "network/basictypes.hpp"
#include "network/machine_guard.hpp"
#include "network/watcher_nub.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

/**
 *
 */
class DBApp : public MachineGuardMessage::ReplyHandler
{
public:
	DBApp( WatcherNub & watcherNub );

	bool init();

	void setStatus( const BW::string & status );

	// MachineGuardMessage::ReplyHandler interface
	bool onProcessStatsMessage( ProcessStatsMessage & psm, uint32 addr );

private:
	WatcherNub &		watcherNub_;
	Mercury::Address	addr_;
};

BW_END_NAMESPACE

#endif // MF_CONSOLIDATE_DBAPP_HPP
