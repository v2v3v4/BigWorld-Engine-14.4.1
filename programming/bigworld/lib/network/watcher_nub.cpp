#include "pch.hpp"

#include "cstdmf/config.hpp" // defines ENABLE_WATCHERS 

#if ENABLE_WATCHERS

#include "network/watcher_nub.hpp"

#include "cstdmf/bwversion.hpp"
#include "cstdmf/profile.hpp"

#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/machine_guard.hpp"
#include "network/misc.hpp"
#include "network/portmap.hpp"
#include "network/watcher_connection.hpp"
#include "network/watcher_packet_handler.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

DECLARE_DEBUG_COMPONENT2( "Network", 0 )


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( WatcherNub )

/**
 *	This is the constructor.
 */
WatcherNub::WatcherNub() :
	id_( -1 ),
	registered_( false ),
	pExtensionHandler_( NULL ),
	insideReceiveRequest_( false ),
	isInitialised_( false ),
	requestPacket_(new char[WN_PACKET_SIZE] ),
	errorMsg_(),
	udpSocket_(),
	tcpSocket_(),
	pDispatcher_( NULL )
{
}


/**
 *	This method initialises the watcher nub.
 */
bool WatcherNub::init( const char * listeningInterface, uint16 listeningPort )
{
	if (isInitialised_)
	{
		// WARNING_MSG( "WatcherNub::init: Already initialised\n" );
		return this->isOkay();
	}

	isInitialised_ = true;

	INFO_MSG( "WatcherNub::init: "
				"listeningInterface = '%s', listeningPort = %hd\n",
			listeningInterface ? listeningInterface : "",
			listeningPort );

	// open the socket
	udpSocket_.socket( SOCK_DGRAM );
	if (!udpSocket_.good())
	{
		ERROR_MSG( "WatcherNub::init: socket() failed\n" );
		return false;
	}

	u_int32_t ifaddr = INADDR_ANY;
#ifndef _WIN32
	// If the interface resolves to an address, use that instead of
	// searching for a matching interface.
	if (inet_aton( listeningInterface, (struct in_addr *)&ifaddr ) == 0)
#endif
	{
		// ask endpoint to parse the interface specification into a name
		char ifname[IFNAMSIZ];
		bool listeningInterfaceEmpty =
			(listeningInterface == NULL || listeningInterface[0] == 0);
		if (udpSocket_.findIndicatedInterface( listeningInterface, ifname ) == 0)
		{
			INFO_MSG( "WatcherNub::init: creating on interface '%s' (= %s)\n",
				listeningInterface, ifname );
			if (udpSocket_.getInterfaceAddress( ifname, ifaddr ) != 0)
			{
				WARNING_MSG( "WatcherNub::init: couldn't get addr of interface %s "
					"so using all interfaces\n", ifname );
			}
		}
		else if (!listeningInterfaceEmpty)
		{
			WARNING_MSG( "WatcherNub::init: couldn't parse interface spec %s "
				"so using all interfaces\n", listeningInterface );
		}
	}

	bool socketsBound = false;
	const int MAX_RETRIES = 100;
	int count = 0;

	while (!socketsBound && count < MAX_RETRIES)
	{
		socketsBound = this->bindSockets( listeningPort, ifaddr );
		++count;
	}

	if (!socketsBound)
	{
		errorMsg_ = "Failed to bind sockets to same port";
	}
	else if (tcpSocket_.listen( 5 ) != 0)
	{
		errorMsg_ = "TCP listen failed";
	}

	if (!errorMsg_.empty())
	{
		ERROR_MSG( "WatcherNub::init: %s\n", errorMsg_.c_str() );
		return false;
	}

	return true;
}


/**
 *	This method attempts to bind the UDP and TCP sockets to the same port.
 */
