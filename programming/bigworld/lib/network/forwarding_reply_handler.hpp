#ifndef FORWARDING_REPLY_HANDLER_HPP
#define FORWARDING_REPLY_HANDLER_HPP

#include "basictypes.hpp"
#include "interfaces.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;

namespace Mercury
{
class UDPBundle;
class NetworkInterface;
}


/**
 *	This class is used to handle reply messages and forward it on.
 */
class ForwardingReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	ForwardingReplyHandler( const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID,
			Mercury::NetworkInterface & networkInterface );

protected:
	void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg );

	void handleException( const Mercury::NubException & ne, void * arg );

	virtual void prependData( BinaryIStream & inStream,
			BinaryOStream & outStream ) {};
	virtual void appendErrorData( BinaryOStream & stream ) {};

private:
	Mercury::Address 	srcAddr_;
	Mercury::ReplyID	replyID_;
	Mercury::NetworkInterface & interface_;
};

BW_END_NAMESPACE

#endif // FORWARDING_REPLY_HANDLER_HPP
