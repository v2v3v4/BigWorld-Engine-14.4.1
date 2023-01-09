#include "pch.hpp"

#include "python_server.hpp"

#include "cstdmf/config.hpp"

#if ENABLE_PYTHON_TELNET_SERVICE

#include "network/event_dispatcher.hpp"

#include "pyscript/python_input_substituter.hpp"

#include "script/script_output_hook.hpp"

#include "input/input.hpp"

#include <node.h>
#include <errcode.h>


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
#define KEY_ENTER			13
#define KEY_ESC				27

#define MAX_HISTORY_LINES	100


BW_BEGIN_NAMESPACE

namespace {

unsigned char keyToChar( KeyCode::Key key )
{
	char character = '\0';

	switch (key)
	{
	case KeyCode::KEY_A: character = 'a'; break;
	case KeyCode::KEY_B: character = 'b'; break;
	case KeyCode::KEY_C: character = 'c'; break;
	case KeyCode::KEY_D: character = 'd'; break;
	case KeyCode::KEY_E: character = 'e'; break;
	case KeyCode::KEY_F: character = 'f'; break;
	case KeyCode::KEY_G: character = 'g'; break;
	case KeyCode::KEY_H: character = 'h'; break;
	case KeyCode::KEY_I: character = 'i'; break;
	case KeyCode::KEY_J: character = 'j'; break;
	case KeyCode::KEY_K: character = 'k'; break;
	case KeyCode::KEY_L: character = 'l'; break;
	case KeyCode::KEY_M: character = 'm'; break;
	case KeyCode::KEY_N: character = 'n'; break;
	case KeyCode::KEY_O: character = 'o'; break;
	case KeyCode::KEY_P: character = 'p'; break;
	case KeyCode::KEY_Q: character = 'q'; break;
	case KeyCode::KEY_R: character = 'r'; break;
	case KeyCode::KEY_S: character = 's'; break;
	case KeyCode::KEY_T: character = 't'; break;
	case KeyCode::KEY_U: character = 'u'; break;
	case KeyCode::KEY_V: character = 'v'; break;
	case KeyCode::KEY_W: character = 'w'; break;
	case KeyCode::KEY_X: character = 'x'; break;
	case KeyCode::KEY_Y: character = 'y'; break;
	case KeyCode::KEY_Z: character = 'z'; break;

	case KeyCode::KEY_0: character = '0'; break;
	case KeyCode::KEY_1: character = '1'; break;
	case KeyCode::KEY_2: character = '2'; break;
	case KeyCode::KEY_3: character = '3'; break;
	case KeyCode::KEY_4: character = '4'; break;
	case KeyCode::KEY_5: character = '5'; break;
	case KeyCode::KEY_6: character = '6'; break;
	case KeyCode::KEY_7: character = '7'; break;
	case KeyCode::KEY_8: character = '8'; break;
	case KeyCode::KEY_9: character = '9'; break;

	case KeyCode::KEY_COMMA      : character = ','; break;
	case KeyCode::KEY_PERIOD     : character = '.'; break;
	case KeyCode::KEY_SLASH      : character = '/'; break;
	case KeyCode::KEY_SEMICOLON  : character = ';'; break;
	case KeyCode::KEY_APOSTROPHE : character = '\''; break;
	case KeyCode::KEY_LBRACKET	: character = '['; break;
	case KeyCode::KEY_RBRACKET	: character = ']'; break;
	case KeyCode::KEY_GRAVE      : character = '`'; break;
	case KeyCode::KEY_MINUS      : character = '-'; break;
	case KeyCode::KEY_EQUALS     : character = '='; break;
	case KeyCode::KEY_BACKSLASH  : character = '\\'; break;

	case KeyCode::KEY_SPACE      : character = ' '; break;
	case KeyCode::KEY_RETURN     : character = '\r'; break;

	//case KeyCode::KEY_BACKSPACE  :
	//case KeyCode::KEY_DELETE     : character = '\b'; break;

	default                      : break;
	}

	return character;
}

} //namespace


