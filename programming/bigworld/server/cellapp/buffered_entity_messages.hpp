#ifndef BUFFERED_ENTITY_MESSAGES_HPP
#define BUFFERED_ENTITY_MESSAGES_HPP

#include "network/basictypes.hpp"

#include "cstdmf/bw_list.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BufferedEntityMessage;
class CellApp;
class EntityMessageHandler;

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
class BufferedEntityMessages
{
public:
	void playBufferedMessages( CellApp & app );

	void add( EntityMessageHandler & handler,
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, EntityID entityID );

	bool isEmpty() const
		{ return bufferedMessages_.empty(); }

	size_t size() const	{ return bufferedMessages_.size(); }

private:
	BW::list< BufferedEntityMessage * > bufferedMessages_;
};

BW_END_NAMESPACE

#endif // BUFFERED_ENTITY_MESSAGES_HPP
