
#include "pch.hpp"
#include "cstdmf/guard.hpp"
#include "worldeditor/world/we_chunk_saver.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"

BW_BEGIN_NAMESPACE

bool WEChunkSaver::save( Chunk* chunk )
{
	BW_GUARD;
	return EditorChunkCache::instance( *chunk ).edSave();
}
bool WEChunkSaver::isDeleted( Chunk& chunk ) const
{
	BW_GUARD;
	return EditorChunkCache::instance( chunk ).edIsDeleted();
}
BW_END_NAMESPACE

