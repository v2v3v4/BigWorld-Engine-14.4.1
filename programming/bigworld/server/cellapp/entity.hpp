#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "Python.h"

#include "pyscript/pyobject_plus.hpp"

#include "cellapp_interface.hpp"
#include "controller.hpp"
#include "entity_type.hpp"
#include "history_event.hpp"
#include "entity_callback_buffer.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/time_queue.hpp"

#include "entitydef/mailbox_base.hpp"
#include "entitydef/property_event_stamps.hpp"
#include "entitydef/property_owner.hpp"
#include "entitydef/py_volatile_info.hpp"

#include "delegate_interface/entity_delegate.hpp"

#include "physics2/worldtri.hpp" // For WorldTriangle::Flags

#include "pyscript/script.hpp"

#include "server/backup_hash.hpp"
#include "server/entity_profiler.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

// Forward declarations
class Cell;
class CellApp;
class CellAppChannel;
class Chunk;
class ChunkSpace;
class Controllers;
class ControllersVisitor;
class Entity;
class EntityCache;
class EntityExtra;
class EntityExtraInfo;
class EntityRangeListNode;
class EntityType;
class MemoryOStream;
class RangeList;
class RangeTrigger;
class RealEntity;
class PropertyChange;
class Space;
class Watcher;

typedef SmartPointer< Watcher > WatcherPtr;

typedef uint8 AoIUpdateSchemeID;

typedef SmartPointer< Entity >                  EntityPtr;
typedef Entity						            BaseOrEntity;
typedef BW::set< EntityPtr >                    EntitySet;
typedef BW::map< EntityID, EntityPtr >          EntityMap;
class EntityPopulation;

// From "space.hpp"
typedef BW::vector< EntityPtr >				SpaceEntities;
typedef SpaceEntities::size_type				SpaceRemovalHandle;
const SpaceRemovalHandle NO_SPACE_REMOVAL_HANDLE = SpaceRemovalHandle( -1 );
typedef BW::list< RangeTrigger * > RangeTriggerList;


/**
 *	This interface is used to implement an object that wants to visit a set of
 *	entities.
 */
class EntityVisitor
{
public:
	virtual ~EntityVisitor() {};
	virtual void visit( Entity * pEntity ) = 0;
};


/*~ class BigWorld.Entity
 *  @components{ cell }
 *  Instances of the Entity class represent generic game objects on the cell.
 *  An Entity may be "real" or "ghosted", where a "ghost" Entity is a copy of
 *  of a "real" Entity that lives on an adjacent cell. For each entity there is
 *  one authoritative "real" Entity instance, and 0 or more "ghost" Entity
 *  instances which copy it.
 *
 *  An Entity instance handles the entity's positional data, including its
 *  space and rotation. It also controls how frequently (if ever) this data is
 *  sent to clients. Positional data can be altered by updates from an
 *  authoritative client, by controller objects, and by the teleport member
 *  function. Controllers are non-python objects that can be applied to
 *  cell entities to change their positional data over time. They are added
 *  to an Entity via member functions like "trackEntity" and "turnToYaw", and can
 *  be removed via "cancel".
 *
 *  Area of Interest, or "AoI" is an important concept for all BigWorld
 *  entities which belong to clients. The AoI of an entity is the area around
 *  it which the entity's client (if it has one) is aware of. This is used for
 *  culling the amount of data that the client is sent. The actual shape of an
 *  AoI is defined by a range of equal length on both the x and z axis, with a
 *  hysteresis area of similar shape extending out further. An Entity enters
 *  another Entity's AoI when it enters the AoI area, but doesn't leave it
 *  until it has moved outside the hysteresis area. An Entity can change its
 *  AoI size via "setAoIRadius".
 *  Entities within a particular distance can be found via "entitiesInRange",
 *  and changes to the set of entities within a given range can be observed via
 *  "addProximity".
 *
 *  The basis for a stealth system resides in Entity with noise event
 *  being handled by "makeNoise", and a movement noise handling system
 *  being accessible through "getStealthFactor" and "setStealthFactor".
 *
 *	A new Entity on a cellApp can be created using BigWorld.createEntity or
 *	BigWorld.createEntityFromFile function. An entity could also be spawned
 *	from remote baseApp BigWorld.createCellEntity function call.
 *
 *  An Entity can access the equivalent entity on base and client applications
 *  via a BaseEntityMailbox and a series of PyClient objects. These allow
 *  a set of function calls (which are specified in the entity's .def file) to
 *  be called remotely.
 */
