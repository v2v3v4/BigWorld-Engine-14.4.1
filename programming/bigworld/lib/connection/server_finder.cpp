#include "pch.hpp"

#include "server_finder.hpp"

#include "connection/login_interface.hpp"

#include "cstdmf/debug.hpp"

#include "network/event_dispatcher.hpp"
#include "network/machine_guard.hpp"
#include "network/network_interface.hpp"
#include "network/portmap.hpp"
#include "network/udp_bundle.hpp"


DECLARE_DEBUG_COMPONENT2( "Connection", 0 )


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: ServerFinder
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
ServerFinder::ServerFinder() :
		pNetworkInterface_( NULL ),
		endpoint_()
{
	endpoint_.socket( SOCK_DGRAM );
	endpoint_.bind();
	endpoint_.setbroadcast( true );
	endpoint_.setnonblocking( true );
}


/**
 *	Destructor.
 */
ServerFinder::~ServerFinder()
{
	if (pNetworkInterface_)
	{
		pNetworkInterface_->dispatcher().deregisterFileDescriptor(
			endpoint_.fileno() );
	}

	timerHandle_.cancel();
}


/**
 *	This method starts the finding of LoginApp processes on the local network.
 *	For each LoginApp found, the onServerFound() virtual method will be called.
 */
void ServerFinder::findServers( Mercury::NetworkInterface * pInterface,
		float timeout )
{
	if (pNetworkInterface_)
	{
		ERROR_MSG( "ServerFinder::findServers: Already searching\n" );
		this->onFinished();
		return;
	}

	if (!endpoint_.good())
	{
		ERROR_MSG( "ServerFinder::findServers: Could not make endpoint\n" );
		this->onFinished();
		return;
	}

	pNetworkInterface_ = pInterface;
	pNetworkInterface_->dispatcher().registerFileDescriptor(
		endpoint_.fileno(), this );

	timerHandle_ = pNetworkInterface_->dispatcher().addOnceOffTimer(
			uint64( timeout * 1000000 ), this );

	this->sendFindMGM();
}


/**
 *	This method sends a probe message to a LoginApp to get more details about
 *	the server.
 */
void ServerFinder::sendProbe( const Mercury::Address & address,
	ServerProbeHandler * pHandler )
{
	Mercury::UDPBundle bundle;
	bundle.startRequest( LoginInterface::probe,
		pHandler,
		NULL,
		Mercury::DEFAULT_REQUEST_TIMEOUT,
		Mercury::RELIABLE_NO );
	pNetworkInterface_->send( address, bundle );
}


/**
 *	This method is called when there is network activity to respond to.
 */
int ServerFinder::handleInputNotification( int fd )
{
	while (this->recv())
	{
		;	// receive messages
	}

	if (responseChecker_.isDone())
	{
		this->onFinished();
	}

	return 0;
}


/**
 *	This method all responses were not received in an appropriate amount of
 *	time.
 */
void ServerFinder::handleTimeout( TimerHandle handle, void * pUser )
{
	this->onFinished();
}


/**
 *	This private method sends a 'find' machine guard message.
 */
void ServerFinder::sendFindMGM()
{
	ProcessStatsMessage psm;

	psm.param_ = psm.PARAM_USE_CATEGORY | psm.PARAM_USE_NAME;
	psm.category_ = psm.SERVER_COMPONENT;
	psm.name_ = "LoginInterface";
	psm.sendto( endpoint_, htons( PORT_MACHINED ) );
}


/**
 *	This private method recevies an outstanding packet.  It will return true
 *  if it is able to read any data off the wire, whether or not that data is
 *  useful.
 */
bool ServerFinder::recv()
{
	char recvbuf[ MGMPacket::MAX_SIZE ];
	uint32 srcaddr;
	int len = endpoint_.recvfrom( recvbuf, sizeof( recvbuf ), NULL, &(u_int32_t&)srcaddr );

	if (len <= 0)
	{
		return false;
	}

	MemoryIStream is( recvbuf, len );
	MGMPacket packet( is );

	if (is.error())
	{
		ERROR_MSG( "ServerFinder::recv: Garbage MGM received\n" );
		return true;
	}

	responseChecker_.receivedPacket( srcaddr, packet );

	for (unsigned i=0; i < packet.messages_.size(); i++)
	{
		ProcessStatsMessage &msg =
			static_cast< ProcessStatsMessage& >( *packet.messages_[i] );

		Mercury::Address srcAddr( srcaddr, msg.port_ );

		this->onMessage( msg, srcAddr );
	}

	return true;
}


/**
 *	This method is called for each ProcessMessage response that is received.
 */
void ServerFinder::onMessage( const ProcessMessage & msg,
		const Mercury::Address & srcAddr )
{
	// Verify message type
	if (msg.message_ != msg.PROCESS_STATS_MESSAGE)
	{
		ERROR_MSG( "ServerFinder::onMessage: Unexpected message type %d\n",
			msg.message_ );

		return;
	}

	// pid == 0 means no process found
	if (msg.pid_ == 0)
	{
		return;
	}

	this->onServerFound( ServerInfo( srcAddr, msg.uid_ ) );
}


/**
 *	This method is called when the server discovery process has completed. 
 *
 *	The default behaviour is to delete itself. Subclasses should override this
 *	method to either do additional teardown or, if desired, prevent the 
 *	deletion. 
 */
void ServerFinder::onFinished()
{
	timerHandle_.cancel();
}


void ServerFinder::onRelease( TimerHandle handle, void * pUser )
{
	delete this;
}


// -----------------------------------------------------------------------------
// Section: ServerProbeHandler
// -----------------------------------------------------------------------------


/**
 *	This method handles the probe response. It splits the response into calls
 *	to the onKeyValue and onSuccess virtual methods.
 */
void ServerProbeHandler::handleMessage( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	BW::string key;
	BW::string value;

	while (data.remainingLength() > 0)
	{
		data >> key >> value;
		this->onKeyValue( key, value );
	}

	this->onSuccess();

	this->onFinished();
}


/**
 *	This method is called if the probe fails.
 */
void ServerProbeHandler::handleException( const Mercury::NubException & e,
	void * arg )
{
	this->onFailure();
	this->onFinished();
}


/**
 *	This method is called when the server probe has finished. 
 *
 *	The default behaviour is to delete itself. Subclasses should override this
 *	method to either do additional teardown or, if desired, prevent the 
 *	deletion. 
 */
void ServerProbeHandler::onFinished()
{
	delete this;
}

BW_END_NAMESPACE

// server_finder.cpp
