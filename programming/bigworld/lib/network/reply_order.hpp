#ifndef REPLY_ORDER_HPP
#define REPLY_ORDER_HPP

#include "misc.hpp"

#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

/**
 * 	@internal
 * 	This class represents a request that requires a reply.
 * 	It is used internally by Mercury.
 */
class ReplyOrder
{
public:
	/// The user reply handler.
	ReplyMessageHandler * handler;

	/// User argument passed to the handler.
	void * arg;

	/// Timeout in microseconds.
	int microseconds;

	/// Pointer to the reply ID for this request, written in 
	/// NetworkInterface::send().
	void * pReplyID;
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // REPLY_ORDER_HPP
