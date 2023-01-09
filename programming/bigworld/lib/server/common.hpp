#ifndef COMMON_HPP
#define COMMON_HPP

#include "cstdmf/bwversion.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Default values
// -----------------------------------------------------------------------------

/// Should we enable content updating (will updater be run)?
const bool DEFAULT_UPDATER_ENABLED = false;

/// This is the frequency at which the game runs. Packets are sent to the client
/// at this rate.
const long DEFAULT_GAME_UPDATE_HERTZ = 10;

/// This is the bandwidth to clients in bytes per second.
const long DEFAULT_BANDWIDTH_TO_CLIENT = 256;

/// This is the default amount of CPU load that is attempted to be offloaded
/// each balancing iteration.
const float DEFAULT_MAX_CPU_OFFLOAD = 0.02f;

enum ShutDownStage
{
	// This means 'not shutting down' and is what shutdown state variables are
	// typically initialised to.
	SHUTDOWN_NONE,

	// This means please start your phase of a controlled shutdown (which may
	// involve requesting other processes to shutdown)
	SHUTDOWN_REQUEST,

	// This is used to inform a parent process that is waiting for you to shut
	// down about the stage of the shutdown you're up to.
	SHUTDOWN_INFORM,

	// This is part of the BaseApp shutdown sequence
	SHUTDOWN_DISCONNECT_PROXIES,

	// This means please shut down this process (i.e. don't propagate anything
	// to dependent processes, just shut down now now NOW!).
	SHUTDOWN_PERFORM,

	// This is only understood by the BaseAppMgr and means please trigger a
	// controlled shutdown from the most 'senior' process that currently exists,
	// i.e. try the LoginApp first, then the DBApp, then the BaseAppMgr.
	SHUTDOWN_TRIGGER
};


/**
 *	Used with updating the logon record for a persistent entity.
 */
enum UpdateAutoLoad
{
	/** Don't modify the auto load state. */
	UPDATE_AUTO_LOAD_RETAIN,
	/** Set the auto-load state to true. */
	UPDATE_AUTO_LOAD_TRUE,
	/** Set the auto-load state to false. */
	UPDATE_AUTO_LOAD_FALSE
};

BW_END_NAMESPACE

#endif // COMMON_HPP
