/**
 * @file
 *
 * This file implements the CellViewerServer class.
 *
 */

#include "script/first_include.hpp"		// See http://docs.python.org/api/includes.html

#include "cell_viewer_server.hpp"
#include "cell_viewer_connection.hpp"

#include "cell.hpp"
#include "space.hpp"

#include "entity.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

/**
 *	This is the constructor. It does not do any initialisation work, just
 *	puts the object into an initial sane state. Call startup to start
 *	the server.
 *
 *	@see startup
 */
CellViewerServer::CellViewerServer( const CellApp & cellApp ) :
	pDispatcher_( NULL ), cellApp_( cellApp )
{
}

/**
 *	This is the destructor. It calls shutDown to ensure that the server
 *	has shut down.
 *
 *	@see shutDown
 */
CellViewerServer::~CellViewerServer()
{
	this->shutDown();
}

/**
 *	This method starts up the server, and begins listening on the given port.
 *
 *	@param dispatcher	A dispatcher with which to register file descriptors.
 *	@param port		The port on which to listen.
 */
bool CellViewerServer::startup( Mercury::EventDispatcher & dispatcher,
		uint16 port )
{
	pDispatcher_ = &dispatcher;

	listener_.socket(SOCK_STREAM);
	listener_.setnonblocking(true);

	int val = 1;
	setsockopt( listener_.fileno(), SOL_SOCKET, SO_REUSEADDR,
		(const char*)&val, sizeof(val) );

	if (listener_.bind(htons(port)) == -1)
	{
		if (listener_.bind(0) == -1)
		{
			WARNING_MSG( "CellViewerServer::startup: "
				"Failed to bind to port %d\n", port );
			this->shutDown();
			return false;
		}
	}

	listener_.getlocaladdress(&port, NULL);
	port = ntohs(port);

	listen(listener_.fileno(), 1);
	pDispatcher_->registerFileDescriptor( listener_.fileno(), this,
		"CellViewerServer" );

	// INFO_MSG( "CellViewerServer::startup: "
	//		"Viewer Cell Server is running on port %d\n", port );

	return true;
}

/**
 * 	This method shuts down the Python server.
 * 	It closes the listener port, disconnects all connections,
 * 	and restores Python stderr and stdout.
 */
void CellViewerServer::shutDown()
{
	BW::vector<CellViewerConnection *>::iterator it;

	// Disconnect all connections, and clear our connection list.

	for(it = connections_.begin(); it != connections_.end(); it++)
	{
		delete *it;
	}

	connections_.clear();

	// Shut down the listener socket if it is open.

	if (listener_.good())
	{
		MF_ASSERT(pDispatcher_ != NULL);
		pDispatcher_->deregisterFileDescriptor( listener_.fileno() );
		listener_.close();
	}

	pDispatcher_ = NULL;
}

/**
 *	This method is called by Mercury when our file descriptor is
 *	ready for reading.
 */
int CellViewerServer::handleInputNotification( int fd )
{
	(void)fd;
	MF_ASSERT(fd == listener_.fileno());

	sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int socket = accept( listener_.fileno(), (sockaddr *)&addr, &size );

	if (socket == -1)
	{
		TRACE_MSG("CellViewerServer: Failed to accept connection: %d\n", errno);
		return 1;
	}

	// This object deletes itself when the connection is closed.
	new CellViewerConnection( *pDispatcher_, socket, cellApp_ );

	return 1;
}

/**
 * 	This method returns the port on which our file descriptor is listening.
 */
uint16 CellViewerServer::port() const
{
	uint16 port = 0;
	listener_.getlocaladdress(&port, NULL);
	port = ntohs(port);
	return port;
}

BW_END_NAMESPACE

// cell_viewer_server.cpp 
