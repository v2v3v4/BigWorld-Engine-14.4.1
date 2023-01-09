#ifndef LOGIN_REQUEST_PROTOCOL_HPP
#define LOGIN_REQUEST_PROTOCOL_HPP


#include "cstdmf/smartpointer.hpp"

#include "network/misc.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;

class LoginHandler;
class LoginRequest;
typedef SmartPointer< LoginRequest > LoginRequestPtr;
class LoginRequestProtocol;
typedef SmartPointer< LoginRequestProtocol > LoginRequestProtocolPtr;
class LoginRequestTransport;

namespace Mercury
{
	class Address;
	class NetworkInterface;
} // end namespace Mercury


/**
 *	This abstract class is used as a base class for classes that encapsulate
 *	login protocol interactions between the client and a specific server
 *	process.
 *
 *	Protocol objects are stateless, and per-request protocol state is stored as
 *	part of the LoginRequest object.
 */
class LoginRequestProtocol : public ReferenceCount
{
public:
	static LoginRequestProtocolPtr getForLoginApp();
	static LoginRequestProtocolPtr getForBaseApp();

	virtual const char * appName() const = 0;

	// TODO: Make these configurable.
	// TODO: Make these dependent on transport. For example, for TCP, we need
	// to do a round-trips to connect before we send the request, so we may
	// want to double the timeout relative to UDP.

	/**
	 *	This method returns the number of request attempts to be made.
	 */
	virtual uint numRequests() const 			{ return 10; }

	/**
	 *	This method returns the number of seconds before timing out a request.
	 */
	virtual float timeoutInterval() const 		{ return 8.f; }

	/**
	 *	This method returns the number of seconds between consecutive requests.
	 */
	virtual float retryInterval() const 		{ return 1.f; }

	/**
	 *	This method returns the appropriate address of application that this
	 *	protocol will interact with.
	 */
	virtual const Mercury::Address & address(
		LoginHandler & handler ) const = 0;


	/**
	 *	This method returns a network interface for the given request to use.
	 *
	 *	@param request 	The login request to give a network interface to.
	 *	@return 		A network interface to be used by the login request.
	 */
	virtual Mercury::NetworkInterface * setUpInterface(
		LoginRequest & request ) const = 0;


	/**
	 *	This method cleans up the network interface given to a login request.
	 *
	 *	@param request 		The request.
	 *	@param pInterface 	The network interface to be disposed of.
	 */
	virtual void cleanUpInterface( LoginRequest & request,
		Mercury::NetworkInterface * pInterface ) const = 0;


	void onAttemptFailed( LoginRequest & request,
		Mercury::Reason reason, const BW::string & message ) const;


	/**
	 *	This method handles the case where there are no more attempts to be
	 *	made, and none of the attempts have been successful.
	 *
	 *	@param handler 					The login handler.
	 *	@param hasReceivedNoSuchPort 	Whether an ICMP error has been received
	 *									by any network interface during the
	 *									attempts.
	 */
	virtual void onAttemptsExhausted( LoginHandler & handler,
		bool hasReceivedNoSuchPort ) const = 0;


	/**
	 *	This method is called to send a request to the server process
	 *	appropriate to this protocol.
	 *
	 *	@param transport	The transport to use.
	 *	@param timeout 		The timeout, in seconds, for the request.
	 */
	virtual void sendRequest( LoginRequestTransport & transport,
		LoginRequest & request, float timeout ) const = 0;


	/**
	 *	This method is called to handle a reply message from the server.
	 *
	 *	@param request 		The login request.
	 *	@param data 		The message data.
	 */
	virtual void onReply( LoginRequest & request,
		BinaryIStream & data ) const = 0;

protected:

	LoginRequestProtocol();

	/** Destructor. */
	virtual ~LoginRequestProtocol();
};


BW_END_NAMESPACE

#endif // LOGIN_REQUEST_PROTOCOL_HPP

