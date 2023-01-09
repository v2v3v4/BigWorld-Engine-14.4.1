#ifndef BUFFERED_GHOST_MESSAGE_FACTORY_HPP
#define BUFFERED_GHOST_MESSAGE_FACTORY_HPP

#include "network/basictypes.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BufferedGhostMessage;

namespace Mercury
{
class Address;
class InputMessageHandler;
class UnpackedMessageHeader;
}

namespace CellAppInterface
{
	struct ghostSetRealArgs;
}

namespace BufferedGhostMessageFactory
{

BufferedGhostMessage * createBufferedMessage(
		const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		EntityID entityID,
		Mercury::InputMessageHandler * pHandler );

BufferedGhostMessage * createBufferedCreateGhostMessage(
		const Mercury::Address & srcAddr, EntityID entityID,
		SpaceID spaceID, BinaryIStream & data );

BufferedGhostMessage * createGhostSetRealMessage(
		EntityID entityID, const CellAppInterface::ghostSetRealArgs & args );

} // namespace BufferedGhostMessageFactory

BW_END_NAMESPACE

#endif // BUFFERED_GHOST_MESSAGE_FACTORY_HPP
