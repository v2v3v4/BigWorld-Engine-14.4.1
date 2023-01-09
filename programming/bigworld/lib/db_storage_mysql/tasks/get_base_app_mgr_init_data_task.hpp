#ifndef GET_BASE_APP_MGR_INIT_DATA_TASK_HPP
#define GET_BASE_APP_MGR_INIT_DATA_TASK_HPP

#include "db_storage/idatabase.hpp"
#include "background_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements the getBaseAppMgrInitData() function as a thread task.
 */
class GetBaseAppMgrInitDataTask : public MySqlBackgroundTask
{
public:
	GetBaseAppMgrInitDataTask(
			IDatabase::IGetBaseAppMgrInitDataHandler & handler );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	static bool getGameTime( MySql & connection, GameTime & gameTime );

	IDatabase::IGetBaseAppMgrInitDataHandler & 	handler_;
	GameTime	gameTime_;
};

BW_END_NAMESPACE

#endif // GET_BASE_APP_MGR_INIT_DATA_TASK_HPP
