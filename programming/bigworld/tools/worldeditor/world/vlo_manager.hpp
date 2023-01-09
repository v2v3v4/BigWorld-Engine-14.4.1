#ifndef VLO_MANAGER_HPP
#define VLO_MANAGER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "cstdmf/unique_id.hpp"
#include "chunk/chunk_vlo.hpp"
#include "math/boundbox.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class is a centralised place for methods and data about the state of
 *  VLOs in the world, to allow correct results in all scenarios.
 */
class VLOManager
{
public:
	static VLOManager* instance();
	static void fini();

	void clearLists();

	void clearDirtyList();

	void markAsDirty( const UniqueID& id, bool mark = true );
	bool isDirty( const UniqueID& id ) const;

	void markAsDeleted( const UniqueID& id, bool mark = true );
	bool isDeleted( const UniqueID& id ) const;

	bool needsCleanup( const UniqueID& id ) const;

	void setOriginalBounds( const UniqueID& id,
			const BoundingBox& bb, const Matrix& matrix );

	void setMovedBounds( const UniqueID& id,
			const BoundingBox& bb, const Matrix& matrix );

	void removeFromLists( const UniqueID& id );

	bool contains( const Chunk* pChunk, const UniqueID& id ) const;

	void columnsTouchedByBounds(
			const BoundingBox& bb, const Matrix& matrix,
			BW::set< BW::string >& returnSet ) const;

	void getDirtyColumns( BW::set< BW::string >& columns ) const;

	void postSave();

	void deleteFromAllChunks( const ChunkVLO * pVLORef );

	void updateChunkReferences( Chunk* pChunk);

	void enableUpdateChunkReferences( bool enable );

	bool createReference( const VeryLargeObjectPtr pVLO, Chunk* pChunk );
	bool deleteReference( const VeryLargeObjectPtr pVLO, Chunk* pChunk );

	void markChunksChanged( const VeryLargeObjectPtr pVLO, BoundingBox & vloBB );

	void updateReferences( const VeryLargeObjectPtr pVLO );
	void updateReferencesImmediately( const VeryLargeObjectPtr pVLO );

	bool writable( const VeryLargeObjectPtr pVLO, BoundingBox & bb ) const;

	void saveDirtyChunks();

private:
	friend class UpdateReferencesTickable;

	// Internal typedefs.
	typedef BW::set< UniqueID > VLOIDSet;
	typedef std::pair< BoundingBox, Matrix > Bounds;
	typedef BW::map< UniqueID, Bounds > BoundsMap;
	typedef BW::map< UniqueID, BW::set< BW::string > > ChunksMap;

	// Instance pointer.
	static VLOManager* s_pInstance_;

	// Flag to temporarily disable updating references when loading chunks.
	bool enableUpdateChunkReferences_;

	// Mutex to ensure thread-safety in all methods that affect any list.
	mutable SimpleMutex listMutex_;

	// Set of IDs of VLOs that have been modified.
	VLOIDSet dirtySet_;
	// Set of IDs of VLOs that have been deleted.
	VLOIDSet deletedSet_;
	// Set of IDs of deleted VLOs stored after save, used to later remove the
	// .vlo file from disk.
	VLOIDSet needsCleanupSet_;

	// Map of VLO oriented bounding boxes as when they were last saved.
	BoundsMap originalBoundsMap_;
	// Map of VLO oriented bounding boxes containing their current position.
	BoundsMap movedBoundsMap_;

	ChunksMap chunksToRemoveReference_;

	VLOManager();

	void updateReferencesInChunk(
		const VeryLargeObjectPtr pVLO, Chunk* pChunk,
		BW::set< Chunk* >& removedChunks,
		VeryLargeObject::ChunkItemList& movableItems,
		const EditorChunkVLO* pSelectedItem );

	void updateReferencesInternal( const ChunkVLO * pVLORef, const BW::vector<ChunkItemPtr> *selectedItems);
	void removeVloReferenceFromUnloadedChunk( const BW::string& id, const BW::string& dir, const BW::string& chunkName );
};

BW_END_NAMESPACE

#endif // VLO_MANAGER_HPP
