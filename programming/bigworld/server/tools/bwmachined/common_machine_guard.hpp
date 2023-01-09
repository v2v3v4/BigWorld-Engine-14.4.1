#pragma once
#ifndef COMMON_MACHINE_GUARD_HPP
#define COMMON_MACHINE_GUARD_HPP

#include "message_with_destination.hpp"

#include "network/portmap.hpp"
#include "network/machine_guard.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"

#include <sys/types.h>
#include <sys/resource.h>

#define MAX_BIT_RATE (1<<27)
#define BIT_INCREMENT (MAX_BIT_RATE / 0xFF)
#define MAX_PACKET_RATE 256000
#define PACK_INCREMENT (MAX_PACKET_RATE / 0xFF)

// Version 2: Makes sure to write empty string to name field in MACHINED_VERSION
//            messages
// Version 3: Listener notifications are now sent back to the registration
//            address instead of 127.0.0.1.
// Version 4: Added support for tags specified in /etc/bwmachined.conf
// Version 5: Using mgm.id to pass back replyID for QUERY_TAG_MESSAGE
// Version 6: mgm.pid == -1 in QUERY_TAG_MESSAGE means category is undefined
// Version 7: Now mgm.uid > mgm.pid means category is undefined
// Version 8: MACHINED_VERSION_MESSAGE replies marked as outgoing
// Version 9: Broadcast replies are now staggered (bigworld-1-7-patch only)
// Version 10:Machined's now have complete knowledge of the network
// Version 11:MGM's are variable length and there can be many on a single packet
// Version 12:Messages are sized prefixed and UnknownMessages are added
// Version 13:Bugfixes for dead watcher nubs and SMP cpu stat calculation
// Version 14:Critical fix for possible segfault on user info init
// Version 15:Dead listeners are now auto-deregistered
// Version 16:Memory leak and death notification fixes
// Version 17:Refactored monolithic machine_guard.cpp into classes
// Version 18:Broadcast-reply-based fault tolerance, handles segmentation
// Version 19:Added ResetMessage
// Version 20:Will not create processes under non-existent UIDs
// Version 21:Dropped packet stats are current values, not deltas
// Version 22:Added ErrorMessage
// Version 23:Bugfix for buddy selection post-bootstrap
// Version 24:Check filehandles for /proc files before reading
// Version 25:Fixed fd leak; more error handling for possible fopen() failures
// Version 26:Added support for ANY_UID ListenerMessages
// Version 27:Repeatedly-offset updates fixed, uses 64-bit timestamps
// Version 28:Coredump reporting is done in UserMessages
// Version 29:Fix for segfault in updateProcessStats()
// Version 30:Added UserMessage::PARAM_REFRESH_ENV
// Version 31:Fixed CPU load calculation on SMP systems
// Version 32:Won't refuse to start processes under UID's not in usermap
// Version 33:Reload usermap after a flush request
// Version 34:Broadcast route interface discovery
// Version 35:Preserve processes on restart; set BW_TIMING_METHOD for children
// Version 36:Take caching into account in system memory calculations
// Version 37:Handles changes to system time (fix in cstdmf/time_queue.*)
// Version 38:setgid() before fork() so users have permissions on their procs
// Version 39:No longer send oversized MGMPacket responses. Max 10 core files.
// Version 40:Added CreateWithArgsMessage
// Version 41:Added HighPrecisionMachineMessage
// Version 42:Added version number to ProcessMessage
// Version 43:Fixed a bug where ResetMessage was not streaming itself correctly.
// Version 44:Delay PidMessage reply in order to determine child process status.
// Version 45:Fixed a memory leak and a daemon shutdown on error handling.
// Version 46:New server binary location / BW_CONFIG changed Hybrid -> hybrid
// Version 47:Add MachinePlatformMessage
// Version 48:Binary directory support for 'el' vs 'centos'/'rhel'
// Version 49:New server binary location. Dropped 32 bit binary support for Linux
// Version 50:Add PARAM_GET_VERSION to UserMessage.

// NOTE: This should stay in sync with the value in pycommon/messages.py
#define BWMACHINED_VERSION 50


BW_BEGIN_NAMESPACE

extern const char * machinedConfFile;
extern const char * bigworldConfFile;

// every platform should implement these
void initProcessState( bool daemon );
void cleanupProcessState();
// Returns the maximum Fd it added to one of the sets
int getInterestingFds( fd_set *readfds, fd_set *writefds, fd_set *exceptfds );
void handleInterestingFds( fd_set *readfds, fd_set *writefds, fd_set *exceptfds );

// bool updateProcessStats(ThingData & td);
void getProcessorSpeeds( BW::vector<float> &speeds );

class BWMachined;

// Returns true if the pPmwd is ready to send
//  (having updated the values for PidMessage::pid_ and pidMessage::running_)
// otherwise the machine-guard code has claimed it
// and will dispose of it later
bool startProcess( const char * bwBinaryDir,
	const char * bwResPath,
	const char * config,
	const char * type,
	MachineGuardMessage::UserId uid,
	uint16 gid,
	const char * home,
	int argc,
	const char ** argv,
	BWMachined &machined,
	PidMessageWithDestination * pPmwd );

struct ProcessInfo;
bool validateProcessInfo( const ProcessInfo &processInfo );

/**
 * Structs used for maintaining high-resolution statistics.
 */

template <class T>
class Stat
{
public:
	Stat() : v1_(), v2_(), v1curr_( true ) {}
	inline T& cur() { return v1curr_ ? v1_ : v2_; }
	inline T& old() { return v1curr_ ? v2_ : v1_; }
	inline T delta() const { return v1curr_ ? v1_ - v2_ : v2_ - v1_; }
	inline void update (const T &t) {
		if (v1curr_)
			v2_ = t;
		else
			v1_ = t;
		v1curr_ = !v1curr_;
	}
	inline T& next() {
		v1curr_ = !v1curr_;
		return this->cur();
	}
	inline bool state() const { return v1curr_; }

private:
	T v1_, v2_;
	bool v1curr_;
};

typedef Stat< uint64 > HighResStat;

struct MaxStat
{
	HighResStat val, max;
};

struct InterfaceInfo
{
	BW::string name;
	HighResStat bitsTotIn, bitsTotOut, packTotIn, packTotOut;
};

struct SystemInfo
{
	uint nCpus, cpuSpeed;
	BW::vector< MaxStat > cpu;	//!< Per cpu load information
	MaxStat iowait;		//!< Total time spent waiting for IO
	MaxStat mem;		//!< System wide memory usage

	HighResStat packTotIn, packDropIn, packTotOut, packDropOut;
	BW::vector< struct InterfaceInfo > ifInfo;

	WholeMachineMessage m;
	HighPrecisionMachineMessage hpm;
};

struct ProcessInfo
{
	ProcessInfo() { starttime = 0; }
	HighResStat cpu, mem;
	int affinity;

	ProcessStatsMessage m;

	// Time (since OS boot) that the process was started
	unsigned long int starttime;

	// Platform specific implementation
	void init( const ProcessMessage &pm );
};

bool updateProcessStats( ProcessInfo &pi );

bool raiseFileDescriptorHardLimit( unsigned long desiredLimit );

typedef enum __rlimit_resource bw_rlimit_resource;
int bw_prlimit( pid_t pid, bw_rlimit_resource resource, const struct rlimit *new_limit,
	struct rlimit *old_limit );

BW_END_NAMESPACE

#endif // COMMON_MACHINE_GUARD_HPP
