#ifndef COMMON_MESSAGE_HANDLERS_HPP
#define COMMON_MESSAGE_HANDLERS_HPP

BW_BEGIN_NAMESPACE

template <class OBJECT_TYPE>
class MessageHandlerFinder
{
public:
	// This should be specialised for non-ServerApp types
	static OBJECT_TYPE * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream )
	{
		return &ServerApp::getApp< OBJECT_TYPE >( header );
	}
};

// -----------------------------------------------------------------------------
// Section: Message Handler implementation
// -----------------------------------------------------------------------------

/**
 *	Objects of this type are used to handle empty messages.
 */
template <class OBJECT_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class EmptyMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)();

		/**
		 *	Constructor.
		 */
		EmptyMessageHandler( Handler handler ) : handler_( handler ) {}

		// Override from Mercury::InputMessageHandler
		void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data ) /* override */
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				(pObject ->*handler_)();
			}
			else
			{
				ERROR_MSG( "EmptyMessageHandler::handleMessage: "
					"Could not find object\n" );
			}
		}

		Handler handler_;
};


/**
 *	Objects of this type are used to handle empty messages. This version
 *	supplies the source address and the header to the handler.
 */
template <class OBJECT_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class EmptyMessageHandlerEx : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)( const Mercury::Address &,
			const Mercury::UnpackedMessageHeader & );

		/**
		 *	Constructor.
		 */
		EmptyMessageHandlerEx( Handler handler ) : handler_( handler ) {}

		/* Override from Mercury::InputMessageHandler. */
		void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data ) /* override */
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				(pObject ->*handler_)( srcAddr, header );
			}
			else
			{
				ERROR_MSG( "EmptyMessageHandlerEx::handleMessage: "
					"Could not find object\n" );
			}
		}

		Handler handler_;
};


/**
 *	Objects of this type are used to handle structured messages.
 */
template <class OBJECT_TYPE, class ARGS_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class StructMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)( const ARGS_TYPE & args );

		/**
		 *	Constructor.
		 */
		StructMessageHandler( Handler handler ) : handler_( handler ) {}

		// Override
		void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data ) /* override */
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				ARGS_TYPE args;
				data >> args;

				(pObject->*handler_)( args );
			}
			else
			{
				ERROR_MSG( "StructMessageHandler::handleMessage(%s): "
					"%s (id %d). Could not find object\n",
					srcAddr.c_str(), header.msgName(), header.identifier );

				data.finish();
			}
		}

		Handler handler_;
};


/**
 *	Objects of this type are used to handle structured messages. This version
 *	supplies the source address and header.
 */
template <class OBJECT_TYPE, class ARGS_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class StructMessageHandlerEx : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)(
					const Mercury::Address & srcAddr,
					const Mercury::UnpackedMessageHeader & header,
					const ARGS_TYPE & args );

		/**
		 *	Constructor.
		 */
		StructMessageHandlerEx( Handler handler ) : handler_( handler ) {}

		// Override
		void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data ) /* override */
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				ARGS_TYPE args;
				data >> args;

				(pObject->*handler_)( srcAddr, header, args );
			}
			else
			{
				ERROR_MSG( "StructMessageHandlerEx::handleMessage: "
					"Could not find object\n" );

				data.finish();
			}
		}

		Handler handler_;
};


/**
 *	Objects of this type are used to handle variable length messages.
 */
template <class OBJECT_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class StreamMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)( BinaryIStream & stream );

		/**
		 *	Constructor.
		 */
		StreamMessageHandler( Handler handler ) : handler_( handler ) {}

		// Override
		virtual void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data )
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				(pObject ->*handler_)( data );
			}
			else
			{
				ERROR_MSG( "StreamMessageHandler::handleMessage: "
					"Could not find object\n" );
			}
		}

		Handler handler_;
};


/**
 *	Objects of this type are used to handle variable length messages. This
 *	version supplies the source address and header.
 */
template <class OBJECT_TYPE,
		 class FIND_POLICY = MessageHandlerFinder< OBJECT_TYPE > >
class StreamMessageHandlerEx : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)(
			const Mercury::Address & addr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

		/**
		 *	Constructor.
		 */
		StreamMessageHandlerEx( Handler handler ) : handler_( handler ) {}

		// Override
		virtual void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data )
		{
			OBJECT_TYPE * pObject = FIND_POLICY::find( srcAddr, header, data );

			if (pObject != NULL)
			{
				(pObject->*handler_)( srcAddr, header, data );
			}
			else
			{
				// OK we give up then
				ERROR_MSG( "StreamMessageHandlerEx::handleMessage: "
						"Do not have object for message from %s\n",
					srcAddr.c_str() );
			}
		}

		Handler handler_;
};

BW_END_NAMESPACE

#endif // COMMON_MESSAGE_HANDLERS_HPP
