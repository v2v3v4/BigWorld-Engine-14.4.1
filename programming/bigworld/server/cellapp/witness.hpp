#ifndef WITNESS_HPP
#define WITNESS_HPP

#include "real_entity.hpp"
#include "entity_cache.hpp"

#include "network/msgtypes.hpp"

namespace Mercury
{
	class Bundle;
} // end namespace Mercury


BW_BEGIN_NAMESPACE

class AoITrigger;
class RangeListNode;


/**
 *	This class is a witness to the movements and perceptions of a RealEntity.
 *	It is created when a client is attached to this entity. Its main activity
 *	centres around the management of an Area of Interest list.
 */
class Witness : public Updatable
{
	static Witness * getSelf( PyObject * self )
	{
		Entity * pEntity = (Entity*)self;
		
		if (pEntity->isDestroyed())
		{
			PyErr_SetString( PyExc_TypeError,
				"Entity for Witness method no longer exists" );
			return NULL;
		}
		
		if (!pEntity->isRealToScript())
		{
			PyErr_Format( PyExc_TypeError,
					"Failed to get real for Entity %d", int( pEntity->id() ) );
			return NULL;
		}

		Witness * pWitness = pEntity->pReal()->pWitness();

		if (!pWitness)
		{
			PyErr_Format( PyExc_TypeError,
					"Failed to get witness for Entity %d", int( pEntity->id() ) );
			return NULL;
		}

		return pWitness;
	}

	PY_FAKE_PYOBJECTPLUS_BASE_DECLARE()
	Py_FakeHeader( Witness, PyObjectPlus )

public:
	// Creation/Destruction
	Witness( RealEntity & owner, BinaryIStream & data,
			CreateRealInfo createRealInfo, bool hasChangedSpace = false );
	virtual ~Witness();

private:
	void readOffloadData( BinaryIStream & data, CreateRealInfo createRealInfo,
		bool hasChangedSpace );

	void readSpaceDataEntries( BinaryIStream & data, bool hasChangedSpace );
	void writeSpaceDataEntries( BinaryOStream & data ) const;

	void readAoI( BinaryIStream & data );
	void writeAoI( BinaryOStream & data ) const;

	void init();

public:
	RealEntity & real()					{ return real_; }
	const RealEntity & real() const		{ return real_; }

	Entity & entity()					{ return entity_; }
	const Entity & entity() const		{ return entity_; }


	// Ex-overrides from RealEntity
	void writeOffloadData( BinaryOStream & data ) const;
	void writeBackupData( BinaryOStream & data ) const;

	bool sendToClient( EntityID entityID,
			Mercury::MessageID msgID, MemoryOStream & stream, int msgSize );
	void sendToProxy( Mercury::MessageID msgID, MemoryOStream & stream );

	void flushToClient();

	void setWitnessCapacity( EntityID id, int bps );

	void requestEntityUpdate( EntityID id,
			EventNumber * pEventNumbers, int size );

	void addToAoI( Entity * pEntity, bool setManuallyAdded );
	void removeFromAoI( Entity * pEntity, bool clearManuallyAdded );

	void newPosition( const Vector3 & position );

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	void updateReferencePosition( uint8 seqNum );
	void cancelReferencePosition();
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
	void dumpAoI();
	void debugDump();

	// Misc
	void update();
	void onSendReliablePosition( Position3D position, Direction3D direction );

	bool isAoIRooted() const;
	const RangeListNode * pAoIRoot() const { return pAoIRoot_; }

	void setAoIRoot( float x, float z );
	void clearAoIRoot();
	PyObject * pyGet_aoiRoot();
	int pySet_aoiRoot( PyObject * pArgs );

	// Python
	void pyAdditionalMembers( const ScriptList & pList ) const
		{ }
	void pyAdditionalMethods( const ScriptList & pList ) const
		{ }
	PY_RW_ATTRIBUTE_DECLARE( maxPacketSize_, bandwidthPerUpdate );

