#ifndef FACADE_APP_HPP
#define FACADE_APP_HPP


#include "cstdmf/main_loop_task.hpp"
#include "romp/time_of_day.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}
/**
 *	Facade task
 */
class FacadeApp : public MainLoopTask
{
public:
	FacadeApp();
	~FacadeApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

private:
	static FacadeApp							instance;

	float										dGameTime_;
	ClientSpacePtr								lastCameraSpace_;
	SmartPointer<TimeOfDay::UpdateNotifier>		todUpdateNotifier_;
};

BW_END_NAMESPACE


#endif // FACADE_APP_HPP

// facade_app.hpp
