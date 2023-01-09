#include "pch.hpp"

#include "login_request_transport.hpp"

#include "login_request.hpp"

#include "network/deferred_bundle.hpp"
#include "network/network_interface.hpp"
#include "network/tcp_channel.hpp"
#include "network/tcp_channel_stream_adaptor.hpp"
#include "network/tcp_connection_opener.hpp"
#include "network/udp_bundle.hpp"
#include "network/udp_channel.hpp"
#include "network/websocket_stream_filter.hpp"

BW_BEGIN_NAMESPACE


// ----------------------------------------------------------------------------
// Section: UDPLoginRequestTransport
// ----------------------------------------------------------------------------


/**
 *	This class provides a UDP-based transport for login requests.
 */
class UDPLoginRequestTransport : public LoginRequestTransport
{
public:

	/**
	 *	Constructor.
	 */
	UDPLoginRequestTransport() :
			LoginRequestTransport()
	{
	}


	/** Destructor. */
	virtual ~UDPLoginRequestTransport() 
	{
	}


	// Overrides from LoginRequestTransport

	virtual void doConnect();

	virtual void createChannel();

	virtual void sendBundle( Mercury::DeferredBundle & bundle,
		bool isOffChannel );

	/* Override from LoginRequestTransport. */
	virtual void doFinish()
	{
	}
};


/*
 *	Override from LoginRequestTransport.
 */
void UDPLoginRequestTransport::doConnect()
{
	BW_GUARD;

	// We can notify out to LoginRequest straight away.
	pLoginRequest_->onTransportConnect();
}


/*
 *	Override from LoginRequestTransport.
 */
void UDPLoginRequestTransport::createChannel()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( this->pChannel() == NULL )
	{
		WARNING_MSG( "UDPLoginRequestTransport::createChannel: "
				"A channel %s already exists, re-using\n",
			this->pChannel()->c_str() );
		return;
	}

	Mercury::UDPChannel * pChannel = new Mercury::UDPChannel(
		pLoginRequest_->networkInterface(),
		pLoginRequest_->address(),
		Mercury::UDPChannel::EXTERNAL );

	pChannel->isLocalRegular( false );
	pChannel->isRemoteRegular( false );

	this->pChannel( pChannel );
}


/*
 *	Override from LoginRequestTransport.
 */
void UDPLoginRequestTransport::sendBundle(
		Mercury::DeferredBundle & deferredBundle,
		bool shouldSendOnChannel )
{
	BW_GUARD;

	Mercury::NetworkInterface * pInterface =
		&(pLoginRequest_->networkInterface());

	MF_ASSERT( pInterface != NULL );

	if (!shouldSendOnChannel)
	{
		Mercury::UDPBundle bundle;

		deferredBundle.applyToBundle( bundle );

		pInterface->send( pLoginRequest_->address(), bundle );
	}
	else
	{
		if (this->pChannel() == NULL)
		{
			this->createChannel();
		}

		deferredBundle.applyToBundle( this->pChannel()->bundle() );

		this->pChannel()->send();
	}
}



// ----------------------------------------------------------------------------
// Section: TCPLoginRequestTransport
// ----------------------------------------------------------------------------


/**
 *	This class provides a TCP-based transport for login requests.
 */
class TCPLoginRequestTransport : public LoginRequestTransport,
		private Mercury::TCPConnectionOpenerListener,
		private Mercury::ChannelListener
{
public:
	/**
	 *	Constructor.
	 */
	TCPLoginRequestTransport() :
			LoginRequestTransport(),
			Mercury::TCPConnectionOpenerListener(),
			pOpener_( NULL )
	{
	}

	/** Destructor. */
	virtual ~TCPLoginRequestTransport()
	{
		this->finish();
	}

	// Overrides from LoginRequestTransport
	virtual void createChannel();

	virtual void sendBundle( Mercury::DeferredBundle & bundle,
		bool isOffChannel );

protected:
	virtual void doConnect();

	virtual void doFinish();

	// Overrides from Mercury::TCPConnectionOpenerListener
	virtual void onTCPConnect( Mercury::TCPConnectionOpener & opener,
		Mercury::TCPChannel * pChannel );

private:
	virtual void onTCPConnectFailure( Mercury::TCPConnectionOpener & opener,
		Mercury::Reason reason );

	// Member data
	Mercury::TCPConnectionOpener * pOpener_;
};


