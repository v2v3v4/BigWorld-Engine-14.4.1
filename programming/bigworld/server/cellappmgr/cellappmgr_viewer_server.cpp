#include "cellappmgr_viewer_server.hpp"

#include "cellapp.hpp"
#include "cellappmgr.hpp"
#include "space.hpp"

#include "cstdmf/memory_stream.hpp"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CellAppMgrViewerConnection
// -----------------------------------------------------------------------------

/**
 *	This constructor intitialises the CellAppMgrViewerConnection given an
 *	existing socket.
 */
CellAppMgrViewerConnection::CellAppMgrViewerConnection( CellAppMgr & cellAppMgr,
		Mercury::EventDispatcher & dispatcher, int fd ) :
	dispatcher_( dispatcher ),
	cellAppMgr_( cellAppMgr )
{
	socket_.setFileDescriptor( fd );

	dispatcher_.registerFileDescriptor( this->fileno(), this,
		"CellAppMgrViewerConnection" );
}


namespace
{
/**
 *	This is a helper function used by handleInputNotification to satisfy the
 *	GET_CELLS request. This is done as a static function because we do not know
 *	about CellAppMgr::Cells in the header file.
 */
void sendCells( CellAppMgrViewerConnection & self, const Space & space )
{
	MemoryOStream stream;
	space.addToStream( stream, true );
	int size = stream.size();
	uint8 * data = (uint8*)stream.retrieve( size );

	self.sendReply( data, size );
	self.sendReply( NULL, 0 ); // ??
}

} // namespace ()

/**
 *	This is the destructor.
 *	It deregisters the socket with Mercury. Note that the Endpoint
 *	destructor will close the socket.
 */
CellAppMgrViewerConnection::~CellAppMgrViewerConnection()
{
	dispatcher_.deregisterFileDescriptor( socket_.fileno() );
}

/**
 *	This method is called by Mercury when the socket is ready for reading.
 *	It processes user input from the socket, and sends it to Python.
 *
 *	@param fd	Socket that is ready for reading.
 *  something interesting on fd, read something off it.
 */