bool WatcherNub::bindSockets( uint16 listeningPort, u_int32_t ifaddr )
{
	if (tcpSocket_.good())
	{
		tcpSocket_.close();
		tcpSocket_.detach();
	}

	// Bind to TCP first as the available list is typically smaller
	tcpSocket_.socket( SOCK_STREAM );

	if (tcpSocket_.bind( listeningPort, ifaddr ) != 0)
	{
		tcpSocket_.close();

		WARNING_MSG( "WatcherNub::init: tcpSocket_.bind() to %s failed\n",
		   Mercury::Address( ifaddr, listeningPort ).c_str() );
		return false;
	}

	u_int16_t networkPort = 0;
	u_int32_t networkAddr = 0;
	tcpSocket_.getlocaladdress( &networkPort, &networkAddr );

	if (udpSocket_.good())
	{
		udpSocket_.close();
		udpSocket_.detach();
	}

	udpSocket_.socket( SOCK_DGRAM );

	if (udpSocket_.bind( networkPort, networkAddr ) == -1)
	{
		tcpSocket_.close();

		WARNING_MSG( "WatcherNub::init: udpSocket_.bind() to %s failed\n",
		   Mercury::Address( networkAddr, networkPort ).c_str() );
		return false;
	}

	return true;
}


/**
 *	This is the destructor.
 */
WatcherNub::~WatcherNub()
{
	if (registered_)
	{
		this->deregisterWatcher();
	}

	if (udpSocket_.good())
	{
		udpSocket_.close();
	}

	if (requestPacket_ != NULL)
	{
		bw_safe_delete_array(requestPacket_);
	}
}


/**
 *	This method broadcasts a watcher register message for this watcher.
 *
 *	@param id			Numeric id for this watcher.
 *	@param abrv			Short name for this watcher.
 *	@param listeningInterface	The name of the network interface to listen on.
 *	@param listeningPort		The port to listen on.
 */
int WatcherNub::registerWatcher( int id, const char *abrv,
		const char * listeningInterface, uint16 listeningPort )
{
	if (!this->init( listeningInterface, listeningPort ))
	{
		ERROR_MSG( "WatcherNub::registerWatcher: init failed.\n" );
		return -1;
	}

	// make sure we're not already registered...
	if (registered_)
		this->deregisterWatcher();

	// set up a few things
	id_ = id;

	strncpy( abrv_, abrv, sizeof(abrv_) );
	abrv_[sizeof(abrv_)-1]=0;

	registered_ = true;
	this->notifyMachineGuard();

	return 0;
}


/**
 *	This method broadcasts a watcher deregister message for this watcher.
 */
int WatcherNub::deregisterWatcher()
{
	if (!registered_)
	{
		return 0;
	}

	registered_ = false;
	this->notifyMachineGuard();

	return 0;
}


/**
 *	This method associates this with the given dispatcher.
 */
void WatcherNub::attachTo( Mercury::EventDispatcher & dispatcher )
{
	dispatcher.registerFileDescriptor( udpSocket_.fileno(), this, "WatcherUDP" );
	dispatcher.registerFileDescriptor( tcpSocket_.fileno(), this, "WatcherTCP" );

	pDispatcher_ = &dispatcher;
}


/**
 *	This method sends a message to the machined process.
 */
void WatcherNub::notifyMachineGuard()
{
	u_int16_t port = 0;
	udpSocket_.getlocaladdress( &port, NULL );

	ProcessMessage pm;
	pm.param_ = pm.PARAM_IS_MSGTYPE |
		(registered_ ? pm.REGISTER : pm.DEREGISTER);
	pm.category_ = pm.WATCHER_NUB;
	pm.port_ = port;
	pm.id_ = id_;
	pm.name_ = abrv_;

	pm.majorVersion_ = BWVersion::majorNumber();
	pm.minorVersion_ = BWVersion::minorNumber();
	pm.patchVersion_ = BWVersion::patchNumber();

	uint32 destip = htonl( 0x7F000001U );
	int reason;
	if ((reason = pm.sendAndRecv( 0, destip )) != Mercury::REASON_SUCCESS)
	{
		ERROR_MSG( "Couldn't register watcher nub with machined: %s\n",
			Mercury::reasonToString( (Mercury::Reason)reason ) );
	}
}


