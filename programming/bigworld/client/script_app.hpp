#ifndef SCRIPT_APP_HPP
#define SCRIPT_APP_HPP


#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Script task
 */
class ScriptApp : public MainLoopTask
{
public:
	ScriptApp();
	~ScriptApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void inactiveTick( float dGameTime, float dRenderTime );
	virtual void draw();

private:
	static ScriptApp instance;
};

BW_END_NAMESPACE


#endif // SCRIPT_APP_HPP

// script_app.hpp
