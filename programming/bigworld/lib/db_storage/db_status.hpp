#ifndef DB_STATUS_HPP
#define DB_STATUS_HPP

#include "cstdmf/bw_namespace.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class Watcher;

#define DBSTATUS_WATCHER_STATUS_DETAIL_PATH "statusDetail"

class DBStatus
{
public:
	enum Status
	{
		STARTING,				// Starting up
		STARTUP_CONSOLIDATING,	// Running consolidation during start-up
		WAITING_FOR_APPS,		// Waiting for apps to become ready.
		RESTORING_STATE,		// Restoring entities, spaces, etc.
		RUNNING,				// Running
		SHUTTING_DOWN,			// Shutting down
		SHUTDOWN_CONSOLIDATING	// Running consolidation during shutdown.	
	};

public:
	DBStatus( Status status = STARTING, 
			const BW::string& detail = "Starting" );

	Status status() const 				{ return Status( status_ ); }
	const BW::string& detail() const 	{ return detail_; }
	void detail( const BW::string& detail ) { detail_ = detail; }

	void registerWatchers( Watcher & watcher );

	void set( Status status, const BW::string& detail );

	// Watchers
	bool hasStarted() const		{ return status_ == RUNNING; }

private:
	int			status_;
	BW::string	detail_;
};

BW_END_NAMESPACE

#endif 	// DB_STATUS_HPP
