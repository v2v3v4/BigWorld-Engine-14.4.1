#ifndef MYSQL_BACKGROUND_TASK_HPP
#define MYSQL_BACKGROUND_TASK_HPP

#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class DatabaseException;
class MySql;
class MySqlDatabase;


class MySqlBackgroundTask : public BackgroundTask
{
public:
	MySqlBackgroundTask( const char * taskName );

	void doBackgroundTask( TaskManager & mgr ) {};
	void doBackgroundTask( TaskManager & mgr,
			BackgroundTaskThread * pThread );

	void doMainThreadTask( TaskManager & mgr );

	void setFailure()		{ succeeded_ = false; }

protected:
	virtual void performBackgroundTask( MySql & conn ) = 0;
	virtual void performMainThreadTask( bool succeeded ) = 0;

	virtual void onRetry() {}
	virtual void onException( const DatabaseException & e ) {}

	bool succeeded_;
};

BW_END_NAMESPACE

#endif // MYSQL_BACKGROUND_TASK_HPP
