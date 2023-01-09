#include "script/first_include.hpp"

#include "buffered_entity_messages.hpp"
#include "buffered_ghost_message_factory.hpp"
#include "buffered_ghost_messages.hpp"
#include "buffered_input_messages.hpp"
#include "cellapp_interface.hpp"
#include "cell.hpp"
#include "cellapp.hpp"
#include "entity.hpp"
#include "buffered_input_message_handler.hpp"
#include "entity_message_handler.hpp"
#include "entity_population.hpp"
#include "real_entity.hpp"
#include "space.hpp"

#include "network/channel_sender.hpp"
#include "network/common_message_handlers.hpp"
#include "network/interfaces.hpp"
#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BufferedInputMessageHandler
// -----------------------------------------------------------------------------

/**
 *	This method handles this message. It is called from the InputMessageHandler
 *	override and also when replaying buffered messages.
 */
template< class MESSAGE_HANDLER >
void BufferedInputMessageHandler< MESSAGE_HANDLER >::handleMessage(
	const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	CellApp & app = ServerApp::getApp< CellApp >( header );

	if (app.nextTickPending())
	{
		static GameTime s_lastMessageTime = 0;
		BufferedInputMessages & bufferedInputMessages =
			app.bufferedInputMessages();

		if (s_lastMessageTime != app.time())
		{
			WARNING_MSG( "BufferedInputMessageHandler::handleMessage: No time "
					"left in tick (%u). Buffering '%s' and similar messages this "
					"tick. Num in buffer = %zu.\n",
				app.time(),
				header.msgName(),
				bufferedInputMessages.size() );

			s_lastMessageTime = app.time();
		}

		bufferedInputMessages.add( *this,
				srcAddr, header, data );
	}
	else
	{
		MESSAGE_HANDLER::handleMessage( srcAddr, header, data );
	}
}


// -----------------------------------------------------------------------------
// Section: EntityMessageHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param reality Whether the message is destined for a real entity, ghost
 *		entity or entity with a witness.
 *	@param shouldBufferIfTickPending If true, messages will be delayed if the
 *		next tick is pending.
 */
EntityMessageHandler::EntityMessageHandler( EntityReality reality,
			bool shouldBufferIfTickPending ) :
		reality_( reality ),
		shouldBufferIfTickPending_( shouldBufferIfTickPending )
{
}


/*
 *	Override from InputMessageHandler.
 */
void EntityMessageHandler::handleMessage( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data )
{
	EntityID entityID;
	data >> entityID;

	this->handleMessage( srcAddr, header, data, entityID );
}


/**
 *	This method handles this message. It is called from the InputMessageHandler
 *	override and from handling of buffered messages.
 */