/**
 *	This method sends the handler to receive events for this watcher.
 *
 *	@param wrh	The WatcherRequestHandler object to receive events.
 */
void WatcherNub::setRequestHandler( WatcherRequestHandler * pHandler )
{
	MF_ASSERT( !insideReceiveRequest_ );

	pExtensionHandler_ = pHandler;
}


/**
 * 	This method is called by Mercury when there is data to read on the
 * 	watcher socket. It calls the receiveRequest method to actually
 * 	process the request.
 */
int WatcherNub::handleInputNotification( int fd )
{
	if (fd == tcpSocket_.fileno())
	{
		WatcherConnection::handleAccept( tcpSocket_, *pDispatcher_, *this );

		return 0;
	}

	MF_ASSERT( fd == udpSocket_.fileno() );

	// ok, go fetch now!
	this->receiveUDPRequest();

	return 0;
}


/**
 *	This method should be called to handle requests on the socket.
 */
bool WatcherNub::receiveUDPRequest()
{
	AUTO_SCOPED_PROFILE( "watchersUDP" );

	if (!isInitialised_)
	{
		// TODO: Allow calls to this when not initialised before the client
		// currently does this. Should really fix the client so that this is
		// only called once initialised.
		return false;
	}

	sockaddr_in		senderAddr;
	int				len;

	MF_ASSERT( !insideReceiveRequest_ );

	insideReceiveRequest_ = true;

	// try to recv
	len = udpSocket_.recvfrom( requestPacket_, WN_PACKET_SIZE, senderAddr );

	if (len == -1)
	{
		// EAGAIN = no packets waiting, ECONNREFUSED = rejected outgoing packet

#ifdef _WIN32
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK && err != WSAECONNREFUSED && err != WSAECONNRESET)
#else
		int err = errno;
		if (err != EAGAIN && err != ECONNREFUSED)
#endif
		{
			ERROR_MSG( "WatcherNub::receiveUDPRequest: recvfrom failed\n" );
		}

		insideReceiveRequest_ = false;
		return false;
	}

	WatcherEndpoint watcherEndpoint( udpSocket_, senderAddr );

	return this->processRequest( requestPacket_, len, watcherEndpoint );
}


