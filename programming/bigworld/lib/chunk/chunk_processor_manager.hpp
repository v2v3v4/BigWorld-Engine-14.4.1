#ifndef CHUNK_PROCESSOR_MANAGER_HPP
#define CHUNK_PROCESSOR_MANAGER_HPP

#include "locked_chunks.hpp"
#include "unsaved_terrain_blocks.hpp"
#include "unsaved_chunks.hpp"
#include "dirty_chunk_list.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_vector.hpp"

#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class CleanChunkList;
class GeometryMapping;
class Chunk;
class ScopedLockedChunkHolder;

typedef SmartPointer<CleanChunkList> CleanChunkListPtr;


class UnsavedList
{
public:
	UnsavedTerrainBlocks unsavedTerrainBlocks_;
	UnsavedChunks unsavedChunks_;
	UnsavedChunks unsavedCdatas_;
};

/**
 *	Manages chunk processor tasks.
 */
class ChunkProcessorManager : 
	public TaskManager,
	protected UnsavedList
{
public:
	typedef BW::set<Chunk*> ChunkSet;
	typedef BW::set<BW::string> ChunkIdSet;
	typedef BW::vector<Chunk*> ChunkVector;

	ChunkProcessorManager( int threadNum = 0 );
	virtual ~ChunkProcessorManager();

	void startNumThreads( int threadNum );

	virtual void stopAll( bool discardPendingTasks = true,
		bool waitForThreads = true );

	static bool isChunkReadyToProcess(
		Chunk* chunk, int expandOnGridX, int expandOnGridZ );
	static bool loadChunkForProcessing(
		Chunk* chunk, int expandOnGridX, int expandOnGridZ );

	static bool isChunkReadyToProcess( Chunk* chunk, int portalDepth );
	static bool loadChunkForProcessing( Chunk* chunk, int portalDepth );

	void spreadInvalidate( Chunk* chunk, int expandOnGridX, int expandOnGridZ, 
				int cacheIndex, EditorChunkItem* pChangedItem );
	void spreadInvalidate( Chunk* chunk, int portalDepth, 
				int cacheIndex, EditorChunkItem* pChangedItem );

	void processMainthreadTask();
	void processBackgroundTask( const Vector3& cameraPosition,
		const BW::set<Chunk*>& excludedChunks );
	bool processChunk( Chunk* );

	typedef UnsavedChunks::IChunkSaver IChunkSaver;
	bool saveChunks(
		const ChunkIdSet& chunksToSave,
		IChunkSaver& chunkSaver,
		IChunkSaver& thumbnailSaver,
		Progress* progress,
		bool bProgressiveUnloading = false);
	bool invalidateAllChunks( GeometryMapping* mapping, Progress* progress,
			bool invalidateOnlyNavMesh = false );

	/**
	 * Clear unsaved data.
	 * @brief Will clear all unsaved terrain blocks, chunks, and cdatas.
	 */
	void clearUnsavedData();

	void lockChunkInMemory(
		Chunk* chunk,
		int expandOnGridX,
		int expandOnGridZ,
		ScopedLockedChunkHolder& chunks );

	void lockChunkInMemory(
		Chunk* chunk,
		int portalDepth,
		ScopedLockedChunkHolder& chunks ) const;

	void onChangeSpace();
	void onChunkDirtyStatusChanged( Chunk* pChunk );
	void updateChunkDirtyStatus( Chunk* pChunk );
	void cleanChunkDirtyStatus( Chunk* pChunk );

	virtual bool isChunkEditable( Chunk* pChunk ) const;
	virtual bool isChunkEditable( const BW::string& chunk ) const;

	virtual void chunkShadowUpdated( Chunk * pChunk ) {}

	void writeCleanList();

	void mark();

	static ChunkProcessorManager& instance()
	{
		return *s_instance;
	}

	virtual GeometryMapping* geometryMapping() = 0;
	virtual const GeometryMapping* geometryMapping() const = 0;

	RecursiveMutex& dirtyChunkListsMutex() const { return dirtyChunkListsMutex_; }
	const DirtyChunkLists& dirtyChunkLists() const;

	LockedChunks& allLockedChunks() { return allLockedChunks_; }
	const LockedChunks& allLockedChunks() const { return allLockedChunks_; }

protected:
	/// Clean and dirty chunk lists
	CleanChunkListPtr cleanChunkList_;
	mutable RecursiveMutex dirtyChunkListsMutex_;
	DirtyChunkLists dirtyChunkLists_;

	virtual bool isMemoryLow( bool testNow = false ) const { return false; }
	virtual void unloadChunks() = 0;
	// offline processor and WE have to implement it
	// if they want to use aggressive unloading flag in saveChunks
	virtual void unloadChunksOutsideOfGridBox(int minX, int maxX, int minZ, int maxZ, const BW::set<Chunk*>& keepLoadedChunks);

	void unloadRemovableChunks();

	bool tick();

	virtual void tickSavingChunks() { }

	static bool ensureNeighbourChunkLoadedInternal(
		Chunk* chunk, int portalDepth );

	static void loadNeighbourChunkInternal(
		Chunk* chunk, int portalDepth, BW::map<Chunk*, int>& chunkDepths );

	void spreadInvalidateInternal( Chunk* chunk, int portalDepth, 
			int cacheIndex, EditorChunkItem* pChangedItem );
	void spreadInvalidateInternal( Chunk* chunk, int expandOnGridX, int expandOnGridZ, 
			int cacheIndex, EditorChunkItem* pChangedItem );

	virtual bool needToUnload() const;

private:
	bool loadAndLockChunkInMemory( Chunk* chunk,
		ScopedLockedChunkHolder& chunks );

	void lockChunkInMemoryInternal(
		Chunk* chunk, int portalDepth, BW::set<Chunk*>& chunks ) const;

	void findDirtyChunks( const ChunkIdSet& chunksToSave,
		ChunkVector& dirtyOutdoorChunks, ChunkVector& dirtyIndoorChunks );

	void takeChunkLock( ChunkVector& pendingChunks,
		ScopedLockedChunkHolder& activeChunks );

	bool processActiveChunks( ScopedLockedChunkHolder& activeChunks );
	void processActiveChunk( Chunk& chunk );
	void processPendingIndoorChunks(const ScopedLockedChunkHolder& activeChunks,
		ChunkVector& pendingChunks, ChunkVector& pendingIndoorChunks);


	void removeFinishedChunks( ScopedLockedChunkHolder& activeChunks );

	void saveAllUnsaved( IChunkSaver& chunkSaver,
		IChunkSaver& thumbnailSaver );

	/// Global list of all chunks that have been locked
	/// (Reference counts number locked)
	LockedChunks allLockedChunks_;

	/// Singleton instance
	static ChunkProcessorManager* s_instance;
};

BW_END_NAMESPACE

#endif // CHUNK_PROCESSOR_MANAGER_HPP
