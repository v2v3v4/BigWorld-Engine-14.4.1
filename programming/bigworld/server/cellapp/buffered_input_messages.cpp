#include "script/first_include.hpp"

#include "buffered_input_messages.hpp"
#include "cellapp.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BufferedInputMessage
// -----------------------------------------------------------------------------

/**
 *	This class is used to buffer messages if the current tick is taking too
 *	long.
 */
class BufferedInputMessage
{
public:
	BufferedInputMessage( Mercury::InputMessageHandler & handler,
			const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data ) :
		handler_( handler ),
		srcAddr_( srcAddr ),
		header_( header )
	{
		data_.transfer( data, data.remainingLength() );
	}

	void play()
	{
		handler_.handleMessage( srcAddr_, header_, data_ );
	}

private:
	Mercury::InputMessageHandler & handler_;

	Mercury::Address srcAddr_;
	Mercury::UnpackedMessageHeader header_;
	MemoryOStream data_;
};


// -----------------------------------------------------------------------------
// Section: BufferedInputMessages
// -----------------------------------------------------------------------------

/**
 *	This method plays back any buffered messages.
 */
void BufferedInputMessages::playBufferedMessages( CellApp & app )
{
	const size_t startingBufferSize = bufferedMessages_.size();
	while (!bufferedMessages_.empty() &&
			!app.nextTickPending())
	{
		BufferedInputMessage * pMsg = bufferedMessages_.front();
		bufferedMessages_.pop_front();
		pMsg->play();
		delete pMsg;
	}

	if (startingBufferSize > 0)
	{
		const size_t endingBufferSize = bufferedMessages_.size();
		const size_t playedCount = startingBufferSize - endingBufferSize ;
		if (endingBufferSize > 0)
		{
			WARNING_MSG( "BufferedInputMessages::playBufferedMessages: Played "
				"%lu message%s from buffer, buffer still contains "
				"%lu message%s\n",
				playedCount, playedCount == 1 ? "":"s",
				endingBufferSize, endingBufferSize == 1 ? "":"s" );
		}
		else
		{
			INFO_MSG( "BufferedInputMessages::playBufferedMessages: Played "
				"%lu message%s, buffer now empty\n",
				playedCount, playedCount == 1 ? "":"s" );
		}
	}
}


/**
 *	This adds a message to be handled later.
 */
void BufferedInputMessages::add( Mercury::InputMessageHandler & handler,
		const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	bufferedMessages_.push_back(
		new BufferedInputMessage( handler, srcAddr, header, data ) );
}

BW_END_NAMESPACE

// buffered_input_messages.cpp
