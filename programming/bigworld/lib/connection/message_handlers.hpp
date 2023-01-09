#ifndef MESSAGE_HANDLERS_HPP
#define MESSAGE_HANDLERS_HPP

#include "server_connection.hpp"

#include "network/interfaces.hpp"
#include "network/network_interface.hpp"
#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: Various Mercury InputMessageHandler implementations
// -----------------------------------------------------------------------------


class BinaryIStream;

namespace Mercury
{
	class Address;
} // namespace Mercury


/**
 * 	@internal
 *	Objects of this type are used to handle messages destined for the client
 *	application.
 */
template <class ARGS>
class ClientMessageHandler : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single message.
	typedef void (ServerConnection::*Handler)( const ARGS & args );

	/// This is the constructor
	ClientMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
#if !defined( _BIG_ENDIAN ) && !defined( BW_ENFORCE_ALIGNED_ACCESS )
		ARGS & args = *(ARGS*)data.retrieve( sizeof(ARGS) );
#else
		// Poor old big-endian clients can't just refer directly to data
		// on the network stream, they have to stream it off.
		// NOTE: Armv7 processors also have this issue
		ARGS args;
		data >> args;
#endif

		ServerConnection * pServConn =
			(ServerConnection *)header.pInterface->pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientMessageHandler::handleMessage: "
					"Dropping packet from unexpected source: %s\n",
				srcAddr.c_str() );

			*(header.pBreakLoop) = true;

			data.finish();
			return;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		(pServConn->*handler_)( args );
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle messages destined for the client
 *	application.
 */
template <>
class ClientMessageHandler< void > : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single message.
	typedef void (ServerConnection::*Handler)();

	/// This is the constructor
	ClientMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ServerConnection * pServConn =
			(ServerConnection *)header.pInterface->pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientMessageHandler< void >::handleMessage: "
					"Dropping packet from unexpected source: %s\n",
				srcAddr.c_str() );

			*(header.pBreakLoop) = true;

			data.finish();
			return;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		(pServConn->*handler_)();
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle messages destined for the client
 *	application. The handler is also passed the source address and the message
 *	header.
 */
template <class ARGS>
class ClientMessageHandlerEx : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single message.
	typedef void (ServerConnection::*Handler)(
		const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		const ARGS & args );

	/// This is the constructor
	ClientMessageHandlerEx( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
#if !defined( _BIG_ENDIAN ) && !defined( _ARM_ARCH_7 )
		ARGS & args = *(ARGS*)data.retrieve( sizeof(ARGS) );
#else
		// Poor old big-endian clients can't just refer directly to data
		// on the network stream, they have to stream it off.
		// NOTE: Armv7 processors also have this issue
		ARGS args;
		data >> args;
#endif

		ServerConnection * pServConn =
			(ServerConnection *)header.pInterface->pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientMessageHandlerEx::handleMessage: "
					"Dropping packet from unexpected source: %s\n",
				srcAddr.c_str() );

			*(header.pBreakLoop) = true;

			data.finish();
			return;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		(pServConn->*handler_)( srcAddr, header, args );
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle variable length messages destined
 *	for the client application.
 */
class ClientVarLenMessageHandler : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single variable length message.
	typedef void (ServerConnection::*Handler)( BinaryIStream & stream );

	/// This is the constructor.
	ClientVarLenMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ServerConnection * pServConn =
			(ServerConnection *)header.pInterface->pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientVarLenMessageHandler::handleMessage: "
					"Dropping packet from unexpected source: %s\n",
				srcAddr.c_str() );

			*(header.pBreakLoop) = true;

			data.finish();
			return;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		(pServConn->*handler_)( data );
	}

	Handler handler_;
};


/**
 * 	@internal
 *	Objects of this type are used to handle variable length messages destined
 *	for the client application. The handler is also passed the source address
 *	and the message header.
 */
class ClientVarLenMessageHandlerEx : public Mercury::InputMessageHandler
{
public:
	/// This typedef declares a member function on the ServerConnection
	/// class that handles a single variable length message.
	typedef void (ServerConnection::*Handler)(
			const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

	/// This is the constructor.
	ClientVarLenMessageHandlerEx( Handler handler ) :
		handler_( handler ) {}

private:
	/**
	 * 	This method handles a Mercury message, and dispatches it to
	 * 	the correct member function of the ServerConnection object.
	 */
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ServerConnection * pServConn =
			(ServerConnection *)header.pInterface->pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientVarLenMessageHandlerEx::handleMessage: "
					"Dropping packet from unexpected source: %s\n",
				srcAddr.c_str() );

			*(header.pBreakLoop) = true;

			data.finish();
			return;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		(pServConn->*handler_)( srcAddr, header, data );
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: ClientCallbackMessageHandler declaration
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle handleDataFromServer message from the server.
 */
class ClientCallbackMessageHandler : public ClientVarLenMessageHandlerEx
{
public:
	typedef int (ServerConnection::*StreamSizeHandler)(
			Mercury::MessageID msgID ) const;

	ClientCallbackMessageHandler(
			std::pair< Handler, StreamSizeHandler > args ) :
		ClientVarLenMessageHandlerEx( args.first ),
		streamSizeHandler_( args.second )
	{}

protected:

	// Override from ClientVarLenMessageHandlerEx / Mercury::InputMessageHandler
	virtual int getMessageStreamSize(
			Mercury::NetworkInterface & networkInterface,
			const Mercury::Address & srcAddr,
			Mercury::MessageID msgID ) const
	{
		ServerConnection * pServConn =
			(ServerConnection *)networkInterface.pExtensionData();

		if (srcAddr != pServConn->addr())
		{
			ERROR_MSG( "ClientCallbackMessageHandler::getMessageStreamSize: "
					"Source address (%s) does not match expected server "
					"address (%s)\n",
				srcAddr.c_str(), pServConn->addr().c_str() );

			return Mercury::INVALID_STREAM_SIZE;
		}

		pServConn->processingEarlyMessageNow(
			this->processingEarlyMessageNow() );
		return (pServConn->*streamSizeHandler_)( msgID );
	}

	StreamSizeHandler streamSizeHandler_;
};


BW_END_NAMESPACE


#endif // MESSAGE_HANDLERS_HPP

