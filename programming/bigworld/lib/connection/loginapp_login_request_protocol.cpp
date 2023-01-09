#include "pch.hpp"

#include "loginapp_login_request_protocol.hpp"

#include "client_server_protocol_version.hpp"
#include "log_on_params.hpp"
#include "login_challenge.hpp"
#include "login_challenge_factory.hpp"
#include "login_handler.hpp"
#include "login_interface.hpp"
#include "login_request.hpp"
#include "login_request_transport.hpp"
#include "server_connection.hpp"

#include "network/bundle.hpp"
#include "network/deferred_bundle.hpp"
#include "network/encryption_filter.hpp"


BW_BEGIN_NAMESPACE


/*
 *	Override from LoginRequestProtocol.
 */
const Mercury::Address & LoginAppLoginRequestProtocol::address(
		LoginHandler & handler ) const
{
	return handler.loginAppAddr();
}


/*
 *	Override from LoginRequestProtocol.
 */
Mercury::NetworkInterface * LoginAppLoginRequestProtocol::setUpInterface(
		LoginRequest & request ) const
{
	// Always borrow the ServerConnection's interface.
	Mercury::NetworkInterface * pInterface =
		&(request.loginHandler().pServerConnection()->networkInterface());
	pInterface->setNoSuchPortCallback( &(request.loginHandler()) );
	return pInterface;
}


/*
 *	Override from LoginRequestProtocol.
 */
void LoginAppLoginRequestProtocol::cleanUpInterface(
		LoginRequest & request, Mercury::NetworkInterface * pInterface ) const
{
	// The interface should still be on the ServerConnection.
	pInterface->setNoSuchPortCallback( NULL );
}


/*
 *	Override from LoginRequestProtocol.
 */
void LoginAppLoginRequestProtocol::onAttemptsExhausted( LoginHandler & handler,
		bool hasReceivedNoSuchPort ) const
{
	BW::string ipStr( handler.loginAppAddr().c_str() );

	BW::ostringstream messageStream;

	messageStream << "No response from LoginApp " << ipStr;

	LogOnStatus status = LogOnStatus::LOGIN_REJECTED_NO_LOGINAPP_RESPONSE;

	if (hasReceivedNoSuchPort)
	{
		messageStream << " (connection refused)";

		ERROR_MSG( "LoginAppLoginRequestProtocol::onAttemptsExhausted: "
				"The LoginApp port %s is closed. Check that LoginApp is "
				"running.\n",
			ipStr.c_str() );
	}

	ERROR_MSG( "LoginAppLoginRequestProtocol::onAttemptsExhausted: "
			"There was no response from LoginApp %s.\n"
			"Check that LoginApp is running and the firewall policy "
				"is not blocking its external port.\n"
			"If not running behind a NAT, make sure that the LoginApp's "
				"external address is %s.\n"
			"If running behind a NAT, make sure that packets to %s are "
				"correctly forwarded to the LoginApp's external port.\n",
		ipStr.c_str(), ipStr.c_str(), ipStr.c_str() );

	handler.fail( status, messageStream.str() );
}


/*
 *	Override from LoginRequestProtocol.
 */
void LoginAppLoginRequestProtocol::sendRequest(
		LoginRequestTransport & transport,
		LoginRequest & request,
		float timeout ) const
{
	BW_GUARD;

	Mercury::DeferredBundle bundle;

	if (request.loginHandler().pChallengeData())
	{
		bundle.startMessage( LoginInterface::challengeResponse,
			Mercury::RELIABLE_NO );
		BinaryIStream * pResponse = request.loginHandler().pChallengeData();

		bundle << request.loginHandler().challengeCalculationDuration();

		bundle.addBlob( pResponse->retrieve( 0 ),
			pResponse->remainingLength() );
	}

	bundle.startRequest( LoginInterface::login, &request, NULL,
		int( timeout * 1000000 ), Mercury::RELIABLE_NO );

	this->addRequestDataToStream( request, bundle );

	transport.sendBundle( bundle, /* shouldSendOnChannel */ false );
}


/**
 *	This method adds the request data to the given stream.
 */
void LoginAppLoginRequestProtocol::addRequestDataToStream(
		LoginRequest & request,
		Mercury::Bundle & bundle ) const
{
	LoginHandler & handler = request.loginHandler();

	LogOnParamsPtr pParams = handler.pParams();

	bundle << ClientServerProtocolVersion::currentVersion();

	if (!pParams->addToStream( bundle, LogOnParams::HAS_ALL,
			handler.pServerConnection()->pLogOnParamsEncoder() ))
	{
		ERROR_MSG( "LoginAppLoginRequestProtocol::addRequest: "
			"Failed to assemble login bundle\n" );

		request.finish();

		// Fail permanently in this case.
		handler.fail( LogOnStatus::PUBLIC_KEY_LOOKUP_FAILED,
			"Failed to encode log on parameters" );
	}
}


/*
 *	Override from LoginRequestProtocol.
 */
