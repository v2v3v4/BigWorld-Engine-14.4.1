#include "pch.hpp"

#include "bw_null_connection.hpp"

#include "connection/server_message_handler.hpp"
#include "cstdmf/memory_stream.hpp"

BW_BEGIN_NAMESPACE

int BWNullConnection_Token = 0;

/**
 *	Constructor.
 *
 *	@param entityFactory 	The entity factory to use when requested to create
 *							entities.
 *	@param spaceDataStorage	The BWSpaceDataStorage for storing space data.
 *	@param entityDefConstants	The constant entity definition values to use
 *								during initialisation.
 *	@param initialClientTime	The client time of BWNullConnection creation
 *	@param	spaceID			The SpaceID to tell the client it is in.
 */
BWNullConnection::BWNullConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		const EntityDefConstants & entityDefConstants,
		double initialClientTime, SpaceID spaceID ) :
	BWConnection( entityFactory, spaceDataStorage, entityDefConstants ),
	playerID_( NULL_ENTITY_ID ),
	spaceID_( spaceID ),
	hasCell_( false ),
	time_( initialClientTime ),
	lastSendTime_( 0. )
{
}


/**
 *	Destructor.
 */
BWNullConnection::~BWNullConnection()
{
}


/**
 *	This method sends a new base entity to the client, as is done immediately
 *	after connection to a server, or when the client is given to a different
 *	proxy on the same BaseApp.
 *
 *	@param entityTypeID		The type of entity to create a base of
 *	@param basePlayerData	The stream of properties as per
 *							ServerMessageHandler::onBasePlayerCreate
 *
 *	@return The EntityID of the newly created base player entity.
 */
EntityID BWNullConnection::createNewPlayerBaseEntity( EntityTypeID entityTypeID,
	BinaryIStream & basePlayerData )
{
	if (playerID_ != NULL_ENTITY_ID)
	{
		// Ensure that an immediate callback to BWConnection::clearAllEntities
		// will not be blocked.
		playerID_ = NULL_ENTITY_ID;
		hasCell_ = false;

		this->serverMessageHandler().onEntitiesReset(
			/* keepPlayerOnBase */ false );
	}

	// A client with multiple connections is not reasonably guaranteed to see
	// the same EntityID for a specific entity on each connection, Entity IDs
	// are only valid for the connection they come with.
	playerID_ = 1;

	this->serverMessageHandler().onBasePlayerCreate( playerID_, entityTypeID,
		basePlayerData );

	// Confirm that our BWConnection-facing player is in the correct state.
	MF_ASSERT( this->pPlayer()->entityID() == playerID_ );
	MF_ASSERT( this->pPlayer()->entityTypeID() == entityTypeID );
	MF_ASSERT( !this->pPlayer()->isInAoI() );

	return playerID_;
}


/**
 *	This method sends a cell entity to the client for the current base player
 *	entity, as is done when the Base player entity gains a Cell entity or
 *	when the player entity already has a Cell entity when the client connects.
 *
 *	@param	spaceID			The SpaceID to tell the client it is in.
 *	@param	position		The Position3D in the space the entity is at
 *	@param	direction		The Direction3D of the client's entity's facing.
 *	@param	cellPlayerData	The stream of properties as per
 *							ServerMessageHandler::onCellPlayerCreate
 *
 *	@return					true if the creation succeeded, false if there was
 *							no base player in existence, or it already had a
 *							cell player.
 */
bool BWNullConnection::giveCellToPlayerBaseEntity( const Position3D & position,
		const Direction3D & direction, BinaryIStream & cellPlayerData )
{
	if (playerID_ == NULL_ENTITY_ID)
	{
		ERROR_MSG( "BWNullConnection::giveCellToPlayerBaseEntity: "
				"Tried to give a Cell to the player without a player Base\n" );
		return false;
	}

	if (hasCell_)
	{
		ERROR_MSG( "BWNullConnection::giveCellToPlayerBaseEntity: "
			"Tried to give a Cell to the player which already has a Cell\n" );
		return false;
	}

	hasCell_ = true;

	this->serverMessageHandler().onCellPlayerCreate( playerID_, spaceID_,
		NULL_ENTITY_ID, position,
		direction.yaw, direction.pitch, direction.roll, cellPlayerData );

	// Confirm that our BWConnection-facing player is in the correct state.
	MF_ASSERT( this->pPlayer()->entityID() == playerID_ );
	// No vehicle, so this should be true immediately.
	MF_ASSERT( this->pPlayer()->isInAoI() );

	return true;
}


