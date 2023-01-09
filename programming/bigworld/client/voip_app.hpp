#ifndef VOIP_APP_HPP
#define VOIP_APP_HPP


#include "cstdmf/main_loop_task.hpp"

class VOIPClient;

BW_BEGIN_NAMESPACE


/**
 *	This class is a singleton main loop task for managing and ticking the VoIP
 *	client.
 *	This task must be initialised before any script code that could call
 *	BigWorld.VOIP.* is executed.
 *
 *	@see	VOIPClient
 *	@see	PyVOIP
 */
class VOIPApp : public MainLoopTask
{
public:
	VOIPApp();
	~VOIPApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();

	static VOIPClient & getVOIPClient();

private:
	VOIPClient *	voipClient_;

	static VOIPApp	s_instance_;
};

BW_END_NAMESPACE


#endif // VOIP_APP_HPP

// voip_app.hpp