// -----------------------------------------------------------------------------
// Section: TelnetConnection
// -----------------------------------------------------------------------------


/**
 *	This constructor intitialises the TelnetConnection given an existing
 *	socket.
 *
 *	@param	dispatcher A dispatcher with which to register file descriptors.
 *	@param	fd		file descriptor of a socked that's ready for reading.
 */
TelnetConnection::TelnetConnection(
			Mercury::EventDispatcher & dispatcher, socket_t fd ) :
		pDispatcher_( &dispatcher ),
		telnetSubnegotiation_( false ),
		active_( false )
{
	BW_GUARD;
	socket_.setFileDescriptor( fd );
	socket_.setnonblocking(true);
	pDispatcher_->registerFileDescriptor(socket_.fileno(), this,
		"TelnetConnection" );

	unsigned char options[] =
	{
		TELNET_IAC, TELNET_WILL, TELNET_ECHO,
		TELNET_IAC, TELNET_WONT, TELNET_LINEMODE, 0
	};

	this->write((char*)options);
}

/**
 *	This is the destructor.
 *	It deregisters the socket with Mercury. Note that the Endpoint
 *	destructor will close the socket.
 */
TelnetConnection::~TelnetConnection()
{
	BW_GUARD;
	pDispatcher_->deregisterFileDescriptor( socket_.fileno() );
}




/**
 *	This method is called by Mercury when the socket is ready for reading.
 *	It processes user input from the socket, and calls virtual fns with it.
 *
 *	@param	fd		file descriptor of a socked that's ready for reading.
 *
 *	@return			always returns 1.
 */
int TelnetConnection::handleInputNotification(int fd)
{
	BW_GUARD;
	//MF_ASSERT_DEV(fd == socket_.fileno());

	char buf[256];
	int i, bytesRead;

	bytesRead = recv(socket_.fileno(), buf, sizeof(buf), 0);

	if(bytesRead == -1)
	{
		return 1;
	}

	if(bytesRead == 0)
	{
		this->connectionBad();
		return 1;
	}

	for(i = 0; i < bytesRead; i++)
	{
		readBuffer_.push_back(buf[i]);
	}

	while(!readBuffer_.empty())
	{
		int c = (unsigned char)readBuffer_[0];

		// Handle (and ignore) telnet protocol commands.

		if(c == TELNET_IAC)
		{
			if(!this->handleTelnetCommand())
				return 1;
			continue;
		}

		if(c == KEY_ESC)
		{
			if(!this->handleVTCommand())
				return 1;
			continue;
		}

		// If we're in telnet subnegotiation mode, ignore normal chars.
		if(telnetSubnegotiation_)
		{
			readBuffer_.pop_front();
			continue;
		}

		// ok, ask ourselves to handle it then
		if (!this->handleChar())
			return 1;
	}

	return 1;
}

/**
 * 	This method handles telnet protocol commands. Well actually it handles
 * 	a subset of telnet protocol commands, enough to get Linux and Windows
 *	telnet working in character mode.
 *
 *	@return	True on sucess, false on error.
 */
bool TelnetConnection::handleTelnetCommand()
{
	BW_GUARD;
	unsigned int cmd = (unsigned char)readBuffer_[1];
	unsigned int bytesNeeded = 2;
	char str[256];

	switch(cmd)
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
			bw_snprintf(str, sizeof(str), "Telnet command %d unsupported.\r\n", cmd);
			this->write(str);
			break;
	}

	if(readBuffer_.size() < bytesNeeded)
		return false;

	while(bytesNeeded)
	{
		bytesNeeded--;
		readBuffer_.pop_front();
	}

	return true;
}


/**
 *	This method sends output to the socket.
 *	We don't care too much about buffer overflows or write errors.
 *	If the connection drops, we'll hear about it when we next read.
 *
 *	@param	str		data to be sent.
 */
void TelnetConnection::write(const char* str)
{
	BW_GUARD;
	int len = static_cast<int>(strlen(str));
	send(socket_.fileno(), str, len, 0);
}


// -----------------------------------------------------------------------------
// Section: PythonConnection
// -----------------------------------------------------------------------------