bool WatcherNub::processRequest( char * packet, int len,
	WatcherEndpoint & watcherEndpoint )
{
	// make sure we haven't picked up a weird packet (from when broadcast
	// was on, say ... hey, it could happen!)
	WatcherDataMsg * wdm = (WatcherDataMsg*)packet;

	if (len < (int)sizeof(WatcherDataMsg))
	{
		ERROR_MSG( "WatcherNub::receiveRequest: Packet is too short\n" );
		insideReceiveRequest_ = false;
		return false;
	}

	if (!(wdm->message == WATCHER_MSG_GET ||
		  wdm->message == WATCHER_MSG_GET_WITH_DESC ||
		  wdm->message == WATCHER_MSG_SET ||
		  wdm->message == WATCHER_MSG_GET2 ||
		  wdm->message == WATCHER_MSG_SET2))
	{
		int messageID = wdm->message;
		char * data = packet + sizeof( wdm->message);
		int dataLen = len - sizeof( wdm->message );

		if (pExtensionHandler_)
		{
			pExtensionHandler_->processExtensionMessage( messageID,
									data, dataLen, watcherEndpoint );
		}
		else
		{
			Mercury::Address addr = watcherEndpoint.remoteAddr();
			WARNING_MSG( "WatcherNub::processRequest: "
							"Unknown message %d from %s. Message len = %d\n",
						messageID, addr.c_str(), dataLen );
		}

		insideReceiveRequest_ = false;

		return true;
	}


	// Our reply handler for the current request.
	WatcherPacketHandler *packetHandler = NULL;

	// and call back to the program
	switch (wdm->message)
	{
		case WATCHER_MSG_GET:
		case WATCHER_MSG_GET_WITH_DESC:
		{
			// Create the packet reply handler for the current incoming request
			packetHandler = new WatcherPacketHandler( watcherEndpoint,
					wdm->count, WatcherPacketHandler::WP_VERSION_1, false );

			char	*astr = wdm->string;
			for(int i=0;i<wdm->count;i++)
			{
				this->processWatcherGetRequest( *packetHandler, astr,
						  (wdm->message == WATCHER_MSG_GET_WITH_DESC) );
				astr += strlen(astr)+1;
			}
		}
		break;

		case WATCHER_MSG_GET2:
		{
			// Create the packet reply handler for the current incoming request
			packetHandler = new WatcherPacketHandler( watcherEndpoint,
					wdm->count, WatcherPacketHandler::WP_VERSION_2, false );

			char	*astr = wdm->string;
			for(int i=0;i<wdm->count;i++)
			{
				unsigned int & seqNum = (unsigned int &)*astr;
				astr += sizeof(unsigned int);
				this->processWatcherGet2Request( *packetHandler, astr, seqNum );
				astr += strlen(astr)+1;
			}
		}
		break;


		case WATCHER_MSG_SET:
		{
			// Create the packet reply handler for the current incoming request
			packetHandler = new WatcherPacketHandler( watcherEndpoint,
					wdm->count, WatcherPacketHandler::WP_VERSION_1, true );

			char	*astr = wdm->string;
			for(int i=0;i<wdm->count;i++)
			{
				char	*bstr = astr + strlen(astr)+1;
				this->processWatcherSetRequest( *packetHandler, astr,bstr );
				astr = bstr + strlen(bstr)+1;
			}
		}
		break;


		case WATCHER_MSG_SET2:
		{
			// Create the packet reply handler for the current incoming request
			packetHandler = new WatcherPacketHandler( watcherEndpoint,
					wdm->count, WatcherPacketHandler::WP_VERSION_2, true );

			char	*astr = wdm->string;
			for(int i=0;i<wdm->count;i++)
			{
				this->processWatcherSet2Request( *packetHandler, astr );
			}
		}
		break;

		default:
		{
			WARNING_MSG( "WatcherNub::receiveRequest: "
						"Unknown message %d from %s\n",
					wdm->message, watcherEndpoint.remoteAddr().c_str() );
		}
		break;
	}

	// Start running the packet handler now, it will delete itself when
	// complete.
	if (packetHandler)
	{
		packetHandler->run();
		packetHandler = NULL;
	}

	// and we're done!
	insideReceiveRequest_ = false;

	return true;
}


void WatcherNub::processDisconnect( WatcherEndpoint & watcherEndpoint )
{
	if (pExtensionHandler_)
	{
		pExtensionHandler_->processDisconnect( watcherEndpoint );
	}
}


// -----------------------------------------------------------------------------
// Section: Message handlers
// -----------------------------------------------------------------------------

/**
 *	This function is called when a WATCHER_MSG_GET request is received.
 *
 * @param	packetHandler	A reference to the PacketHandler object which
 *					is responsible collecting responses generated by
 *					WatcherPathRequests.
 *	@param	path	The object to get. It has no leading slash
 *					and will only have a trailing slash for list-style 'GET'
 *					requests. I think you can probably ignore it in any
 *					case.
 *	@param	withDesc Boolean flag that indicates whether watcher description
 *					should be returned.
 *	@see	processWatcherSetRequest
 */
void WatcherNub::processWatcherGetRequest(
	WatcherPacketHandler & packetHandler, const char * path, bool withDesc )
{
#if ENABLE_WATCHERS
	BW::string newPath( path );
	WatcherPathRequestV1 *pRequest =
			(WatcherPathRequestV1 *)packetHandler.newRequest( newPath );
	pRequest->useDescription( withDesc );
#endif
}


