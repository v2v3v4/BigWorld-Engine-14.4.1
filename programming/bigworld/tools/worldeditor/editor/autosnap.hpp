#ifndef AUTOSNAP_HPP
#define AUTOSNAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_boundary.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class SnappedChunkSet
{
	BW::vector<Chunk*> chunks_;
	BW::vector<ChunkBoundary::Portal*> snapPortals_;
	BW::vector<Matrix> portalTransforms_;
public:
	explicit SnappedChunkSet( const BW::set<Chunk*>& chunks );
	unsigned int chunkSize() const	{ return static_cast<uint>(chunks_.size());	}
	Chunk* chunk( unsigned int index ) const	{	return chunks_.at( index );	}
	unsigned int portalSize() const
	{
		MF_ASSERT( snapPortals_.size() <= UINT_MAX );
		return ( unsigned int ) snapPortals_.size();
	}

	ChunkBoundary::Portal* portal( unsigned int index ) const	{	return snapPortals_.at( index );	}
	const Matrix& transform( unsigned int index ) const	{	return portalTransforms_.at( index );	}
};


class SnappedChunkSetSet
{
	bool isConnected( Chunk* chunk1, Chunk* chunk2 ) const;
	void init( BW::set<Chunk*> chunks );
	BW::vector<SnappedChunkSet> chunkSets_;
public:
	explicit SnappedChunkSetSet( const BW::vector<Chunk*>& chunks );
	explicit SnappedChunkSetSet( const BW::set<Chunk*>& chunks );
	size_t size() const	{	return chunkSets_.size();	}
	const SnappedChunkSet& item( unsigned int index ) const	{	return chunkSets_.at( index );	}
};


/**
 * Find the minimum translation required to translate snapChunks (as a group)
 * so they will have a connecting portal to snapToChunk
 */
Matrix findAutoSnapTransform( Chunk* snapChunk, Chunk* snapToChunk );
Matrix findAutoSnapTransform( Chunk* snapChunk, BW::vector<Chunk*> snapToChunks );
Matrix findRotateSnapTransform( const SnappedChunkSet& snapChunk, bool clockWise, Chunk* referenceChunk );
void clearSnapHistory();

BW_END_NAMESPACE
#endif // AUTOSNAP_HPP
