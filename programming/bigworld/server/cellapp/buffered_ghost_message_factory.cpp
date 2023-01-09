#include "script/first_include.hpp"

#include "buffered_ghost_message_factory.hpp"
#include "buffered_ghost_message.hpp"
#include "buffered_ghost_messages.hpp"

#include "cellapp.hpp"
#include "entity.hpp"
#include "space.hpp"

#include "cellapp_interface.hpp"

#include "cstdmf/memory_stream.hpp"

#include "network/interfaces.hpp"
#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CommonBufferedGhostMessage
// -----------------------------------------------------------------------------

/**
 *	This class is used to store a real->ghost message that has arrived too
 *	early.  This could mean the ghost doesn't exist yet, or that it is a
 *	message that has been reordered as a side-effect of offloading (since we
 *	cannot strictly guarantee the ordering of messages from two channels).
 */
class CommonBufferedGhostMessage : public BufferedGhostMessage
{
public:
	CommonBufferedGhostMessage( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		EntityID entityID,
		Mercury::InputMessageHandler * pHandler,
		bool isSubsequenceStart,
		bool isSubsequenceEnd );

	virtual void play();

private:
	Mercury::MessageID msgID() const { return header_.identifier; }

	Mercury::Address srcAddr_;
	Mercury::UnpackedMessageHeader header_;
	MemoryOStream data_;
	Mercury::InputMessageHandler * pHandler_;
};

/**
 *	Constructor.
 */
CommonBufferedGhostMessage::CommonBufferedGhostMessage(
		const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		EntityID entityID,
		Mercury::InputMessageHandler * pHandler,
		bool isSubsequenceStart, bool isSubsequenceEnd ) :
	BufferedGhostMessage( isSubsequenceStart, isSubsequenceEnd ),
	srcAddr_( srcAddr ),
	header_( header ),
	data_( sizeof( EntityID ) + data.remainingLength() ),
	pHandler_( pHandler )
{
	data_ << entityID;
	data_.transfer( data, data.remainingLength() );
}


/**
 *	This method plays this buffered message.
 */
void CommonBufferedGhostMessage::play()
{
	DEBUG_MSG( "CommonBufferedGhostMessage::play: "
		"Replaying buffered message '%s' from %s\n",
		header_.msgName(), srcAddr_.c_str() );

	pHandler_->handleMessage( srcAddr_, header_, data_ );
}


// -----------------------------------------------------------------------------
// Section: BufferedLastGhostMessage
// -----------------------------------------------------------------------------

class BufferedGhostSetRealMessage : public BufferedGhostMessage
{
public:
	BufferedGhostSetRealMessage( EntityID entityID,
			const CellAppInterface::ghostSetRealArgs & args ) :
		BufferedGhostMessage( /*isSubseqStart:*/true, /*isSubseqEnd:*/false ),
		entityID_( entityID ),
		args_( args )
	{
	}

	virtual void play()
	{
		Entity * pEntity = CellApp::instance().findEntity( entityID_ );

		MF_ASSERT( pEntity );

		pEntity->ghostSetReal( args_ );
	}

private:
	EntityID entityID_;
	CellAppInterface::ghostSetRealArgs args_;
};


class BufferedCreateGhostMessage : public BufferedGhostMessage
{
public:
	BufferedCreateGhostMessage( const Mercury::Address & srcAddr,
			EntityID entityID, SpaceID spaceID,
			BinaryIStream & data ) :
		BufferedGhostMessage( /*isSubseqStart:*/true, /*isSubseqEnd:*/false ),
		srcAddr_( srcAddr ),
		entityID_( entityID ),
		spaceID_( spaceID ),
		data_()
	{
		data_.transfer( data, data.remainingLength() );
	}

	virtual void play()
	{
		CellApp & app = CellApp::instance();
		Entity * pEntity = app.findEntity( entityID_ );

		if (pEntity)
		{
			WARNING_MSG( "BufferedCreateGhostMessage::play: "
					"Entity %u already exists\n", entityID_ );
			BufferedGhostMessage * pMsg =
				BufferedGhostMessageFactory::createBufferedCreateGhostMessage(
					srcAddr_, entityID_, spaceID_, data_ );
			app.bufferedGhostMessages().delaySubsequence(
				entityID_, srcAddr_, pMsg );
		}
		else
		{
			Space * pSpace = app.findSpace( spaceID_ );

			if (pSpace)
			{
				pSpace->createGhost( entityID_, data_ );
			}
			else
			{
				ERROR_MSG( "BufferedCreateGhostMessage::play: "
						"Could not find space %d\n", spaceID_ );
			}
		}
	}

	bool isCreateGhostMessage() const
	{
		return true;
	}


private:
	Mercury::Address srcAddr_;
	EntityID entityID_;
	SpaceID spaceID_;
	MemoryOStream data_;
};


// -----------------------------------------------------------------------------
// Section: Factory method
// -----------------------------------------------------------------------------

namespace BufferedGhostMessageFactory
{

BufferedGhostMessage * createBufferedMessage(
	const Mercury::Address & srcAddr,
	const Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	EntityID entityID,
	Mercury::InputMessageHandler * pHandler )
{
	Mercury::MessageID id = header.identifier;

	bool isSubsequenceStart = BufferedGhostMessages::isSubsequenceStart( id );
	bool isSubsequenceEnd = BufferedGhostMessages::isSubsequenceEnd( id );

	return new CommonBufferedGhostMessage(
			srcAddr, header, data, entityID, pHandler,
			isSubsequenceStart, isSubsequenceEnd );
}


BufferedGhostMessage * createBufferedCreateGhostMessage(
		const Mercury::Address & srcAddr,
		EntityID entityID, SpaceID spaceID, BinaryIStream & data )
{
	return new BufferedCreateGhostMessage( srcAddr, entityID, spaceID, data );
}


BufferedGhostMessage * createGhostSetRealMessage( EntityID entityID,
			const CellAppInterface::ghostSetRealArgs & args )
{
	return new BufferedGhostSetRealMessage( entityID, args );
}

} // namespace BufferedGhostMessageFactory

BW_END_NAMESPACE

// buffered_ghost_message.cpp
