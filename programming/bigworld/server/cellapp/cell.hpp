#ifndef CELL_HPP
#define CELL_HPP

#include "cellapp_interface.hpp"
#include "cell_info.hpp"
#include "cell_profiler.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"

#include "math/math_extra.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

#include "server/common.hpp"

#include <algorithm>
#include <deque>
#include <functional>


BW_BEGIN_NAMESPACE

class Cell;
class Entity;
class CellAppChannel;
class Space;
class ScriptDict;

typedef SmartPointer<Entity> EntityPtr;

typedef BW::vector< EntityPtr >::size_type EntityRemovalHandle;
const EntityRemovalHandle NO_ENTITY_REMOVAL_HANDLE = EntityRemovalHandle( -1 );


class Chunk;
class ReplayDataCollector;


/**
 *	This class is used to represent a cell.
 */
class Cell
{
public:
	/**
	 *	This class is used to store the collection of real entities.
	 */
	class Entities
	{
	public:
		typedef BW::vector< EntityPtr > Collection;
		typedef Collection::iterator iterator;
		typedef Collection::const_iterator const_iterator;
		typedef Collection::size_type size_type;
		typedef Collection::value_type value_type;
		typedef Collection::reference reference;

		iterator begin()				{ return collection_.begin(); }
		const_iterator begin() const	{ return collection_.begin(); }
		iterator end()					{ return collection_.end(); }
		const_iterator end() const		{ return collection_.end(); }

		bool empty() const				{ return collection_.empty(); }
		size_type size() const			{ return collection_.size(); }

		bool add( Entity * pEntity );
		bool remove( Entity * pEntity );

		EntityPtr front()				{ return collection_.front(); }
		EntityPtr front() const			{ return collection_.front(); }

	private:
		void swapWithBack( Entity * pEntity );


	private:
		Collection collection_;
	};

	// Constructor/Destructor
	Cell( Space & space, const CellInfo & cellInfo );

	~Cell();

	void shutDown();

	// Accessors and inline methods
	const CellInfo & cellInfo()	{ return *pCellInfo_; }

	// Entity Maintenance C++ methods
	void offloadEntity( Entity * pEntity, CellAppChannel * pChannel,
			bool isTeleport = false );

	void addRealEntity( Entity * pEntity, bool shouldSendNow );

	void entityDestroyed( Entity * pEntity );

	EntityPtr createEntityInternal(	BinaryIStream & data,
		const ScriptDict & properties, bool isRestore = false,
		Mercury::ChannelVersion channelVersion = Mercury::SEQ_NULL,
		EntityPtr pNearbyEntity = NULL );

	void backup( int index, int period );

	bool checkOffloadsAndGhosts();
	void checkChunkLoading();

	void onSpaceGone();

	void debugDump();

	// Communication message handlers concerning entities
	void createEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data,
		EntityPtr pNearbyEntity );

	void createEntity( const Mercury::Address& srcAddr,
		const Mercury::UnpackedMessageHeader& header,
		BinaryIStream & data )
	{
		this->createEntity( srcAddr, header, data, NULL );
	}

	// Instrumentation
	static WatcherPtr pWatcher();

	SpaceID spaceID() const;
	Space & space()						{ return space_; }

	const Space & space() const			{ return space_; }

	const BW::Rect & rect() const		{ return pCellInfo_->rect(); }

	int numRealEntities() const;

	void sendEntityPositions( Mercury::Bundle & bundle ) const;

	Entities & realEntities();

	// Load balancing
	bool shouldOffload() const;
	void shouldOffload( bool shouldOffload );
	void shouldOffload( BinaryIStream & data );

	void retireCell( BinaryIStream & data );
	void removeCell( BinaryIStream & data );

	void notifyOfCellRemoval( BinaryIStream & data );

	void ackCellRemoval( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	bool reuse();

	void handleCellAppDeath( const Mercury::Address & addr );

	void writeBounds( BinaryOStream & stream ) const;

	bool isRemoved() const	{ return isRemoved_; }

	ReplayDataCollector * pReplayData() { return pReplayData_; }
	bool startRecording( const SpaceEntryID & entryID, const BW::string & name, 
		bool shouldRecordAoIEvents, bool isInitial = true );
	void tickRecording();
	void stopRecording( bool isInitial );

	// Profiling
	const CellProfiler & profiler() const { return profiler_; }
	void tickProfilers( uint64 tickDtInStamps, float smoothingFactor );

private:
	bool isReadyForDeletion() const;

	// General private data
	Entities realEntities_;

	bool shouldOffload_;

	mutable float lastERTFactor_;
	mutable uint64 lastERTCalcTime_;

	// Load balance related data
	float initialTimeOfDay_;
	float gameSecondsPerSecond_;
	bool isRetiring_;
	bool isRemoved_;

	Space & space_;

	int backupIndex_;
	ConstCellInfoPtr pCellInfo_;

	typedef BW::multiset< Mercury::Address > RemovalAcks;
	RemovalAcks pendingAcks_;
	RemovalAcks receivedAcks_;

	ReplayDataCollector * pReplayData_;

	enum StoppingState {
		STOPPING_STATE_NOT_STOPPING,
		STOPPING_STATE_STOPPING_SHOULD_USE_CALLBACKS,
		STOPPING_STATE_STOPPING_SHOULD_NOT_USE_CALLBACKS
	};

	StoppingState stoppingState_;

	friend class CellViewerConnection;

	// Profiling
	CellProfiler profiler_;

};


#ifdef CODE_INLINE
#include "cell.ipp"
#endif

BW_END_NAMESPACE

#endif // CELL_HPP
