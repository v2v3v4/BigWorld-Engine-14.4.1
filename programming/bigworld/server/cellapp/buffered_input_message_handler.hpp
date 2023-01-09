#ifndef BUFFERED_INPUT_MESSAGE_HANDLER_HPP
#define BUFFERED_INPUT_MESSAGE_HANDLER_HPP

#include "buffered_input_messages.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class is a template child to make InputMessageHandler types
 *  autobuffering if the current tick has expired.
 */

// MH shoud be a MessageHandler as per network/common_messgage_handlers.hpp
// with a constructor of MESSAGE_HANDLER( MESSAGE_HANDLER::Handler )
// Under C++11, the constructor type doesn't matter, as we can inherit
// MESSAGE_HANDLER's constructor with 'using', which would allow this to
// wrap _any_ (duck-typed) Mercury::InputMessageHandler

// In the mean-time, specialise this with a new constructor if you need
// to wrap a different type of handler. The handleMessage() method only needs
// MESSAGE_HANDLER to have public or protected MESSAGE_HANDLER::handleMessage()
template< class MESSAGE_HANDLER >
class BufferedInputMessageHandler : public MESSAGE_HANDLER
{
public:
	typedef typename MESSAGE_HANDLER::Handler Handler;

	BufferedInputMessageHandler( Handler handler ) :
		MESSAGE_HANDLER( handler ) {}

	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );
};

BW_END_NAMESPACE

#endif // BUFFERED_INPUT_MESSAGE_HANDLER_HPP
