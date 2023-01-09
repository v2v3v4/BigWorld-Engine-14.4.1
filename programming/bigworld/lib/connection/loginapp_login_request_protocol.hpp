#ifndef LOGINAPP_LOGIN_REQUEST_PROTOCOL_HPP
#define LOGINAPP_LOGIN_REQUEST_PROTOCOL_HPP

#include "login_request_protocol.hpp"

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class LoginChallengeTask;
typedef SmartPointer< LoginChallengeTask > LoginChallengeTaskPtr;
class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;

namespace Mercury 
{
	class Bundle;
} // end namespace Mercury


/**
 *	This class implements the protocol interaction between the LoginApp and the
 *	client.
 */
class LoginAppLoginRequestProtocol : public LoginRequestProtocol
{
public:

	/**
	 *	Constructor.
	 */
	LoginAppLoginRequestProtocol() :
			LoginRequestProtocol()
	{}


	/** Destructor. */
	~LoginAppLoginRequestProtocol()
	{}

	/* Override from LoginRequestProtocol.*/
	virtual const char * appName() const 		{ return "LoginApp"; }

	/* Override from LoginRequestProtocol.*/
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

private:
	void addRequestDataToStream( LoginRequest & request,
		Mercury::Bundle & bundle ) const;

	void handleChallengeFromStream( LoginRequest & request,
		BinaryIStream & data ) const;

	void readReplyRecordFromStream( LoginRequest & request,
		BinaryIStream & data ) const;
};


BW_END_NAMESPACE

#endif // LOGINAPP_LOGIN_REQUEST_PROTOCOL_HPP
