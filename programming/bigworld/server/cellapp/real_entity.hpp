#ifndef REAL_ENTITY_HPP
#define REAL_ENTITY_HPP

#include "cellapp_interface.hpp"
#include "cell_app_channel.hpp"
#include "controller.hpp"
#include "entity_cache.hpp"
#include "entity_type.hpp"
#include "history_event.hpp"
#include "mailbox.hpp"
#include "server/updatable.hpp"
#include "entity.hpp"

#include "network/udp_channel.hpp"

#include "pyscript/pyobject_plus.hpp"

#include "server/auto_backup_and_archive.hpp"
#include "server/util.hpp"

#include <math.h>


BW_BEGIN_NAMESPACE

typedef SmartPointer<Entity> EntityPtr;

namespace Mercury
{
	class Bundle;
} // end namespace Mercury

class MemoryOStream;
class Space;
class Witness;

// From cell.hpp
typedef BW::vector< EntityPtr >::size_type EntityRemovalHandle;

typedef SmartPointer<BaseEntityMailBox> BaseEntityMailBoxPtr;

/**
 *	An object of this type is used by Entity to store additional data when the
 *	entity is "real" (as opposed to ghosted).
 */
class RealEntity
{
	static RealEntity * getSelf( PyObject * self )
	{
		Entity * pEntity = (Entity*)self;
		
		if (pEntity->isDestroyed())
		{
			PyErr_SetString( PyExc_TypeError,
				"Entity for RealEntity method no longer exists" );
			return NULL;
		}
		
		if (!pEntity->isRealToScript())
		{
			PyErr_Format( PyExc_TypeError,
					"Failed to get real for Entity %d", int( pEntity->id() ) );
			return NULL;
		}

		return pEntity->pReal();
	}

	PY_FAKE_PYOBJECTPLUS_BASE_DECLARE()
	Py_FakeHeader( RealEntity, PyObjectPlus )

public:
	/**
	 *	This class is used to represent the location of a ghost.
	 */
	class Haunt
	{
	public:
		Haunt( CellAppChannel * pChannel, GameTime creationTime ) :
			pChannel_( pChannel ),
			creationTime_( creationTime )
		{}

		// A note about these accessors.  We don't need to guard their callers
		// with ChannelSenders because having haunts guarantees that the
		// underlying channel is regularly sent.  If haunts are destroyed and
		// the channel becomes irregular, unsent data is sent immediately.
		CellAppChannel & channel() { return *pChannel_; }
		Mercury::Bundle & bundle() { return pChannel_->bundle(); }
		const Mercury::Address & addr() const { return pChannel_->addr(); }

		void creationTime( GameTime time )	{ creationTime_ = time; }
		GameTime creationTime() const		{ return creationTime_; }

	private:
		CellAppChannel * pChannel_;
		GameTime creationTime_;
	};

	typedef BW::vector< Haunt > Haunts;

	static void addWatchers();

	RealEntity( Entity & owner );

	bool init( BinaryIStream & data, CreateRealInfo createRealInfo,
			Mercury::ChannelVersion channelVersion =
				Mercury::SEQ_NULL,
			const Mercury::Address * pBadHauntAddr = NULL );

	void destroy( const Mercury::Address * pNextRealAddr = NULL );

	void writeOffloadData( BinaryOStream & data, bool isTeleport );

	void enableWitness( BinaryIStream & data, Mercury::ReplyID replyID );
	void disableWitness( bool isRestore = false );

	Entity & entity()								{ return entity_; }
	const Entity & entity() const					{ return entity_; }

	Witness * pWitness()							{ return pWitness_; }
	const Witness * pWitness() const				{ return pWitness_; }

	Haunts::iterator hauntsBegin() { return haunts_.begin(); }
	Haunts::iterator hauntsEnd() { return haunts_.end(); }
	int numHaunts() const { return haunts_.size(); }

	void addHaunt( CellAppChannel & channel );
	Haunts::iterator delHaunt( Haunts::iterator iter );

    HistoryEvent * addHistoryEvent( uint8 type,
		MemoryOStream & stream,
		const MemberDescription & description,
		int16 msgStreamSize,
		HistoryEvent::Level level = FLT_MAX );

