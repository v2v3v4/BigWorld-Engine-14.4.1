#ifndef SET_GAME_TIME_TASK_HPP
#define SET_GAME_TIME_TASK_HPP

#include "background_task.hpp"
#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class SetGameTimeTask : public MySqlBackgroundTask
{
public:
	SetGameTimeTask( GameTime gameTime );

	virtual void performBackgroundTask( MySql & conn );
	virtual void performMainThreadTask( bool succeeded );

private:
	GameTime gameTime_;
};

BW_END_NAMESPACE

#endif // SET_GAME_TIME_TASK_HPP