void EntityMessageHandler::handleMessage( const Mercury::Address & srcAddr,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	EntityID entityID )
{
	CellApp & app = ServerApp::getApp< CellApp >( header );
	Entity * pEntity = app.findEntity( entityID );

	AUTO_SCOPED_ENTITY_PROFILE( pEntity );

	BufferedGhostMessages & bufferedMessages = app.bufferedGhostMessages();

	bool shouldBufferGhostMessage =
		!pEntity ||
		pEntity->shouldBufferMessagesFrom( srcAddr ) ||
		bufferedMessages.isDelayingMessagesFor( entityID, srcAddr );

	bool isForDestroyedGhost = false;

	// Message is for a destroyed ghost if it is out of subsequence order.
	if (reality_ == GHOST_ONLY)
	{
		// Destroyed by restored real.
		if (pEntity && pEntity->isReal())
		{
			isForDestroyedGhost = true;
		}

		// Destroyed by restored real, then recreated when offload to sender.
		else if (pEntity && pEntity->isOffloadingTo( srcAddr ) &&
				!BufferedGhostMessages::isSubsequenceStart( header.identifier ))
		{
			isForDestroyedGhost = true;
		}

		// Destroyed when senders death was handled.
		else if (shouldBufferGhostMessage && 
				!bufferedMessages.hasMessagesFor( entityID, srcAddr ) &&
				!BufferedGhostMessages::isSubsequenceStart( header.identifier ))
		{
			isForDestroyedGhost = true;
		}

		// TODO: We can not detect if a subsequence start message is for a
		// destroyed ghost as there is no preceeding message to reference its
		// order. Buffering/handling redundant subsequence starts would be OK
		// if the message is idempotent.
	}

	// Drop GHOST_ONLY messages for destroyed ghost.
	if (isForDestroyedGhost)
	{
		WARNING_MSG( "EntityMessageHandler::handleMessage( %s [id: %d] ): "
				"Dropping ghost message for entity %u from %s, "
				"ghost was destroyed while in-flight.\n",
			header.msgName(), int( header.identifier ),
			entityID, srcAddr.c_str() );

		this->sendFailure( srcAddr, header, data, entityID );
	}

	// Buffer GHOST_ONLY messages that are out of sender order.
	else if (reality_ == GHOST_ONLY && shouldBufferGhostMessage)
	{
		BufferedGhostMessage * pMsg =
			BufferedGhostMessageFactory::createBufferedMessage(
				srcAddr, header, data, entityID, this );

		bufferedMessages.add( entityID, srcAddr, pMsg );

		WARNING_MSG( "EntityMessageHandler::handleMessage( %s [id: %d] ): "
				   "Buffered ghost message for entity %u from %s\n",
				   header.msgName(), int( header.identifier ),
				   entityID, srcAddr.c_str() );
	}

	// REAL_ONLY messages should be forwarded if we don't have the real.
	else if (reality_ >= REAL_ONLY && (!pEntity || !pEntity->isReal()))
	{
		// We only try to look up the cached channel for the entity if it
		// doesn't exist, since calling findRealChannel() for ghosts will
		// cause an assertion.
		CellAppChannel * pChannel = pEntity ?
			pEntity->pRealChannel() :
			Entity::population().findRealChannel( entityID );

		if (pChannel)
		{
			Entity::forwardMessageToReal( *pChannel, entityID,
				header.identifier, data, srcAddr, header.replyID );
		}
		else
		{
			ERROR_MSG( "EntityMessageHandler::handleMessage( %s [id: %d] ): "
				"Dropped real message for unknown entity %u\n",
				header.msgName(), int( header.identifier ), entityID );

			this->sendFailure( srcAddr, header, data, entityID );
		}
	}

	// Drop WITNESS_ONLY message for entities without a witness.
	else if (reality_ == WITNESS_ONLY && !pEntity->pReal()->pWitness())
	{
		DEBUG_MSG( "EntityMessageHandler::handleMessage( %s [id: %d] ): "
			"Received witness message for entity %u with no witness "
			"from %s\n",
			header.msgName(), int( header.identifier ),
			entityID, srcAddr.c_str() );

		this->sendFailure( srcAddr, header, data, entityID );
	}

	// Message is good, call through to the handler.
	else if (shouldBufferIfTickPending_ && app.nextTickPending())
	{
		static GameTime s_lastMessageTime = 0;
		BufferedEntityMessages & bufferedEntityMessages =
			app.bufferedEntityMessages();

		if (s_lastMessageTime != app.time())
		{
			WARNING_MSG( "EntityMessageHandler::handleMessage: No time left in "
					"tick (%u). Buffering '%s' and similar messages this tick. "
					"Num previously in buffer = %zu.\n",
				app.time(),
				header.msgName(),
				bufferedEntityMessages.size() );

			s_lastMessageTime = app.time();
		}

		bufferedEntityMessages.add( *this,
				srcAddr, header, data, entityID );
	}
	else
	{
		this->callHandler( srcAddr, header, data, pEntity );
	}
}


/**
 *	This method is called when handling the message has failed.
 */
void EntityMessageHandler::sendFailure( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		EntityID entityID )
{
	data.finish();

	if (header.replyID != Mercury::REPLY_ID_NONE)
	{
		CellApp & app = ServerApp::getApp< CellApp >( header );

		BW::stringstream errorMsg;
		errorMsg << "No such entity " << entityID <<
			" on CellApp " << app.id() << " at " <<
			app.interface().address().c_str();

		MethodDescription::sendReturnValuesError( "BWNoSuchEntityError",
				errorMsg.str().c_str(),	header.replyID, srcAddr,
				app.interface() );
	}
}


