#include "pch.hpp"
#include "lighting_influence.hpp"
#include "chunk/chunk_space.hpp"
#include "math/colour.hpp"

BW_BEGIN_NAMESPACE

using namespace LightingInfluence;

ChunkVisitor::ChunkVisitor( uint32 maxDepth ) :
	maxDepth_( maxDepth )
{
	BW_GUARD;
}


/**
*	This method visit chunks through portals
*/
void LightingInfluence::visitChunks( Chunk* srcChunk,
								 ChunkVisitor* pVisitor,
								 BW::set<Chunk*>& searchedChunks/* = BW::set<Chunk*>()*/,
								 uint32 currentDepth/* = 0*/ )
{
	BW_GUARD;

	searchedChunks.insert( srcChunk );

	pVisitor->action( srcChunk );

	// Stop if we've reached the maximum portal traversal depth
	if (currentDepth == pVisitor->maxDepth())
	{
		return;
	}

	for (Chunk::piterator pit = srcChunk->pbegin(); pit != srcChunk->pend(); pit++)
	{
		if (!pit->hasChunk() || !pit->pChunk->isBound())
		{
			continue;
		}

		// We've already searched it, skip
		if (searchedChunks.find( pit->pChunk ) != searchedChunks.end())
		{
			continue;
		}

		if (!pVisitor->considerChunk( pit->pChunk ))
		{
			continue;
		}

		// TODO: Check that we can actually see the portal. Only really of value if
		// we get lights going across > 2 chunks.

		visitChunks( pit->pChunk, pVisitor, 
			searchedChunks, currentDepth + 1 );
	}
}

BW_END_NAMESPACE

