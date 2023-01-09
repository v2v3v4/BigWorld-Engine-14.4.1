#ifndef __RATE_LIMIT_MESSAGE_FILTER_HPP__
#define __RATE_LIMIT_MESSAGE_FILTER_HPP__

#include "network/message_filter.hpp"
#include "network/unpacked_message_header.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/timestamp.hpp"

#include "proxy.hpp"

#include <queue>


BW_BEGIN_NAMESPACE

class BufferedMessage;
class RateLimitConfig;


/**
 *	All messages from external clients get passed through an instance of this
 *	class. It is responsible for enforcing rate limits on client messages,
 *	buffering and playing back messages when limits are no longer exceeded.
 */
class RateLimitMessageFilter : public Mercury::MessageFilter
{
public:
	typedef RateLimitConfig Config;

public:

	RateLimitMessageFilter( const Mercury::Address & addr );

	~RateLimitMessageFilter();

public: // from Mercury::MessageFilter
	void filterMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		Mercury::InputMessageHandler * pHandler );

public:

	void tick();

	void setProxy( ProxyPtr pProxy );

private:

	void replayAny();

	bool canSendNow( uint dataLen );


	/**
	 *	Return the BufferedMessage at the front of the buffered message queue,
	 *	or NULL if the queue is empty.
	 */
	BufferedMessage * front()
	{ return queue_.empty() ? NULL : queue_.front(); }


	void pop_front();

	void dispatch( Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Mercury::InputMessageHandler * pHandler );

	void dispatch( BufferedMessage * pMessage );

	bool buffer( BufferedMessage * pMsg );

	void clear();

private:
	// Warn bit flag consts
	/**
	 *	Warn bit flat constant for when the number of received messages exceeds
	 *	the warning limit.
	 */
	static const uint8 WARN_MESSAGE_COUNT 		= 0x01;
	/**
	 *	Warn bit flat constant for when the size of received messages exceeds
	 *	the warning limit.
	 */
	static const uint8 WARN_MESSAGE_SIZE 		= 0x02;
	/**
	 *	Warn bit flat constant for when the number of buffered messages exceeds
	 *	the warning limit.
	 */
	static const uint8 WARN_MESSAGE_BUFFERED 	= 0x04;
	/**
	 *	Warn bit flat constant for when the size of buffered messages exceeds
	 *	the warning limit.
	 */
	static const uint8 WARN_BYTES_BUFFERED 		= 0x08;

	/**
	 *	Message queue data type.
	 */
	typedef std::queue< BufferedMessage * > MsgQueue;

	// The callback object.
	ProxyPtr 			pProxy_;

	// This is a bitflag indicating whether we have warned about a particular
	// condition or not, see FilterForAddress::WARN_* constants.
	uint8				warnFlags_;

	// The number of received messages coming into the filter since we were
	// last ticked.
	uint 				numReceivedSinceLastTick_;

	// The total size of received messages coming into the filter since we were
	// last ticked.
	uint 				receivedBytesSinceLastTick_;

	// The number of dispatches since we were last ticked.
	uint 				numDispatchedSinceLastTick_;

	// The total size of the dispatched messages since we were last ticked.
	uint 				dispatchedBytesSinceLastTick_;

	// The buffer message queue.
	MsgQueue			queue_;

	// This is always the sum of all the messages in queue_.
	uint				sumBufferedSizes_;

};

typedef SmartPointer< RateLimitMessageFilter > RateLimitMessageFilterPtr;

BW_END_NAMESPACE

#endif // __RATE_LIMIT_MESSAGE_FILTER_HPP__
