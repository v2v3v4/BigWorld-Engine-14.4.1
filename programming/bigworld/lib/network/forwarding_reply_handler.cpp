#include "pch.hpp"

#include "forwarding_reply_handler.hpp"

#include "channel_sender.hpp"
#include "network_interface.hpp"
#include "nub_exception.hpp"
#include "bundle.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ForwardingReplyHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ForwardingReplyHandler::ForwardingReplyHandler(
			const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID,
			Mercury::NetworkInterface & networkInterface ) :
		srcAddr_( srcAddr ),
		replyID_( replyID ),
		interface_( networkInterface )
{
}


/**
 *	This method handles the reply message.
 */
void ForwardingReplyHandler::handleMessage( const Mercury::Address& /*srcAddr*/,
								Mercury::UnpackedMessageHeader & header,
								BinaryIStream & data, void * /*arg*/)
{
	Mercury::ChannelSender sender( interface_.findOrCreateChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( replyID_ );

	// For classes that derive.
	this->prependData( data, bundle );

	bundle.transfer( data, data.remainingLength() );

	delete this;
}


/**
 *	This method handles an exception to the request.
 */
void ForwardingReplyHandler::handleException( const Mercury::NubException & ne,
		void* /*arg*/ )
{
	WARNING_MSG( "ForwardingReplyHandler::handleException: reason %s\n",
			Mercury::reasonToString( ne.reason() ) );
	Mercury::ChannelSender sender( interface_.findOrCreateChannel( srcAddr_ ) );
	Mercury::Bundle & bundle = sender.bundle();

	bundle.startReply( replyID_ );

	this->appendErrorData( bundle );

	delete this;
}

BW_END_NAMESPACE

// forwarding_reply_handler.cpp
