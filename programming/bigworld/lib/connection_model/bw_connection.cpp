#include "pch.hpp"

#include "bw_connection.hpp"

#include "bw_server_message_handler.hpp"

#include "connection/movement_filter.hpp"
#include "connection/server_finder.hpp"

#include <sys/stat.h>


BW_BEGIN_NAMESPACE

extern int BWServerConnection_Token;
extern int BWNullConnection_Token;
extern int BWReplayConnection_Token;
extern int SpaceDataMappings_Token;
extern int SimpleSpaceDataStorage_Token;
extern int ServerEntityMailBox_Token;
#if !defined( BW_CLIENT )
extern int AvatarFilter_Token;
#endif
int ConnectionModel_Token = BWServerConnection_Token |
	BWNullConnection_Token |
	BWReplayConnection_Token |
	SpaceDataMappings_Token |
	SimpleSpaceDataStorage_Token |
	ServerEntityMailBox_Token
#if !defined( BW_CLIENT )
	| AvatarFilter_Token
#endif	
	;

#ifdef _MSC_VER
#pragma warning( push )
// C4355: 'this' : used in base member initializer list
#pragma warning( disable: 4355 )
#endif // _MSC_VER

/**
 *	Constructor.
 *
 *	@param entityFactory 	The entity factory to use when requested to create
 *							entities.
 *	@param entityDefConstants	The constant entity definition values to use
 *								during initialisation.
 */
