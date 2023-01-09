#include "chunk/chunk.hpp"

#include "chunk_loading_ref_count.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkLoadingRefCount
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkLoadingRefCount::ChunkLoadingRefCount( Chunk & /* chunk */ ) :
	numOverlapped_( 0 )
{
}

/**
 *	Destructor
 */
ChunkLoadingRefCount::~ChunkLoadingRefCount()
{
}

/**
 *	Get the ChunkLoadingRefCount's identifier.
 *	@return name of the type of chunk cache.
 */
const char* ChunkLoadingRefCount::id() const
{
	return "ChunkLoadingRefCount";
}

// static instance initialiser
ChunkLoadingRefCount::Instance< ChunkLoadingRefCount >
			ChunkLoadingRefCount::instance;

BW_END_NAMESPACE

// chunk_loading_ref_count.cpp
