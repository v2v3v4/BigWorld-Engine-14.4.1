#ifndef _PYTHON_SERVER_HEADER
#define _PYTHON_SERVER_HEADER

#include "cstdmf/config.hpp"

#if ENABLE_PYTHON_TELNET_SERVICE


#include <deque>
#include "cstdmf/bw_string.hpp"

typedef unsigned short uint16_t;

#include "network/interfaces.hpp"
#include "network/endpoint.hpp"
#include "script/script_output_hook.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class EventDispatcher;
}

class PythonServer;

/**
 *	This class implements a subset of the telnet protocol.
 */
class TelnetConnection : public Mercury::InputNotificationHandler
{
public:
	TelnetConnection( Mercury::EventDispatcher & dispatcher, socket_t fd );
	virtual ~TelnetConnection();

	int 						handleInputNotification(int fd);
	void 						write(const char* str);
	bool						active() const	{ return active_; }

protected:
	/**
	 *	Handle a character.
	 *	Return false if you need more input.
	 *	Otherwise remove the input you use and return true.
	 */
	virtual bool				handleChar()
		{ readBuffer_.pop_front(); return false; }
	virtual bool				handleVTCommand()
		{ return this->handleChar(); }
	virtual void				connectionBad() = 0;

	std::deque<unsigned char>	readBuffer_;
	bool						active_;

private:
	Mercury::EventDispatcher *	pDispatcher_;
	Endpoint					socket_;
	bool						handleTelnetCommand();

	bool						telnetSubnegotiation_;
};

/**
 *	This class represents a single TCP connection to the Python interpreter.
 */
class PythonConnection : public TelnetConnection
{
public:
	PythonConnection( PythonServer* owner,
		Mercury::EventDispatcher & dispatcher, socket_t fd);
	~PythonConnection();

	void						writePrompt();

private:
	virtual bool				handleVTCommand();
	virtual bool				handleChar();
	virtual void				connectionBad();

	void						handleLine();
	void						handleDel();
	void						handlePrintableChar();
	void						handleUp();
	void						handleDown();
	void						handleLeft();
	void						handleRight();

	std::deque<BW::string>		historyBuffer_;
	BW::string					currentLine_;
	BW::string					currentCommand_;
	PythonServer*				owner_;
	int							historyPos_;
	unsigned int				charPos_;
};


class KeyboardConnection;

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
	PythonServer();
	virtual ~PythonServer();

	bool			startup( Mercury::EventDispatcher & dispatcher,
						uint16_t port);
	void			shutdown();
	void			deleteConnection(TelnetConnection* pConnection);
	uint16_t		port() const;

	void			pollInput();

private:
	int	handleInputNotification(int fd);

	// ScriptOutputHook implementation
	virtual void onScriptOutput( const BW::string & output, bool /* isStderr */ );
	virtual void onOutputWriterDestroyed( ScriptOutputWriter * pOwner );

	BW::vector<PythonConnection*> connections_;

	Endpoint		listener_;
	Mercury::EventDispatcher *	pDispatcher_;

	Endpoint		kbListener_;
	BW::vector<KeyboardConnection*> kbConnections_;

	ScriptOutputWriter * pHookedWriter_;
};

BW_END_NAMESPACE

#endif // ENABLE_PYTHON_TELNET_SERVICE


#endif // _PYTHON_SERVER_HEADER

