#ifndef PROFILER_APP_HPP
#define PROFILER_APP_HPP

#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Profiler task
 */
class ProfilerApp : public MainLoopTask
{
public:
	ProfilerApp();
	~ProfilerApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

	void				updateProfiler( float dTime );
	void				updateCamera( float dTime );

	static ProfilerApp	instance;

private:

	float				cpuStall_;

	float				filteredDTime_;
	float				filteredFps_;

public:
};

BW_END_NAMESPACE

#endif // PROFILER_APP_HPP

// profiler_app.hpp
