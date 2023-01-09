#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "filter.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"

#include "moo/moo_math.hpp"

#include "network/basictypes.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bgtask_manager.hpp"

#include "script/script_object.hpp"

#include "entity_type.hpp"	// for the pClass() accessor

#include "duplo/pymodel.hpp"
#include "duplo/py_attachment.hpp"
#include "duplo/chunk_embodiment.hpp"

#include "connection/movement_filter_target.hpp"

#include "connection_model/bw_entity.hpp"
#include "connection_model/entity_entry_blocking_condition.hpp"

#include "chunk/chunk_space.hpp"
#include "space/forward_declarations.hpp"
#include "space/entity_embodiment.hpp"

BW_BEGIN_NAMESPACE

class Matrix;
class DataDescription;
class ChunkSpace;
class Physics;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;

typedef SmartPointer<Filter> FilterPtr;

#ifndef PyModel_CONVERTERS
PY_SCRIPT_CONVERTERS_DECLARE( PyModel )
#define PyModel_CONVERTERS
#endif

class ResourceRef;
typedef BW::vector<ResourceRef> ResourceRefs;

class Entity;
typedef SmartPointer<Entity> EntityPtr;

class PyEntity;
typedef ScriptObjectPtr< PyEntity > PyEntityPtr;


/**
 *	This helper class encapsulates the prerequisites of an entity instance.
 */
class PrerequisitesOrder : public BackgroundTask
{
public:
	PrerequisitesOrder( const BW::vector<BW::string> & resourceIDs,
			const ResourceRefs & resourceRefs );

	bool loaded() const	{ return loaded_; };

	const ResourceRefs & resourceRefs() const { return resourceRefs_; }


protected:
	virtual void doBackgroundTask( TaskManager & mgr );

private:
	bool		loaded_;

	BW::vector<BW::string>	resourceIDs_;
	ResourceRefs				resourceRefs_;
};

typedef SmartPointer< PrerequisitesOrder > PrerequisitesOrderPtr;


/*~ class BigWorld.Entity
 *
 *	This is the Client component of the Distributed Entity Model. 
 *	It is responsible for handling the visual aspects of the Entity,
 *	updates from the server, etc...
 *
 *	It is possible to create a client-only Entity using
 *	BigWorld.createEntity function.
 */
/**
 *	Instances of this class are client representation of entities,
 *	the basic addressable object in the BigWorld system.
 */
class Entity : public BWEntity
{
public:
	Entity( EntityType & type, BWConnection * pBWConnection );
	~Entity();

	/* Entry and exit */
	bool checkPrerequisites();
	void onEnterWorld();
	void onPositionUpdated();
	void onChangeSpace();
	void onLeaveWorld();
	void onDestroyed();

	bool loadingPrerequisites() const { return loadingPrerequisites_; }
	void abortLoadingPrerequisites();

	/* Regular timekeeping */
	void tick( double timeNow, double timeLast );

	/* Script stuff */
	PyEntityPtr pPyEntity() const { return pPyEntity_; }

	void addModel( const IEntityEmbodimentPtr& pCA );
	bool delModel( PyObjectPtr pEmbodimentPyObject );
	PyObject * addTrap( float radius, ScriptObject callback );
	bool delTrap( int trapID );
	bool setInvisible( bool invis = true );
	void setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags );

	void onProperty( int propertyID, BinaryIStream & data,
		bool isInitialising );
	void onMethod( int methodID, BinaryIStream & data );

	void onNestedProperty( BinaryIStream & data, bool isSlice,
		bool isInitialising );

	int getMethodStreamSize( int methodID ) const;
	int getPropertyStreamSize( int propertyID ) const;

	const BW::string entityTypeName() const;

	/* Targeting methods */
	bool isATarget() const;
	void targetFocus();
	void targetBlur();

	/* Accessors */
	Vector3 & localVelocity()				{ return localVelocity_; }
	const Vector3 & localVelocity() const	{ return localVelocity_; }
	void localVelocity( const Vector3 & vel ) { localVelocity_ = vel; }

	EntityType & type()					{ return type_; }
	const EntityType & type() const		{ return type_; }

	IEntityEmbodiment * pPrimaryEmbodiment() const
		{ return primaryEmbodiment_.get(); }
	void pPrimaryEmbodiment( const IEntityEmbodimentPtr& pNewEmbodiment );
	PyModel * pPrimaryModel() const;

	EntityEmbodiments & auxiliaryEmbodiments()	{ return auxiliaryEmbodiments_;}

	ClientSpacePtr pSpace() const;

	Entity * pVehicle() const  /* override */
	{
		return static_cast< Entity * >( this->BWEntity::pVehicle() );
	}

	Physics * pPhysics() const			{ return pPhysics_ != (Physics*)-1 ?
											pPhysics_ : NULL; }
	void setPhysics( int * pStyle );

	void physicsMovement( double time, const Position3D & position,
		EntityID vehicleID, const Direction3D & direction );

	bool hasTargetCap( uint i ) const	{ return targetCapabilities_.has( i ); }
	const Capabilities & targetCaps() const		{ return targetCapabilities_; }
	Capabilities & targetCaps() 				{ return targetCapabilities_; }

	bool	targetFullBounds() const { return targetFullBounds_; }
	bool &	targetFullBounds()		 { return targetFullBounds_; }

	/* Miscellaneous */
	const Matrix & fallbackTransform() const;

