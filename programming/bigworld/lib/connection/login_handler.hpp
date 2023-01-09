#ifndef LOGIN_HANDLER_HPP
#define LOGIN_HANDLER_HPP

#include "connection_transport.hpp"
#include "log_on_params.hpp"
#include "log_on_status.hpp"
#include "login_reply_record.hpp"
#include "login_request.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"

#include "network/basictypes.hpp"
#include "network/packet_receiver.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class ServerConnection;
namespace Mercury
{
	class NetworkInterface;
}

class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;
class LoginChallengeTask;
typedef SmartPointer< LoginChallengeTask > LoginChallengeTaskPtr;
class LoginRequestProtocol;
typedef SmartPointer< LoginRequestProtocol > LoginRequestProtocolPtr;

class LoginHandler;
typedef SmartPointer< LoginHandler > LoginHandlerPtr;


/**
 *	Interface for login completion callbacks.
 */
class LoginCompletionCallback
{
public:
	/**
	 *	This method is called when the given LoginHandler has completed.
	 */ 
	virtual void onLoginComplete( LoginHandlerPtr pLoginHandler ) = 0;
};


/**
 *	This class is used to manage the various stages of logging into the server.
 *	This covers logging into both the LoginApp and the BaseApp.
 */
class LoginHandler :
		public Mercury::INoSuchPort,
		private TimerHandler,
		public SafeReferenceCount
{
public:
	LoginHandler( ServerConnection * pServerConnection, 
			LogOnStatus loginNotSent = LogOnStatus::NOT_SET );
	virtual ~LoginHandler();

	void start( const Mercury::Address & loginAppAddr,
		ConnectionTransport transport, LogOnParamsPtr pParams );
	void startWithBaseAddr( const Mercury::Address & baseAppAddr,
		ConnectionTransport transport, SessionKey loginKey );
	void finish();

	/** This method sets the login completion callback. */
	void setLoginCompletionCallback( 
			LoginCompletionCallback * pCompletionCallback )
	{ 
		pCompletionCallback_ = pCompletionCallback; 
	}

	// LoginRequest management and events.
	void sendNextRequest();
	void removeChildRequest( LoginRequest & request );
	void onRequestFailed( LoginRequest & request, Mercury::Reason reason,
		const BW::string & errorMessage );
	void fail( LogOnStatus status, const BW::string & errorMessage  );

	// Specific login events.
	void onLoginAppLoginChallengeIssued( LoginChallengePtr pChallenge );
	void onLoginChallengeCompleted();
	void onLoginAppLoginSuccess( const LoginReplyRecord & replyRecord,
		const BW::string & serverMessage );
	void onBaseAppLoginSuccess( Mercury::Channel * pChannel,
		SessionKey sessionKey );

	void addCondemnedInterface( Mercury::NetworkInterface * pInterface );

	// Various accessors
	ServerConnection * pServerConnection() const { return pServerConnection_; }

	const Mercury::Address & loginAppAddr() const { return loginAppAddr_; }
	const Mercury::Address & baseAppAddr() const { return baseAppAddr_; }

	bool isDone() const						{ return isDone_; }
	LogOnStatus status() const				{ return status_; }
	const BW::string & serverMsg() const	{ return serverMsg_; }
	const BW::string & errorMsg() const		{ return this->serverMsg(); }
	const LoginReplyRecord & replyRecord() const { return replyRecord_; }

	LogOnParamsPtr pParams()				{ return pParams_; }

	/** Return whether the login being handled is for a BaseApp switch. */
	bool isBaseAppSwitch() const
	{
		return ((loginAppAddr_ == Mercury::Address::NONE) &&
				(baseAppAddr_ != Mercury::Address::NONE));
	}

	LoginChallengePtr createChallenge( const BW::string & challengeType );
	float challengeCalculationDuration() const;
	BinaryIStream * pChallengeData();

private:
	void finishChildRequests();

	// Override from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * arg );

	// Override from INoSuchPort
	virtual void onNoSuchPort( const Mercury::Address & addr );

	ServerConnection * 	pServerConnection_;

	ConnectionTransport transport_;
	LoginRequestProtocolPtr pProtocol_;
	Mercury::Address	loginAppAddr_;
	Mercury::Address	baseAppAddr_;
	TimerHandle			retryTimer_;

	LoginChallengeTaskPtr pChallengeTask_;

	LogOnParamsPtr		pParams_;

	LoginReplyRecord	replyRecord_;
	LogOnStatus			status_;
	BW::string			serverMsg_;

	bool				isDone_;

	/// Number of attempts made for the current login protocol.
	uint 				numAttemptsMade_;

	/// Currently active LoginRequests
	typedef BW::set< LoginRequestPtr > ChildRequests;
	ChildRequests 		childRequests_;

	bool 				hasReceivedNoSuchPort_;

	LoginCompletionCallback * pCompletionCallback_;
};

BW_END_NAMESPACE


#endif // LOGIN_HANDLER_HPP

