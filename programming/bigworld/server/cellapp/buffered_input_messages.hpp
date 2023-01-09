#include "Python.h"

#ifndef BUFFERED_INPUT_MESSAGES_HPP
#define BUFFERED_INPUT_MESSAGES_HPP

#include "network/basictypes.hpp"
#include "network/interfaces.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BufferedInputMessage;
class CellApp;

namespace Mercury
{
class Address;
class UnpackedMessageHeader;
}


/**
 *	Objects of this type are used to handle variable length messages destined
 *	for a cell. They simply pass all the mercury parameters to the handler
 *	function.
 */
class BufferedInputMessages
{
public:
	void playBufferedMessages( CellApp & app );

	void add( Mercury::InputMessageHandler & handler,
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	bool isEmpty() const
		{ return bufferedMessages_.empty(); }

	size_t size() const	{ return bufferedMessages_.size(); }

private:
	BW::list< BufferedInputMessage * > bufferedMessages_;
};

BW_END_NAMESPACE

#endif // BUFFERED_INPUT_MESSAGES_HPP