/**
 *	This function is called a WATCHER_MSG_GET2 request is received.
 *
 *	@param	packetHandler	A reference to the PacketHandler object which
 *					is responsible collecting responses generated by
 *					WatcherPathRequests.
 *	@param	path	The object to get. It has no leading slash
 *					and will only have a trailing slash for list-style 'GET'
 *					requests. I think you can probably ignore it in any
 *					case.
 * @param seqNum  The sequence number associated with the path request (which
 *					needs to be used in the response).
 *
 *	@see	processWatcherSet2Request
 */
void WatcherNub::processWatcherGet2Request(
	WatcherPacketHandler & packetHandler, const char * path, uint32 seqNum )
{
#if ENABLE_WATCHERS
	BW::string newPath( path );
	WatcherPathRequestV2 *pRequest = (WatcherPathRequestV2 *)
								packetHandler.newRequest( newPath );
	pRequest->setSequenceNumber( seqNum );
#endif
}

/**
 *	This method handles watcher set requests.
 *
 *	@param path			The path of the watcher object.
 *	@param valueString	The value to which it should be set.
 *	@param packetHandler	The WatcherPacketHandler to use to notify of
 *	                         watcher results upon completion of the set.
 */
/**
 *	This method is called a WATCHER_MSG_SET request is received.
 *
 *	@param	packetHandler	A reference to the PacketHandler object which
 *					is responsible collecting responses generated by
 *					WatcherPathRequests.
 *	@param	path	The object to set. It will have no trailing slash.
 *	@param	valueString			The value to set the object to.
 *
 *	@see	processWatcherGetRequest
 */
void WatcherNub::processWatcherSetRequest(
		WatcherPacketHandler & packetHandler, const char * path,
		const char * valueString )
{
#if ENABLE_WATCHERS
	BW::string newPath( path );
	WatcherPathRequestV1 *pRequest =
			(WatcherPathRequestV1 *)packetHandler.newRequest( newPath );

	pRequest->setValueData( valueString );
#endif
}


/**
 *	This method is called a WATCHER_MSG_SET2 request is received.
 *
 *	@param	packetHandler	A reference to the PacketHandler object which
 *					is responsible collecting responses generated by
 *					WatcherPathRequests.
 *	@param	packet	This is a reference to the raw packet which contains
 *					the data to set the watcher path value.
 *
 *	@see	processWatcherGet2Request
 */
void WatcherNub::processWatcherSet2Request(
		WatcherPacketHandler & packetHandler, char* & packet )
{
#if ENABLE_WATCHERS
/* TODO: cleanup the passing in of packet as a reference here */
	uint32 & seqNum = (uint32 &)*packet;
	const char *path = packet + sizeof(uint32);
	char *curr = (char *)path + strlen(path)+1;

	// Determine the size of the contained data
	// then add the size of the prefixed information;
	// ie: type (1 byte), size data (1 or 4 bytes)
	//
	// Structure of stream pointed to by curr: <type> <data size> <data>.
	// If first byte after <type> is 0xff, then <data size> is packed in the
	// next 3 bytes.  Otherwise, this byte is the data size.
	// Also see pycommon/watcher_data_type.py.
	uint32 size = 0;
	uint8 sizeHint = (uint8)*(curr+1); // Skips "type" byte.
	if (sizeHint == 0xff)
	{
		// Skips "type" and "sizeHint" bytes.
		size = BW_UNPACK3( (curr+2) );
		size += 5;  // Size of prefixed information.
	}
	else
	{
		size = sizeHint + 2;
	}

	// Construct the path request handler
	BW::string newPath( path );

	WatcherPathRequestV2 *pRequest = (WatcherPathRequestV2 *)
								packetHandler.newRequest( newPath );
	pRequest->setSequenceNumber( seqNum );

	// Notify the path request of the outgoing data stream
	pRequest->setPacketData( size, curr );

	// Update where packet is pointing to for the next loop
	packet = curr + size;
#endif
}

BW_END_NAMESPACE

#endif /* ENABLE_WATCHERS */

// watcher_nub.cpp
