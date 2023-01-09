#include "script/first_include.hpp"

#include "python_server.hpp"
#include "server_app.hpp"

#include "cstdmf/config.hpp"

#include "network/network_interface.hpp"
#include "network/machined_utils.hpp"

#if ENABLE_PYTHON_TELNET_SERVICE

#include "pyscript/python_input_substituter.hpp"

#include "script/script_output_hook.hpp"

#include <node.h>
#include <grammar.h>
#include <parsetok.h>
#include <errcode.h>

extern grammar _PyParser_Grammar;

DECLARE_DEBUG_COMPONENT(0)

#define TELNET_ECHO			1
#define TELNET_LINEMODE		34
#define TELNET_SE			240
#define TELNET_SB			250
#define TELNET_WILL			251
#define TELNET_DO			252
#define TELNET_WONT			253
#define TELNET_DONT			254
#define TELNET_IAC			255

#define ERASE_EOL			"\033[K"

#define KEY_CTRL_C			3
#define KEY_CTRL_D			4
#define KEY_BACKSPACE		8
#define KEY_DEL				127
#define KEY_ESC				27

#define MAX_HISTORY_LINES	50


BW_BEGIN_NAMESPACE

/**
 *	This constructor intitialises the PythonConnection given an existing
 *	socket.
 */
PythonConnection::PythonConnection( PythonServer * owner,
			Mercury::EventDispatcher & dispatcher, int fd,
			BW::string welcomeString ) :
		dispatcher_( dispatcher ),
		owner_( owner ),
		telnetSubnegotiation_( false ),
		historyPos_( -1 ),
		charPos_( 0 ),
		active_( false ),
		multiline_()
{
	socket_.setFileDescriptor( fd );
	dispatcher_.registerFileDescriptor( socket_.fileno(), this,
		"PythonConnection" );

	unsigned char options[] =
	{
		TELNET_IAC, TELNET_WILL, TELNET_ECHO,
		TELNET_IAC, TELNET_WONT, TELNET_LINEMODE, 0
	};

	this->write( (char*)options );
	this->write( welcomeString.c_str() );
	this->write( "\r\n" );
	this->writePrompt();
}

/**
 *	This is the destructor.
 *	It deregisters the socket with Mercury. Note that the Endpoint
 *	destructor will close the socket.
 */
PythonConnection::~PythonConnection()
{
	dispatcher_.deregisterFileDescriptor( socket_.fileno() );
}

/**
 *	This method is called by Mercury when the socket is ready for reading.
 *	It processes user input from the socket, and sends it to Python.
 *
 *	@param fd	Socket that is ready for reading.
 */
int PythonConnection::handleInputNotification(int fd)
{
	MF_ASSERT(fd == socket_.fileno());

	char buf[256];
	int i, bytesRead;

	bytesRead = recv(fd, buf, sizeof(buf), 0);

	if (bytesRead == -1)
	{
		ERROR_MSG( "PythonConnection %d: Read error\n", fd );
		owner_->deleteConnection(this);
		return 1;
	}

	if (bytesRead == 0)
	{
		owner_->deleteConnection(this);
		return 1;
	}

	for(i = 0; i < bytesRead; i++)
	{
		readBuffer_.push_back(buf[i]);
	}

	while (!readBuffer_.empty())
	{
		int c = (unsigned char)readBuffer_[0];

		// Handle (and ignore) telnet protocol commands.

		if (c == TELNET_IAC)
		{
			if (!this->handleTelnetCommand())
				return 1;
			continue;
		}

		if (c == KEY_ESC)
		{
			if (!this->handleVTCommand())
				return 1;
			continue;
		}

		// If we're in telnet subnegotiation mode, ignore normal chars.

		if (telnetSubnegotiation_)
		{
			readBuffer_.pop_front();
			continue;
		}

		switch (c)
		{
			case '\r':
				this->handleLine();
				break;

			case KEY_BACKSPACE:
			case KEY_DEL:
				this->handleDel();
				break;

			case KEY_CTRL_C:
			case KEY_CTRL_D:
				owner_->deleteConnection(this);
				return 1;

			case '\0':
			case '\n':
				// Ignore these
				readBuffer_.pop_front();
				break;

			default:
				this->handleChar();
				break;
		}
	}

	return 1;
}

/**
 * 	This method handles telnet protocol commands. Well actually it handles
 * 	a subset of telnet protocol commands, enough to get Linux and Windows
 *	telnet working in character mode.
 */
