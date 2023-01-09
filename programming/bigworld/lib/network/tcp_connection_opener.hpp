#ifndef TCP_CONNECTION_OPENER
#define TCP_CONNECTION_OPENER

#include "basictypes.hpp"
#include "interfaces.hpp"
#include "misc.hpp"

#include "cstdmf/timer_handler.hpp"

#include <memory>


BW_BEGIN_NAMESPACE


class Endpoint;


namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
class TCPChannel;
class TCPConnectionOpener;


/**
 *	Callback interface for TCPConnectionOpener.
 */
class TCPConnectionOpenerListener
{
public:
	/** Destructor. */
	virtual ~TCPConnectionOpenerListener() {}

	/**
	 *	This method is called when a TCP connection has been established.
	 *
	 *	@param opener	The TCPConnectionOpener object that established the
	 *					connection.
	 *	@param pChannel The resulting channel.
	 */
	virtual void onTCPConnect( TCPConnectionOpener & opener,
		TCPChannel * pChannel ) = 0;

	/**
	 *	This method is called when a TCP connection has failed to connect.
	 *
	 *	@param opener	The TCPConnectionOpener object that was attempting the
	 *					connection request. 
	 *	@param reason 	The reason for the connection failure.
	 */
	virtual void onTCPConnectFailure( TCPConnectionOpener & opener, 
			Reason reason ) = 0;
};


/**
 *	This class establishes a connection to a TCP server asynchronously, calling
 *	back on the TCPConnectionOpenerListener interface.
 */
class TCPConnectionOpener : public Mercury::InputNotificationHandler,
			public TimerHandler
{
public:
	TCPConnectionOpener( TCPConnectionOpenerListener & listener,
		NetworkInterface & networkInterface,
		const Address & connectAddress,
		void * pUserData = NULL,
		float timeoutSeconds = 10.f );

	virtual ~TCPConnectionOpener();

	/** This method returns the connection address. */
	const Address & connectAddress() const { return connectAddress_; }

	/** This method returns the user data associated with this request. */
	void * pUserData() { return pUserData_; }

	void cancel();

	// Override from Mercury::InputNotificationHandler.
	virtual int handleInputNotification( int fd );
	virtual bool handleErrorNotification( int fd );

	// Override from TimerHandler.
	virtual void handleTimeout( TimerHandle handle, void * pUser );

private:
	EventDispatcher & dispatcher();
	void handleSocketError( int error );

	TCPConnectionOpenerListener & 	listener_;
	NetworkInterface & 				networkInterface_;
	std::auto_ptr< Endpoint > 		pEndpoint_;
	Address 						connectAddress_;
	TimerHandle 					timerHandle_;
	void * 							pUserData_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // TCP_CONNECTION_OPENER