/**
 *	This method tells the client its player's cell entity has been destroyed.
 *
 *	@return	true if destruction succeeded, false if there was no cell entity to
 *			destroy.
 */
bool BWNullConnection::destroyPlayerCellEntity()
{
	if (!hasCell_)
	{
		return false;
	}

	MF_ASSERT( playerID_ != NULL_ENTITY_ID );

	hasCell_ = false;
	this->serverMessageHandler().onEntitiesReset( /* keepPlayerOnBase */ true );

	return true;
}


/**
 *	This method destroys the client's base entity in the same way a server-
 *	driven disconnection does. It has no effect if no base entity existed.
 */
void BWNullConnection::destroyPlayerBaseEntity()
{
	if (playerID_ == NULL_ENTITY_ID)
	{
		MF_ASSERT( hasCell_ == false );
		return;
	}

	// Done first to ensure that an immediate callback to
	// BWConnection::clearAllEntities will not be blocked.
	playerID_ = NULL_ENTITY_ID;
	hasCell_ = false;

	this->serverMessageHandler().onEntitiesReset(
		/* keepPlayerOnBase */ false );
}


/**
 *	This method handles data associated with a space.
 */
void BWNullConnection::spaceData( SpaceID spaceID, SpaceEntryID entryID,
	uint16 key, const BW::string & data )
{
	this->serverMessageHandler().spaceData(
		spaceID, entryID, key, data );
}


/**
 *	This method returns the space ID.
 */
SpaceID BWNullConnection::spaceID() const /* override */
{
	return spaceID_;
}


/**
 *	This method clears out all the entities if we are disconnected. If they are
 *	not cleared out after disconnection, they will be on the next reconnection.
 */
void BWNullConnection::clearAllEntities( 
		bool keepLocalEntities /* = false */ ) /* override */
{
	if (playerID_ == NULL_ENTITY_ID)
	{
		this->BWConnection::clearAllEntities( keepLocalEntities );
	}
}


/**
 *	This returns the client-side elapsed time.
 */
double BWNullConnection::clientTime() const /* override */
{
	return time_;
}


/**
 *	This method returns the server time.
 */
double BWNullConnection::serverTime() const /* override */
{
	return time_;
}


/**
 *	This method returns the last time a message was received from the server.
 */
double BWNullConnection::lastMessageTime() const /* override */
{
	return time_;
}


/**
 *	This method returns the last time a message was sent to the server.
 */
double BWNullConnection::lastSentMessageTime() const /* override */
{
	return lastSendTime_;
}


/**
 *	This method ticks the connection before the entities are ticked to process
 *	any new messages received.
 *
 *	@param dt 	The time delta.
 */
void BWNullConnection::preUpdate( float dt ) /* override */
{
	time_ += dt;
}


/**
 *	This method adds a local entity movement to be sent in the next update.
 */
void BWNullConnection::enqueueLocalMove( EntityID entityID, SpaceID spaceID,
	EntityID vehicleID, const Position3D & localPosition,
	const Direction3D & localDirection, bool isOnGround,
	const Position3D & worldReferencePosition ) /* override */
{
}


/**
 *	This method gets a BinaryOStream for an entity message to be sent to
 *	the server in the next update.
 */
BinaryOStream * BWNullConnection::startServerMessage( EntityID entityID,
	int methodID, bool isForBaseEntity, bool & shouldDrop ) /* override */
{
	shouldDrop = true;
	return NULL;
}


/**
 *	This method sends any updated state to the server after entities have
 *	updated their state.
 */
void BWNullConnection::postUpdate() /* override */
{
	lastSendTime_ = time_;
}


/**
 *	This method checks that we're not trying to send too quickly for the
 *	ServerConnection's current settings.
 */
bool BWNullConnection::shouldSendToServerNow() const /* override */
{
	static const double sendInterval = 0.1; // 100ms between "sends".
	const double timeSinceLastSend = time_ - lastSendTime_;

	return timeSinceLastSend > sendInterval;
}


BW_END_NAMESPACE

// bw_null_connection.cpp

