#ifndef ENTITY_MESSAGE_HANDLER_HPP
#define ENTITY_MESSAGE_HANDLER_HPP

#include "cellapp_interface.hpp"


BW_BEGIN_NAMESPACE

class Entity;

/**
 *  This class is a base class for entity message handler types.
 */
class EntityMessageHandler : public Mercury::InputMessageHandler
{
public:
	void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
	   	EntityID entityID );

protected:
	EntityMessageHandler( EntityReality reality,
				bool shouldBufferIfTickPending = false );

	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	/**
	 *  This method actually causes the message to be fed through to the
	 *  handler, after it has passed the various checks imposed in
	 *  handleMessage().
	 */
	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity ) = 0;

	virtual void sendFailure( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			EntityID entityID );

	EntityReality reality_;
	bool shouldBufferIfTickPending_;
};

BW_END_NAMESPACE

#endif // ENTITY_MESSAGE_HANDLER_HPP
