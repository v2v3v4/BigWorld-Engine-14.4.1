#ifndef MYSQL_THREAD_DATA_HPP
#define MYSQL_THREAD_DATA_HPP

#include "connection_info.hpp"
#include "wrapper.hpp"
#include "cstdmf/bgtask_manager.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to store data assoicated with background threads.
 */
class MySqlThreadData : public BackgroundThreadData
{
public:
	MySqlThreadData( const DBConfig::ConnectionInfo & connInfo );

	virtual bool onStart( BackgroundTaskThread & thread );
	virtual void onEnd( BackgroundTaskThread & thread );

	MySql & connection()		{ return *pConnection_; }

	bool reconnect();

private:
	MySql * pConnection_;
	DBConfig::ConnectionInfo connectionInfo_;
};

BW_END_NAMESPACE

#endif // MYSQL_THREAD_DATA_HPP
