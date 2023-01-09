#ifndef LOCKED_CHUNKS_HPP_
#define LOCKED_CHUNKS_HPP_


#include "chunk.hpp"
#include "chunk_overlapper.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class Chunk;


class LockedChunks
{
	BW::map<Chunk*, int> chunks_;

public:
	void lock( Chunk* pChunk )
	{
		BW_GUARD;

		++chunks_[ pChunk ];
	}

	void unlock( Chunk* pChunk )
	{
		BW_GUARD;

		MF_ASSERT( chunks_[ pChunk ] > 0 );

		--chunks_[ pChunk ];

		if (chunks_[ pChunk ] == 0)
		{
			chunks_.erase( pChunk );
		}
	}

	void unlock( const BW::set<Chunk*>& chunks )
	{
		BW_GUARD;

		for (BW::set<Chunk*>::const_iterator iter = chunks.begin();
			iter != chunks.end(); ++iter)
		{
			unlock( *iter );
		}
	}

	void clear()
	{
		BW_GUARD;

		chunks_.clear();
	}

	bool empty() const
	{
		BW_GUARD;

		return chunks_.empty();
	}

	BW::map<Chunk*, int>::iterator begin()
	{
		return chunks_.begin();
	}

	BW::map<Chunk*, int>::iterator end()
	{
		return chunks_.end();
	}

	BW::map<Chunk*, int>::const_iterator cbegin() const
	{
		return chunks_.cbegin();
	}

	BW::map<Chunk*, int>::const_iterator cend() const
	{
		return chunks_.cend();
	}

	void mark()
	{
		BW_GUARD;
		MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

		BW::map<Chunk*, int>::iterator iter;
		for (iter = chunks_.begin(); iter != chunks_.end(); ++iter)
		{
			Chunk* pChunk = iter->first;
			pChunk->removable( false );

			if (pChunk->isOutsideChunk())
			{
				const ChunkOverlappers::Overlappers& overlappers =
					ChunkOverlappers::instance( *pChunk ).overlappers();

				ChunkOverlappers::Overlappers::const_iterator oIter;
				for (oIter = overlappers.begin();
					oIter != overlappers.end();
					++oIter)
				{
					ChunkOverlapperPtr pChunkOverlapper = *oIter;
					pChunkOverlapper->pOverlappingChunk()->removable( false );
				}
			}
		}
	}
};

BW_END_NAMESPACE

#endif//LOCKED_CHUNKS_HPP_
