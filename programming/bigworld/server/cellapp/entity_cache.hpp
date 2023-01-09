#ifndef ENTITY_CACHE_HPP
#define ENTITY_CACHE_HPP

#include "aoi_update_schemes.hpp"

#include "cstdmf/binary_stream.hpp"
#include "network/msgtypes.hpp"
#include "cstdmf/smartpointer.hpp"
#include "entity.hpp"
#include "cellapp_config.hpp"

#include <limits.h>
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class Entity;
typedef ConstSmartPointer< Entity > EntityConstPtr;

namespace Mercury
{
	class Bundle;
}


const IDAlias NO_ID_ALIAS = 0xff;

/**
 *	This class is used by RealEntityWithWitnesses to cache information about
 *	other entities.
 */
class EntityCache
{
public:
	// TODO: Remove this restriction.
	static const int MAX_LOD_LEVELS = 4;

	typedef double Priority;

	EntityCache( const Entity * pEntity );

	INLINE void resetClientState();
	INLINE void onEntityRemovedFromClient();

	void updatePriority( const Vector3 & origin );
	float getLoDPriority( const Vector3 & origin ) const;

	bool updateDetailLevel( Mercury::Bundle & bundle, float lodPriority,
		bool hasSelectedEntity );
	void addOuterDetailLevel( BinaryOStream & stream );

	void addEntitySelectMessage( Mercury::Bundle & bundle ) const;

	void addLeaveAoIMessage( Mercury::Bundle & bundle, EntityID id ) const;

	void reuse();

	INLINE int numLoDLevels() const;
	INLINE static int numLoDLevels( const Entity & e );

	// Accessors

	EntityConstPtr pEntity() const	{ return pEntity_; }
	EntityConstPtr & pEntity()		{ return pEntity_; }

	// When offloading/onloading, vehicleChangeNum_ is changed to one of these
	// states.
	enum
	{
		VEHICLE_CHANGE_NUM_OLD,			// Cache not sync'ed with vehicle id.
		VEHICLE_CHANGE_NUM_HAS_VEHICLE,		// Cache sync'ed and has a vehicle
		VEHICLE_CHANGE_NUM_HAS_NO_VEHICLE	// Cache sync'ed and has no vehicle
	};
	typedef uint8 VehicleChangeNum;

	VehicleChangeNum vehicleChangeNum() const		{ return vehicleChangeNum_; }
	void vehicleChangeNum( VehicleChangeNum num )	{ vehicleChangeNum_ = num; }

	Priority priority() const;
	void priority( Priority newPriority );

	void lastEventNumber( EventNumber eventNumber );
	EventNumber lastEventNumber() const;

	void lastVolatileUpdateNumber( VolatileNumber number );
	VolatileNumber lastVolatileUpdateNumber() const;

	void detailLevel( DetailLevel detailLevel );
	DetailLevel detailLevel() const;

	IDAlias idAlias() const;
	void idAlias( IDAlias idAlias );

	bool isAlwaysDetailed() const;
	void isAlwaysDetailed( bool value );

	AoIUpdateSchemeID updateSchemeID() const { return updateSchemeID_; }
	void updateSchemeID( AoIUpdateSchemeID id ) { updateSchemeID_ = id; }

	void lodEventNumbers( EventNumber * pEventNumbers, int size );
	// EntityCache( const EntityCache & );

	void setEnterPending()		{ flags_ |= ENTER_PENDING; }
	void setRequestPending()	{ flags_ |= REQUEST_PENDING; }
	void setCreatePending()		{ flags_ |= CREATE_PENDING; }
	void setGone()				{ flags_ |= GONE; }
	void setWithheld()			{ flags_ |= WITHHELD; }
	void setRefresh()			{ flags_ |= REFRESH; }
	void setPrioritised()		{ flags_ |= PRIORITISED; }
	void setManuallyAdded()		{ flags_ |= MANUALLY_ADDED; }
	void setAddedByTrigger()	{ flags_ |= ADDED_BY_TRIGGER; }

	void clearEnterPending()		{ flags_ &= ~ENTER_PENDING; }
	void clearRequestPending()		{ flags_ &= ~REQUEST_PENDING; }
	void clearCreatePending()		{ flags_ &= ~CREATE_PENDING; }
	void clearGone()				{ flags_ &= ~GONE; }
	void clearWithheld()			{ flags_ &= ~WITHHELD; }
	void clearRefresh() 			{ flags_ &= ~REFRESH; }
	void clearPrioritised()			{ flags_ &= ~PRIORITISED; }
	void clearManuallyAdded()		{ flags_ &= ~MANUALLY_ADDED; }
	void clearAddedByTrigger()		{ flags_ &= ~ADDED_BY_TRIGGER; }

