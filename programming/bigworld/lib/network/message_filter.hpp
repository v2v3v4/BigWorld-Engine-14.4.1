#ifndef __MESSAGE_FILTER_HPP__
#define __MESSAGE_FILTER_HPP__

#include "network/basictypes.hpp"
#include "cstdmf/binary_stream.hpp"
#include "network/interfaces.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{
/**
 *	Abstract class for a message filter.
 *
 *	A message filter has first-pass handling of messages before their
 *	destination message handlers. They are associated with channels.
 *
 *	They may or may not decide to pass, alter the message to the handler. One
 *	example of use is to implement rate-limiting of message dispatches, where
 *	message handling can be deferred.
 *
 *	Message filters are reference-counted.
 */
class MessageFilter: public ReferenceCount
{
public:

	/** Constructor. */
	MessageFilter(): ReferenceCount()
	{}

	/** Destructor. */
	virtual ~MessageFilter() {}

	/**
	 *	Override this method to implement the message filter.
	 *
	 *	@param srcAddr 		The source address of the message.
	 *	@param header 		The message header.
	 *	@param data 		The message data stream.
	 *	@param pHandler 	The destination message handler.
	 */
	virtual void filterMessage( const Address & srcAddr,
		UnpackedMessageHeader & header,
		BinaryIStream & data,
		Mercury::InputMessageHandler * pHandler ) = 0;

};


typedef SmartPointer< MessageFilter > MessageFilterPtr;

} // namespace Mercury

BW_END_NAMESPACE

#endif // __MESSAGE_FILTER_HPP__