/*
 *	Override from LoginRequestTransport.
 */
void TCPLoginRequestTransport::createChannel()
{
	// Shouldn't be called here as we have set the channel before calling back
	// on onTransportConnect.
	MF_ASSERT( this->pChannel() != NULL );

	DEV_CRITICAL_MSG( "TCPLoginRequestTransport::createChannel: "
			"Attempt #%u to %s %s: "
			"Unexpectedly called; existing channel is %s\n",
		pLoginRequest_->attemptNum(),
		pLoginRequest_->appName(),
		pLoginRequest_->address().c_str(),
		this->pChannel() ? this->pChannel()->c_str() : "(none)" );
}


/*
 *	Override from Mercury::LoginRequestTransport.
 */
void TCPLoginRequestTransport::sendBundle( Mercury::DeferredBundle & bundle,
		bool /* isOffChannel: not supported in TCP */ )
{
	BW_GUARD;

	// We always send on-channel for TCP. 
	Mercury::Channel * pChannel = this->pChannel();

	MF_ASSERT( pChannel != NULL );

	bundle.applyToBundle( pChannel->bundle() );
	pChannel->send();
}


/*
 *	Override from LoginRequestTransport.
 */
void TCPLoginRequestTransport::doConnect()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( pOpener_ == NULL )
	{
		// Only one connection request per LoginRequest object.
		ERROR_MSG( "TCPLoginRequestTransport::doConnect: Attempt #%u to %s %s: "
				"TCP connection already pending establishment\n",
			pLoginRequest_->attemptNum(),
			pLoginRequest_->appName(),
			pLoginRequest_->address().c_str() );
		return;
	}

	pOpener_ = new Mercury::TCPConnectionOpener( *this,
		pLoginRequest_->networkInterface(),
		pLoginRequest_->address(),
		/* pUserData */ NULL,
		pLoginRequest_->timeLeft() );
}


/*
 *	Override from LoginRequestTransport.
 */
void TCPLoginRequestTransport::doFinish()
{
	if (pOpener_ != NULL)
	{
		pOpener_->cancel();
		bw_safe_delete( pOpener_ );
	}
}


/*
 *	Override from Mercury::TCPConnectionOpenerListener.
 */
void TCPLoginRequestTransport::onTCPConnect(
		Mercury::TCPConnectionOpener & opener,
		Mercury::TCPChannel * pChannel )
{
	BW_GUARD;

	this->pChannel( pChannel );
	
	MF_ASSERT( pOpener_ == &opener );

	bw_safe_delete( pOpener_ );

	pLoginRequest_->onTransportConnect();
}


/*
 *	Override from Mercury::TCPConnectionOpenerListener.
 */
void TCPLoginRequestTransport::onTCPConnectFailure(
		Mercury::TCPConnectionOpener & opener,
		Mercury::Reason reason )
{
	pLoginRequest_->onTransportFailed( reason,
		"Failed to make TCP connection" );
}


// ----------------------------------------------------------------------------
// Section: WebSocketsLoginRequestTransport
// ----------------------------------------------------------------------------


/**
 *	This class provides a WebSockets-based transport for login requests.
 */
class WebSocketsLoginRequestTransport : public TCPLoginRequestTransport
{
public:
	/**
	 *	Constructor.
	 */
	WebSocketsLoginRequestTransport() :
			TCPLoginRequestTransport()
	{
	}


	/** Destructor. */
	virtual ~WebSocketsLoginRequestTransport()
	{
	}


	// Overrides from TCPLoginRequestTransport.
	virtual void onTCPConnect( Mercury::TCPConnectionOpener & opener,
		Mercury::TCPChannel * pChannel );
};


