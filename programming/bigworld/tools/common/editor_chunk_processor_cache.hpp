#ifndef EDITOR_CHUNK_PROCESSOR_CACHE_HPP
#define EDITOR_CHUNK_PROCESSOR_CACHE_HPP


#include "chunk/chunk_cache.hpp"
#include "chunk/chunk_processor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a base class for the editor extension chunk caches
 *	that has ChunkProcessor in it.
 */
class EditorChunkProcessorCache : 
	public ChunkCache,
	public ChunkProcessorListener 
{
public:
	virtual bool isBeingCalculated() const;

	virtual bool cancelAllCalculating( ChunkProcessorManager* pManager,
		ChunkProcessors& outUnfinishedProcessors );

	virtual void onChunkProcessorFinished( ChunkProcessor* pProcessor,
		bool& hasUnregisteredOffListener );

	void registerProcessor( ChunkProcessorPtr processor );

protected:
	// current effective processor.
	ChunkProcessorPtr currentProcessor_;

private:
	// the processors that have been sent to the
	// background tasks and not finished processing,
	// can be the currentProcessor_,
	// can be one that has been given up since a 
	// new currentProcessor_ is set.
	ChunkProcessors processingChunkProcessors_;
};

BW_END_NAMESPACE
#endif // EDITOR_CHUNK_PROCESSOR_CACHE_HPP
