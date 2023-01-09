#include "thread_data.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/time_queue.hpp"

#include "connection_info.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: MySqlThreadData
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
MySqlThreadData::MySqlThreadData( const DBConfig::ConnectionInfo & connInfo ) :
	pConnection_( NULL ),
	connectionInfo_( connInfo )
{
}


/**
 *	This method is called in the background thread when the thread has just
 *	started.
 */
bool MySqlThreadData::onStart( BackgroundTaskThread & thread )
{
	mysql_thread_init();

	try
	{
		pConnection_ = new MySql( connectionInfo_ );
	}
	catch ( std::exception& e )
	{
		ERROR_MSG( "MySqlThreadData::onStart: "
				"Failed to set up a mysql connection\n" );
		return false;
	}
	
	return true;
}


/**
 *	This method is called in the background thread when the thread is just about
 *	to stop.
 */
void MySqlThreadData::onEnd( BackgroundTaskThread & thread )
{
	delete pConnection_;
	pConnection_ = NULL;
	mysql_thread_end();
}


/**
 *	This method will attempt to reconnect this thread to the database.
 */
bool MySqlThreadData::reconnect()
{
	return pConnection_ ? pConnection_->reconnectTo( connectionInfo_ ) : false;
}

BW_END_NAMESPACE

// thread_data.cpp
