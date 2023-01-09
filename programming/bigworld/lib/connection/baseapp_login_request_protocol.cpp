#include "pch.hpp"

#include "baseapp_login_request_protocol.hpp"

#include "baseapp_ext_interface.hpp"
#include "log_on_params.hpp"
#include "login_handler.hpp"
#include "login_request.hpp"
#include "login_request_transport.hpp"
#include "server_connection.hpp"

#include "network/bundle.hpp"
#include "network/deferred_bundle.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE


/*
 *	Override from LoginRequestProtocol.
 */
const Mercury::Address & BaseAppLoginRequestProtocol::address(
		LoginHandler & handler ) const /* override */
{
	return handler.baseAppAddr();
}


/*
 *	Override from LoginRequestProtocol.
 */
Mercury::NetworkInterface * BaseAppLoginRequestProtocol::setUpInterface(
		LoginRequest & request ) const
{
	BW_GUARD;

	// For BaseApp logins, the first request steals the existing NetworkInterface
	// from the ServerConnection, for the subsequent requests, a new
	// NetworkInterface is created (with different source port each time).
	// The NetworkInterface is always attached to pServerConnection->dispatcher()

	Mercury::NetworkInterface * pInterface = NULL;

	ServerConnection * pServerConnection =
		request.loginHandler().pServerConnection();

	MF_ASSERT( pServerConnection != NULL );

	if (pServerConnection->hasInterface())
	{
		pInterface = &(pServerConnection->networkInterface());
		pServerConnection->pInterface( NULL );
	}
	else
	{
		pInterface = new Mercury::NetworkInterface(
			&(pServerConnection->dispatcher()),
			Mercury::NETWORK_INTERFACE_EXTERNAL );

		pServerConnection->registerInterfaces( *pInterface );
	}

	pInterface->setNoSuchPortCallback( &(request.loginHandler()) );

	return pInterface;
}


/*
 *	Override from LoginRequestProtocol.
 */
void BaseAppLoginRequestProtocol::cleanUpInterface( LoginRequest & request,
		Mercury::NetworkInterface * pInterface ) const
{
	pInterface->setNoSuchPortCallback( NULL );
	pInterface->detach();
	request.loginHandler().addCondemnedInterface( pInterface );
}


/*
 *	Override from LoginRequestProtocol.
 */
