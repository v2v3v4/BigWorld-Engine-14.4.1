#ifndef SCRIPT_APP_HPP
#define SCRIPT_APP_HPP

#include "server_app.hpp"

#include "pyscript/script_events.hpp"

#define SCRIPT_APP_HEADER SERVER_APP_HEADER


BW_BEGIN_NAMESPACE

class PythonServer;

/**
 *	This class is a comman base class for apps with Python scripting.
 */
class ScriptApp : public ServerApp
{
public:
	ScriptApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	virtual ~ScriptApp();

	virtual bool init( int argc, char * argv[] );

	bool initPersonality();
	bool triggerOnInit( bool isReload = false );

	void startPythonServer( uint16 port, int appID );

	ScriptEvents & scriptEvents() { return scriptEvents_; }

	bool triggerDelayableEvent( const char * eventName, PyObject * pArgs );

protected:
	virtual void fini();

	bool initScript( const char * componentName,
			const char * scriptPath1, const char * scriptPath2 = NULL );

private:
	ScriptEvents scriptEvents_;
	PythonServer * pPythonServer_;
};

BW_END_NAMESPACE

#endif // SCRIPT_APP_HPP