/**
 *	Instances of this class are used to represent a generic game object on the
 *	cell. An entity may be <i>real</i> or <i>ghosted</i>. A <i>ghost</i> entity
 *	is an entity that is a copy of a <i>real</i> entity that lives on an
 *	adjacent cell.
 */
class Entity : public PyObjectPlus
{
	friend class BuildEntityTypeDict;

	Py_Header( Entity, PyObjectPlus )

public:
	static const EntityPopulation & population()	{ return population_; }
	static void addWatchers();

	// Preventing NaN's getting through, hopefully
	static bool isValidPosition( const Position3D &c )
	{
		const float MAX_ENTITY_POS = 1000000000.f;
		return (-MAX_ENTITY_POS < c.x && c.x < MAX_ENTITY_POS &&
			-MAX_ENTITY_POS < c.y && c.y < MAX_ENTITY_POS &&
			-MAX_ENTITY_POS < c.z && c.z < MAX_ENTITY_POS);
	}

	/// @name Construction and Destruction
	//@{
	Entity( EntityTypePtr pEntityType );
	void setToInitialState( EntityID id, Space * pSpace );
	~Entity();

	bool initReal( BinaryIStream & data, const ScriptDict & properties,
		bool isRestore,
		Mercury::ChannelVersion channelVersion,
	   	EntityPtr pNearbyEntity );

	void initGhost( BinaryIStream & data );

	bool isOffloadingTo( const Mercury::Address & addr ) const;

	void offload( CellAppChannel * pChannel, bool isTeleport );

