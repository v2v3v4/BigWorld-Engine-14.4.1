#ifndef BASEAPP_LOGIN_REQUEST_PROTOCOL_HPP
#define BASEAPP_LOGIN_REQUEST_PROTOCOL_HPP

#include "login_request_protocol.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class implements the protocol interaction between the BaseApp and the
 *	client.
 */
class BaseAppLoginRequestProtocol : public LoginRequestProtocol
{
public:

	/**
	 *	Constructor.
	 */
	BaseAppLoginRequestProtocol() :
			LoginRequestProtocol()
	{
	}

	/** Destructor. */
	virtual ~BaseAppLoginRequestProtocol()
	{
	}

	/* Overrides from LoginRequestProtocol */
	virtual const char * appName() const 		{ return "BaseApp"; }

	/* Overrides from LoginRequestProtocol */
	virtual const Mercury::Address & address( LoginHandler & handler ) const;

	virtual Mercury::NetworkInterface * setUpInterface(
		LoginRequest & request ) const;
	virtual void cleanUpInterface(
		LoginRequest & request, Mercury::NetworkInterface * pInterface ) const;
	virtual void onAttemptsExhausted( LoginHandler & handler,
		bool hasReceivedNoSuchPort ) const;
	virtual void sendRequest( LoginRequestTransport & transport,
		LoginRequest & request, float timeout ) const;
	virtual void onReply( LoginRequest & request,
		BinaryIStream & data ) const;

};

BW_END_NAMESPACE

#endif // BASEAPP_LOGIN_REQUEST_PROTOCOL_HPP
