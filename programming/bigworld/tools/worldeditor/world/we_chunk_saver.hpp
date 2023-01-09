#ifndef WE_CHUNK_SAVER_HPP
#define WE_CHUNK_SAVER_HPP

#include "chunk/unsaved_chunks.hpp"

BW_BEGIN_NAMESPACE

class WEChunkSaver : public UnsavedChunks::IChunkSaver
{
public:
	virtual bool save( Chunk* chunk );
	virtual bool isDeleted( Chunk& chunk ) const;
};

BW_END_NAMESPACE
#endif // WE_CHUNK_SAVER_HPP