int CellAppMgrViewerConnection::handleInputNotification( int fd )
{
	// MF_ASSERT( fd == (int)socket_ );

	char buf;

	int bytesRead = socket_.recv( &buf, sizeof(char) );

	if (bytesRead == -1)
	{
		INFO_MSG( "CellAppMgrViewerConnection %d: Read error\n", fd );
		delete this;
		return 1;
	}

	if (bytesRead == 0)
	{
		delete this;
		return 1;
	}

	switch (buf)
	{
		case CellAppMgrViewerExport::GET_CELLS:
		{
			// socket_ is non-blocking. This will be here, since this is sent
			// with command (upon which this handleInputNotification executed).
			int32 spaceID;
			// TODO: What happens if we receive a partial packet.
			bytesRead = socket_.recv( &spaceID, sizeof(spaceID) );

			if (bytesRead < 0)
			{
				WARNING_MSG(
					"CellAppMgrViewerConnection::handleInputNotification: "
						   "Error reading Space index.\n" );
			}
			else if (bytesRead == sizeof(spaceID))
			{
				Space * pSpace = cellAppMgr_.findSpace( spaceID );

				if (pSpace != NULL)
				{
					sendCells( *this, *pSpace );
				}
				else
				{
					WARNING_MSG(
						"CellAppMgrViewerConnection::handleInputNotifiction: "
							"No such space: %u.\n", spaceID );
				}
			}
			break;
		}

		case CellAppMgrViewerExport::REMOVE_CELL:
		{
			bool isOkay = false;
			SpaceID spaceID = 0;
			Mercury::Address appAddr( 0, 0 );
			bytesRead = socket_.recv( &appAddr, sizeof( appAddr ) ) +
						 socket_.recv( &spaceID, sizeof( spaceID ) );

			INFO_MSG( "CellAppMgrViewerExport::REMOVE_CELL: "
					"addr = %s. spaceID = %u\n", appAddr.c_str(), spaceID );

			if (bytesRead == sizeof( appAddr ) + sizeof( spaceID ))
			{
				CellApp * pApp = cellAppMgr_.findApp( appAddr );

				if (pApp != NULL)
				{
					CellData * pCell = pApp->findCell( spaceID );

					if (pCell != NULL)
					{
						INFO_MSG( "CellAppMgrViewerConnection:: "
								"handleInputNotifiction: Start removing\n" );
						pCell->startRetiring();
						isOkay = true;
					}
				}
			}

			if (!isOkay)
			{
				ERROR_MSG( "CellAppMgrViewerConnection::handleInputNotifiction:"
						"Failed to remove cell %u on app %s. (Read = %d)\n",
					spaceID, appAddr.c_str(), bytesRead );
			}

			break;
		}

		case CellAppMgrViewerExport::STOP_CELL_APP:
		{
			Mercury::Address cellAppAddr;
			bytesRead = socket_.recv( &cellAppAddr, sizeof( cellAppAddr ) );
			INFO_MSG( "CellAppMgrViewerExport::STOP_CELL_APP: addr = %s\n",
					cellAppAddr.c_str() );

			if (bytesRead == sizeof( cellAppAddr ))
			{
				CellApp * pApp = cellAppMgr_.findApp( cellAppAddr );

				if (pApp != NULL)
				{
					pApp->startRetiring();
				}
			}
			else
			{
				ERROR_MSG( "CellAppMgrViewerConnection::handleInputNotifiction:"
							" Bad arguments to STOP_CELLAPP (read = %d)\n",
						bytesRead );
			}

			break;
		}

		case CellAppMgrViewerExport::GET_VERSION:
		{
			const uint16 version = 1;
			this->sendReply( &version, sizeof(version) );
			break;
		}

		case CellAppMgrViewerExport::GET_SPACE_GEOMETRY_MAPPINGS:
		{
			int32 spaceID;
			// Non-blocking read.
			// TODO: Handle case where we receive a partial packet.
			bytesRead = socket_.recv( &spaceID, sizeof(spaceID) );

			if (bytesRead < 0)
			{
				WARNING_MSG(
						"CellAppMgrViewerConnection::handleInputNotification: "
						"Ignoring request for space geometry mappings due to "
						"failure to read space ID.\n" );
				this->sendReply( NULL, 0 );
			}
			else if (bytesRead == sizeof(spaceID))
			{
				Space * pSpace = cellAppMgr_.findSpace( spaceID );

				if (pSpace != NULL)
				{
					this->sendSpaceGeometryMappings( *pSpace );
				}
				else
				{
					WARNING_MSG(
						"CellAppMgrViewerConnection::handleInputNotification: "
						"Ignoring request for space geometry mappings because "
						"%u is not a valid space.\n", spaceID );
					this->sendReply( NULL, 0 );
				}
			}
			break;
		}

		default:
		{
			this->sendReply( NULL, 0 );
			break;
		}

	}

	return 1;
}

/**
 *	This method sends all the space geometry mappings for the specified
 * 	space.
 */
void CellAppMgrViewerConnection::sendSpaceGeometryMappings( const Space& space )
{
	MemoryOStream stream;
	space.addGeometryMappingsToStream( stream );

	int size = stream.size();
	uint8 * data = (uint8*)stream.retrieve( size );
	this->sendReply( data, size );
}


/**
 *	This method sends a reply.
 *
 *	@param buf	A pointer to the data to send.
 *	@param len	The size of the data to send.
 */
void CellAppMgrViewerConnection::sendReply( const void * buf, uint32 len )
{
	if (socket_.send( &len, sizeof(len) ) != sizeof(len))
	{
		WARNING_MSG( "CellAppMgrViewerConnection::sendReply: "
						"Error on write (0)\n" );
		return;
	}

	if (len > 0)
	{
		if (socket_.send( buf, len ) != int(len))
		{
			WARNING_MSG( "CellAppMgrViewerConnection::sendReply: "
						"Error on write.\n" );
		}
	}
}


