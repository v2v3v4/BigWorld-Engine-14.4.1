#ifndef NETWORK_MISC_HPP
#define NETWORK_MISC_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/timer_handler.hpp"

BW_BEGIN_NAMESPACE

class BinaryIStream;

namespace Mercury
{

class Address;
class Channel;
class InterfaceElement;
class NubException;
class ReplyMessageHandler;
class UnpackedMessageHeader;


/**
 *  The type of sequence numbers that are used to order and identify packets.
 */
typedef uint32 SeqNum;
const SeqNum SEQ_SIZE = 0x10000000U;
const SeqNum SEQ_MASK = SEQ_SIZE-1;
const SeqNum SEQ_NULL = SEQ_SIZE;


/**
 * 	This function masks the given integer with the sequence mask.
 */
inline SeqNum seqMask( SeqNum x )
{
	return x & SEQ_MASK;
}


/**
 *	This function returns whether or not the first argument is "before" the
 *	second argument.
 */
inline bool seqLessThan( SeqNum a, SeqNum b )
{
	return seqMask( a - b ) > SEQ_SIZE/2;
}


/**
 *	This class is responsible for doling out sequence numbers for packets.
 */
class SeqNumAllocator
{
public:

	/**
	 *	Constructor.
	 *
	 *	@param  firstSeqNum		The first sequence number to issue.
	 */
	SeqNumAllocator( SeqNum firstSeqNum ) : nextNum_( firstSeqNum )
	{
	}

	/**
	 *	This method returns the next sequence number.
	 */
	SeqNum getNext()
	{
		SeqNum retVal = nextNum_;
		nextNum_ = seqMask( nextNum_ + 1 );

		return retVal;
	}

	/**
	 *	This method returns the next sequence number that will be issued.
	 */
	operator SeqNum() const	{ return nextNum_; }

private:
	SeqNum nextNum_;
};


/**
 *	The type of the message identifiers on Packets.
 */
typedef uint8 MessageID;


/**
 *  The type of indexed channel identifiers.
 */
typedef int32 ChannelID;
const ChannelID CHANNEL_ID_NULL = 0;


/**
 *  The type of channel versions.  We re-use the SeqNum type here to take
 *  advantage of the wrapping logic already implemented in seqLessThan() etc.
 */
typedef SeqNum ChannelVersion;


/**
 *  The type of reply IDs used for requests.
 */
typedef int32 ReplyID;
const ReplyID REPLY_ID_NONE = -1;
const ReplyID REPLY_ID_MAX = 1000000;

/**
 *	@internal
 *	This is the default once-off resend period in microseconds.
 *
 *	@see Mercury::OnceOffResender::onceOffResendPeriod
 */
const int DEFAULT_ONCEOFF_RESEND_PERIOD = 200 * 1000; /* 200 ms */

/**
 *	@internal
 *	This is the default maximum resends for each once-off reliablility send.
 *
 *	@see Mercury::OnceOffResender::onceOffMaxResends
 */
const int DEFAULT_ONCEOFF_MAX_RESENDS = 50;


/**
 *	This enumeration contains reasons for Mercury errors,
 *	exceptions, and return codes.
 *
 *	@ingroup mercury
 */
enum Reason
{
	REASON_SUCCESS = 0,				 ///< No reason.
	REASON_TIMER_EXPIRED = -1,		 ///< Timer expired.
	REASON_NO_SUCH_PORT = -2,		 ///< Destination port is not open.
	REASON_GENERAL_NETWORK = -3,	 ///< The network is stuffed.
	REASON_CORRUPTED_PACKET = -4,	 ///< Got a bad packet.
	REASON_NONEXISTENT_ENTRY = -5,	 ///< Wanted to call a null function.
	REASON_WINDOW_OVERFLOW = -6,	 ///< Channel send window overflowed.
	REASON_INACTIVITY = -7,			 ///< Channel inactivity timeout.
	REASON_RESOURCE_UNAVAILABLE = -8,///< Corresponds to EAGAIN
	REASON_CLIENT_DISCONNECTED = -9, ///< Client disconnected voluntarily.
	REASON_TRANSMIT_QUEUE_FULL = -10,///< Corresponds to ENOBUFS
	REASON_CHANNEL_LOST = -11,		 ///< Corresponds to channel lost
	REASON_SHUTTING_DOWN = -12,		 ///< Corresponds to shutting down app.
	REASON_MESSAGE_TOO_LONG = -13 	 ///< Corresponds to EMSGSIZE.
};


/**
 *	This function returns the string representation of a Mercury::Reason.
 */
inline
const char * reasonToString( Reason reason )
{
	const char * reasons[] =
	{
		"REASON_SUCCESS",
		"REASON_TIMER_EXPIRED",
		"REASON_NO_SUCH_PORT",
		"REASON_GENERAL_NETWORK",
		"REASON_CORRUPTED_PACKET",
		"REASON_NONEXISTENT_ENTRY",
		"REASON_WINDOW_OVERFLOW",
		"REASON_INACTIVITY",
		"REASON_RESOURCE_UNAVAILABLE",
		"REASON_CLIENT_DISCONNECTED",
		"REASON_TRANSMIT_QUEUE_FULL",
		"REASON_CHANNEL_LOST",
		"REASON_SHUTTING_DOWN",
		"REASON_MESSAGE_TOO_LONG"
	};

	unsigned int index = -reason;

	if (index < sizeof(reasons)/sizeof(reasons[0]))
	{
		return reasons[index];
	}

	return "REASON_UNKNOWN";
}

} // namespace Mercury

BW_END_NAMESPACE

#endif // NETWORK_MISC_HPP
