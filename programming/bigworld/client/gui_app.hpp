#ifndef GUI_APP_HPP
#define GUI_APP_HPP


#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	GUI task
 */
class GUIApp : public MainLoopTask
{
public:
	GUIApp();
	~GUIApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

public:
	static GUIApp instance;
};

BW_END_NAMESPACE

#endif // GUI_APP_HPP

// gui_app.hpp