BWConnection::BWConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		const EntityDefConstants & entityDefConstants ) :
	entities_( *this, entityFactory ),
	pMessageHandler_(
		new BWServerMessageHandler( entities_, spaceDataStorage ) )
{
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER


/**
 *	Destructor.
 */
BWConnection::~BWConnection()
{
}


/**
 *	This method ticks the connection. It needs to be called regularly to receive
 *	network data and also check for connection timeouts.
 *
 *	@param dt 	The time delta.
 */
void BWConnection::update( float dt )
{
	this->preUpdate( dt );
	entities_.tick( this->clientTime(), dt );
}


/**
 *	This method sends any updated state to the server. It should be called
 *	paired with BWConnection::update after any local state changes are made or
 *	game logic is run.
 */
void BWConnection::updateServer()
{
	if (!this->shouldSendToServerNow())
	{
		// Our implementation says it's too soon to send to the server.
		return;
	}

	entities_.updateServer( this->clientTime() );
	this->postUpdate();
}


/**
 *	This method returns whether the given entity ID references a
 *	player/witness entity.
 *
 *	@param entityID		The entity ID.
 *
 *	@return true if the entity referenced by the entity ID is a witness entity.
 */
bool BWConnection::isWitness( EntityID entityID ) const
{
	// By default, only players are witnesses. If subclasses support non-player
	// witnesses they can override this method.

	return (this->pPlayer() && 
		(this->pPlayer()->entityID() == entityID));
}


/**
 *	This method returns whether an entity is in the given player's AoI.
 *
 *	@param entityID		The entity ID.
 *	@param playerID		The player/witness ID.
 *
 *	@return true if the entity referenced by playerID is a witness and the
 *			entity referenced by entityID is a member of its AoI.
 */
bool BWConnection::isInAoI( EntityID entityID, EntityID playerID ) const
{
	// By default, our list of entities _is_ the AoI collection.
	// For some subclasses, this may not be the case.
	return (this->BWConnection::isWitness( playerID ) &&
		this->entities().find( entityID ));
}


/**
 *	This method returns the size of the AoI for the given player ID.
 *
 *	@param playerID		The player/witness ID.
 *
 *	@return the number of entities in the AoI of the referenced witness.
 */
size_t BWConnection::numInAoI( EntityID playerID ) const
{
	if (playerID != this->pPlayer()->entityID())
	{
		return 0;
	}

	return entities_.size();
}


/**
 *	This method visits all the entities in a witness entity's AoI.
 */
void BWConnection::visitAoIEntities( EntityID witnessID,
		AoIEntityVisitor & visitor )
{
	if (witnessID != this->pPlayer()->entityID())
	{
		return;
	}

	for (BWEntities::const_iterator iter = entities_.begin();
		iter != entities_.end();
		++iter)
	{
		visitor.onVisitAoIEntity( iter->second.get() );
	}
}


/**
 *	This method clears out all the entities if we are disconnected. If they are
 *	not cleared out after disconnection, they will be on the next reconnection.
 *
 *	Subclasses may override this purely to apply policy against calls during
 *	invalid state, such as attempts to call this while connected to a server.
 *
 *	@param keepLocalEntities	If true, then locally created entities (also
 *								known as client-only entities) will not be
 *								cleared with entities created from the server.
 *								If false, then both locally created and server
 *								created entities will be cleared.
 *	@see createLocalEntity
 *	@see deleteLocalEntity
 */
void BWConnection::clearAllEntities( bool keepLocalEntities /* = false */)
{
	if (this->spaceID() != NULL_SPACE_ID)
	{
		pMessageHandler_->clearSpace( this->spaceID() );
	}

	entities_.clear( /* keepPlayerBaseEntity */ false, keepLocalEntities );
}


/**
 * This method creates a client-only entity
 */
BWEntity * BWConnection::createLocalEntity( EntityTypeID entityTypeID,
	SpaceID spaceID, EntityID vehicleID,
	const Position3D & position, const Direction3D & direction,
	BinaryIStream & data )
{
	BWEntity * pEntity = entities_.createLocalEntity( entityTypeID,	spaceID,
		vehicleID, position, direction, data );

	MF_ASSERT( pEntity == NULL || pEntity->isLocalEntity() );

	return pEntity;
}


/**
 *	This method destroys a client-only entity
 */
bool BWConnection::destroyLocalEntity( EntityID entityID )
{
	MF_ASSERT( BWEntities::isLocalEntity( entityID ) );
	return entities_.handleEntityLeave( entityID );
}


/**
 *	This method adds a new local movement to be sent to the server on next
 *	update.
 */
void BWConnection::addLocalMove( EntityID entityID, SpaceID spaceID,
	EntityID vehicleID, const Position3D & localPosition,
	const Direction3D & localDirection, bool isOnGround,
	const Position3D & worldReferencePosition )
{
	this->enqueueLocalMove( entityID, spaceID, vehicleID, localPosition,
		localDirection, isOnGround, worldReferencePosition );
}


/**
 *	This method adds a new entity message to be sent to the server on next
 *	update.
 */
BinaryOStream * BWConnection::addServerMessage( EntityID entityID, int methodID,
	bool isForBaseEntity, bool & shouldDrop )
{
	shouldDrop = false;
	return this->startServerMessage( entityID, methodID, isForBaseEntity,
		shouldDrop );
}


/**
 *	This method registers a listener that receives notifications when
 *	entities enter or leave the client-side AoI.
 *
 *	@see removeEntitiesListener
 */
void BWConnection::addEntitiesListener( BWEntitiesListener * pListener )
{
	entities_.addListener( pListener );
}


/**
 *	This method deregisters a listener registered by addEntitiesListener().
 *
 *	@return	true if the listener was already registered, false otherwise.
 *	@see addEntitiesListener
 */
bool BWConnection::removeEntitiesListener( BWEntitiesListener * pListener )
{
	return entities_.removeListener( pListener );
}


/**
 *	This method registers a new space data listener.
 *
 *	@param listener 	The space data listener to register.
 */
void BWConnection::addSpaceDataListener( BWSpaceDataListener & listener )
{
	pMessageHandler_->addSpaceDataListener( &listener );
}


/**
 *	This method deregisters an existing space data listener.
 *
 *	@param listener 	The space data listener to deregister.
 */
void BWConnection::removeSpaceDataListener( BWSpaceDataListener & listener )
{
	pMessageHandler_->removeSpaceDataListener( &listener );
}


/**
 *	This method registers a stream data handler.
 *
 *	@param pListener 	The stream data handler to register
 *	@param id		 	The stream ID to register for
 */
void BWConnection::registerStreamDataHandler( BWStreamDataHandler & handler,
	uint16 streamID )
{
	pMessageHandler_->registerStreamDataHandler( &handler, streamID );
}


/**
 *	This method deregisters a stream data handler. 
 *
 *	@param pListener 	The stream data handler to deregister
 *	@param id		 	The stream ID to deregister from
 */
void BWConnection::deregisterStreamDataHandler( BWStreamDataHandler & handler,
	uint16 streamID )
{
	pMessageHandler_->deregisterStreamDataHandler( &handler, streamID );
}


/**
 *	This method registers a fallback stream data handler for streams which do
 *	not have a registered stream data handler.
 *
 *	@param handler 	The stream data handler to register.
 */
void BWConnection::setStreamDataFallbackHandler( BWStreamDataHandler & handler )
{
	pMessageHandler_->setStreamDataFallbackHandler( &handler );
}


/**
 *	This method removes any fallback stream data handler.
 */
void BWConnection::clearStreamDataFallbackHandler()
{
	pMessageHandler_->setStreamDataFallbackHandler( NULL );
}


/**
 *	This method exposes our ServerMessageHandler for BWConnection subclasses.
 */
ServerMessageHandler & BWConnection::serverMessageHandler() const
{
	MF_ASSERT( pMessageHandler_.get() != NULL );
	return *pMessageHandler_;
}

BW_END_NAMESPACE

// bw_connection.cpp

