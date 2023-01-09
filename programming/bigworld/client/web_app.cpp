#include "pch.hpp"

#include "web_app.hpp"

#include "app.hpp"
#include "app_config.hpp"

#include "web_render/awesomium_provider.hpp"


BW_BEGIN_NAMESPACE

// Static vars
WebApp WebApp::instance;

int WebApp_token = 1;


/**
 *	Constructor.
 */
WebApp::WebApp()
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Web/App", NULL );
}


/**
 *	Destructor.
 */
WebApp::~WebApp()
{
}


/**
 * Init the web stuff
 */
bool WebApp::init()
{
	if (App::instance().isQuiting())
	{
		return false;
	}
#if ENABLE_AWESOMIUM
	PyAwesomiumProvider::init();
#endif
	return true;
}


/**
 * Fini the web stuff
 */
void WebApp::fini()
{
#if ENABLE_AWESOMIUM
	PyAwesomiumProvider::fini();
#endif
}


/**
 *	Override from MainLoopTasks::tick().
 */
void WebApp::tick( float /* dGameTime */, float /* dRenderTime */ )
{
#if ENABLE_AWESOMIUM
	PyAwesomiumProvider::tick();
#endif
}

BW_END_NAMESPACE

// web_app.cpp

