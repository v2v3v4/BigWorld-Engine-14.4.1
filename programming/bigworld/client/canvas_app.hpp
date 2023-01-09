#ifndef CANVAS_APP_HPP
#define CANVAS_APP_HPP


#include "cstdmf/main_loop_task.hpp"
#include "adaptive_lod_controller.hpp"


BW_BEGIN_NAMESPACE

class Distortion;

/**
 *	Canvas task
 */
class CanvasApp : public MainLoopTask
{
public:
	typedef BW::vector<BW::wstring> StringVector;

	CanvasApp();
	~CanvasApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

	static CanvasApp instance;

	Distortion* distortion()			{ return distortion_; }	

	void updateDistortionBuffer();

	const StringVector pythonConsoleHistory() const;
	void setPythonConsoleHistory(const StringVector & history);

private:
	bool setPythonConsoleHistoryNow(const StringVector & history);	

	AdaptiveLODController	lodController_;
	float gammaCorrectionOutside_;
	float gammaCorrectionInside_;
	float gammaCorrectionSpeed_;

	Distortion *			distortion_;

	float	dGameTime_;

	StringVector history_;
public:
	uint32 drawSkyCtrl_;

	void finishFilters();
};

BW_END_NAMESPACE

#endif // CANVAS_APP_HPP

// canvas_app.hpp