	void backup();
	void autoBackup();

	friend PyObject * calcCellBackupData( PyObjectPtr pEnt );
	void writeBackupProperties( BinaryOStream & data ) const;

	void debugDump();

	void pyAdditionalMembers( const ScriptList & pList ) const
		{ }
	void pyAdditionalMethods( const ScriptList & pList ) const
		{ }

	void sendPhysicsCorrection();
	void newPosition( const Vector3 & position );

	void addDelGhostMessage( Mercury::Bundle & bundle );
	void deleteGhosts();

	const Vector3 & velocity() const			{ return velocity_; }

	EntityRemovalHandle removalHandle() const		{ return removalHandle_; }
	void removalHandle( EntityRemovalHandle h )		{ removalHandle_ = h; }

	const EntityMailBoxRef & controlledByRef() const{ return controlledBy_; }

	GameTime creationTime() const	{ return creationTime_; }

	void delControlledBy( EntityID deadID );

	Mercury::UDPChannel & channel()	{ return *pChannel_; }

	bool controlledBySelf() const	{ return entity_.id() == controlledBy_.id; }
	bool controlledByOther() const
			{ return !this->controlledBySelf() && (controlledBy_.id != 0); }

	void teleport( const EntityMailBoxRef & dstMailBoxRef );

	// ---- Python methods ----
	bool teleport( const EntityMailBoxRef & nearbyMBRef,
		const Vector3 & position, const Vector3 & direction );
	PY_AUTO_METHOD_DECLARE( RETOK, teleport, ARG( EntityMailBoxRef,
		ARG( Vector3, ARG( Vector3, END ) ) ) )

	BaseEntityMailBoxPtr controlledBy();
	void controlledBy( BaseEntityMailBoxPtr pNewMaster );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		BaseEntityMailBoxPtr, controlledBy, controlledBy )


	float artificialMinLoad() const
		{ return entity_.profiler().artificialMinLoad(); }
	void artificialMinLoad( float v )
		{ entity_.profiler().artificialMinLoad( v ); }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, artificialMinLoad, artificialMinLoad );


	bool isWitnessed() const;
	PY_RO_ATTRIBUTE_DECLARE( isWitnessed(), isWitnessed )

	PY_RO_ATTRIBUTE_DECLARE( !!pWitness_, hasWitness )

	PY_RW_ATTRIBUTE_DECLARE( shouldAutoBackup_, shouldAutoBackup )

	/**
	 *	This method sets the recording space entry ID, referring to the last
	 *	time an entity was added to a recording.
	 *
	 *	@param value 	The space entry ID of the recording.
	 */
	void recordingSpaceEntryID( const SpaceEntryID & value ) 	
	{ 
		recordingSpaceEntryID_ = value;
	}

	/**
	 *	This method returns the recording space entry ID, referring to the last
	 *	time an entity was added to a recording.
	 */
	const SpaceEntryID & recordingSpaceEntryID() const 
	{ 
		return recordingSpaceEntryID_; 
	}

private:
	// ---- Private methods ----
	~RealEntity();

	bool readOffloadData( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr = NULL,
		bool * pHasChangedSpace = NULL );
	void readBackupData( BinaryIStream & data );
	void readBackupDataInternal( BinaryIStream & data );

	void writeBackupData( BinaryOStream & data ) const;
	void writeBackupDataInternal( BinaryOStream & data ) const;

	void setWitness( Witness * pWitness );

	void notifyWardOfControlChange( bool hasControl );



	// ---- Private data ----
	Entity & entity_;

	Witness * pWitness_;

	Haunts haunts_;

	// Used by cell to quick remove the entity from the real entities.
	EntityRemovalHandle	removalHandle_;

	EntityMailBoxRef	controlledBy_;

	Vector3			velocity_;
	Vector3			positionSample_;
	GameTime		positionSampleTime_;

	GameTime		creationTime_;

	AutoBackupAndArchive::Policy shouldAutoBackup_;

	Mercury::UDPChannel *		pChannel_;

	SpaceEntryID 	recordingSpaceEntryID_;
};

BW_END_NAMESPACE

#endif // REAL_ENTITY_HPP

// real_entity.hpp