// -----------------------------------------------------------------------------
// Section: Other handlers
// -----------------------------------------------------------------------------

/**
 * Objects of this type are used to handle messages destined for an entity.
 */
template <class ARGS_TYPE>
class CellEntityMessageHandler : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)( const ARGS_TYPE & args );

	/**
	 *	Constructor.
	 */
	CellEntityMessageHandler( std::pair<Handler, EntityReality> args ) :
		EntityMessageHandler( args.second ),
		handler_( args.first )
	{}

	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		ARGS_TYPE args;
		data >> args;
		(pEntity->*handler_)( args );
	}

protected:
	Handler			handler_;
};


/**
 * Objects of this type are used to handle messages destined for an entity.
 */
template <>
class CellEntityMessageHandler< void > : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)();

	/**
	 *	Constructor.
	 */
	CellEntityMessageHandler( std::pair<Handler, EntityReality> args ) :
		EntityMessageHandler( args.second ),
		handler_( args.first )
	{}

	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		(pEntity->*handler_)();
	}

protected:
	Handler			handler_;
};

/**
 * Objects of this type are used to handle messages destined for an entity.
 * This class also redirects failures to CellApp to handle cases where an
 * entity no longer exists but the message needs specific treatment
 */
template <class ARGS_TYPE>
class CellEntityMessageHandlerWithFailure : public CellEntityMessageHandler< ARGS_TYPE >
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)( const ARGS_TYPE & args );

	/**
	 * This type is the CellApp function pointer that handles the failure
	 */
	typedef void (CellApp::*FailureHandler)( EntityID entityID,
			const Mercury::Address &oldBaseAddr,
			const ARGS_TYPE & args );

	/**
	 *	Constructor.
	 */
	CellEntityMessageHandlerWithFailure( const std::pair< FailureHandler,
							std::pair<Handler, EntityReality> > & args ) :
		CellEntityMessageHandler< ARGS_TYPE >( args.second ),
		failureHandler_( args.first )
	{}

	void sendFailure( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			EntityID entityID )
	{
		ARGS_TYPE args;
		data >> args;

		data.finish();

		CellApp & app = ServerApp::getApp< CellApp >( header );
		(app.*failureHandler_)( entityID, srcAddr, args );
	}

protected:
	FailureHandler			failureHandler_;
};


/**
 *	Objects of this type are used to handle variable length messages destined
 *	for an entity.
 */
class EntityVarLenMessageHandler : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)( BinaryIStream & stream );

	/**
	 *	Constructor.
	 */
	EntityVarLenMessageHandler( std::pair<Handler, EntityReality> args ) :
		EntityMessageHandler( args.second ),
		handler_( args.first )
	{}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		(pEntity->*handler_)( data );
	}

	Handler			handler_;
};

/**
 *	Objects of this type are used to handle variable length messages destined
 *	for an entity. This also passes the source address and header to the
 *	handling function.
 */
class RawEntityVarLenMessageHandler : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & stream );

	/**
	 *	Constructor.
	 */
	RawEntityVarLenMessageHandler( std::pair<Handler, EntityReality> args ) :
		EntityMessageHandler( args.second ),
		handler_( args.first )
	{}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		(pEntity->*handler_)( srcAddr, header, data );
	}

	Handler			handler_;
};

/**
 *	Objects of this type are used to handle variable length messages destined
 *	for an entity.
 */
class EntityVarLenRequestHandler : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Entity::*Handler)( const Mercury::Address& srcAddr,
			Mercury::UnpackedMessageHeader& header, BinaryIStream & stream );

	/**
	 *	Constructor.
	 */
	EntityVarLenRequestHandler( std::pair<Handler, EntityReality> args ) :
		EntityMessageHandler( args.second ),
		handler_( args.first )
	{}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		(pEntity->*handler_)( srcAddr, header, data );
	}

	Handler			handler_;
};


/**
 *	This class sends all our entity positions to an interested source,
 *	such as a CellAppMgr that wants an overview of the world (for debugging)
 */
