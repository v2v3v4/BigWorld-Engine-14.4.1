#ifndef LOGIN_REQUEST_TRANSPORT_HPP
#define LOGIN_REQUEST_TRANSPORT_HPP


#include "connection_transport.hpp"

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class LoginRequest;
typedef SmartPointer< LoginRequest > LoginRequestPtr;

class LoginRequestTransport;
typedef SmartPointer< LoginRequestTransport > LoginRequestTransportPtr;

namespace Mercury
{
	class Address;
	class Channel;
	class DeferredBundle;
	class NetworkInterface;
}


/**
 *	This class is responsible for sending to the server whatever requests
 *	are required to further the login process by communicating with a BigWorld
 *	server process.
 */
class LoginRequestTransport : public ReferenceCount
{
public:

	static LoginRequestTransportPtr createForTransport(
		ConnectionTransport transport );

	void init( LoginRequest & loginRequest );


	/**
	 *	This method creates the channel to the server process appropriate for
	 *	this transport. It will be stored on the associated LoginRequest.
	 */
	virtual void createChannel() = 0;

	/**
	 *	This method sends a request using the given protocol on this transport.
	 *
	 *	@param bundle 		A deferred bundle containing the streamed request.
	 *	@param shouldSendOnChannel
	 *						Whether to send the bundle off-channel. If true,
	 *						the channel on the parent LoginRequest is used, if
	 *						not present, a new one is made via createChannel()
	 *						and set on the parent LoginRequest. If false, the
	 *						request is sent off-channel.
	 *
	 *	@see LoginRequestTransport::createChannel
	 */
	virtual void sendBundle( Mercury::DeferredBundle & bundle,
		bool shouldSendOnChannel ) = 0;

	void finish();

protected:
	LoginRequestTransport();
	virtual ~LoginRequestTransport();

	Mercury::Channel * pChannel();
	void pChannel( Mercury::Channel * pChannel );

	/**
	 *	This method connects to the given interface using this transport
	 *	object.
	 *
	 *	On successful connection, the login request will be called back upon
	 *	with onTransportConnect(). On failure, onTransportConnectFailed() will
	 *	be called. The callbacks can be called synchronously or asynchronously
	 *	with respect to the call to this method.
	 */
	virtual void doConnect() = 0;


	/**
	 *	This method is called to finalise the connection attempt, and release
	 *	any resources held by this object, or cancel any outstanding requests.
	 */
	virtual void doFinish() = 0;


protected:
	LoginRequestPtr pLoginRequest_;
};


BW_END_NAMESPACE

#endif // LOGIN_REQUEST_TRANSPORT_HPP
