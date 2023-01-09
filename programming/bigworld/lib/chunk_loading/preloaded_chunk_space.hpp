#ifndef PRELOADED_CHUNK_SPACE_HPP
#define PRELOADED_CHUNK_SPACE_HPP

#include "edge_geometry_mappings.hpp"

#include "chunk/chunk_space.hpp"

BW_BEGIN_NAMESPACE

class GeometryMapper;

/**
 *	This is a ChunkSpace with an EdgeGeometryMapping that is incremently
 *	loaded each tick. The EdgeGeometryMapping is never unloaded.
 */
class PreloadedChunkSpace
{
public:
	PreloadedChunkSpace( BW::Matrix m, const BW::string & path,
			const SpaceEntryID & entryID, SpaceID spaceID,
			GeometryMapper & mapper );
	~PreloadedChunkSpace() {}

	void chunkTick();

	void prepareNewlyLoadedChunksForDelete();

	ChunkSpacePtr pChunkSpace() const { return pChunkSpace_; }

	SpaceEntryID & entryID() { return entryID_; }

private:
	ChunkSpacePtr pChunkSpace_;

	EdgeGeometryMappings geometryMappings_;

	SpaceEntryID entryID_;
};

BW_END_NAMESPACE

#endif // PRELOADED_CHUNK_SPACE_HPP

// preloaded_chunk_space.hpp
