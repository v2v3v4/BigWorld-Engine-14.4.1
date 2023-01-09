#ifndef NETWORK_INTERFACES_HPP
#define NETWORK_INTERFACES_HPP

#include "misc.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Address;
class Channel;
class Bundle;
class NetworkInterface;


/**
 *	This class declares an interface for receiving Mercury messages.
 *	Classes that can handle general messages from Mercury needs to
 *	implement this.
 *
 *	@ingroup mercury
 */
class InputMessageHandler
{
public:
	/** Constructor */
	InputMessageHandler() : isEarly_( false ) {}

	/** Destructor. */
	virtual ~InputMessageHandler() {}

	/**
	 * 	This method is called by Mercury to deliver a message.
	 *
	 * 	@param source	The address at which the message originated.
	 * 	@param header	This contains the message type, size, and flags.
	 * 	@param data		The actual message data.
	 */
	virtual void handleMessage( const Address & source,
			UnpackedMessageHeader & header,
			BinaryIStream & data ) = 0;

	/**
	 * 	This method reports the size of the message for this ID, but only if
	 * 	they are of CALLBACK_LENGTH_MESSAGE length style. Subclasses should
	 * 	only need to override this method if they are expecting to handle
	 * 	messages with that particular length style. 
	 *
	 *	@param networkInterface		The network interface this message was read
	 *								from.
	 *	@param srcAddr				The source address of the message.
	 * 	@param msgID				The message type being processed.
	 *
	 *	@return						Fixed-length sized messages are indicated
	 *								by the return of a non-negative lengths.
	 *								For variable-length sized messages, these
	 *								are indicated by a negative number, which
	 *								is the negative of the preferred number of
	 *								bytes used in the nominal case.
	 *								INVALID_STREAM_SIZE can be returned to
	 *								indicate that the bundle is invalid and
	 *								should not be processed further.
	 */
	virtual int getMessageStreamSize( 
			Mercury::NetworkInterface & networkInterface,
			const Mercury::Address & srcAddr,
			Mercury::MessageID msgID ) const
	{
		MF_ASSERT( 0 );
		return -1;
	}

	/**
	 * Sets whether messages being delivered to handleMessage() are being
	 * delivered out of order (early)
	 * @newValue value to be set
	 */
	virtual void processingEarlyMessageNow( bool newValue )
	{
		isEarly_ = newValue;
	}
	
	/**
	 * Returns whether messages being delivered to handleMessage() are being
	 * delivered out of order (early)
	 */
	virtual bool processingEarlyMessageNow() const
	{
		return isEarly_;
	}
private:
	bool isEarly_;
};


/**
 *	This class declares an interface for receiving reply messages.
 *	When a client issues a request, an interface of this type should
 *	be provided to handle the reply.
 *
 *	@see Bundle::startRequest
 *	@see Bundle::startReply
 *
 *	@ingroup mercury
 */
class ReplyMessageHandler
{
public:
	virtual ~ReplyMessageHandler() {};

	/**
	 * 	This method is called by Mercury to deliver a reply message.
	 *
	 * 	@param source	The address at which the message originated.
	 * 	@param header	This contains the message type, size, and flags.
	 * 	@param data		The actual message data.
	 * 	@param arg		This is user-defined data that was passed in with
	 * 					the request that generated this reply.
	 */
	virtual void handleMessage( const Address & source,
			UnpackedMessageHeader & header,
			BinaryIStream & data,
			void * arg ) = 0;

	/**
	 * 	This method is called by Mercury when the request fails. The
	 * 	normal reason for this happening is a timeout.
	 *
	 * 	@param exception	The reason for failure.
	 * 	@param arg			The user-defined data associated with the request.
	 */
	virtual void handleException( const NubException & exception,
			void * arg ) = 0;

	virtual void handleShuttingDown( const NubException & /* exception */,
			void * /* arg */ )
	{
		WARNING_MSG( "ReplyMessageHandler::handleShuttingDown: "
				"Not handled. Possible memory leak.\n" );
	}
};


/**
 *	This class implements ReplyMessageHandler and can be used by handlers with
 *	a handleException method that is safe to call while shutting down.
 */
class ShutdownSafeReplyMessageHandler : public ReplyMessageHandler
{
public:
	virtual void handleShuttingDown( const NubException & exception,
			void * arg )
	{
		this->handleException( exception, arg );
	}
};


/**
 * 	This class defines an interface for receiving socket events.
 *
 * 	Since Mercury runs the event loop, it is useful to be able to register
 * 	additional file descriptors, and receive callbacks when they are ready for
 * 	IO.
 *
 * 	@see EventDispatcher::registerFileDescriptor
 * 	@see EventDispatcher::registerWriteFileDescriptor
 *
 * 	@ingroup mercury
 */
class InputNotificationHandler
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~InputNotificationHandler() {}

	/**
	 *	This method is called when a file descriptor is ready for reading (or
	 *	writing, if registered using registerWriteFileDescriptor()).
	 *
	 *	@param fd	The file descriptor
	 *
	 * 	@return The return value is ignored. Implementors should return 0.
	 */
	virtual int handleInputNotification( int fd ) = 0;

	/**
	 *	This method is called when an error condition has been detected on a
	 *	file descriptor. Implementors should clear the error state by either
	 *	reading from the socket or by reading off the per-socket error queue,
	 *	if one is enabled, and returning true.
	 *
	 *	Returning false indicates that the error condition was not handled, and
	 *	handleInputNotification() will be called to handle the error after
	 *	reading from the socket. This is the default behaviour if this method
	 *	is not overridden.
	 *
	 *	@param fd	The file descriptor.
	 *
	 *	@return 	Whether the error condition was handled and cleared.
	 */
	virtual bool handleErrorNotification( int fd )
	{
		return false;
	}


	/**
	 *	This method is called when the event dispatcher is shutting down
	 *	in order to determine if there is more processing required for this
	 *	input notification handler.
	 *
	 *	@return True if more processing require, false otherwise
	 */
	 virtual bool isReadyForShutdown() const
	 {
		return true;
	 }
};


/**
 *	This class declares an interface for receiving notifications when a bundle
 *	is processed.
 *
 *	@see InterfaceTable::pBundleEventHandler
 *
 *	@ingroup mercury
 */
class BundleEventHandler
{
public:
	virtual ~BundleEventHandler() {}


	/**
	 * 	This method is called when the bundle has started processing, before
	 * 	any of its messages are delivered.
	 */
	virtual void onBundleStarted( Channel * /* pChannel */ ) {}


	/**
	 * 	This method is called after all messages in a bundle have
	 * 	been delivered.
	 */
	virtual void onBundleFinished( Channel * /* pChannel */ ) {}
};


/**
 *  This class defines an interface for objects used for priming bundles on
 *  channels with data.  It is used by ServerConnection and Proxy to write the
 *  'authenticate' message to the start of each bundle.
 *
 *  @see Channel::bundlePrimer
 */
class BundlePrimer
{
public:
	virtual ~BundlePrimer() {}

	/**
	 *  This method is called by the channel just after the bundle is cleared.
	 */
	virtual void primeBundle( Bundle & bundle ) = 0;

	/**
	 *  This method should return the number of non RELIABLE_DRIVER messages
	 *  that the primer writes to the bundle.
	 */
	virtual int numUnreliableMessages() const = 0;
};


} // namespace Mercury

BW_END_NAMESPACE

#endif // NETWORK_INTERFACES_HPP