void LoginAppLoginRequestProtocol::onReply( LoginRequest & request,
		BinaryIStream & data ) const
{
	LoginHandlerPtr pHandler( &(request.loginHandler()) );

	uint8 statusValue;
	data >> statusValue;

	LogOnStatus::Status status = LogOnStatus::Status( statusValue );

	if (status == LogOnStatus::LOGIN_CHALLENGE_ISSUED)
	{
		this->handleChallengeFromStream( request, data );
		return;
	}

	BW::string serverMessage;

	if (status != LogOnStatus::LOGGED_ON)
	{

		if (data.remainingLength() > 0)
		{
			data >> serverMessage;
		}

		INFO_MSG( "LoginAppLoginRequestProtocol::onReply: "
				"Attempt #%u: Server rejected our login."
				" Reject status: %d, reject data (optional):"
				" %s\n",
			request.attemptNum(),
			status,
			serverMessage.empty() ?
				"none" : serverMessage.c_str() );

		if (serverMessage.empty()) // this really shouldn't happen
		{
			serverMessage =
				(status == LogOnStatus::LOGIN_CUSTOM_DEFINED_ERROR) ?
					"Unspecified error" :
					"Unelaborated error";
		}

		request.finish();

		pHandler->fail( status, serverMessage );

		return;
	}

	ServerConnection * pServerConnection = pHandler->pServerConnection();

	// The reply record is symmetrically encrypted.
	if (pServerConnection->pBlockCipher().exists())
	{
		MemoryOStream clearText;
		Mercury::EncryptionFilterPtr pFilter =
			Mercury::EncryptionFilter::create(
				pServerConnection->pBlockCipher() );

		if (!pFilter->decryptStream( data, clearText ))
		{
			ERROR_MSG( "LoginAppLoginRequestProtocol::handleMessage: "
					"Failed decrypt\n" );

			data.finish();
			clearText.finish();

			request.finish();

			pHandler->fail( LogOnStatus::CONNECTION_FAILED,
				"Failed to decrypt response from LoginApp" );

			return;
		}

		this->readReplyRecordFromStream( request, clearText );
	}
	else
	{
		this->readReplyRecordFromStream( request, data );
	}
}


/**
 *	This method handles a challenge issued from the LoginApp.
 */
void LoginAppLoginRequestProtocol::handleChallengeFromStream(
		LoginRequest & request,
		BinaryIStream & data ) const
{
	LoginHandlerPtr pHandler( &(request.loginHandler()) );

	BW::string challengeType;
	data >> challengeType;

	LoginChallengePtr pChallenge = pHandler->createChallenge( challengeType );

	if (!pChallenge)
	{
		ERROR_MSG( "LoginAppLoginRequestProtocol::onReply: "
				"Attempt #%u: Could not create challenge of type "
				"\"%s\"\n",
			request.attemptNum(), challengeType.c_str() );

		data.finish();
		request.finish();

		pHandler->fail( LogOnStatus::LOGIN_MALFORMED_REQUEST,
			"Unrecognised challenge type received from the server");
		return;
	}

	DEBUG_MSG( "LoginAppLoginRequestProtocol::handleChallengeFromStream: "
			"Attempt #%u: Server issued a challenge of type: \"%s\"\n",
		request.attemptNum(),
		challengeType.c_str() );

	pChallenge->readChallengeFromStream( data );

	request.finish();
	pHandler->onLoginAppLoginChallengeIssued( pChallenge );
}


/**
 *	This method reads the ReplyRecord from the given stream and notifies the
 *	LoginHandler appropriately.
 *
 *	@param request 	The login request.
 *	@param data 	The streamed reply record and possible server message.
 */
void LoginAppLoginRequestProtocol::readReplyRecordFromStream(
		LoginRequest & request, BinaryIStream & data ) const
{
	LoginHandler & handler = request.loginHandler();

	LoginReplyRecord replyRecord;
	BW::string serverMessage;

	if (data.remainingLength() < static_cast< int >(sizeof(replyRecord)))
	{
		ERROR_MSG( "LoginAppLoginRequestProtocol::readReplyRecordFromStream: "
				"Attempt #%u: Length of received cleartext data insufficient "
				"for reply record: got %d bytes, needed %" PRIzu " bytes\n",
			request.attemptNum(),
			data.remainingLength(),
			sizeof( replyRecord ) );

		data.finish();
		request.finish();

		handler.fail( LogOnStatus::CONNECTION_FAILED,
			"Insufficient data received from server" );

		return;
	}

	data >> replyRecord;

	if (data.remainingLength() > 0)
	{
		data >> serverMessage;

		if (!serverMessage.empty())
		{
			INFO_MSG( "LoginAppLoginRequestProtocol::"
					"readReplyRecordFromStream: "
					"Attempt #%u: Got message with success result from "
					"LoginApp: \"%s\"\n",
				request.attemptNum(),
				serverMessage.c_str() );
		}
	}

	if (data.error())
	{
		ERROR_MSG( "LoginAppLoginRequestProtocol::readReplyRecordFromStream: "
				"Attempt #%u: Got streaming error while streaming reply\n",
			request.attemptNum() );

		request.finish();

		handler.fail( LogOnStatus::CONNECTION_FAILED,
			"Failed to parse response from server" );

		return;
	}

	// Correct sized reply was received.

	request.finish();

	handler.onLoginAppLoginSuccess( replyRecord, serverMessage );
}


BW_END_NAMESPACE

// loginapp_login_request_protocol.cpp

