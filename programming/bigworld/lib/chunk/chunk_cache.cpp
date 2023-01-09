#include "pch.hpp"

#include "chunk.hpp"
#include "chunk_cache.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "chunk_cache.ipp"
#endif

int ChunkCache::s_nextCacheIndex_ = 0;


#ifdef EDITOR_ENABLED

/**
*	This function is called whenever the chunk
*	associated this chunk cache is changed,
*	only if the passed flags include my flag
*	then I will need update.
*
*	@param InvalidateFlags	the flags to notify which 
*								type of ChunkCache need update.
*	@param mgr				the processor manager.
*	@param pChangedItem		the chunk item that caused the chunk to be changed.
*/
void ChunkCache::onChunkChanged( InvalidateFlags flags,
							ChunkProcessorManager* mgr,
							EditorChunkItem* pChangedItem/* = NULL*/)
{
	if (flags.match( this->invalidateFlag_ ))
	{
		this->invalidate( mgr, true, pChangedItem );
	}
}


void ChunkCache::writeBool(
	DataSectionPtr ds,
	const BW::string& sectionName,
	bool value )
{
	BW_GUARD;
	uint32 flag = value ? 1 : 0;

	ds->writeBinary( sectionName,
		new BinaryBlock( &flag, sizeof( flag ), "ChunkCache" ) );
}


bool ChunkCache::readBool(
	DataSectionPtr ds,
	const BW::string& sectionName,
	bool defaultValue )
{
	BW_GUARD;
	if (ds)
	{
		BinaryPtr bp = ds->readBinary( sectionName );

		if (bp && bp->len() == sizeof( uint32 ))
		{
			defaultValue = !!*(uint32*)bp->data();
		}
	}

	return defaultValue;
}
#endif// EDITOR_ENABLED


int ChunkCache::registerCache( TouchFunction tf )
{
	BW_GUARD;
	touchType().push_back( tf );

	return s_nextCacheIndex_++;
}


int ChunkCache::getTouchTypeIndex( void* id )
{
	BW_GUARD;
	if (touchTypeIndexMap().find( id ) != touchTypeIndexMap().end())
	{
		return touchTypeIndexMap().find( id )->second;
	}

	return INVALID_CACHE_INDEX;
}


void ChunkCache::setTouchTypeIndex( void* id, int index )
{
	BW_GUARD;
	touchTypeIndexMap()[ id ] = index;
}

BW_END_NAMESPACE

// chunk_cache.cpp
