#ifndef LENS_APP_HPP
#define LENS_APP_HPP


#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Lens task
 */
class LensApp : public MainLoopTask
{
public:
	LensApp();
	~LensApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

private:
	static LensApp instance;

	float dGameTime_;
};

BW_END_NAMESPACE


#endif // LENS_APP_HPP

// lens_app.hpp