bool PythonConnection::handleTelnetCommand()
{
	// TODO: Need to check that there is a second byte on readBuffer_.
	unsigned int cmd = (unsigned char)readBuffer_[1];
	unsigned int bytesNeeded = 2;
	char str[256];

	switch (cmd)
	{
		case TELNET_WILL:
		case TELNET_WONT:
		case TELNET_DO:
		case TELNET_DONT:
			bytesNeeded = 3;
			break;

		case TELNET_SE:
			telnetSubnegotiation_ = false;
			break;

		case TELNET_SB:
			telnetSubnegotiation_ = true;
			break;

		case TELNET_IAC:
			// A literal 0xff. We don't care!
			break;

		default:
			sprintf(str, "Telnet command %u unsupported.\r\n", cmd);
			this->write(str);
			break;
	}

	if (readBuffer_.size() < bytesNeeded)
		return false;

	while (bytesNeeded)
	{
		bytesNeeded--;
		readBuffer_.pop_front();
	}

	return true;
}

bool PythonConnection::handleVTCommand()
{
	// Need 3 chars before we are ready.
	if (readBuffer_.size() < 3)
		return false;

	// Eat the ESC.
	readBuffer_.pop_front();

	if (readBuffer_.front() != '[' && readBuffer_.front() != 'O')
		return true;

	// Eat the [
	readBuffer_.pop_front();

	switch (readBuffer_.front())
	{
		case 'A':
			this->handleUp();
			break;

		case 'B':
			this->handleDown();
			break;

		case 'C':
			this->handleRight();
			break;

		case 'D':
			this->handleLeft();
			break;

		default:
			return true;
	}

	readBuffer_.pop_front();
	return true;
}


/**
 * 	This method handles a single character. It appends or inserts it
 * 	into the buffer at the current position.
 */
void PythonConnection::handleChar()
{
	// @todo: Optimise redraw
	currentLine_.insert(charPos_, 1, (char)readBuffer_.front());
	int len = currentLine_.length() - charPos_;
	this->write(currentLine_.substr(charPos_, len).c_str());

	for(int i = 0; i < len - 1; i++)
		this->write("\b");

	charPos_++;
	readBuffer_.pop_front();
}

#if 0
/**
 * 	This method returns true if the command would fail because of an EOF
 * 	error. Could use this to implement multiline commands.. but later.
 */
static bool CheckEOF(char *str)
{
	node *n;
	perrdetail err;
	n = PyParser_ParseString(str, &_PyParser_Grammar, Py_single_input, &err);

	if (n == NULL && err.error == E_EOF )
	{
		printf("EOF\n");
		return true;
	}

	printf("OK\n");
	PyNode_Free(n);
	return false;
}
#endif

/**
 * 	This is a variant on PyRun_SimpleString. It does basically the
 *	same thing, but uses Py_single_input, so the Python compiler
 * 	will mark the code as being interactive, and print the result
 *	if it is not None.
 *
 *	@param command		Line of Python to execute.
 */
static int MyRun_SimpleString(char *command)
{
	// Ignore lines that only contain comments
	{
		char * pCurr = command;
		while (*pCurr != '\0' && *pCurr != ' ' && *pCurr != '\t')
		{
			if (*pCurr == '#')
				return 0;
			++pCurr;
		}
	}

	PyObject *m, *d, *v;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);

	v = PyRun_String(command, Py_single_input, d, d);

	if (v == NULL) {
		PyErr_Print();
		return -1;
	}
	Py_DECREF(v);
	if (Py_FlushLine())
		PyErr_Clear();
	return 0;
}

/**
 * 	This method handles an end of line. It executes the current command,
 *	and adds it to the history buffer.
 */
void PythonConnection::handleLine()
{
	readBuffer_.pop_front();
	this->write("\r\n");

	if (currentLine_.empty())
	{
		currentLine_ = multiline_;
		multiline_ = "";
	}
	else
	{
		historyBuffer_.push_back(currentLine_);

		if (historyBuffer_.size() > MAX_HISTORY_LINES)
		{
			historyBuffer_.pop_front();
		}

		if (!multiline_.empty())
		{
			multiline_ += "\n" + currentLine_;
			currentLine_ = "";
		}
	}

	if (!currentLine_.empty())
	{
		currentLine_ += "\n";

		if (currentLine_[ currentLine_.length() - 2 ] == ':')
		{
			multiline_ += currentLine_;
		}
		else
		{
			active_ = true;
			MyRun_SimpleString((char *)currentLine_.c_str());
			// PyObject * pRes = PyRun_String( (char *)currentLine_.c_str(), true );
			active_ = false;
		}
	}

	currentLine_ = "";
	historyPos_ = -1;
	charPos_ = 0;

	this->writePrompt();
}