	void onload( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void createGhost( Mercury::Bundle & bundle );
	//@}

	void callback( const char * methodName );

	/// @name Accessors
	//@{
	EntityID id() const;
	void setShouldReturnID( bool shouldReturnID );
	const Position3D & position() const;
	const Direction3D & direction() const;

	const VolatileInfo & volatileInfo() const;

	bool isReal() const;
	bool isRealToScript() const;
	RealEntity * pReal() const;

	const Mercury::Address & realAddr() const;
	const Mercury::Address & nextRealAddr() const { return nextRealAddr_; }

	CellAppChannel * pRealChannel() { return pRealChannel_; }
	void pRealChannel( CellAppChannel * channel );

	bool checkIfZombied( const Mercury::Address & dyingAddr );

	Space & space();
	const Space & space() const;

	Cell & cell();
	const Cell & cell() const;

	EventHistory & eventHistory();
	const EventHistory & eventHistory() const;

	bool isDestroyed() const;
	bool inDestroy() const		{ return inDestroy_; }
	void destroy();

	EntityTypeID entityTypeID() const;
	EntityTypeID clientTypeID() const;

	VolatileNumber volatileUpdateNumber() const
											{ return volatileUpdateNumber_; }

	float topSpeed() const				{ return topSpeed_; }
	float topSpeedY() const				{ return topSpeedY_; }

	EntityRangeListNode * pRangeListNode() const;

	ChunkSpace * pChunkSpace() const;

	AoIUpdateSchemeID aoiUpdateSchemeID() const	{ return aoiUpdateSchemeID_; }

	//@}

	void incRef() const;
	void decRef() const;

	void addToRangeList( RangeList & rangeList,
		RangeTriggerList & appealRadiusList );
	void removeFromRangeList( RangeList & rangeList,
		RangeTriggerList & appealRadiusList );

	HistoryEvent * addHistoryEventLocally( uint8 type,
		MemoryOStream & stream,
		const MemberDescription & description,
		int16 msgStreamSize,
		HistoryEvent::Level level = FLT_MAX );

	bool writeClientUpdateDataToBundle( Mercury::Bundle & bundle,
		const Vector3 & basePos,
		EntityCache & cache,
		float lodPriority ) const;

	void writeVehicleChangeToBundle( Mercury::Bundle & bundle,
		EntityCache & cache ) const;

	void writeVolatileDetailedDataToBundle( Mercury::Bundle & bundle,
			IDAlias idAlias, bool isReliable ) const;

	static void forwardMessageToReal( CellAppChannel & realChannel,
		EntityID entityID,
		uint8 messageID,
		BinaryIStream & data,
		const Mercury::Address & srcAddr, Mercury::ReplyID replyID );

	bool sendMessageToReal( const MethodDescription * pDescription,
		ScriptTuple args );

	bool shouldBufferMessagesFrom( const Mercury::Address & addr ) const;

	void trimEventHistory( GameTime cleanUpTime );

	void setPositionAndDirection( const Position3D & position,
		const Direction3D & direction );
	void setAndDropGlobalPositionAndDirection( const Position3D & position,
		const Direction3D & direction );

	// DEBUG
	int numHaunts() const;

	INLINE EntityTypePtr pType() const;

	void reloadScript();	///< deprecated
	bool migrate();
	void migratedAll();

	/// @name Message handlers
	//@{
	void avatarUpdateImplicit(
		const CellAppInterface::avatarUpdateImplicitArgs & args );
	void avatarUpdateExplicit(
		const CellAppInterface::avatarUpdateExplicitArgs & args );
	void ackPhysicsCorrection();

	void ghostPositionUpdate(
		const CellAppInterface::ghostPositionUpdateArgs & args );
	void ghostHistoryEvent( BinaryIStream & data );
	void ghostedDataUpdate( BinaryIStream & data );
	void ghostSetReal( const CellAppInterface::ghostSetRealArgs & args );
	void ghostSetNextReal(
		const CellAppInterface::ghostSetNextRealArgs & args );
	void delGhost();

	void ghostVolatileInfo(
		const CellAppInterface::ghostVolatileInfoArgs & args );
	void ghostControllerCreate( BinaryIStream & data );
	void ghostControllerDelete( BinaryIStream & data );
	void ghostControllerUpdate( BinaryIStream & data );

	void witnessed();
	void checkGhostWitnessed();


	void aoiUpdateSchemeChange(
			const CellAppInterface::aoiUpdateSchemeChangeArgs & args );

	void delControlledBy( const CellAppInterface::delControlledByArgs & args );

	void forwardedBaseEntityPacket( BinaryIStream & data );

	void onBaseOffloaded( const CellAppInterface::onBaseOffloadedArgs & args );

	void onBaseOffloadedForGhost(
		const CellAppInterface::onBaseOffloadedForGhostArgs & args );

	void teleport( const CellAppInterface::teleportArgs & args );

	void onTeleportSuccess( Entity * pNearbyEntity );

	void addDetailedPositionToHistory( bool isLocalOnly );

	void enableWitness( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void witnessCapacity( const CellAppInterface::witnessCapacityArgs & args );

	void requestEntityUpdate( BinaryIStream & data );

	void writeToDBRequest( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & stream );

	void destroyEntity( const CellAppInterface::destroyEntityArgs & args );

	void runScriptMethod( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void callBaseMethod( BinaryIStream & data );
	void callClientMethod( BinaryIStream & data );
	void recordClientMethod( BinaryIStream & data );
	void recordClientProperties( BinaryIStream & data );
	const ScriptDict & exposedForReplayClientProperties() const
	{
		return exposedForReplayClientProperties_;
	}

	// General (script) message handler
	void runExposedMethod( BinaryIStream & data );
	//@}

	/// @name Script related methods
	//@{
	PY_METHOD_DECLARE( py_destroy )

	PY_METHOD_DECLARE( py_cancel )
	PY_METHOD_DECLARE( py_isReal )
	PY_METHOD_DECLARE( py_isRealToScript )
	PY_METHOD_DECLARE( py_clientEntity )
	PY_METHOD_DECLARE( py_debug )

	PY_PICKLING_METHOD_DECLARE( MailBox )

	PY_AUTO_METHOD_DECLARE( RETOWN, getComponent, ARG( BW::string, END ) );
	PyObject * getComponent( const BW::string & name );
	
	PY_AUTO_METHOD_DECLARE( RETOK, destroySpace, END );
	bool destroySpace();

	PY_AUTO_METHOD_DECLARE( RETOK, writeToDB, END );
	bool writeToDB();

	PY_AUTO_METHOD_DECLARE( RETOWN, entitiesInRange, ARG( float,
								OPTARG( ScriptObject, ScriptObject(),
								OPTARG( ScriptObject, ScriptObject(), END ) ) ) )

	PyObject * entitiesInRange( float range,
			ScriptObject pClass = ScriptObject(),
			ScriptObject pActualPos = ScriptObject() );


	bool outdoorPropagateNoise( float range,
								int event,
							   	int info);

	PY_AUTO_METHOD_DECLARE( RETOK,
			makeNoise, ARG( float, ARG( int, OPTARG( int, 0, END ) ) ) )
	bool makeNoise( float noiseLevel, int event, int info=0);

	PY_AUTO_METHOD_DECLARE( RETDATA, getGroundPosition, END )
	const Position3D getGroundPosition( ) const;

	PY_RO_ATTRIBUTE_DECLARE( periodsWithoutWitness_, periodsWithoutWitness )
	PY_RO_ATTRIBUTE_DECLARE( pType()->name(), className );
	PY_RO_ATTRIBUTE_DECLARE( id_, id )

	PY_RO_ATTRIBUTE_DECLARE( isDestroyed_, isDestroyed )

	PY_RO_ATTRIBUTE_DECLARE( numTimesRealOffloaded_,
			debug_numTimesRealOffloaded )

	PyObject * pyGet_spaceID();
	PY_RO_ATTRIBUTE_SET( spaceID )

	SpaceID spaceID() const;

	// PY_READABLE_ATTRIBUTE_GET( globalPosition_, position )
	PyObject * pyGet_position();

	int pySet_position( PyObject * value );

	PyObject * pyGet_direction();
	int pySet_direction( PyObject * value );

	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.yaw, yaw )
	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.pitch, pitch )
	PY_RO_ATTRIBUTE_DECLARE( globalDirection_.roll, roll )

	PY_RO_ATTRIBUTE_DECLARE( (Vector3 &)localPosition_, localPosition );
	PY_RO_ATTRIBUTE_DECLARE( localDirection_.yaw, localYaw );
	PY_RO_ATTRIBUTE_DECLARE( localDirection_.pitch, localPitch );
	PY_RO_ATTRIBUTE_DECLARE( localDirection_.roll, localRoll );

	PY_RO_ATTRIBUTE_DECLARE( pVehicle_, vehicle );

	bool isOutdoors() const;
	bool isIndoors() const;

	PY_RO_ATTRIBUTE_DECLARE( isOutdoors(), isOutdoors )
	PY_RO_ATTRIBUTE_DECLARE( isIndoors(), isIndoors )

	PY_READABLE_ATTRIBUTE_GET( volatileInfo_, volatileInfo )
	int pySet_volatileInfo( PyObject * value );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, isOnGround, isOnGround )

