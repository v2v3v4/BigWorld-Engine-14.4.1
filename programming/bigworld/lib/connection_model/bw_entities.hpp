#ifndef BW_ENTITIES_HPP
#define BW_ENTITIES_HPP

#include "bw_entity.hpp"

#include "active_entities.hpp"
#include "deferred_passengers.hpp"
#include "pending_entities.hpp"

#include "network/basictypes.hpp"
#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class BWConnection;
class BWEntitiesListener;
class BWEntityFactory;


/**
 *	This class holds a collection of entities.
 */
class BWEntities
{
public: /// C++ Housekeeping
	BWEntities( BWConnection & connection, BWEntityFactory & entityFactory );
	~BWEntities();

	// ServerMessageHandler handling 
	void handleBasePlayerCreate( EntityID id, EntityTypeID entityTypeID,
			BinaryIStream & data );

	void handleCellPlayerCreate( EntityID id, SpaceID spaceID,
			EntityID vehicleID, const Position3D & pos, 
			float yaw, float pitch, float roll, 
			BinaryIStream & data );

	void handleEntityControl( EntityID entityID, bool isControlling );

	void handleEntityCreate( EntityID id, EntityTypeID entityTypeID,
			SpaceID spaceID, EntityID vehicleID, const Position3D & position,
			float yaw, float pitch, float roll, BinaryIStream & data );

	bool handleEntityLeave( EntityID entityID );

	void handleEntityMethod( EntityID entityID, int methodID,
		BinaryIStream & data );

	void handleEntityProperty( EntityID entityID, int propertyID,
		BinaryIStream & data );

	void handleNestedEntityProperty( EntityID entityID, BinaryIStream & data,
		bool isSlice );

	int getEntityMethodStreamSize(
		EntityID entityID,
		int methodID,
		bool isFailAllowed = false ) const;

	int getEntityPropertyStreamSize(
		EntityID entityID,
		int propertyID,
		bool isFailAllowed = false ) const;

	void handleEntityMoveWithError( const EntityID & entityID,
		const SpaceID & spaceID, const EntityID & vehicleID,
		const Position3D & position, const Vector3 & posError, 
		float yaw, float pitch, float roll, bool isVolatile );

	void handleEntityProperties( EntityID entityID, BinaryIStream & data );

	void handleEntitiesReset( bool keepPlayerOnBase )
	{
		this->clear( keepPlayerOnBase );
	}

	void clear( bool keepPlayerBaseEntity = false,
		bool keepLocalEntities = false );

	void handleRestoreClient( EntityID entityID, SpaceID spaceID,
		EntityID vehicleID, const Position3D & pos, const Direction3D & dir,
		BinaryIStream & data );

	/// Local Entity management interface
	// Local Entities are useable with onEntityLeave onwards of the
	// ServerMessageHandler interface above
	BWENTITY_API BWEntity * createLocalEntity( EntityTypeID entityTypeID,
		SpaceID spaceID, EntityID vehicleID, const Position3D & position,
		const Direction3D & direction, BinaryIStream & data );

	BWENTITY_API static bool isLocalEntity( EntityID entityID );

	/// Entity management interface
	void onEntityConsistent( BWEntity * pEntity );
	void onEntityInconsistent( BWEntity * pEntity,
		PassengersVector * pFormerPassengers );
	void onEntityUnblocked( BWEntity * pEntity );
	void tick( double clientTime, double deltaTime );
	void updateServer( double clientTime );

	/// BWEntitiesListener registration
	void addListener( BWEntitiesListener * pListener );
	bool removeListener( BWEntitiesListener * pListener );

	/// InWorld Entity-list operations and accessors
	BWENTITY_API BWEntity * find( EntityID entityID ) const;

	/**
	 *	Return the entity with the given ID
	 *
	 *	@param entityID 	The entity ID to search for.
	 *
	 *	@return 			The entity with the given ID, or NULL if no such
	 *						entity exists.
	 */
	BWENTITY_API BWEntity * operator[]( EntityID entityID ) const
	{ 
		return this->find( entityID ); 
	}

	/** 
	 *	This method returns the count of entities. 
	 *
	 *	@return 	The count of entities.
	 */
	BWENTITY_API size_t size() const { return activeEntities_.size(); }

	/** 
	 *	This method returns the player.
	 *
	 *	@return 	The player entity, or NULL if no player is set.
	 */
	BWENTITY_API BWEntity * pPlayer() const { return pPlayer_.get(); }

	typedef ActiveEntities::const_iterator const_iterator;

	const_iterator begin() const	{ return activeEntities_.begin(); }
	const_iterator end() const	{ return activeEntities_.end(); }

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif // ENABLE_WATCHERS

private:
	/// Entity management internal interface
	void addEntity( BWEntity * pEntity, const SpaceID & spaceID,
		const EntityID & vehicleID, const Position3D & position,
		const Direction3D & direction );

	/// Entity lists
	BWEntity * findAny( EntityID entityID ) const;

	BWConnection & connection_;
	BWEntityFactory & entityFactory_;

	/// A brief note about these collections:
	/// Entities may move:
	/// DeferredPassengers to PendingEntities or ActiveEntities
	/// PendingEntities to DeferredPassengers or ActiveEntities
	/// ActiveEntities to DeferredPassengers (and NOT to PendingEntities)
	ActiveEntities activeEntities_;
	DeferredPassengers pendingPassengers_;
	PendingEntities appPendingEntities_;

	BWEntityPtr pPlayer_;

	static EntityID s_nextLocalEntityID_;
};

BW_END_NAMESPACE

#endif // BW_ENTITIES_HPP

