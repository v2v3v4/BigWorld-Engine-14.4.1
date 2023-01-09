#include "background_task.hpp"

#include "db_storage_mysql/database_exception.hpp"
#include "db_storage_mysql/transaction.hpp"

#include "cstdmf/debug.hpp"

#include "db_storage_mysql/thread_data.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
MySqlBackgroundTask::MySqlBackgroundTask( const char * taskName ) :
	BackgroundTask( taskName ),
	succeeded_( false )
{
}


/**
 *	This method is called in a background thread to perform the task.
 */
void MySqlBackgroundTask::doBackgroundTask( TaskManager & mgr,
		BackgroundTaskThread * pThread )
{
	MySqlThreadData & threadData =
		*static_cast< MySqlThreadData * >( pThread->pData().get() );

	bool retry;

	do
	{
		retry = false;

		try
		{
			succeeded_ = true;
			MySqlTransaction transaction( threadData.connection() );
			this->performBackgroundTask( threadData.connection() );
			transaction.commit();
		}
		catch (DatabaseException & e)
		{
			if (e.isLostConnection())
			{
				INFO_MSG( "MySqlBackgroundTask::doBackgroundTask: "
						"Thread %p lost connection to database. Exception: %s. "
						"Attempting to reconnect.\n",
					&threadData,
					e.what() );

				int attempts = 1;

				while (!threadData.reconnect())
				{
					ERROR_MSG( "MySqlBackgroundTask::doBackgroundTask: "
									"Thread %p reconnect attempt %d failed.\n",
								&threadData,
								attempts );
					timespec t = { 1, 0 };
					nanosleep( &t, NULL );

					++attempts;
				}

				INFO_MSG( "MySqlBackgroundTask::doBackgroundTask: "
							"Thread %p reconnected. Attempts = %d\n",
						&threadData,
						attempts );

				retry = true;
				this->onRetry();
			}
			else if (e.shouldRetry())
			{
				WARNING_MSG(
						"MySqlBackgroundTask::doBackgroundTask: Retrying %s\n",
						this->name() );

				retry = true;
				this->onRetry();
			}
			else
			{
				WARNING_MSG( "MySqlBackgroundTask::doBackgroundTask: "
						"Exception: %s\n",
					e.what() );

				this->setFailure();
				this->onException( e );
			}
		}
	}
	while (retry);

	mgr.addMainThreadTask( this );
}


/**
 *	This method is called in the main thread to complete the task.
 */
void MySqlBackgroundTask::doMainThreadTask( TaskManager & mgr )
{
	this->performMainThreadTask( succeeded_ );
}

BW_END_NAMESPACE

// background_task.cpp
