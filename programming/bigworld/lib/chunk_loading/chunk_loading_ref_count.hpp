#ifndef CHUNK_LOADING_REF_COUNT
#define CHUNK_LOADING_REF_COUNT

#include "chunk/chunk_cache.hpp"

class Chunk;

BW_BEGIN_NAMESPACE

/**
 *	This class is used to add extra data to chunks
 *	that is used by the chunk_loading.
 */
class ChunkLoadingRefCount: public ChunkCache
{
public:
	ChunkLoadingRefCount( Chunk & chunk );
	virtual ~ChunkLoadingRefCount();

	virtual const char* id() const;

	void incNumOverlapped() { ++numOverlapped_; }
	void decNumOverlapped() { --numOverlapped_; }

	// The number of loaded outside chunks that we overlap.
	int numOverlapped() const	{ return numOverlapped_; }

	static Instance< ChunkLoadingRefCount > instance;

private:
	// The number of loaded outside chunks that this overlapps
	int		numOverlapped_;
};

BW_END_NAMESPACE

#endif //CHUNK_LOADING_REF_COUNT
