#include "set_game_time_task.hpp"

#include "../query.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: SetGameTimeTask
// -----------------------------------------------------------------------------

SetGameTimeTask::SetGameTimeTask( GameTime gameTime ) :
	MySqlBackgroundTask( "SetGameTimeTask" ),
	gameTime_( gameTime )
{
}


/**
 *	This method updates the game time in the database.
 */
void SetGameTimeTask::performBackgroundTask( MySql & conn )
{
	static const Query query( "UPDATE bigworldGameTime SET time=?" );

	query.execute( conn, gameTime_, NULL );
}


void SetGameTimeTask::performMainThreadTask( bool succeeded )
{
}

BW_END_NAMESPACE

// set_game_time_task.cpp
