#ifndef WEB_APP_HPP
#define WEB_APP_HPP

#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	WebApp task
 */
class WebApp : public MainLoopTask
{
public:
	WebApp();
	~WebApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );

	static WebApp instance;
};

BW_END_NAMESPACE

#endif // WEB_APP_HPP