/**
 *	This constructor intitialises the PythonConnection given an existing
 *	socket.
 *
 *	@param	owner	The PythonServer owning this connection.
 *	@param	dispatcher		A dispatcher with which to register file descriptors.
 *	@param	fd		file descriptor of a socked that's ready for reading.
 */
PythonConnection::PythonConnection( PythonServer* owner,
		Mercury::EventDispatcher & dispatcher, socket_t fd ) :
	TelnetConnection( dispatcher, fd ),
	owner_(owner),
	historyPos_(-1),
	charPos_(0)
{
	BW_GUARD;
	this->write("Welcome to PythonServer.\r\n");
	this->writePrompt();
}

/**
 *	Destructor
 */
PythonConnection::~PythonConnection()
{
}

/**
 *	We got a character
 *
 *	@return	true on success, false on error
 */
bool PythonConnection::handleChar()
{
	BW_GUARD;
	int c = (unsigned char)readBuffer_[0];

	// If we got something printable, echo it and append it to
	// the current line.

	if(isprint(c))
	{
		this->handlePrintableChar();
		return true;
	}

	switch(c)
	{
		case KEY_ENTER:
			this->handleLine();
			break;

		case KEY_BACKSPACE:
		case KEY_DEL:
			this->handleDel();
			break;

		case KEY_CTRL_C:
		case KEY_CTRL_D:
			this->connectionBad();
			return false;

		default:
			readBuffer_.pop_front();
			break;
	}

	return true;
}

/**
 *	Handles VT commands comming down the pipe.
 *
 *	@return	true on success, false on error
 */
