#ifndef LOGIN_REQUEST_HPP
#define LOGIN_REQUEST_HPP


#include "cstdmf/smartpointer.hpp"
#include "cstdmf/timer_handler.hpp"

#include "network/basictypes.hpp"
#include "network/channel_listener.hpp"
#include "network/interfaces.hpp"
#include "network/misc.hpp"


BW_BEGIN_NAMESPACE

class LoginHandler;
typedef SmartPointer< LoginHandler > LoginHandlerPtr;

class LoginRequest;
typedef SmartPointer< LoginRequest > LoginRequestPtr;

class LoginRequestTransport;
typedef SmartPointer< LoginRequestTransport > LoginRequestTransportPtr;

class LoginRequestProtocol;
typedef SmartPointer< LoginRequestProtocol > LoginRequestProtocolPtr;


namespace Mercury
{
	class Channel;
	typedef SmartPointer< Channel > ChannelPtr;
	class NetworkInterface;
} // end namespace Mercury


/**
 *	This class represents an attempt to connect to a server process. 
 */
class LoginRequest : 
		public SafeReferenceCount,
		public Mercury::ShutdownSafeReplyMessageHandler,
		private Mercury::ChannelListener
{
public:
	LoginRequest( LoginHandler & parent,
		LoginRequestTransport & transport,
		LoginRequestProtocol & protocol,
		uint attemptNum );

	virtual ~LoginRequest();

	void finish();

	void onTransportConnect();

	void relinquishChannelAndInterface();

	// Accessors

	/** This method returns the parent LoginHandler object.*/
	LoginHandler & loginHandler() 				{ return *pParent_; }

	/** This method returns the attempt number of this request. */
	uint attemptNum() const 					{ return attemptNum_; }

	const char * appName() const;
	const Mercury::Address & address() const;

	/** This method returns whether this request has completed. */
	bool isFinished() const { return isFinished_; }

	float timeSinceStart() const;
	float timeLeft() const;

	/** Set accessor for the channel used to communicate to the server. */
	void pChannel( Mercury::Channel * pChannel );
	/** Get accessor for the channel used to communicate to the server. */
	Mercury::Channel * pChannel() const { return pChannel_.get(); }

	/** This method returns the NetworkInterface used for communication. */
	Mercury::NetworkInterface & networkInterface() { return *pInterface_; }

	void onTransportFailed( Mercury::Reason reason,
		const BW::string & errorMessage );

	const Mercury::Address & sourceAddress() const;

private:
	
	// Overrides from Mercury::ShutdownSafeReplyMessageHandler
	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );
	virtual void handleException( const Mercury::NubException & exception,	
		void * arg );

	// Override from ChannelListener
	virtual void onChannelGone( Mercury::Channel & channel );


	// Member data
	LoginHandlerPtr 			pParent_;

	LoginRequestTransportPtr 	pTransport_;
	LoginRequestProtocolPtr 	pProtocol_;

	bool 						isFinished_;

	/// This is the attempt number of the LoginRequest.
	const uint 					attemptNum_;

	/// This is the number of seconds to wait for each asynchronous operation.
	float 						timeoutInterval_;
	uint64						startTime_;

	Mercury::NetworkInterface * pInterface_;
	Mercury::ChannelPtr 		pChannel_;
};


BW_END_NAMESPACE


#endif // LOGIN_REQUEST_HPP
