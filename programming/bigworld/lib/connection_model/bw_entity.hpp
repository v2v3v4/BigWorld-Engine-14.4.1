#ifndef BW_ENTITY_HPP
#define BW_ENTITY_HPP

#include "entity_extensions.hpp"

#include "connection/movement_filter_target.hpp"
#include "connection/movement_filter.hpp"
#include "connection/server_connection.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/smartpointer.hpp"
#include "math/vector3.hpp"
#include "network/basictypes.hpp"

#include <memory>
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

class BWConnection;
class BWEntity;
class EntityEntryBlocker;
class EntityExtensionFactoryBase;

BWENTITY_EXPORTED_TEMPLATE_CLASS ConstSmartPointer< BWEntity >;
BWENTITY_EXPORTED_TEMPLATE_CLASS SmartPointer< BWEntity >;
typedef SmartPointer< BWEntity > BWEntityPtr;

// A vector of vehicle passengers, used to carry around sets of passengers
// which do not have a currently valid vehicle, to avoid accidental early
// deletion while moving between containers.
// TODO: Can I put this somewhere better?
typedef BW::vector< std::pair< BWEntityPtr, EntityID > > PassengersVector;

/**
 *	This class is the base class of all entities. Your common entity
 *	implementation should inherit from this class.
 */
class BWEntity : public MovementFilterTarget, public SafeReferenceCount
{
public:
	BWENTITY_API BWEntity( BWConnection * pBWConnection );
	BWENTITY_API virtual ~BWEntity();

	void createExtensions( EntityExtensionFactoryManager & factories );

	bool init( EntityID entityID, EntityTypeID entityTypeID, SpaceID spaceID,
		BinaryIStream & data );

	bool onRestoreProperties( BinaryIStream & data );

	// Required override from MovementFilterTarget
	BWENTITY_API void setSpaceVehiclePositionAndDirection(
		const SpaceID & spaceID, const EntityID & vehicleID,
		const Position3D & position,  const Direction3D & direction );

	/**
	 *	This method is called when a method call has been sent from the server.
	 */
	virtual void onMethod( int messageID, BinaryIStream & data ) = 0;

	/**
	 *	This method is called when a property change has been sent from the
	 *	server.
	 */
	virtual void onProperty( int messageID, BinaryIStream & data,
		   bool isInitialising ) = 0;

	/**
	 *	This method is called when a nested property change has been sent from
	 *	the server.
	 */
	virtual void onNestedProperty( BinaryIStream & data, bool isSlice,
			bool isInitialising ) = 0;

	/**
	 *	This method is called when the network layer needs to know how big
	 *	a given method message is.
	 */
	virtual int getMethodStreamSize( int methodID ) const = 0;

	/**
	 *	This method is called when the network layer needs to know how big
	 *	a given property message is.
	 */
	virtual int getPropertyStreamSize( int propertyID ) const = 0;

	/**
	 *	This method returns the type name of this entity.
 	 */
	virtual const BW::string entityTypeName() const = 0;

	/**
	 *	This method is called when the server sends a batch of
	 *	tagged properties because the entity has changed detail
	 *	levels
	 */
	BWENTITY_API void onProperties( BinaryIStream & data, bool isInitialising );

	bool updateCellPlayerProperties( BinaryIStream & data );

	BWENTITY_API EntityID entityID() const	{ return entityID_; }
	BWENTITY_API EntityTypeID entityTypeID() const { return entityTypeID_; }


	BWENTITY_API BWConnection * pBWConnection() const { return pBWConnection_; }

	/**
	 *	This method returns true if this entity is the player.
	 */
	BWENTITY_API bool isPlayer() const 	{ return isPlayer_; }

	BWENTITY_API bool isWitness() const;

	BWENTITY_API const Position3D position() const;
	BWENTITY_API const Direction3D direction() const;

	BWENTITY_API
	const Position3D & localPosition() const { return localPosition_; }
	BWENTITY_API
	const Direction3D & localDirection() const { return localDirection_; }

	BWENTITY_API void transformCommonToVehicle( Position3D & position,
		Direction3D & direction ) const;
	BWENTITY_API void transformVehicleToCommon( Position3D & position,
		Direction3D & direction ) const;

	void onMoveFromServer( double time, const Position3D & position,
		EntityID vehicleID, SpaceID spaceID, const Direction3D & direction,
		const Vector3 & positionError, bool isTeleport );

	BWENTITY_API void getLatestMove( Position3D & position,
		EntityID & vehicleID, SpaceID & spaceID, Direction3D & direction,
		Vector3 & positionError ) const;

	BWENTITY_API void onMoveLocally( double time, const Position3D & position,
		EntityID vehicleID, bool is2DPosition, const Direction3D & direction );

	void sendLatestLocalMove() const;

	/**
	 *	This method returns the vehicle ID.
	 *
	 *	@return the vehicle ID if we have a vehicle, the ID of a vehicle
	 *	which has not yet entered the world if we are waiting for one,
	 *	or NULL_ENTITY_ID if this entity is not on a vehicle.
	 */
	BWENTITY_API EntityID vehicleID() const			{ return vehicleID_; }

	/**
	 *	This method returns the vehicle. 
	 *
	 *	This can be overridden to support covariance in the return type.
	 */
	BWENTITY_API virtual BWEntity * pVehicle() const		{ return pVehicle_.get(); }

	BWENTITY_API SpaceID spaceID() const			{ return spaceID_; }

	BWENTITY_API bool isInAoI() const				{ return isInAoI_; }

	BWENTITY_API bool isInWorld() const				{ return isInWorld_; }

	BWENTITY_API bool isControlled() const			{ return isControlled_; }

	BWENTITY_API bool isReceivingVolatileUpdates() const
		{ return isReceivingVolatileUpdates_; }

