#include "get_base_app_mgr_init_data_task.hpp"

#include "../query.hpp"
#include "../result_set.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
GetBaseAppMgrInitDataTask::GetBaseAppMgrInitDataTask(
		IDatabase::IGetBaseAppMgrInitDataHandler & handler ) :
	MySqlBackgroundTask( "GetBaseAppMgrInitDataTask" ),
	handler_( handler ),
	gameTime_( 0 )
{
}


/**
 *
 */
void GetBaseAppMgrInitDataTask::performBackgroundTask( MySql & connection )
{
	GetBaseAppMgrInitDataTask::getGameTime( connection, gameTime_ );
}


/**
 *
 */
void GetBaseAppMgrInitDataTask::performMainThreadTask( bool succeeded )
{
	handler_.onGetBaseAppMgrInitDataComplete( gameTime_ );
}


/**
 *	This method returns the game time stored in the database. Returns result
 *	via the gameTime parameter.
 */
bool GetBaseAppMgrInitDataTask::getGameTime( MySql & connection,
		GameTime & gameTime )
{
	static const Query query( "SELECT * FROM bigworldGameTime" );

	ResultSet resultSet;
	query.execute( connection, &resultSet );

	return resultSet.getResult( gameTime );
}


BW_END_NAMESPACE

// get_base_app_mgr_init_data_task.cpp
