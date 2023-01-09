#include "script/first_include.hpp"

#include "buffered_entity_messages.hpp"
#include "cellapp.hpp"
#include "entity_message_handler.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BufferedEntityMessage
// -----------------------------------------------------------------------------

/**
 *	This class is used to buffer messages if the current tick is taking too
 *	long.
 */
class BufferedEntityMessage
{
public:
	BufferedEntityMessage( EntityMessageHandler & handler,
			const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			EntityID entityID ) :
		handler_( handler ),
		srcAddr_( srcAddr ),
		header_( header ),
		entityID_( entityID )
	{
		data_.transfer( data, data.remainingLength() );
	}

	void play()
	{
		handler_.handleMessage( srcAddr_, header_, data_, entityID_ );
	}

private:
	EntityMessageHandler & handler_;

	Mercury::Address srcAddr_;
	Mercury::UnpackedMessageHeader header_;
	MemoryOStream data_;
	EntityID entityID_;
};


// -----------------------------------------------------------------------------
// Section: BufferedEntityMessages
// -----------------------------------------------------------------------------

/**
 *	This method plays back any buffered messages.
 */
void BufferedEntityMessages::playBufferedMessages( CellApp & app )
{
	const size_t startingBufferSize = bufferedMessages_.size();

	while (!bufferedMessages_.empty() && !app.nextTickPending())
	{
		BufferedEntityMessage * pMsg = bufferedMessages_.front();
		bufferedMessages_.pop_front();
		pMsg->play();
		delete pMsg;
	}

	const size_t endingBufferSize = bufferedMessages_.size();

	if (startingBufferSize != endingBufferSize)
	{
		WARNING_MSG( "BufferedEntityMessages::playBufferedMessages: "
				"There %s buffered messages. Played %zd of %zd\n",
			bufferedMessages_.empty() ? "were" : "are",
			startingBufferSize - endingBufferSize,
			startingBufferSize );
	} 
}


/**
 *	This adds a message to be handled later.
 */
void BufferedEntityMessages::add( EntityMessageHandler & handler,
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, EntityID entityID )
{
	bufferedMessages_.push_back(
		new BufferedEntityMessage( handler, srcAddr, header, data, entityID ) );
}

BW_END_NAMESPACE

// buffered_entity_messages.cpp