/*
 *	Override from TCPLoginRequestTransport.
 */
void WebSocketsLoginRequestTransport::onTCPConnect(
		Mercury::TCPConnectionOpener & opener,
		Mercury::TCPChannel * pChannel )
{
	BW_GUARD;

	// TODO: Have the host name passed through from ServerConnection.
	BW::string host( pLoginRequest_->appName() );

	// TODO: Use this URI for something.
	static const BW::string URI( "/" );

	Mercury::NetworkStreamPtr pChannelStream(
		new Mercury::TCPChannelStreamAdaptor( *pChannel ) );

	Mercury::StreamFilter * pFilter =
		new Mercury::WebSocketStreamFilter( *pChannelStream, host, URI );
	pChannel->pStreamFilter( pFilter );

	this->TCPLoginRequestTransport::onTCPConnect( opener, pChannel );
}


// ----------------------------------------------------------------------------
// Section: LoginRequestTransport
// ----------------------------------------------------------------------------


/**
 *	Constructor.
 */
LoginRequestTransport::LoginRequestTransport() :
		ReferenceCount(),
		pLoginRequest_()
{
}


/** 
 *	Destructor.
 */
LoginRequestTransport::~LoginRequestTransport() 
{
}


/**
 *	This method creates a transport for the given ConnectionTransport value.
 *
 *	@param transport 	The transport value.
 */
LoginRequestTransportPtr LoginRequestTransport::createForTransport(
		ConnectionTransport transport )
{
	BW_GUARD;

	// UDP transport is stateless, no need to have multiple instances.
	static LoginRequestTransportPtr pUDPTransport =
		new UDPLoginRequestTransport();

	switch (transport)
	{
	case CONNECTION_TRANSPORT_UDP:
		return pUDPTransport;

	case CONNECTION_TRANSPORT_TCP:
		return new TCPLoginRequestTransport();

	case CONNECTION_TRANSPORT_WEBSOCKETS:
#if defined( __EMSCRIPTEN__ )
		// Emscripten already wraps TCP sockets as WebSockets.
		return new TCPLoginRequestTransport();
#else // !defined( __EMSCRIPTEN__ )
		return new WebSocketsLoginRequestTransport();
#endif // defined( __EMSCRIPTEN__ )

	default:
		return LoginRequestTransportPtr();
	}

	return pUDPTransport;
}


/**
 *	This method initialises this object and starts the connection process.
 *
 *	This process will callback on either LoginRequest::onTransportConnect() or
 *	LoginRequest::onAttemptFailed(), which may be synchronously
 *	called (that is before this method terminates).
 *
 *	@param pLoginRequest	The associated login request.
 *
 *	@see LoginRequest::onTransportConnect
 *	@see LoginRequest::onAttemptFailed
 */
void LoginRequestTransport::init( LoginRequest & loginRequest )
{
	BW_GUARD;

	if (pLoginRequest_.hasObject())
	{
		this->finish();
	}

	pLoginRequest_ = &loginRequest;

	MF_ASSERT( loginRequest.address() != Mercury::Address::NONE );
	MF_ASSERT( (&loginRequest.networkInterface()) != NULL );

	this->doConnect();
}

/**
 *	This method is called to release resources held by this object, such as
 *	outstanding requests.
 */
void LoginRequestTransport::finish()
{
	BW_GUARD;

	if (!pLoginRequest_.hasObject())
	{
		// Already finished, or not initialised.
		return;
	}

	this->doFinish();

	pLoginRequest_ = NULL;
}


/**
 *	This method gets the channel to be used by this transport.
 */
Mercury::Channel * LoginRequestTransport::pChannel()
{
	return pLoginRequest_->pChannel();
}


/**
 *	This method sets the channel to be used by this transport.
 */
void LoginRequestTransport::pChannel( Mercury::Channel * pChannel )
{
	// Our parent LoginRequest holds it and destroys it when done.
	pLoginRequest_->pChannel( pChannel );
}


BW_END_NAMESPACE


// login_request_transport.cpp
