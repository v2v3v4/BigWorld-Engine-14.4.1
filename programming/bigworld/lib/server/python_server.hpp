#ifndef _PYTHON_SERVER_HEADER
#define _PYTHON_SERVER_HEADER

#include "cstdmf/config.hpp"

#if ENABLE_PYTHON_TELNET_SERVICE


#include "Python.h"

#include <deque>
#include "cstdmf/bw_string.hpp"

#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"

#include "script/script_output_hook.hpp"


BW_BEGIN_NAMESPACE

class PythonServer;

#ifndef _WIN32  // WIN32PORT
#else //ifndef _WIN32  // WIN32PORT
typedef unsigned short uint16_t;
#endif //ndef _WIN32  // WIN32PORT


/**
 *	This class represents a single TCP connection to the Python interpreter.
 */
class PythonConnection : public Mercury::InputNotificationHandler
{
public:
	PythonConnection( PythonServer* owner,
			Mercury::EventDispatcher & dispatcher,
			int fd,
			BW::string welcomeString );
	virtual ~PythonConnection();

	void 						write( const char* str );
	void						writePrompt();

	bool						active() const	{ return active_; }

private:
	int 						handleInputNotification( int fd );
	bool						handleTelnetCommand();
	bool						handleVTCommand();
	void						handleLine();
	void						handleDel();
	void						handleChar();
	void						handleUp();
	void						handleDown();
	void						handleLeft();
	void						handleRight();

	Mercury::EventDispatcher &	dispatcher_;
	std::deque<unsigned char>	readBuffer_;
	std::deque<BW::string>		historyBuffer_;
	BW::string					currentLine_;
	BW::string					currentCommand_;
	Endpoint					socket_;
	PythonServer*				owner_;
	bool						telnetSubnegotiation_;
	int							historyPos_;
	unsigned int				charPos_;
	bool						active_;
	BW::string					multiline_;
};

/**
 *	This class provides access to the Python interpreter via a TCP connection.
 *	It starts listening on a given port, and creates a new PythonConnection
 *	for every connection it receives.
 */
class PythonServer :
	public ScriptOutputHook,
	public Mercury::InputNotificationHandler
{
public:
	PythonServer( BW::string welcomeString = "Welcome to PythonServer." );
	virtual ~PythonServer();

	bool			startup( Mercury::EventDispatcher & dispatcher,
						uint32_t ip, uint16_t port, const char * ifspec = 0 );
	void			shutdown();
	void			deleteConnection( PythonConnection* pConnection );
	uint16_t		port() const;

private:
	int	handleInputNotification(int fd);

	// ScriptOutputHook implementation
	void onScriptOutput( const BW::string & output, bool /* isStderr */ );
	void onOutputWriterDestroyed( ScriptOutputWriter * pOwner );

	BW::vector<PythonConnection*> connections_;

	Endpoint		listener_;
	Mercury::EventDispatcher *	pDispatcher_;
	BW::string		welcomeString_;

	ScriptOutputWriter * pHookedWriter_;
};

BW_END_NAMESPACE

#endif // ENABLE_PYTHON_TELNET_SERVICE


#endif // _PYTHON_SERVER_HEADER

