#include "pch.hpp"

#include "login_request_protocol.hpp"

#include "baseapp_ext_interface.hpp"
#include "baseapp_login_request_protocol.hpp"
#include "client_server_protocol_version.hpp"
#include "log_on_params.hpp"
#include "login_handler.hpp"
#include "login_interface.hpp"
#include "login_request.hpp"
#include "login_request_transport.hpp"
#include "loginapp_login_request_protocol.hpp"
#include "server_connection.hpp"

#include "network/bundle.hpp"
#include "network/deferred_bundle.hpp"
#include "network/encryption_filter.hpp"
#include "network/interfaces.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
LoginRequestProtocol::LoginRequestProtocol() :
		ReferenceCount()
{
}


/**
 *	Destructor.
 */
LoginRequestProtocol::~LoginRequestProtocol()
{
}


/**
 *	This method gets the protocol object for communicating with the LoginApp.
 */
LoginRequestProtocolPtr LoginRequestProtocol::getForLoginApp()
{
	static LoginRequestProtocolPtr protocol(
		new LoginAppLoginRequestProtocol() );
	return protocol;
}


/**
 *	This method gets the protocol object for communicating with the BaseApp.
 */
LoginRequestProtocolPtr LoginRequestProtocol::getForBaseApp()
{
	static LoginRequestProtocolPtr protocol(
		new BaseAppLoginRequestProtocol() );
	return protocol;
}


/**
 *	This method handles any Mercury-level exceptions that may come up in
 *	the course of connecting and waiting for a response from the server.
 *
 *	@param reason 	The reason for the failure.
 *	@param message 	A human-readable description of the error.
 */
void LoginRequestProtocol::onAttemptFailed( LoginRequest & request,
		Mercury::Reason reason, const BW::string & message ) const
{
	BW_GUARD;

	LoginHandler & loginHandler = request.loginHandler();

	// We need these to stick around to access any state.
	LoginRequestPtr pLoginRequestHolder( &request );
	LoginHandlerPtr pLoginHandlerHolder( &loginHandler );

	request.finish();

	loginHandler.onRequestFailed( request, reason, message );
}


BW_END_NAMESPACE


// login_request_protocol.cpp