/**
 *	This method handles a del character.
 */
void PythonConnection::handleDel()
{
	if (charPos_ > 0)
	{
		// @todo: Optimise redraw
		currentLine_.erase(charPos_ - 1, 1);
		this->write("\b" ERASE_EOL);
		charPos_--;
		int len = currentLine_.length() - charPos_;
		this->write(currentLine_.substr(charPos_, len).c_str());

		for(int i = 0; i < len; i++)
			this->write("\b");
	}

	readBuffer_.pop_front();
}


/**
 * 	This method handles a key up event.
 */
void PythonConnection::handleUp()
{
	if (historyPos_ < (int)historyBuffer_.size() - 1)
	{
		historyPos_++;
		currentLine_ = historyBuffer_[historyBuffer_.size() -
			historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = currentLine_.length();
	}
}


/**
 * 	This method handles a key down event.
 */
void PythonConnection::handleDown()
{
	if (historyPos_ >= 0 )
	{
		historyPos_--;

		if (historyPos_ == -1)
			currentLine_ = "";
		else
			currentLine_ = historyBuffer_[historyBuffer_.size() -
				historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = currentLine_.length();
	}
}


/**
 * 	This method handles a key left event.
 */
void PythonConnection::handleLeft()
{
	if (charPos_ > 0)
	{
		charPos_--;
		this->write("\033[D");
	}
}


/**
 * 	This method handles a key left event.
 */
void PythonConnection::handleRight()
{
	if (charPos_ < currentLine_.length())
	{
		charPos_++;
		this->write("\033[C");
	}
}


/**
 *	This method sends output to the socket.
 *	We don't care too much about buffer overflows or write errors.
 *	If the connection drops, we'll hear about it when we next read.
 */
void PythonConnection::write(const char* str)
{
	int len = strlen( str );
	send( socket_.fileno(), str, len, 0 );
}

/**
 * 	This method prints a prompt to the socket.
 */
void PythonConnection::writePrompt()
{
	return this->write( multiline_.empty() ? ">>> " : "... " );
}

/**
 *	This is the constructor. It does not do any initialisation work, just
 *	puts the object into an initial sane state. Call startup to start
 *	the server.
 *
 *	@see startup
 */
PythonServer::PythonServer( BW::string welcomeString ) :
	pDispatcher_( NULL ),
	welcomeString_( welcomeString ),
	pHookedWriter_( NULL )
{
}

/**
 *	This is the destructor. It calls shutdown to ensure that the server
 *	has shutdown.
 *
 *	@see shutdown
 */
PythonServer::~PythonServer()
{
	this->shutdown();
}


/**
 *	This method starts up the Python server, and begins listening on the
 *	given port. It redirects Python stdout and stderr, so that they can be
 *	sent to all Python connections as well as stdout.
 *
 *	@param dispatcher The event dispatcher with which to register file descriptors.
 *	@param ip		The ip on which to listen.
 *	@param port		The port on which to listen.
 *	@param ifspec	If specified, the name of the interface to open the port on.
 */
bool PythonServer::startup( Mercury::EventDispatcher & dispatcher,
		uint32_t ip, uint16_t port, const char * ifspec )
{
	pDispatcher_ = &dispatcher;

	pHookedWriter_ = ScriptOutputHook::hookScriptOutput( this );
	if (pHookedWriter_ == NULL)
	{
		ERROR_MSG("PythonServer: Failed to hook Python stdio\n");
		return false;
	}

	listener_.socket(SOCK_STREAM);
	listener_.setnonblocking(true);

#ifndef _WIN32  // WIN32PORT
	// int val = 1;
	// setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));
#endif

	u_int32_t ifaddr = ip;
	if (ifspec)
	{	// try binding somewhere else if ifspec != 0
		// note that we never bind everywhere now ... oh well...
		//  prolly good for security that
		char ifname[IFNAMSIZ];

		if (strcmp( ifspec, Mercury::NetworkInterface::USE_BWMACHINED ) == 0)
		{
			if (ServerApp::discoveredInternalIP != 0)
			{
				// Use the value found during initialisation
				ifaddr = ServerApp::discoveredInternalIP;
			}
			else
			{
				WARNING_MSG( "PythonServer::startup: "
				    "No stored address, so using the IP passed in.\n" );
			}
		}
		else
		{
			listener_.findIndicatedInterface( ifspec, ifname );
			listener_.getInterfaceAddress( ifname, ifaddr );
		}
		// if anyone fails then ifaddr simple does not get set
	}
	if (listener_.bind( htons( port ), ifaddr ) == -1)
	{
		if (listener_.bind(0) == -1)
		{
			WARNING_MSG("PythonServer: Failed to bind to port %d\n", port);
			this->shutdown();
			return false;
		}
	}

	listener_.getlocaladdress(&port, NULL);
	port = ntohs(port);

	listen( listener_.fileno(), 1 );
	pDispatcher_->registerFileDescriptor( listener_.fileno(), this,
		"PythonConnection" );

	INFO_MSG( "Python server is running on port %d\n", port );

	return true;
}

/**
 * 	This method shuts down the Python server.
 * 	It closes the listener port, disconnects all connections,
 * 	and restores Python stderr and stdout.
 */
void PythonServer::shutdown()
{
	BW::vector<PythonConnection *>::iterator it;

	// Disconnect all connections, and clear our connection list.

	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		delete *it;
	}

	connections_.clear();

	// Shutdown the listener socket if it is open.

	if (listener_.good())
	{
		MF_ASSERT( pDispatcher_ != NULL );
		pDispatcher_->deregisterFileDescriptor( listener_.fileno() );
		listener_.close();
	}

	// If stderr and stdout were redirected, restore them.
	if (pHookedWriter_ != NULL)
	{
		ScriptOutputHook::unhookScriptOutput( pHookedWriter_, this );
	}

	pDispatcher_ = NULL;
}


/**
 *	This method is called whenever there is a new line of output for
 *	sys.stdout or sys.stderr. We redirect it to all the connections,
 *	CRs are substituted for CR/LF pairs to facilitate printing on Windows.
 *
 *	@param output	The BW::string that was output.
 *	@param isStderr	True if the output was to stderr, false otherwise.
 */
void PythonServer::onScriptOutput( const BW::string & output,
	bool /* isStderr */ )
{
	BW_GUARD;

	BW::string cookedMsg;

	for (size_t i = 0; i < output.length(); ++i)
	{
		if (i < output.length() - 1 && 
			output[ i ] == '\r' && output[ i + 1 ] == '\n')
		{
			// Already a DOS newline
			cookedMsg += "\r\n";
			++i;
			continue;
		}
		if (output[ i ] == '\n')
		{
			// Convert UNIX newline to DOS newline
			cookedMsg += "\r\n";
			continue;
		}
		cookedMsg += output[ i ];
	}

	BW::vector<PythonConnection *>::iterator it;

	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		if((*it)->active())
		{
			(*it)->write(cookedMsg.c_str());
		}
	}
}