bool PythonConnection::handleVTCommand()
{
	BW_GUARD;
	// Need 3 chars before we are ready.
	if(readBuffer_.size() < 3)
		return false;

	// Eat the ESC.
	readBuffer_.pop_front();

	if(readBuffer_.front() != '[' && readBuffer_.front() != 'O')
		return true;

	// Eat the [
	readBuffer_.pop_front();

	switch(readBuffer_.front())
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
void PythonConnection::handlePrintableChar()
{
	BW_GUARD;
	// @todo: Optimise redraw
	currentLine_.insert(charPos_, 1, (char)readBuffer_.front());
	int len = static_cast<int>(currentLine_.length() - charPos_);
	this->write(currentLine_.substr(charPos_, len).c_str());

	for(int i = 0; i < len - 1; i++)
		this->write("\b");

	charPos_++;
	readBuffer_.pop_front();
}

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
	BW_GUARD;
	PyObject *m, *d, *v;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);

	v = PyRun_String(command, Py_single_input, d, d);

	if (v == NULL) {
		PyErr_PrintEx(0);
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
	BW_GUARD;
	readBuffer_.pop_front();
	this->write("\r\n");

	if(currentLine_.length())
	{
		historyBuffer_.push_back(currentLine_);

		if(historyBuffer_.size() > MAX_HISTORY_LINES)
		{
			historyBuffer_.pop_front();
		}

		currentLine_ = PythonInputSubstituter::substitute( currentLine_ );
		currentLine_ += "\n";

		active_ = true;
		MyRun_SimpleString((char *)currentLine_.c_str());
		active_ = false;
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
	BW_GUARD;
	if(charPos_ > 0)
	{
		// @todo: Optimise redraw
		currentLine_.erase(charPos_ - 1, 1);
		this->write("\b" ERASE_EOL);
		charPos_--;
		int len = static_cast<int>(currentLine_.length() - charPos_);
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
	BW_GUARD;
	if(historyPos_ < (int)historyBuffer_.size() - 1)
	{
		historyPos_++;
		currentLine_ = historyBuffer_[historyBuffer_.size() -
			historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = static_cast<uint>(currentLine_.length());
	}
}


/**
 * 	This method handles a key down event.
 */
void PythonConnection::handleDown()
{
	BW_GUARD;
	if(historyPos_ >= 0 )
	{
		historyPos_--;

		if(historyPos_ == -1)
			currentLine_ = "";
		else
			currentLine_ = historyBuffer_[historyBuffer_.size() -
				historyPos_ - 1];

		// @todo: Optimise redraw
		this->write("\r" ERASE_EOL);
		this->writePrompt();
		this->write(currentLine_.c_str());
		charPos_ = static_cast<uint>(currentLine_.length());
	}
}


/**
 * 	This method handles a key left event.
 */
void PythonConnection::handleLeft()
{
	BW_GUARD;
	if(charPos_ > 0)
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
	BW_GUARD;
	if(charPos_ < currentLine_.length())
	{
		charPos_++;
		this->write("\033[C");
	}
}

/**
 *	This method is called when the connection goes bad
 */
void PythonConnection::connectionBad()
{
	BW_GUARD;
	INFO_MSG("PythonConnection closed normally.\n");
	owner_->deleteConnection( this );
}


/**
 * 	This method prints a prompt to the socket.
 */
void PythonConnection::writePrompt()
{
	BW_GUARD;
	return this->write(">>> ");
}




// -----------------------------------------------------------------------------
// Section: KeyboardConnection
// -----------------------------------------------------------------------------


/**
 *	This class is a virtual keyboard connection
 */
class KeyboardConnection : public TelnetConnection, public KeyboardDevice
{
public:
	KeyboardConnection(PythonServer* owner, 
		Mercury::EventDispatcher & dispatcher, socket_t fd);
	~KeyboardConnection();

	virtual bool handleChar();
	virtual void connectionBad();

	virtual void update()	{ keyAt_ = 0; }
	virtual bool next( KeyEvent & event )
	{
		if (keyAt_ >= keys_.size())
		{
			keys_.clear();
			return false;
		}
		event = keys_[ keyAt_++ ];
		return true;
	}

private:
	PythonServer	* owner_;

	BW::vector<KeyEvent>	keys_;
	uint					keyAt_;

	char	charDown_[256];
	bool	stickyMode_;

	static KeyCode::Key	s_charKeyMap_[256];
	static bool s_cknmInitted_;
};
KeyCode::Key KeyboardConnection::s_charKeyMap_[256];
bool KeyboardConnection::s_cknmInitted_ = false;

/**
 *	Constructor.
 *
 *	@param	owner	The PythonServer owning this connection.
 *	@param	dispatcher An EventDispatcher with which to register file descriptors.
 *	@param	fd		file descriptor of a socked that's ready for reading.
 */
KeyboardConnection::KeyboardConnection(PythonServer* owner,
		Mercury::EventDispatcher & dispatcher, socket_t fd ) :
	TelnetConnection( dispatcher, fd ),
	owner_( owner ),
	keyAt_( 0 ),
	stickyMode_( false )
{
	BW_GUARD;
	gVirtualKeyboards.push_back( this );
	this->write("Virtual keyboard ready.\r\n");

	for (int i = 0; i < 256; i++) charDown_[i] = 0;

	if (!KeyboardConnection::s_cknmInitted_)
	{
		KeyboardConnection::s_cknmInitted_ = true;
		for (int i = 0; i < 256; i++)
			s_charKeyMap_[i] = KeyCode::KEY_NOT_FOUND;
		for (int i = 0; i < KeyCode::NUM_KEYS; i++)
		{
			KeyCode::Key key( (KeyCode::Key)i );
			s_charKeyMap_[ keyToChar( key ) ] = key;
		}
		s_charKeyMap_[0] = KeyCode::KEY_NOT_FOUND;
	}
}

/**
 *	Destructor.
 */
KeyboardConnection::~KeyboardConnection()
{
	BW_GUARD;
	for (uint i = 0; i < gVirtualKeyboards.size(); i++)
	{
		if (gVirtualKeyboards[i] == this)
		{
			gVirtualKeyboards.erase( gVirtualKeyboards.begin()+i );
			break;
		}
	}
}

/**
 *	Handles next character in read buffer.
 *
 *	@return		Always returns true.
 */
bool KeyboardConnection::handleChar()
{
	BW_GUARD;
	unsigned char c = (unsigned char)readBuffer_[0];
	readBuffer_.pop_front();

	char ts[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	KeyCode::Key fkey = s_charKeyMap_[ c ];
	if (fkey != KeyCode::KEY_NOT_FOUND)
	{
		if (!stickyMode_ || !charDown_[ c ])
			keys_.push_back( KeyEvent::make( fkey, true, 0, Vector2(0,0) ) );
		if (!stickyMode_ || charDown_[ c ])
			keys_.push_back( KeyEvent::make( fkey, false, 0, Vector2(0,0) ) );
	}
	else if (c >= 'A' && c <= 'Z')
	{
		ts[0] = (char)c;
		KeyCode::Key key = KeyCode::stringToKey( ts );
		KeyCode::Key skey = KeyCode::stringToKey( "LSHIFT" );
		if (!stickyMode_ || !charDown_[ c ])
		{
			keys_.push_back( KeyEvent::make( skey, true, 0, Vector2(0,0) ) );
			keys_.push_back( KeyEvent::make( key, true, MODIFIER_SHIFT, Vector2(0,0) ) );
		}
		if (!stickyMode_ || charDown_[ c ])
		{
			keys_.push_back( KeyEvent::make( key, false, MODIFIER_SHIFT, Vector2(0,0) ) );
			keys_.push_back( KeyEvent::make( skey, false, 0, Vector2(0,0) ) );
		}
	}
	else if (c == (unsigned char)'\xe9') // e-acute, unicode 0xe9
	{
		stickyMode_ = !stickyMode_;
		this->write( stickyMode_ ? "Sticky mode on.\r\n" : "Sticky mode off.\r\n" );
	}

	if (stickyMode_)
	{
		charDown_[ c ] = !charDown_[ c ];

		this->write("\r" ERASE_EOL);
		char buf[512];
		int bufat = 0;
		for (int i=0; i<256;i++)
		{
			if (!charDown_[i]) continue;

			if (i == '\n' || i == '\r')
			{
				buf[bufat++] = '\\';
				buf[bufat++] = 'n';
			}
			else
			{
				buf[bufat++] = i;
			}
		}
		buf[bufat] = 0;
		this->write( buf );
	}


	return true;
}

/**
 *	This method is called when the connection goes bad
 */
void KeyboardConnection::connectionBad()
{
	BW_GUARD;
	INFO_MSG("KeyboardConnection closed normally.\n");
	owner_->deleteConnection( this );
}



// -----------------------------------------------------------------------------
// Section: PythonServer
// -----------------------------------------------------------------------------



/**
 *	This is the constructor. It does not do any initialisation work, just
 *	puts the object into an initial sane state. Call startup to start
 *	the server.
 *
 *	@see startup
 */
PythonServer::PythonServer() :
	pDispatcher_(NULL),
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
	BW_GUARD;
	this->shutdown();
}

/**
 *	This method starts up the Python server, and begins listening on the
 *	given port. It redirects Python stdout and stderr, so that they can be
 *	sent to all Python connections as well as stdout.
 *
 *	@param dispatcher		A dispatcher with which to register file descriptors.
 *	@param port		The port on which to listen.
 *
 *	@return	true on success, false on error
 */
bool PythonServer::startup( Mercury::EventDispatcher & dispatcher, uint16_t port)
{
	BW_GUARD;
	pDispatcher_ = &dispatcher;

	pHookedWriter_ = ScriptOutputHook::hookScriptOutput( this );
	if (pHookedWriter_ == NULL)
	{
		ERROR_MSG("PythonServer: Failed to hook Python stdio\n");
		return false;
	}

	listener_.socket(SOCK_STREAM);
	listener_.setnonblocking(true);

#ifdef __unix__
	int val = 1;
	setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
#endif

	if(listener_.bind( htons(port) ) == -1)
	{
		if(listener_.bind(0) == -1)
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
		"PythonServer" );

	kbListener_.socket( SOCK_STREAM );
	kbListener_.setnonblocking( true );
	kbListener_.bind( htons( port+1 ) );
	listen( kbListener_.fileno(), 1 );
	pDispatcher_->registerFileDescriptor( kbListener_.fileno(), this,
		"PythonServer_kbListener" );

	INFO_MSG( "Python server is running on port %d\n", port );
	INFO_MSG( "Keyboard server is running on port %d\n", port+1 );
	return true;
}

/**
 * 	This method shuts down the Python server.
 * 	It closes the listener port, disconnects all connections,
 * 	and restores Python stderr and stdout.
 */
void PythonServer::shutdown()
{
	BW_GUARD;
	BW::vector<PythonConnection *>::iterator it;

	// Disconnect all connections, and clear our connection list.
	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		bw_safe_delete(*it);
	}
	connections_.clear();

	// Shutdown the listener socket if it is open.
	if (listener_.good())
	{
		MF_ASSERT_DEV(pDispatcher_ != NULL);

		if( pDispatcher_ != NULL )
		{
			pDispatcher_->deregisterFileDescriptor( listener_.fileno() );
		}

		listener_.close();
	}

	BW::vector<KeyboardConnection *>::iterator kit;
	for(kit = kbConnections_.begin(); kit != kbConnections_.end(); kit++)
		bw_safe_delete(*kit);
	kbConnections_.clear();

	if(kbListener_.good())
	{
		MF_ASSERT_DEV(pDispatcher_ != NULL);

		if( pDispatcher_ != NULL )
		{
			pDispatcher_->deregisterFileDescriptor( kbListener_.fileno() );
		}

		kbListener_.close();
	}


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
void PythonServer::deleteConnection(TelnetConnection* pConnection)
{
	BW_GUARD;
	BW::vector<PythonConnection *>::iterator it;

	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		if(*it == pConnection)
		{
			bw_safe_delete(*it);
			connections_.erase(it);
			return;
		}
	}

	BW::vector<KeyboardConnection *>::iterator kit;
	for(kit = kbConnections_.begin(); kit != kbConnections_.end(); kit++)
	{
		if(*kit == pConnection)
		{
			bw_safe_delete(*kit);
			kbConnections_.erase(kit);
			return;
		}
	}

	WARNING_MSG("PythonServer::deleteConnection: %p not found",
			pConnection);
}

/**
 *	This is for when the dispatcher input thing doesn't work
 */
void PythonServer::pollInput()
{
	BW_GUARD;
	this->handleInputNotification(0);

	for (uint i = 0; i < connections_.size(); i++)
	{
		connections_[i]->handleInputNotification(0);
	}

	for (uint i = 0; i < kbConnections_.size(); i++)
	{
		kbConnections_[i]->handleInputNotification(0);
	}
}


/**
 *	This method is called by Mercury when our file descriptor is
 *	ready for reading.
 *
 *	@param	fd		file descriptor of a socked that's ready for reading.
 */
int PythonServer::handleInputNotification(int fd)
{
	BW_GUARD;
	//MF_ASSERT_DEV(fd == (int)listener_);

	sockaddr_in addr;
	socklen_t size = sizeof(addr);

	SOCKET socket = accept( listener_.fileno(), (sockaddr *)&addr, &size );

	if(socket != -1)
	{

		TRACE_MSG("PythonServer: Accepted new connection from %s\n",
			inet_ntoa(addr.sin_addr));
		connections_.push_back(new PythonConnection(this, *pDispatcher_, socket));
	}

	socket = accept( kbListener_.fileno(), (sockaddr *)&addr, &size );
	if (socket != -1)
	{
		kbConnections_.push_back(new KeyboardConnection(this, *pDispatcher_, socket));
	}

	return 1;
}

/**
 * 	This method returns the port on which our file descriptor is listening.
 */
uint16_t PythonServer::port() const
{
	BW_GUARD;
	uint16_t port;
	listener_.getlocaladdress(&port, NULL);
	port = ntohs(port);
	return port;
}

BW_END_NAMESPACE

#else

// Avoid LNK4221 warning - no public symbols. This happens when this file is
// compiled in consumer-client build
extern const int dummyPublicSymbol = 0;

#endif // ENABLE_PYTHON_TELNET_SERVICE
