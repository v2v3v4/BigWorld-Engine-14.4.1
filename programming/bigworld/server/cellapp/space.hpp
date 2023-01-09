#ifndef SPACE_HPP
#define SPACE_HPP

#include "cellapp_interface.hpp"

#include "cell_range_list.hpp"

#include "physical_chunk_space.hpp"

#include "chunk_loading/geometry_mapper.hpp"
#include "chunk_loading/edge_geometry_mappings.hpp"

#include "math/math_extra.hpp"
#include "network/basictypes.hpp"
#include "network/space_data_mapping.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class Cell;
class CellInfo;
class CellInfoVisitor;
class Chunk;
class SpaceNode;

class IPhysicalSpace;
typedef SmartPointer< IPhysicalSpace > PhysicalSpacePtr;

class Entity;
typedef SmartPointer< Entity > EntityPtr;

// These do not need to be reference counted since they are added in the
// construction and removed in destruction (or destroy) of entity.
typedef BW::vector< EntityPtr >                 SpaceEntities;
typedef SpaceEntities::size_type                 SpaceRemovalHandle;

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;

class RangeTrigger;
typedef BW::list< RangeTrigger * > RangeTriggerList;

/**
 *	This class is used to represent a space.
 */
class Space : public TimerHandler, public GeometryMapper
{
public:
	Space( SpaceID id );
	virtual ~Space();

	void reuse();

	const CellInfo * pCellAt( float x, float z ) const;
	void visitRect( const BW::Rect & rect, CellInfoVisitor & visitRect );

	// ---- Accessors ----
	SpaceID id() const						{ return id_; }

	Cell * pCell() const					{ return pCell_; }
	void pCell( Cell * pCell );

	ChunkSpacePtr pChunkSpace() const;
	PhysicalSpacePtr pPhysicalSpace() const { return pPhysicalSpace_; }

	// ---- GeometryMapper ----
	virtual void onSpaceGeometryLoaded(
			SpaceID spaceID, const BW::string & name );

	virtual bool initialPoint( Vector3 & result ) const;

	float artificialMinLoad() const			{ return artificialMinLoad_; }

	// ---- Entity ----
	void createGhost( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

		// ( EntityID id, ...
	void createGhost( const EntityID entityID,
					  BinaryIStream & data );
		// ( TypeID type, Position3D & pos,
		//  string & ghostState, Mercury::Address & owner );

	void addEntity( Entity * pEntity );
	void removeEntity( Entity * pEntity );
	void visitLargeEntities( float x, float z, RangeTrigger & visitor );

	EntityPtr newEntity( EntityID id, EntityTypeID entityTypeID );

	Entity * findNearestEntity( const Vector3 & position );

	// ---- Static methods ----
	enum UpdateCellAppMgr
	{
		UPDATE_CELL_APP_MGR,
		DONT_UPDATE_CELL_APP_MGR
	};

	enum DataEffected
	{
		ALREADY_EFFECTED,
		NEED_TO_EFFECT
	};

	static WatcherPtr pWatcher();

	// ---- Space data ----
	void spaceData( BinaryIStream & data );
	void allSpaceData( BinaryIStream & data );

	void updateGeometry( BinaryIStream & data );

	void spaceGeometryLoaded( BinaryIStream & data );

	void setLastMappedGeometry( const BW::string& lastMappedGeometry )
	{ lastMappedGeometry_ = lastMappedGeometry; }

	void shutDownSpace( BinaryIStream & data );
	void requestShutDown();

	CellInfo * findCell( const Mercury::Address & addr ) const;

	SpaceNode * readTree( BinaryIStream & stream, const BW::Rect & rect );

	bool spaceDataEntry( const SpaceEntryID & entryID, uint16 key,
		const BW::string & value,
		UpdateCellAppMgr cellAppMgrAction = UPDATE_CELL_APP_MGR,
		DataEffected effected = NEED_TO_EFFECT );

	void onLoadResourceFailed( const SpaceEntryID & entryID );

	int32 begDataSeq() const	{ return begDataSeq_; }
	int32 endDataSeq() const	{ return endDataSeq_; }

	const BW::string * dataBySeq( int32 seq,
		SpaceEntryID & entryID, uint16 & key ) const;
	int dataRecencyLevel( int32 seq ) const;

	const RangeList & rangeList() const	{ return rangeList_; }
	const RangeTriggerList & appealRadiusList() const
		{ return appealRadiusList_; }

	void debugRangeList();

	SpaceEntities & spaceEntities() { return entities_; }
	const SpaceEntities & spaceEntities() const { return entities_; }

	void writeDataToStream( BinaryOStream & steam ) const;
	void readDataFromStream( BinaryIStream & stream );

	void writeBounds( BinaryOStream & stream ) const;

	void chunkTick();
	void calcLoadedRect( BW::Rect & loadedRect ) const;
	void prepareNewlyLoadedChunksForDelete();

	bool isFullyUnloaded() const;

	float timeOfDay() const;

	bool isShuttingDown() const		{ return shuttingDownTimerHandle_.isSet(); }

	void writeRecoveryData( BinaryOStream & stream ) const;

	void setPendingCellDelete( const Mercury::Address & addr );

	size_t numEntities() const	{ return entities_.size(); }

	size_t numCells() const { return cellInfos_.size(); }

	SpaceDataMapping & spaceDataMapping() { return spaceDataMapping_; }

	/**
	 *  This method is called when new geometry mapping is read
	 */
	void onNewGeometryMapping()
	{
		shouldCheckLoadBounds_ = true;
	}

private:
	typedef BW::map< Mercury::Address, SmartPointer< CellInfo > > CellInfos;

	virtual void handleTimeout( TimerHandle handle, void * arg );

	void writeChunkBounds( BinaryOStream & stream ) const;

	void writeEntityBounds( BinaryOStream & stream ) const;
	void writeEntityBoundsForEdge( BinaryOStream & stream,
			bool isMax, bool isY ) const;

	void updateLoadBounds();
	void checkForShutDown();

	bool hasSingleCell() const;

	SpaceID	id_;

	Cell *	pCell_;

	PhysicalSpacePtr pPhysicalSpace_;

	SpaceDataMapping spaceDataMapping_;

	SpaceEntities	entities_;
	CellInfos		cellInfos_;

	RangeList			rangeList_;
	RangeTriggerList	appealRadiusList_;

	int32	begDataSeq_;
	int32	endDataSeq_;

	/**
	 *	This is the information we record about recent data entries
	 */
	struct RecentDataEntry
	{
		SpaceEntryID	entryID;
		GameTime		time;
		uint16			key;
	};
	typedef BW::vector<RecentDataEntry> RecentDataEntries;
	RecentDataEntries	recentData_;
	// TODO: Need to clean out recent data every so often (say once a minute)
	// Also don't let beg/endDataSeq go negative.
	// Yes that will require extensive fixups.

	float	initialTimeOfDay_;
	float	gameSecondsPerSecond_;

	BW::string lastMappedGeometry_;

	SpaceNode *	pCellInfoTree_;

	TimerHandle shuttingDownTimerHandle_;

	float artificialMinLoad_;
	bool shouldCheckLoadBounds_;

public:
	static uint32 s_allSpacesDataChangeSeq_;
};

BW_END_NAMESPACE

#endif // SPACE_HPP

// space.hpp