/**
 *	This method is notified when the ScriptOutputWriter we're hooked to is
 *	going to be destroyed.
 *
 *	After this call returns, pOwner is no longer a valid pointer and should
 *	not be dereferenced.
 *
 *	@param pOwner	The ScriptOutputWriter that is to be destroyed.
 */
void PythonServer::onOutputWriterDestroyed( ScriptOutputWriter * pOwner )
{
	BW_GUARD;

	MF_ASSERT( pOwner == pHookedWriter_ );
	pHookedWriter_ = NULL;
}


/**
 * 	This method deletes a connection from the python server.
 *
 *	@param pConnection	The connection to be deleted.
 */
void PythonServer::deleteConnection(PythonConnection* pConnection)
{
	BW::vector<PythonConnection *>::iterator it;

	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		if (*it == pConnection)
		{
			delete *it;
			connections_.erase(it);
			return;
		}
	}

	WARNING_MSG("PythonServer::deleteConnection: %p not found",
			pConnection);
}

/**
 *	This method is called by Mercury when our file descriptor is
 *	ready for reading.
 */
int PythonServer::handleInputNotification( int fd )
{
	(void)fd;
	MF_ASSERT(fd == listener_.fileno());

	sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int socket = accept( listener_.fileno(), (sockaddr *)&addr, &size );


	if (socket == -1)
	{
		TRACE_MSG("PythonServer: Failed to accept connection: %d\n", errno);
		return 1;
	}

	TRACE_MSG("PythonServer: Accepted new connection from %s\n",
			inet_ntoa(addr.sin_addr));
	connections_.push_back(
			new PythonConnection( this, *pDispatcher_, socket, welcomeString_ ) );

	return 1;
}

/**
 * 	This method returns the port on which our file descriptor is listening.
 */
uint16_t PythonServer::port() const
{
	uint16_t port = 0;
	listener_.getlocaladdress( &port, NULL );
	port = ntohs( port );
	return port;
}

BW_END_NAMESPACE

#endif // ENABLE_PYTHON_TELNET_SERVICE
