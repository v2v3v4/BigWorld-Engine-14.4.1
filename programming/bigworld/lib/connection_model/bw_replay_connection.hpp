#ifndef BW_REPLAY_CONNECTION_HPP
#define BW_REPLAY_CONNECTION_HPP

#include "bw_connection.hpp"
#include "bw_replay_event_handler.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"

#include "connection/connection_transport.hpp"
#include "connection/replay_controller.hpp"

BW_BEGIN_NAMESPACE

class BinaryOStream;
class BWEntityFactory;
class EntityDefConstants;


/**
 *	This class is a concrete BWConnection implementation used for running a
 *	replay rather than a live game connection.
 */
class BWReplayConnection : public BWConnection,
		public ReplayControllerHandler,
		public BWEntitiesListener
{
public:
	BWReplayConnection( BWEntityFactory & entityFactory,
		BWSpaceDataStorage & spaceDataStorage,
		const EntityDefConstants & entityDefConstants,
		double initialClientTime );
	~BWReplayConnection();

	// BWReplayConnection interface
	void setHandler( BWReplayEventHandler * pHandler );

	bool setReplaySignaturePublicKeyString(
		const BW::string & publicKeyString );

	bool setReplaySignaturePublicKeyPath( const BW::string & publicKeyPath );

	bool startReplayFromFile( TaskManager & taskManager, SpaceID spaceID,
		const BW::string & replayFileName,
		GameTime volatileInjectionPeriod =
			ReplayController::DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS );

	bool startReplayFromStreamData( TaskManager & taskManager, SpaceID spaceID,
		const BW::string & localCacheFileName,
		bool shouldKeepCacheFile,
		GameTime volatileInjectionPeriod =
			ReplayController::DEFAULT_VOLATILE_INJECTION_PERIOD_TICKS );

	void onInitialDataProcessed();

	void stopReplay();

	void addReplayData( BinaryIStream & data );
	void onReplayDataComplete();

	void pauseReplay();
	void resumeReplay();

	float speedScale() const;
	void speedScale( float newValue );

	ReplayController * pReplayController() const
		{ return pReplayController_.get(); }

	// BWConnection overrides
	void disconnect() /* override */;

	SpaceID spaceID() const  /* override */;

	bool isWitness( EntityID entityID ) const /* override */;
	bool isInAoI( EntityID entityID, EntityID playerID ) const /* override */;
	size_t numInAoI( EntityID playerID ) const /* override */;
	void clearAllEntities( bool keepLocalEntities = false ) /* override */;
	void visitAoIEntities( EntityID witnessID,
		AoIEntityVisitor & visitor ) /* override */;

	double clientTime() const /* override */;
	double serverTime() const /* override */;
	double lastMessageTime() const /* override */;
	double lastSentMessageTime() const /* override */;

	// BWEntitiesListener overrides
	void onEntityEnter( BWEntity * pEntity ) /* override */;
	void onEntityLeave( BWEntity * pEntity ) /* override */ {}
	void onEntitiesReset() /* override */ {}

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

	// ReplayControllerHandler overrides
	void onReplayControllerGotBadVersion( ReplayController & controller,
		const ClientServerProtocolVersion & version ) /* override */;
	bool onReplayControllerReadHeader( ReplayController & controller,
		const ReplayHeader & header ) /* override */;
	void onReplayControllerGotCorruptedData() /* override */;
	bool onReplayControllerReadMetaData( ReplayController & controller,
		const ReplayMetaData & metaData ) /* override */;
	void onReplayControllerEntityPlayerStateChange(
		ReplayController & controller, EntityID playerID,
		bool hasBecomePlayer ) /* override */;
	void onReplayControllerAoIChange( ReplayController & controller,
		EntityID playerID, EntityID entityID,
		bool hasEnteredAoI ) /* override */;
	void onReplayControllerError( ReplayController & controller,
		const BW::string & error )
		/* override */;
	void onReplayControllerFinish( ReplayController & controller )
		/* override */;
	void onReplayControllerDestroyed( ReplayController & controller )
		/* override */;
	void onReplayControllerPostTick( ReplayController & controller,
		GameTime tickTime ) /* override */;
	void onReplayControllerIncreaseTotalTime( ReplayController & controller, 
		double dTime ) /* override */;

	// ServerMessageHandler overrides (base class of ReplayControllerHandler)
	void onBasePlayerCreate( EntityID id, EntityTypeID type,
		BinaryIStream & data ) /* override */;
	void onCellPlayerCreate( EntityID id, SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, float yaw, float pitch, float roll,
		BinaryIStream & data ) /* override */;
	void onEntityControl( EntityID id, bool control ) /* override */;
	const CacheStamps * onEntityCacheTest( EntityID id ) /* override */;
	void onEntityLeave( EntityID id,
		const CacheStamps & stamps ) /* override */;
	void onEntityCreate( EntityID id, EntityTypeID type, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, float yaw, float pitch,
		float roll, BinaryIStream & data ) /* override */;
	void onEntityProperties( EntityID id, BinaryIStream & data ) /* override */;
	void onEntityMethod( EntityID id, int methodID, BinaryIStream & data )
		/* override */;
	void onEntityProperty( EntityID id, int propertyID,
		BinaryIStream & data ) /* override */;
	void onNestedEntityProperty( EntityID id, BinaryIStream & data,
		bool isSlice ) /* override */;
	int getEntityMethodStreamSize( EntityID id,
		int methodID, bool isFailAllowed ) const /* override */;
	int getEntityPropertyStreamSize( EntityID id,
		int propertyID, bool isFailAllowed ) const /* override */;
	void onEntityMoveWithError( EntityID id, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, const Vector3 & posError,
		float yaw, float pitch, float roll, bool isVolatile ) /* override */;
	void spaceData( SpaceID spaceID, SpaceEntryID entryID, uint16 key,
		const BW::string & data ) /* override */;
	void spaceGone( SpaceID spaceID ) /* override */;
	void onStreamComplete( uint16 id, const BW::string & desc,
		BinaryIStream & data ) /* override */;
	void onEntitiesReset( bool keepPlayerOnBase ) /* override */;
	void onRestoreClient( EntityID id, SpaceID spaceID, EntityID vehicleID,
		const Position3D & pos, const Direction3D & dir,
		BinaryIStream & data ) /* override */;

	void notifyReplayTerminated(
		BWReplayEventHandler::ReplayTerminatedReason reason );
	void clearReplayController();

	// Properties
	const EntityDefConstants &		entityDefConstants_;
	BWSpaceDataStorage &			spaceDataStorage_;
	BWReplayEventHandler *			pReplayEventHandler_;
	ReplayControllerPtr				pReplayController_;
	double							time_;
	double							lastSendTime_;
	SpaceID							spaceID_;
	BW::string						verificationKey_;
	bool							isHandlingInitialData_;

	typedef BW::set< EntityID > AoISet;
	typedef BW::map< EntityID, AoISet > PlayerAoIMap;
	PlayerAoIMap					playerAoIMap_;
};

BW_END_NAMESPACE

#endif // BW_REPLAY_CONNECTION_HPP