	PyObject * pyGet_velocity();
	PY_RO_ATTRIBUTE_SET( velocity )

	PY_RW_ATTRIBUTE_DECLARE( topSpeed_, topSpeed )
	PY_RW_ATTRIBUTE_DECLARE( topSpeedY_, topSpeedY )

	PyObject * pyGet_aoiUpdateScheme();
	int pySet_aoiUpdateScheme( PyObject * value );

	PyObject * trackEntity( int entityId, float velocity = 2*MATH_PI,
			   int period = 10, int userArg = 0 );
	PY_AUTO_METHOD_DECLARE( RETOWN, trackEntity,
		ARG( int, OPTARG( float, 2*MATH_PI,
				OPTARG( int, 5, OPTARG( int, 0,  END ) ) ) ) )

	bool setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags );
	PY_AUTO_METHOD_DECLARE( RETOK, setPortalState,
			ARG( bool, OPTARG( WorldTriangle::Flags, 0, END ) ) )

	PyObject * getDict();
	PY_AUTO_METHOD_DECLARE( RETOWN, getDict, END )

	//@}

	bool sendToClient( EntityID entityID, const MethodDescription & description,
			MemoryOStream & argStream, 
			bool isForOwn = true, bool isForOthers = false )
	{
		return this->sendToClient( entityID, description, argStream, isForOwn, 
			isForOthers,
			/* isExposedForReplay */ isForOthers );
	}

	bool sendToClient( EntityID entityID, const MethodDescription & description,
			MemoryOStream & argStream, 
			bool isForOwn, bool isForOthers,
			bool isExposedForReplay );

	bool sendToClientViaReal( EntityID entityID, 
			const MethodDescription & description,
			MemoryOStream & argStream, 
			bool isForOwn = true, bool isForOthers = false )
	{
		return this->sendToClientViaReal( entityID, description, argStream, 
			isForOwn, isForOthers,
			/* isExposedForReplay */ isForOthers );
	}

	bool sendToClientViaReal( EntityID entityID,
			const MethodDescription & description, MemoryOStream & argStream,
			bool isForOwn, bool isForOthers,
			bool isExposedForReplay );


	// The following is used by:
	//	void Space::addEntity( Entity * );
	//	void Space::removeEntity( Entity * );
	// It makes removing entities efficient.
	SpaceRemovalHandle removalHandle() const		{ return removalHandle_; }
	void removalHandle( SpaceRemovalHandle handle )	{ removalHandle_ = handle; }

	// This is just used in the Witness constructor.
	bool isInAoIOffload() const;
	void isInAoIOffload( bool isInAoIOffload );

	INLINE bool isOnGround() const;
	void isOnGround( bool isOnGround );

	static WatcherPtr pWatcher();

	static const Vector3 INVALID_POSITION;

	const Position3D & localPosition() const		{ return localPosition_; }
	const Direction3D & localDirection() const		{ return localDirection_; }

	void setLocalPositionAndDirection( const Position3D & localPosition,
			const Direction3D & localDirection );

	void setGlobalPositionAndDirection( const Position3D & globalPosition,
			const Direction3D & globalDirection );

	Entity * pVehicle() const;
	uint8 vehicleChangeNum() const;
	EntityID vehicleID() const	{ return pVehicle_ ? pVehicle_->id() : 0; }

	typedef uintptr SetVehicleParam;
	enum SetVehicleParamEnum
	{
		KEEP_LOCAL_POSITION,
		KEEP_GLOBAL_POSITION,
		IN_LIMBO
	};

	void setVehicle( Entity * pVehicle, SetVehicleParam keepWho );
	void onVehicleMove();

	NumTimesRealOffloadedType numTimesRealOffloaded() const
									{ return numTimesRealOffloaded_; }

	// void setClass( PyObject * pClass );

	EventNumber lastEventNumber() const;
	EventNumber getNextEventNumber();

	const PropertyEventStamps & propertyEventStamps() const;

	void debugDump();

	void fakeID( EntityID id );

	void addTrigger( RangeTrigger * pTrigger );
	void modTrigger( RangeTrigger * pTrigger );
	void delTrigger( RangeTrigger * pTrigger );

	bool hasBase() const { return baseAddr_.ip != 0; }
	const Mercury::Address & baseAddr() const { return baseAddr_; }

	void adjustForDeadBaseApp( const BackupHash & backupHash );

	void informBaseOfAddress( const Mercury::Address & addr, SpaceID spaceID,
		   bool shouldSendNow );

	/// @name PropertyOwnerLink method implementations
	//@{
	bool onOwnedPropertyChanged( PropertyChange & change );

	bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner );

	int getNumOwnedProperties() const;
	PropertyOwnerBase * getChildPropertyOwner( int ref ) const;
	ScriptObject setOwnedProperty( int ref, BinaryIStream & data );
	//@}
	
	bool onOwnedPropertyChanged( const DataDescription * pDescription,
		PropertyChange & change );

	ScriptObject propertyByDataDescription( 
			const DataDescription * pDataDescr ) const;

	Chunk * pChunk() const { return pChunk_; };
	Entity * prevInChunk() const { return pPrevInChunk_;}
	Entity * nextInChunk() const { return pNextInChunk_;}
	void prevInChunk( Entity* pEntity ) { pPrevInChunk_ = pEntity; }
	void nextInChunk( Entity* pEntity ) { pNextInChunk_ = pEntity; }
	void removedFromChunk();

	void heardNoise( const Entity * who, float propRange, float distance,
						int event, int info );

	ControllerID 	addController( ControllerPtr pController, int userArg );
	void			modController( ControllerPtr pController );
	bool			delController( ControllerID controllerID,
						bool warnOnFailure = true );

	bool			visitControllers( ControllersVisitor & visitor );

	static int registerEntityExtra(
		PyMethodDef * pMethods = NULL, PyGetSetDef * pAttributes = NULL );
	EntityExtra * & entityExtra( int eeid )		{ return extras_[eeid]; }
	EntityExtra * entityExtra( int eeid ) const	{ return extras_[eeid]; }

	void checkChunkCrossing();

	void relocated();

	void destroyZombie();

	bool callback( const char * funcName, PyObject * args,
		const char * errorPrefix, bool okIfFunctionNull );
	
	// Create a dictionary 
	ScriptDict createDictWithProperties( int dataDomains ) const;
	ScriptDict createDictWithComponentProperties( int dataDomains ) const;
	ScriptDict createDictWithLocalProperties( int dataDomains ) const;

	// profiling
	const EntityProfiler & profiler() const { return profiler_; }
	EntityProfiler & profiler() { return profiler_; }

	static void callbacksPermitted( bool permitted );
	static bool callbacksPermitted() { return s_callbackBuffer_.isBuffering(); }

	static void nominateRealEntity( Entity & e );
	static void nominateRealEntityPop();

	static void s_init();

	const IEntityDelegatePtr & pEntityDelegate() const 
	{ 
		return pEntityDelegate_; 
	}