class EntityPositionSender : public Mercury::InputMessageHandler
{
public:
	EntityPositionSender( void * ) {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
	{
		SpaceID id;
		data >> id;

		CellApp & app = ServerApp::getApp< CellApp >( header );
		Cell * pCell = app.findCell( id );

		if (pCell != NULL)
		{
			Mercury::ChannelSender sender( CellApp::getChannel( srcAddr ) );
			Mercury::Bundle	& bundle = sender.bundle();

			bundle.startReply( header.replyID );
			pCell->sendEntityPositions( bundle );
		}
		else
		{
			ERROR_MSG( "EntityPositionSender::handleMessage: "
					"Do not have cell with id %u\n", id );
		}
	}
};



/**
 *	Objects of this type are used to handle variable length messages destined
 *	for a cell. They simply pass all the mercury parameters to the handler
 *	function.
 */
class CreateEntityNearEntityHandler : public EntityMessageHandler
{
public:
	/**
	 *	This type is the function pointer type that handles the incoming
	 *	message.
	 */
	typedef void (Cell::*Handler)(
		const Mercury::Address & addr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & stream,
		EntityPtr pNearbyEntity );

	/**
	 *	Constructor.
	 */
	CreateEntityNearEntityHandler( Handler handler ) :
		EntityMessageHandler( REAL_ONLY, /* shouldBufferIfTickPending*/true ),
		handler_( handler )
	{
	}

	virtual void callHandler( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, Entity * pEntity )
	{
		(pEntity->cell().*handler_)( srcAddr, header, data, pEntity );
	}

private:
	virtual void sendFailure( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			EntityID entityID )
	{
		Mercury::ChannelVersion channelVersion;
		bool isRestore;
		EntityID entityToCreateID;
		EntityTypeID entityTypeID;

		data >> channelVersion >> isRestore >>
			entityToCreateID >> entityTypeID;
		data.finish(); // discard the rest

		DEBUG_MSG( "CreateEntityNearEntityHandler::sendFailure: "
				"for request to create cell entity %u of type %s, "
				"no such entity %u on this CellApp\n",
			entityToCreateID,
			EntityType::getType( entityTypeID )->name(),
			entityID );


		// createEntityNearEntity only comes from a BaseApp, it's
		// likely we have a channel to it somewhere.

		Mercury::UDPChannel & channel =
			CellApp::getChannel( srcAddr );
		Mercury::Bundle & bundle = channel.bundle();
		bundle.startReply( header.replyID );
		bundle << (EntityID)0;

		// TODO: Aggregate these sends and those in
		// Cell::createEntity. At the moment, each create cell
		// request from the BaseApp results in a packet with only
		// one reply message sent back to that BaseApp.
		channel.send();
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This works out which space should handle this message by reading from
 *	the start of the input stream.
 */
template <>
class MessageHandlerFinder< Space >
{
public:
	static Space * find( const Mercury::Address & srcAddr,
							const Mercury::UnpackedMessageHeader & header,
							BinaryIStream & data )
	{
		SpaceID id;
		data >> id;

		CellApp & cellApp = ServerApp::getApp< CellApp >( header );
		Space * pResult = cellApp.findSpace( id );

		if (!pResult)
		{
			ERROR_MSG( "MessageHandlerFinder< Space >::find: "
						"Space %u does not exist\n",
					id );
		}

		return pResult;
	}
};


/**
 *	This works out which cell should handle this message by reading from
 *	the start of the input stream.
 */
template <>
class MessageHandlerFinder< Cell >
{
public:
	static Cell * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		SpaceID id;
		data >> id;

		CellApp & cellApp = ServerApp::getApp< CellApp >( header );
		Cell * pResult = cellApp.findCell( id );

		if (!pResult)
		{
			ERROR_MSG( "MessageHandlerFinder< Cell >::find: "
						"No cell for space %u\n",
					id );
		}

		return pResult;
	}
};

BW_END_NAMESPACE

// We serve this interface
#define DEFINE_SERVER_HERE
#include "cellapp_interface.hpp"

// message_handlers.cpp