	BWENTITY_API bool isLocalEntity() const;

	BWENTITY_API bool isPhysicsCorrected() const;
	
	BWENTITY_API void setPhysicsCorrected( double time );

	BWENTITY_API MovementFilter * pFilter() const		{ return pFilter_; }
	BWENTITY_API void pFilter( MovementFilter * pNewFilter );
	BWENTITY_API bool applyFilter( double gameTime );

	/**
	 *	This method indicates whether the entity has been abandoned from its
	 *	BWEntities collection or not.
	 */
	BWENTITY_API bool isDestroyed() const	{ return isDestroyed_; }

	virtual EntityExtension *
		createExtension( EntityExtensionFactoryBase * pFactory )
	{
		return NULL;
	}

	EntityExtension * extensionInSlot( int slot ) const
	{
		return entityExtensions_.getForSlot( slot );
	}

	void triggerOnBecomePlayer();
	void triggerOnEnterAoI( const EntityEntryBlocker & rBlocker );
	void triggerOnEnterWorld();
	void triggerOnChangeSpace();
	void triggerOnLeaveWorld();
	void triggerOnLeaveAoI();
	void triggerOnBecomeNonPlayer();

	void destroyNonPlayer();

	void triggerOnChangeControl( bool isControlling, bool isInitialising );
	void triggerOnChangeReceivingVolatileUpdates();

	typedef BW::list< BWEntity * > Passengers;
	const Passengers & passengers()	{ return passengers_; }

protected:
	EntityExtensions entityExtensions_;

private:
	BWENTITY_API virtual bool initCellEntityFromStream( BinaryIStream & data );

	/**
	 *	This method is called when initialising the player from its base
	 *	entity, and the BASE_AND_CLIENT properties are streamed.
	 */
	virtual bool initBasePlayerFromStream( BinaryIStream & data ) = 0;

	/**
	 *	This method is called when initialising the player from its cell
	 *	entity, and OWN_CLIENT, OTHER_CLIENT and ALL_CLIENT properties are
	 *	streamed.
	 */
	virtual bool initCellPlayerFromStream( BinaryIStream & data ) = 0;

	/**
	 *	This method is called when restoring the player from its cell entity.
	 *
	 *	@param data	The BASE_AND_CLIENT; then OWN_CLIENT, OTHER_CLIENT and
	 *	ALL_CLIENT properties as per initBasePlayerFromStream and
	 *	initCellPlayerFromStream.
	 */
	virtual bool restorePlayerFromStream( BinaryIStream & data ) = 0;

	/**
	 *	This method is called when this entity becomes the player.
	 */
	virtual void onBecomePlayer() {};

	/**
	 *	This method is called when the entity becomes active to the system
	 */
	virtual void onEnterAoI( const EntityEntryBlocker & rBlocker ) {}

	/*
	 *	This method is called when the entity enters the client's world-space.
	 */
	virtual void onEnterWorld() {}

	/*
	 *	This method is called when a our position in the world is updated either
	 *	by a filter or directly by server input.
	 */
	virtual void onPositionUpdated() {};

	/*
	 *	This method is called when the entity changes which space it's in. At
	 *	this point, the space ID, vehicle and position have all been updated
	 *	for the new space.
	 */
	virtual void onChangeSpace() {}

	/*
	 *	This method is called when the entity leaves the client's world-space.
	 */
	virtual void onLeaveWorld() {}

	/**
	 *	This method is used to tell the Entity that it's no longer an active
	 *	part of the system.
	 */
	virtual void onLeaveAoI() {}

	/**
	 *	This method is called when this entity stops being the player.
	 */
	virtual void onBecomeNonPlayer() {}

	/**
	 *	This method is called when this entity is about to cease to exist, to
	 *	give subclasses a chance to clean up any pending reference counting or
	 *	similar.
	 */
	virtual void onDestroyed() {}

	/**
	 *	This method is called when this entity changes its state of being
	 *	server-controlled.
	 */
	virtual void onChangeControl( bool isControlling, bool isInitialising ) {};

	/**
	 *	This method is called when this entity receives a volatile update when
	 *	it has been receiving non-volatile updates or vice-versa.
	 *	If the change is due to the entity becoming server-controlled, then
	 *	onChangeControl will be called first. In both calls,
	 *	BWEntity::isReceivingVolatileUpdates() will be correct.
	 */
	virtual void onChangeReceivingVolatileUpdates() {}

	/// passenger management
	void onPassengerBoard( BWEntity * pEntity );
	void onPassengerAlight( BWEntity * pEntity );

	void onChangeVehicleID( EntityID newVehicleID );

	Passengers passengers_;

	void onMoveInternal( double time, const Position3D & position,
		EntityID vehicleID, SpaceID spaceID, const Direction3D & direction,
		const Vector3 & positionError, bool isReset );

	void populateMoveDefaults( Position3D & position, EntityID & vehicleID,
		SpaceID & spaceID, Direction3D & direction, Vector3 & positionError );


	EntityID 		entityID_;
	EntityTypeID	entityTypeID_;
	SpaceID			spaceID_;
	EntityID		vehicleID_;
	BWEntityPtr 	pVehicle_;

	BWConnection *	pBWConnection_;

	bool			isPlayer_;

	MovementFilter *	pFilter_;

	Position3D 		localPosition_;
	Direction3D 	localDirection_;

	bool			isInAoI_;
	bool			isInWorld_;
	bool 			isDestroyed_;
	bool			isControlled_;
	bool			isReceivingVolatileUpdates_;

	mutable double	physicsCorrectionAckTime_;
	double			latestLocalMoveTime_;
	bool			latestLocalMoveIs2D_;
};

BW_END_NAMESPACE

#endif // BW_ENTITY_HPP