	bool isEnterPending() const		{ return (flags_ & ENTER_PENDING) != 0; }
	bool isRequestPending() const	{ return (flags_ & REQUEST_PENDING) != 0; }
	bool isCreatePending() const	{ return (flags_ & CREATE_PENDING) != 0; }
	bool isGone() const				{ return (flags_ & GONE) != 0; }
	bool isWithheld() const			{ return (flags_ & WITHHELD) != 0; }
	bool isRefresh() const 			{ return (flags_ & REFRESH) != 0; }
	bool isPrioritised() const		{ return (flags_ & PRIORITISED) != 0; }
	bool isUpdatable() const		{ return (flags_ & NOT_UPDATABLE) == 0; }
	bool isManuallyAdded() const	{ return (flags_ & MANUALLY_ADDED) != 0; }
	bool isAddedByTrigger() const	{ return (flags_ & ADDED_BY_TRIGGER) != 0; }

private:
	typedef uint16 Flags;

	enum	// Flags bits
	{
		ENTER_PENDING	= 1 << 0, ///< Waiting to send enterAoI to client
		REQUEST_PENDING	= 1 << 1, ///< Expecting requestEntityUpdate from client
		CREATE_PENDING	= 1 << 2, ///< Waiting to send createEntity to client
		GONE			= 1 << 3, ///< Waiting to remove from priority queue
		WITHHELD		= 1 << 4, ///< Do not send to client
		REFRESH			= 1 << 5, ///< Waiting to be removed and re-added to the AoI 
		PRIORITISED		= 1 << 6, ///< Should be sent in the next Witness::update()
		MANUALLY_ADDED	= 1 << 7, ///< Indicating the entity added manually to the AoI
		ADDED_BY_TRIGGER = 1 << 8, ///< Indicating the entity added automatically
								   ///< by the trigger to the AoI
		IS_ALWAYS_DETAILED 	= 1 << 9, ///< Indicating whether or not volatile position
									  ///< updates are detailed at all distances.

		/// If any of these are set, we shouldn't do a normal update in
		/// RealEntityWithWitnesses::update. (Actually, REQUEST_PENDING should
		/// never be set on something in the queue).
		NOT_UPDATABLE =
			ENTER_PENDING|REQUEST_PENDING|CREATE_PENDING|GONE|WITHHELD|REFRESH,

		// Those are client related state and should be reset when this entity
		// is removed from client.
		CLIENT_STATE = ENTER_PENDING|REQUEST_PENDING|CREATE_PENDING|GONE|REFRESH,
	};

	void lodEventNumber( int level, EventNumber eventNumber );
	EventNumber lodEventNumber( int level ) const;

	float getDistanceSquared( const Vector3 & origin ) const;

	EntityCache & operator=( const EntityCache & );

	bool addChangedProperties( BinaryOStream & stream,
		Mercury::Bundle * pBundleForHeader = NULL,
		bool shouldSelectEntity = false );

	EntityConstPtr	pEntity_;
	Flags			flags_;	// TODO: Not good structure packing.
	AoIUpdateSchemeID updateSchemeID_;

	VehicleChangeNum	vehicleChangeNum_;

	Priority	priority_;	// double
	Priority    lastPriorityDelta_; // double

	EventNumber		lastEventNumber_;			// int32
	VolatileNumber	lastVolatileUpdateNumber_;	// uint16
	DetailLevel		detailLevel_;				// uint8
	IDAlias			idAlias_;					// uint8

	EventNumber		lodEventNumbers_[ MAX_LOD_LEVELS ];		// int32 * num lod levels

	friend BinaryIStream & operator>>( BinaryIStream & stream,
			EntityCache & entityCache );
	friend BinaryOStream & operator<<( BinaryOStream & stream,
			const EntityCache & entityCache );
};

inline
bool operator<( const EntityCache & left, const EntityCache & right )
{
	return left.pEntity() < right.pEntity();
}


/**
 *	This class implements the interface used to visit all EntityCache instances
 *	in an AoI.
 */
class EntityCacheVisitor
{
public:
	virtual void visit( const EntityCache & cache ) = 0;
};


/**
 *	This class implements the interface used to visit all EntityCache instances
 *	in an AoI, potentially modifying each EntityCache in some way.
 */
class EntityCacheMutator
{
public:
	virtual void mutate( EntityCache & cache ) = 0;
};


/**
 *	This class is a map of entity caches
 */
class EntityCacheMap
{
public:
	~EntityCacheMap();

	EntityCache * add( const Entity & e );
	void del( EntityCache * ec );

	EntityCache * find( const Entity & e ) const;
	EntityCache * find( EntityID id ) const;

	uint32 size() const	{ return set_.size(); }

	void writeToStream( BinaryOStream & stream ) const;

	void visit( EntityCacheVisitor & visitor ) const;
	void mutate( EntityCacheMutator & mutator );

	static void addWatchers();

private:
	typedef BW::set< EntityCache > Implementor;

	Implementor set_;
};

#ifdef CODE_INLINE
#include "entity_cache.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_CACHE_HPP