	int bpsToClient();
	PY_READABLE_ATTRIBUTE_GET( this->bpsToClient(), bpsToClient );
	int pySet_bpsToClient( PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( stealthFactor_, stealthFactor )

	void unitTest();
	PY_AUTO_METHOD_DECLARE( RETVOID, unitTest, END )

	PY_AUTO_METHOD_DECLARE( RETVOID, dumpAoI, END )

	PY_AUTO_METHOD_DECLARE( RETOK, withholdFromClient,
			ARG( PyObjectPtr, OPTARG( bool, true, END ) ) )
	PY_AUTO_METHOD_DECLARE( RETOWN, isWithheldFromClient,
			ARG( PyObjectPtr, END ) )

	PY_AUTO_METHOD_DECLARE( RETOK, setPositionDetailed,
			ARG( PyObjectPtr, OPTARG( bool, true, END ) ) )
	PY_AUTO_METHOD_DECLARE( RETOWN, isPositionDetailed,
			ARG( PyObjectPtr, END ) )

	PY_KEYWORD_METHOD_DECLARE( py_entitiesInAoI )

	PY_AUTO_METHOD_DECLARE( RETDATA, isInAoI, ARG( PyObjectPtr, END ) )

	PY_AUTO_METHOD_DECLARE( RETOK, setAoIUpdateScheme,
			ARG( PyObjectPtr, ARG( BW::string, END ) ) )

	PY_AUTO_METHOD_DECLARE( RETOWN, getAoIUpdateScheme,
			ARG( PyObjectPtr, END ) )

	void setAoIRadius( float radius, float hyst = 5.f );
	PY_AUTO_METHOD_DECLARE( RETVOID, setAoIRadius,
		ARG( float, OPTARG( float, 5.f, END ) ) )

	bool updateManualAoI( ScriptSequence list,
		bool ignoreNonManualAoIEntities );
	PY_AUTO_METHOD_DECLARE( RETOK, updateManualAoI,
				ARG( ScriptSequence, OPTARG( bool, false, END ) ) )

	bool addToManualAoI( PyObjectPtr pEntityOrID );
	PY_AUTO_METHOD_DECLARE( RETOK, addToManualAoI, ARG( PyObjectPtr,
		END ) )

	bool removeFromManualAoI( PyObjectPtr pEntityOrID );
	PY_AUTO_METHOD_DECLARE( RETOK, removeFromManualAoI, ARG( PyObjectPtr,
		END ) )

	ScriptList entitiesInManualAoI() const;
	PY_AUTO_METHOD_DECLARE( RETDATA, entitiesInManualAoI, END )

	static void addWatchers();

	void vehicleChanged();


	/**
	 *	This method is used to visit entities in the AoI.
	 *
	 *	@param visitor	The visitor.
	 */
	void visitAoI( EntityCacheVisitor & visitor ) const
	{
		aoiMap_.visit( visitor );
	}


private:
	typedef BW::vector< EntityCache * > KnownEntityQueue;

	bool withholdFromClient( PyObjectPtr pEntityOrID, bool isVisible );
	PyObject * isWithheldFromClient( PyObjectPtr pEntityOrID ) const;

	bool setPositionDetailed( PyObjectPtr pEntityOrID, bool isPositionDetailed );
	PyObject * isPositionDetailed( PyObjectPtr pEntityOrID ) const;

	bool isInAoI( PyObjectPtr pEntityOrID ) const;

	bool setAoIUpdateScheme( PyObjectPtr pEntityOrID, BW::string schemeName );
	PyObject * getAoIUpdateScheme( PyObjectPtr pEntityOrID );

	// Private methods
	RangeListNode * replaceAoITrigger( RangeListNode * pNewAoIRoot );

	void addToSeen( EntityCache * pCache );
	void deleteFromSeen( Mercury::Bundle & bundle,
			KnownEntityQueue::iterator iter );

	void deleteFromClient( Mercury::Bundle & bundle,
		EntityCache * pCache );
	void deleteFromAoI( KnownEntityQueue::iterator iter );

	void handleEnterPending( Mercury::Bundle & bundle, 
		KnownEntityQueue::iterator iter );

	void sendEnter( Mercury::Bundle & bundle, EntityCache * pCache );
	void sendCreate( Mercury::Bundle & bundle, EntityCache * pCache );

	void sendGameTime();

	void onLeaveAoI( EntityCache * pCache, EntityID id );

	Mercury::Bundle & bundle();

	IDAlias allocateIDAlias( const Entity & entity );

	void addSpaceDataChanges( Mercury::Bundle & bundle );

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	bool * addReferencePosition( Mercury::Bundle & bundle );
	void calculateReferencePosition();
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	bool * addDetailedPlayerPosition( Mercury::Bundle & bundle,
		bool isReferencePosition ) const;

	void handleStateChange( EntityCache ** ppCache,
				KnownEntityQueue::iterator & queueEnd );
	bool sendQueueElement( EntityCache * pCache );

	bool selectEntity( Mercury::Bundle & bundle, EntityID targetID ) const;

	static Entity * findEntityFromPyArg( PyObject * pEntityOrID );
	EntityCache * findEntityCacheFromPyArg( PyObject * pEntityOrID ) const;
	template <typename PRED>
	ScriptList entitiesInAoI( PRED predicate ) const;
	PyObject * entitiesInAoI( bool includeAll, bool onlyWithheld ) const;

	bool shouldAddReplayAoIUpdates() const;

	// variables

	RealEntity		& real_;
	Entity			& entity_;

	GameTime		noiseCheckTime_;
	GameTime		noisePropagatedTime_;
	bool			noiseMade_;

	int32			maxPacketSize_;

	KnownEntityQueue	entityQueue_;
	EntityCacheMap		aoiMap_;
	bool				shouldAddReplayAoIUpdates_;

	float stealthFactor_;

	float aoiHyst_;
	float aoiRadius_;
	RangeListNode * pAoIRoot_;

	int32 bandwidthDeficit_;

	IDAlias freeAliases_[ 256 ];
	int		numFreeAliases_;

#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	// This is used as a reference for shorthand positions sent as 3 uint8's
	// relative to this reference position.  (see also RelPosRef). It is used
	// to reduce bandwidth.
	Vector3 referencePosition_;
	// This is the sequence number of the relative position reference sent
	// from the client.
	uint8 	referenceSeqNum_;
	bool hasReferencePosition_;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */

	// This is the first spaceData sequence we have not sent to the client
	// When the client is up-to-date, this will equal pSpace->endDataSeq();
	int32 knownSpaceDataSeq_;

	uint32 allSpacesDataChangeSeq_;

	AoITrigger * pAoITrigger_;

	// These record details of the last reliable detailed position sent to
	// the client, to avoid needless resends every tick.
	GameTime	lastSentReliableGameTime_;
	Position3D	lastSentReliablePosition_;
	Direction3D	lastSentReliableDirection_;

	friend BinaryIStream & operator>>( BinaryIStream & stream,
			EntityCache & entityCache );
	friend BinaryOStream & operator<<( BinaryOStream & stream,
			const EntityCache & entityCache );
};

BW_END_NAMESPACE

#endif // WITNESS_HPP