// -----------------------------------------------------------------------------
// Section: CellAppMgrViewerServer
// -----------------------------------------------------------------------------

/**
 *	This is the constructor. It does not do any initialisation work, just
 *	puts the object into an initial sane state. Call startup to start
 *	the server.
 *
 *	@see startup
 */
CellAppMgrViewerServer::CellAppMgrViewerServer( CellAppMgr & cellAppMgr ) :
	pDispatcher_( NULL ),
	cellAppMgr_( cellAppMgr ),
	port_( 0 )
{
}


/**
 *	This is the destructor. It calls shutDown to ensure that the server has shut
 *	down.
 *
 *	@see shutDown
 */
CellAppMgrViewerServer::~CellAppMgrViewerServer()
{
	this->shutDown();
}


/**
 *	This method starts up the dfgdfg server, and begins listening on the
 *	given port. It redirects Python stdout and stderr, so that they can be
 *	sent to all Python connections as well as stdout.
 *
 *	@param dispatcher		A Mercury Dispatcher with which to register file descriptors.
 *	@param port		The port on which to listen.
 */
bool CellAppMgrViewerServer::startup( Mercury::EventDispatcher& dispatcher,
		uint16 port )
{
	pDispatcher_ = &dispatcher;

	listener_.socket( SOCK_STREAM );
	listener_.setnonblocking( true );

	int val = 1;
	setsockopt( listener_.fileno(), SOL_SOCKET, SO_REUSEADDR,
			(const char*)&val, sizeof(val) );

	if (listener_.bind( htons( port )) == -1)
	{
		if (listener_.bind( 0 ) == -1)
		{
			WARNING_MSG( "CellAppMgrViewerServer::startup: "
				"Failed to bind to port %d\n", port );
			this->shutDown();
			return false;
		}
	}

	listener_.getlocaladdress( &port, NULL );
	port = ntohs( port );
	listen( listener_.fileno(), 1 );

	pDispatcher_->registerFileDescriptor( listener_.fileno(), this,
		"CellAppMgrViewerConnection" );

	port_ = port;

	return true;
}


/**
 * 	This method shuts down the Python server.
 * 	It closes the listener port, disconnects all connections,
 * 	and restores Python stderr and stdout.
 */
void CellAppMgrViewerServer::shutDown()
{
	BW::vector<CellAppMgrViewerConnection *>::iterator it;

	// Disconnect all connections, and clear our connection list.

	for (it = connections_.begin(); it != connections_.end(); ++it)
	{
		delete *it;
	}

	connections_.clear();

	// Shut down the listener socket if it is open.

	if (listener_.good())
	{
		MF_ASSERT( pDispatcher_ != NULL );
		pDispatcher_->deregisterFileDescriptor( listener_.fileno() );
		listener_.close();
	}

	pDispatcher_ = NULL;
}


/**
 *	This method is called by Mercury when our file descriptor is ready for
 *	reading.
 */
int CellAppMgrViewerServer::handleInputNotification( int fd )
{
	(void)fd;
	MF_ASSERT(fd == listener_.fileno());

	sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int socket = accept( listener_.fileno(), (sockaddr *)&addr, &size );

	if (socket == -1)
	{
		TRACE_MSG( "CellAppMgrViewerServer: "
			"Failed to accept connection: %d\n", errno );
		return 1;
	}

	// This object deletes itself when the connection is closed.
	new CellAppMgrViewerConnection( cellAppMgr_, *pDispatcher_, socket );

	return 1;
}


/**
 * 	This method returns the port on which our file descriptor is listening.
 */
uint16 CellAppMgrViewerServer::port() const
{
	return port_;
}

BW_END_NAMESPACE

// cellappmgr_viewer_server.cpp