private:
	// Private methods
	Entity( const Entity & );

	void updateLocalPosition();
	bool updateGlobalPosition( bool shouldUpdateGhosts = true,
		bool isVehicleMovement = false );

	void updateInternalsForNewPositionOfReal( const Vector3 & oldPos, 
		bool isVehicleMovement = false );
	void updateInternalsForNewPosition( const Vector3 & oldPosition,
		bool isVehicleMovement = false );

	bool getEntitiesInRange( EntityVisitor & visitor, float range,
			ScriptObject pClass = ScriptObject(),
			ScriptObject pActualPos = ScriptObject() );

	void callScriptInit( bool isRestore, EntityPtr pNearbyEntity );

	void clearPythonProperties();

	bool readRealDataInEntityFromStreamForInitOrRestore( BinaryIStream & data,
		const ScriptDict & properties );

	void readGhostDataFromStream( BinaryIStream & data );
	void writeGhostDataToStream( BinaryOStream & stream ) const;

	void readGhostDataFromStreamInternal( BinaryIStream & data );
	void writeGhostDataToStreamInternal( BinaryOStream & stream ) const;

	void convertRealToGhost( BinaryOStream * pStream = NULL,
			CellAppChannel * pChannel = NULL,
			bool isTeleport = false );

	// Used by egextra/debug_extension.cpp
	friend PyObject * calcOffloadData( ScriptObject pEnt );
	void writeRealDataToStream( BinaryOStream & data,
			bool isTeleport ) const;

	void convertGhostToReal( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr = NULL );

	void readRealDataFromStreamForOnload( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr = NULL );

	void readRealDataFromStreamForOnloadInternal( BinaryIStream & data,
			const Mercury::Address * pBadHauntAddr );
	void writeRealDataToStreamInternal( BinaryOStream & data,
					bool isTeleport ) const;

	void setGlobalPosition( const Vector3 & v );
	void setGlobalDirection( const Vector3 & v );

	void avatarUpdateCommon( const Position3D & pos, const YawPitchRoll & dir,
		bool onGround, uint8 refNum );

	void setVolatileInfo( const VolatileInfo & newInfo );

	bool writeVolatileDataToBundle( Mercury::Bundle & bundle,
			const Vector3 & basePos, IDAlias idAlias,
			float priorityThreshold, bool isReliable ) const;

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	void pyAdditionalMembers( const ScriptList & pList ) const;
	void pyAdditionalMethods( const ScriptList & pList ) const;

	bool writeCellMessageToBundle( Mercury::Bundle & bundle,
		const MethodDescription * pDescription,
		ScriptTuple args ) const;

	bool writeClientMessageToBundle( Mercury::Bundle & bundle,
		EntityID entityID, const MethodDescription & description,
		MemoryOStream & argstream, int callingMode ) const;

	bool physicallyPossible( const Position3D & newPosition, Entity * pVehicle,
		float propMove = 1.f );

	bool traverseChunks( Chunk * pCurChunk,
		const Chunk * pDstChunk,
		Vector3 cSrcPos, Vector3 cDstPos,
		BW::vector< Chunk * > & visitedChunks );

	bool validateAvatarVehicleUpdate( Entity * pNewVehicle );

	void readGhostControllersFromStream( BinaryIStream & data );
	void writeGhostControllersToStream( BinaryOStream & stream ) const;

	void readRealControllersFromStream( BinaryIStream & data );
	void writeRealControllersToStream( BinaryOStream & stream ) const;

	bool readBasePropertiesExposedForReplayFromStream( BinaryIStream & data );
	void writeBasePropertiesExposedForReplayToStream(
		BinaryOStream & data ) const;

	void startRealControllers();
	void stopRealControllers();

	void runMethodHelper( BinaryIStream & data, int methodID, bool isExposed,
		   int replyID = -1, const Mercury::Address * pReplyAddr = NULL );

	bool sendDBDataToBase( const Mercury::Address * pReplyAddr = NULL,
		Mercury::ReplyID replyID = 0 );

	bool sendCellEntityLostToBase();

	Mercury::MessageID addChangeToExternalStream(
		const PropertyChange & change, BinaryOStream & stream,
		const DataDescription & dataDesciption, int * pStreamSize ) const;

	bool hasVolatilePosition() const;

	void setDestroyed();

	void createReal();
	void destroyReal();
	void offloadReal();

	void createEntityDelegate();
	void createEntityDelegateWithTemplate( const BW::string & templateID );
	void onPositionChanged();

	static const Mercury::InterfaceElement * getAvatarUpdateMessage(
			int index );

	IEntityDelegatePtr pEntityDelegate_;

	// Private data
	Space *				pSpace_;

	// This handle is used to help the speed of Space::removeEntity.
	SpaceRemovalHandle	removalHandle_;

	EntityID		id_;
	EntityTypePtr	pEntityType_;
	Position3D		globalPosition_;
	Direction3D		globalDirection_;

	Position3D		localPosition_;
	Direction3D		localDirection_;

	Mercury::Address baseAddr_;

	Entity *		pVehicle_;
	uint8			vehicleChangeNum_;

	// Default scheme to use
	AoIUpdateSchemeID aoiUpdateSchemeID_; // uint8

	NumTimesRealOffloadedType numTimesRealOffloaded_; // uint16

	CellAppChannel * pRealChannel_;
	Mercury::Address nextRealAddr_;

	RealEntity * pReal_;

	typedef BW::vector<ScriptObject>	Properties;
	Properties	properties_;

	PropertyOwnerLink<Entity>	propertyOwner_;

	EventHistory eventHistory_;

	bool		isDestroyed_;
	bool		inDestroy_;
	bool		isInAoIOffload_;
	bool		isOnGround_;

	VolatileInfo	volatileInfo_;
	VolatileNumber	volatileUpdateNumber_;

	float		topSpeed_;
	float		topSpeedY_;
	uint16		physicsCorrections_;
	uint64		physicsLastValidated_;
	float		physicsNetworkJitterDebt_;

	static		uint16 s_physicsCorrectionsOutstandingWarningLevel;

	PropertyEventStamps	propertyEventStamps_;

	EventNumber lastEventNumber_;

	EntityRangeListNode *	pRangeListNode_;
	RangeTrigger *			pRangeListAppealTrigger_;

	Controllers *			pControllers_;

	bool shouldReturnID_;

	EntityExtra ** extras_;
	static BW::vector< EntityExtraInfo * > & s_entityExtraInfo();

	typedef BW::vector< RangeTrigger * > Triggers;
	Triggers triggers_;

	ScriptDict exposedForReplayClientProperties_;

	// Used when deciding to call onWitnessed.
	// If periodsWithoutWitness_ is this value, the entity is considered not to
	// be witnessed. If periodsWithoutWitness_ gets to 2, the real entity is not
	// witnessed. If it gets to 3, neither the real or its ghosts are being
	// witnessed.
	enum { NOT_WITNESSED_THRESHOLD = 3 };
	mutable int		periodsWithoutWitness_;

	Chunk* pChunk_;
	Entity* pPrevInChunk_;
	Entity* pNextInChunk_;

	// profiling
	EntityProfiler profiler_;

	static EntityPopulation population_;

	friend class RealEntity;
	// friend void EntityRangeListNode::remove();
	friend class EntityRangeListNode;

	static EntityCallbackBuffer s_callbackBuffer_;
};


typedef bool (*CustomPhysicsValidator)( Entity * pEntity,
	const Vector3 & newLocalPos, Entity * pNewVehicle,
	double physValidateTimeDelta );
/**
 *	This function pointer can be set to a function to do further validation
 *	of physical movement than that provided by the BigWorld core code.
 *	The function is called after the speed has been validated, but before
 *	chunk portal traversals have been examined.
 *	It should return true if the move is valid. If it returns false, then
 *	a physics correction will be sent to the client controlling the entity.
 *	(note that even if it returns true, the move may still be disallowed
 *	by the following chunk portal traversal checks)
 */
extern CustomPhysicsValidator g_customPhysicsValidator;


typedef void (*EntityMovementCallback)( const Vector3 & oldPosition,
		Entity * pEntity );
/**
 *	This function pointer can be set to a function that is called whenever an
 *	entity moves. This can be useful when implementing things like custom range
 *	triggers or velocity properties via EntityExtra and Controller classes.
 */
extern EntityMovementCallback g_entityMovementCallback;

#ifdef CODE_INLINE
#include "entity.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_HPP
