#ifndef BW_NULL_CONNECTION_HPP
#define BW_NULL_CONNECTION_HPP

#include "bw_connection.hpp"

#include "bwentity/bwentity_api.hpp"
#include "connection/connection_transport.hpp"

BW_BEGIN_NAMESPACE

class BinaryOStream;
class BWEntityFactory;
class EntityDefConstants;
class ServerMessageHandler;


/**
 *	This class is a concrete BWConnection implementation used to locally
  *	simulate a server connection to a server with no game scripts. All it does
  *	is create a player entity on the client, controlled by the client.
 */
class BWNullConnection : public BWConnection
{
public:
	BWENTITY_API BWNullConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		const EntityDefConstants & entityDefConstants,
		double initialClientTime, SpaceID spaceID );
	~BWNullConnection();

	BWENTITY_API EntityID createNewPlayerBaseEntity( EntityTypeID entityTypeID,
		BinaryIStream & basePlayerData );
	BWENTITY_API bool giveCellToPlayerBaseEntity( const Position3D & position,
		const Direction3D & direction, BinaryIStream & cellPlayerData );

	BWENTITY_API bool destroyPlayerCellEntity();
	BWENTITY_API void destroyPlayerBaseEntity();

	BWENTITY_API void spaceData( SpaceID spaceID, SpaceEntryID entryID,
		uint16 key, const BW::string & data );

	// BWConnection overrides
	BWENTITY_API void disconnect() {} /* override */;

	BWENTITY_API SpaceID spaceID() const /* override */;

	BWENTITY_API void clearAllEntities( bool keepLocalEntities = false )  /* override */;

	BWENTITY_API double clientTime() const /* override */;
	BWENTITY_API double serverTime() const /* override */;
	BWENTITY_API double lastMessageTime() const /* override */;
	BWENTITY_API double lastSentMessageTime() const /* override */;

private:
	void preUpdate( float dt ) /* override */;
	void enqueueLocalMove( EntityID entityID, SpaceID spaceID, EntityID vehicleID,
		const Position3D & localPosition, const Direction3D & localDirection,
		bool isOnGround,
		const Position3D & worldReferencePosition ) /* override */;
	BinaryOStream * startServerMessage( EntityID entityID, int methodID,
		bool isForBaseEntity, bool & shouldDrop ) /* override */;
	void postUpdate() /* override */;
	bool shouldSendToServerNow() const /* override */;

	EntityID playerID_;
	SpaceID spaceID_;
	bool hasCell_;
	double time_;
	double lastSendTime_;
};

BW_END_NAMESPACE

#endif // BW_NULL_CONNECTION_HPP