#if ENABLE_WATCHERS
	static Watcher & watcher();
#endif

	typedef BW::set<Entity*> Census;
	static Census	census_;

	void onChangeControl( bool isControlling, bool isInitialising );
	void onChangeReceivingVolatileUpdates();

	// True if we are in-world on a vehicle, but that vehicle isn't in-world
	// right now.
	// TODO: Get rid of this
	bool waitingForVehicle() const
	{
		return this->vehicleID() != NULL_ENTITY_ID && this->pVehicle() == NULL;
	}

	bool isPhysicsAllowed() const;

	float transparency() const;
	bool  isInvisible() const { return invisible_; }

	bool tickCalled() const { return tickAdvanced_; }
	void setTickAdvanced() { tickAdvanced_ = true; }

	/**
	 *	Single entry in positional-sorted-linked list of entities.
	 */
	class Neighbour
	{
	public:
		void operator++(int)
		{
			BW_GUARD;
			this->nextInterestingEntity();
		}

		Entity * operator->()
		{
			return pNext_;
		}

		bool operator ==( const Neighbour & n )
		{
			return n.pSource_ == pSource_ && n.radius_ == radius_ &&
				n.chain_ == chain_;
		}

		bool operator !=( const Neighbour & n )
		{
			return !this->operator==( n );
		}

	private:
		Neighbour( Entity * pSource, float radius, bool end );

		void nextInterestingEntity();

		Entity		* pSource_;
		float		radius_;

		int			chain_;
		Entity		* pNext_;

		friend class Entity;
	};

	Neighbour	begin( float radius );
	Neighbour	end( float radius );

	friend class Neighbour;

private:
	bool loadingPrerequisites_;

	bool initCellEntityFromStream( BinaryIStream & data );
	bool initBasePlayerFromStream( BinaryIStream & data );
	bool initCellPlayerFromStream( BinaryIStream & data );
	bool restorePlayerFromStream( BinaryIStream & data );

	void onBecomePlayer();
	void onBecomeNonPlayer();
	void onEnterAoI( const EntityEntryBlocker & rBlocker );

	void onEnterSpace( bool transient );
	void onLeaveSpace( bool transient );

	PyEntityPtr pPyEntity_;

	Vector3		localVelocity_;

	EntityType & type_;

	IEntityEmbodimentPtr	primaryEmbodiment_;

	EntityEmbodiments	auxiliaryEmbodiments_;

	ClientSpacePtr		pSpace_;

	Physics		* pPhysics_;
	bool		tickAdvanced_;

	Capabilities	targetCapabilities_;
	//Capabilities	matchCapabilities_;

	int			nextTrapID_;
	struct TrapRecord
	{
		int			id;
		int			num;
		float		radiusSquared;
		PyObject	* function;
	};
	BW::vector<TrapRecord>	traps_;

	Entity		* nextLeft_;
	Entity		* nextRight_;
	Entity		* nextUp_;
	Entity		* nextDown_;
	void		shuffle();
	Entity		* nextOfChain( int chain );

	PrerequisitesOrderPtr		prerequisitesOrder_;
	ResourceRefs				resourceRefs_;
	EntityEntryBlockingCondition condition_;

	bool		targetFullBounds_;

	bool invisible_;
	double visibilityTime_;
};

#ifdef CODE_INLINE
#include "entity.ipp"
#endif

BW_END_NAMESPACE

#endif

/* entity.hpp */
