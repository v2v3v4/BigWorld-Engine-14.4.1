#ifndef BW_CONNECTION_HPP
#define BW_CONNECTION_HPP

#include "cstdmf/md5.hpp"

#include "bw_entities.hpp"

#include "bwentity/bwentity_api.hpp"
#include "network/space_data_mappings.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class BWEntityFactory;
class BWServerMessageHandler;
class BWSpaceDataListener;
class BWSpaceDataStorage;
class BWStreamDataHandler;
class EntityDefConstants;
class ServerMessageHandler;

/**
 *	Interface for visiting entities in an witness entity's AoI.
 */
class AoIEntityVisitor
{
public:
	virtual ~AoIEntityVisitor() {}

	virtual void onVisitAoIEntity( BWEntity * pEntity ) = 0;
};


/**
 *	This class is used to represent a client connection to the BigWorld server.
 *
 *	Applications should create a single instance of this class. Instances can
 *	be used for:
 *
 * - connection management
 * - updating positions of client-controlled entities
 * - calling remote methods on the server
 * - getting server-synchronised time
 * - local server discovery
 */
class BWConnection
{
public:
	BWConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		const EntityDefConstants & entityDefConstants );
	virtual ~BWConnection();

	BWENTITY_API virtual void disconnect() = 0;

	// Connection ticking
	BWENTITY_API void update( float dt );
	BWENTITY_API void updateServer();


	// Entity accessors and manipulators
	BWENTITY_API const BWEntities & entities() const	{ return entities_; }
	BWENTITY_API BWEntity * pPlayer() const		{ return entities_.pPlayer(); }
	BWENTITY_API virtual bool isWitness( EntityID entityID ) const;
	BWENTITY_API virtual bool isInAoI( EntityID entityID,
		EntityID playerID ) const;
	BWENTITY_API virtual size_t numInAoI( EntityID playerID ) const;
	BWENTITY_API virtual void visitAoIEntities( EntityID entityID,
		AoIEntityVisitor & visitor );

	/**
	 *	This method returns the space ID.
	 */
	BWENTITY_API virtual SpaceID spaceID() const = 0;

	BWENTITY_API virtual void clearAllEntities( bool keepLocalEntities = false );

	BWENTITY_API BWEntity * createLocalEntity( EntityTypeID entityTypeID,
		SpaceID spaceID, EntityID vehicleID,
		const Position3D & position, const Direction3D & direction,
		BinaryIStream & data );

	BWENTITY_API bool destroyLocalEntity( EntityID entityID );


	// Propagate local changes out to the world
	BWENTITY_API void addLocalMove( EntityID entityID, SpaceID spaceID,
		EntityID vehicleID, const Position3D & localPosition,
		const Direction3D & localDirection, bool isOnGround,
		const Position3D & worldReferencePosition );

	BWENTITY_API BinaryOStream * addServerMessage( EntityID entityID, int methodID,
		bool isForBaseEntity, bool & shouldDrop );


	// Virtual time querying
	/**
	 *	This returns the client-side elapsed time.
	 */
	BWENTITY_API virtual double clientTime() const = 0;
	/**
	 *	This method returns the last time a message was received from the server.
	 */
	BWENTITY_API virtual double lastMessageTime() const = 0;
	/**
	 *	This method returns the last time a message was sent to the server
	 */
	BWENTITY_API virtual double lastSentMessageTime() const = 0;
	/**
	 *	This method returns the server time.
	 */
	BWENTITY_API virtual double serverTime() const = 0;


	// Event handler registration
	BWENTITY_API void addEntitiesListener( BWEntitiesListener * pListener );
	BWENTITY_API bool removeEntitiesListener( BWEntitiesListener * pListener );

	BWENTITY_API void addSpaceDataListener( BWSpaceDataListener & listener );
	BWENTITY_API void removeSpaceDataListener( BWSpaceDataListener & listener );

	BWENTITY_API void registerStreamDataHandler(
		BWStreamDataHandler & handler, uint16 streamID );
	BWENTITY_API void deregisterStreamDataHandler(
		BWStreamDataHandler & handler, uint16 streamID );

	BWENTITY_API
	void setStreamDataFallbackHandler( BWStreamDataHandler & handler );
	BWENTITY_API void clearStreamDataFallbackHandler();

protected:
	virtual ServerMessageHandler & serverMessageHandler() const;

private:
	// Methods to be overridden by subclasses
	/*	The update routines are called as:
	 *	- preUpdate
	 *	- enqueueLocalMove and startServerMessage as required
	 *	- postUpdate
	 */

	/**
	 *	This method is called before our BWEntities instance is updated, and
	 *	should receive any changes from the world outside the client.
	 */
	virtual void preUpdate( float dt ) = 0;
	/**
	 *	This method adds a local entity movement to be sent in the next update.
	 */
	virtual void enqueueLocalMove( EntityID entityID, SpaceID spaceID, EntityID vehicleID,
		const Position3D & localPosition, const Direction3D & localDirection,
		bool isOnGround, const Position3D & worldReferencePosition ) = 0;
	/**
	 *	This method gets a BinaryOStream for an entity message to be sent to
	 *	the server in the next update.
	 *	This BinaryOStream may be written to up until the next
	 *	startServerMessage or postUpdate call.
	 *
	 *	@return The output stream for streaming data, or NULL. If NULL, the 
	 *			output parameter shouldDrop should be consulted. If shouldDrop is true 
	 *			after this call, then the call has been deemed by the connection to be 
	 *			silently dropped. If shouldDrop is false after this call, a stream could 
	 *			not be set up for this call, and the calling code should treat this as 
	 *			an error.
	 */
	virtual BinaryOStream * startServerMessage( EntityID entityID, int methodID,
		bool isForBaseEntity, bool & shouldDrop ) = 0;
	/**
	 *	This method is called after our BWEntities instance has completed
	 *	updating and is ready to propagate local changes to the world.
	 */
	virtual void postUpdate() = 0;
	/**
	 *	This method is called to determine if we should send an update to the
	 *	server, and allows subclasses to optionally rate-limit updateServer();
	 */
	virtual bool shouldSendToServerNow() const = 0;

	// Properties common to all BWConnection classes
	BWEntities 								entities_;
	std::auto_ptr< BWServerMessageHandler >	pMessageHandler_;
};

BW_END_NAMESPACE

#endif // BW_CONNECTION_HPP

