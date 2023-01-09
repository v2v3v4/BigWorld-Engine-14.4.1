#ifndef SCOPED_LOCKED_CHUNK_HOLDER_HPP
#define SCOPED_LOCKED_CHUNK_HOLDER_HPP

#include "cstdmf/bw_set.hpp"

#include "locked_chunks.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkProcessorManager;
typedef BW::set<Chunk*> ChunkSet;

/**
 *	A class for locking chunks into memory so that they can't be unloaded while
 *	they're being used. Guarantees to unlock all of the chunks when it is
 *	destroyed/goes out of scope. Checks for double-locking and prints warnings.
 */
class ScopedLockedChunkHolder
{
public:
	ScopedLockedChunkHolder( ChunkProcessorManager* manager );
	virtual ~ScopedLockedChunkHolder();

	ChunkSet::iterator begin();
	ChunkSet::const_iterator begin() const;

	ChunkSet::iterator end();
	ChunkSet::const_iterator end() const;

	ChunkSet::reverse_iterator rbegin();
	ChunkSet::const_reverse_iterator rbegin() const;

	ChunkSet::reverse_iterator rend();
	ChunkSet::const_reverse_iterator rend() const;

	bool empty() const;
	ChunkSet::size_type size() const;
	ChunkSet::size_type max_size() const;

	void lock( Chunk* pChunk );
	void lock( Chunk* pChunk, int expandOnGridX, int expandOnGridZ );
	void lock( Chunk* pChunk, int portalDepth );

	ChunkSet::iterator erase( ChunkSet::iterator position );
	void clear();

	ChunkSet::iterator find( Chunk* pChunk );
	ChunkSet::const_iterator find( Chunk* pChunk ) const;

	ChunkSet::size_type count( Chunk* pChunk ) const;

private:
	void insert( Chunk* pChunk );

	/// List of my locked chunks
	ChunkSet chunks_;

	/// Stores global list of locked chunks and space to expand for neighbours
	ChunkProcessorManager* manager_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "scoped_locked_chunk_holder.ipp"
#endif

#endif // SCOPED_LOCKED_CHUNK_HOLDER_HPP