void BaseAppLoginRequestProtocol::onAttemptsExhausted( LoginHandler & handler,
		bool hasReceivedNoSuchPort ) const
{
	// Give some helpful debugging to help diagnose apparent non-response from
	// the BaseApp.
	const Mercury::Address & baseAppAddr = handler.baseAppAddr();
	const Mercury::Address & loginAppAddr= handler.loginAppAddr();

	uint32 baseAppIP = ntohl( baseAppAddr.ip );
	uint32 loginAppIP = ntohl( loginAppAddr.ip );

	uint8 baseAppIPByte1 = (baseAppIP >> 24);
	uint8 baseAppIPByte2 = (baseAppIP >> 16);

	uint8 loginAppIPByte1 = (loginAppIP >> 24);
	uint8 loginAppIPByte2 = (loginAppIP >> 16);

	if ((baseAppIPByte1 != loginAppIPByte1) && (loginAppIPByte1 != 0))
	{
		// TODO: Since we don't currently know the netmask, we guess whether
		// they are on the same subnet based on just the first byte, which
		// won't be totally accurate.

		// Note: Needs to be separate as there are only two character
		// buffers used by Mercury::Address.
		BW::string externalAddrStr = loginAppAddr.ipAsString();

		ERROR_MSG( "BaseAppLoginRequestProtocol::onAttemptsExhausted: "
				"The LoginApp (%s) and BaseApp (%s) are on different "
					"networks.\n"
				"If the server is running behind a NAT the settings in "
					"bw.xml should be set to something like:\n"
				"<networkAddressTranslation>\n"
				"  <externalAddress> %s </externalAddress>\n"
				"  <localNetMask> %d.%d.0.0/16 </localNetMask>\n"
				"</networkAddressTranslation>\n"
				"\n"
				"If the server is not running behind a NAT. Check that the "
					"BaseApp has bound to the machine's external network "
					"interface.\n"
				"This should be configured in bw.xml with something like:\n"
				"<externalInterface> %d.%d.0.0/16 </externalInterface>\n",
			loginAppAddr.c_str(),
			baseAppAddr.c_str(),
			externalAddrStr.c_str(),
			baseAppIPByte1,
			baseAppIPByte2,
			loginAppIPByte1,
			loginAppIPByte2 );
	}
	else
	{
		ERROR_MSG( "LoginHandler::onAttemptsExhausted: "
				"Check that the firewall policy is not blocking packets to the "
					"BaseApp on the external interface.\n"
				"If the server is running behind a NAT, check that %s is "
					"being forwarded to the external address of the "
					"BaseApp.\n",
			baseAppAddr.c_str() );

		if (hasReceivedNoSuchPort)
		{
			ERROR_MSG( "BaseAppLoginRequestProtocol::onAttemptsExhausted: "
					"Note that an ICMP error indicating no-such-port was "
					"received. The port at %s may be closed.\n",
				baseAppAddr.c_str() );
		}
	}

	BW::ostringstream messageStream;
	messageStream << "Exhausted available attempts "
		"(" << this->numRequests() << ") to login to BaseApp " <<
		baseAppAddr.c_str();

	handler.fail( LogOnStatus::LOGIN_REJECTED_NO_BASEAPP_RESPONSE,
		messageStream.str() );
}


/*
 *	Override from LoginRequestProtocol.
 */
void BaseAppLoginRequestProtocol::sendRequest(
		LoginRequestTransport & transport,
		LoginRequest & request,
		float timeout ) const
{
	BW_GUARD;

	Mercury::DeferredBundle bundle;

	// Send the loginKey and number of attempts (for debugging reasons)
	BaseAppExtInterface::baseAppLoginArgs args;
	args.key = request.loginHandler().replyRecord().sessionKey;
	args.numAttempts = uint8( request.attemptNum() );
	bundle.sendRequest( args, &request, NULL,
		int( timeout * 1000000 ), Mercury::RELIABLE_NO );

	transport.sendBundle( bundle, /* shouldSendOnChannel */ false );

	// We need to set encryption in order to receive the reply.

	// Create a channel if we don't have one yet.
	if (request.pChannel() == NULL)
	{
		// This should only occur for requests over UDP.
		transport.createChannel();
	}
	request.pChannel()->setEncryption(
		request.loginHandler().pServerConnection()->pBlockCipher() );
}


/*
 *	Override from LoginRequestProtocol.
 */
void BaseAppLoginRequestProtocol::onReply( LoginRequest & request,
		BinaryIStream & data ) const
{
	BW_GUARD;

	Mercury::Channel * pChannel = request.pChannel();
	LoginHandlerPtr pHandler = &(request.loginHandler());

	IF_NOT_MF_ASSERT_DEV( pChannel != NULL )
	{
		request.finish();
		pHandler->fail( LogOnStatus::CONNECTION_FAILED,
			"Did not get channel from request" );

		return;
	}

	SessionKey sessionKey = 0;
	data >> sessionKey;

	if (data.error())
	{
		request.finish();
		pHandler->fail( LogOnStatus::CONNECTION_FAILED,
			"Failed to de-stream response from BaseApp" );

		return;
	}

	// We won! Hand over ownership of channel (and its associated network
	// interface).
	request.networkInterface().setNoSuchPortCallback( NULL );

	request.relinquishChannelAndInterface();
	request.finish();


	pHandler->onBaseAppLoginSuccess( pChannel, sessionKey );
}


BW_END_NAMESPACE


// baseapp_login_request_protocol.cpp

