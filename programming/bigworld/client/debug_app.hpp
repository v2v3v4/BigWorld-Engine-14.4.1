#ifndef DEBUG_APP_HPP
#define DEBUG_APP_HPP


#include "cstdmf/main_loop_task.hpp"


BW_BEGIN_NAMESPACE

class FrameRateGraph;
class PythonServer;
class VersionInfo;


/**
 *	Debug task
 */
class DebugApp : public MainLoopTask
{
public:
	DebugApp();
	~DebugApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

	static DebugApp instance;

	VersionInfo *		pVersionInfo_;
	FrameRateGraph *	pFrameRateGraph_;

private:
#if ENABLE_PYTHON_TELNET_SERVICE
	bool				enableServers() const;
	void				enableServers( bool enable );

	PythonServer *		pPythonServer_;
#endif

	BW::string			driverName_;
	BW::string			driverDesc_;
	BW::string			deviceName_;
	BW::string			hostName_;

	float				dRenderTime_;
	float				fps_;
	float				fpsAverage_;
	float				maxFps_;
	float				minFps_;
	float				fpsCache_[50];
	float				dTimeCache_[50];
	int					fpsIndex_;
	float				timeSinceFPSUpdate_;

public:
	float				slowTime_;
	bool				drawSpecialConsole_;
	bool				enableServers_;
	bool				shouldBreakOnCritical_;
	bool				shouldAddTimePrefix_;
};

BW_END_NAMESPACE

#endif // DEBUG_APP_HPP
// debug_app.hpp
