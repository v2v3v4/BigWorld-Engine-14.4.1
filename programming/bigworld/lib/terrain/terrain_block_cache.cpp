#include "terrain_block_cache.hpp"

#include "base_terrain_block.hpp"
#include "terrain_settings.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

//------------------------------------------------------------------------------
// TerrainBlockCacheEntry
//------------------------------------------------------------------------------

void TerrainBlockCacheEntry::incRef() const
{
	TerrainBlockCache::instance().incRefCacheEntry( *this );
}

void TerrainBlockCacheEntry::decRef() const
{
	TerrainBlockCache::instance().decRefCacheEntry( *this );
}


//------------------------------------------------------------------------------
// TerrainBlockCache
//------------------------------------------------------------------------------

/**
 *	This static method returns the singleton instance of the terrain cache.
 */
TerrainBlockCache& TerrainBlockCache::instance()
{
	static TerrainBlockCache singleton;
	return singleton;
}

/**
 *	This method returns the BaseTerrainBlock associated with the
 * 	input resourceID. If the BaseTerrainBlock has not yet been loaded, it is
 * 	loaded and returned.
 */
TerrainBlockCacheEntryPtr TerrainBlockCache::findOrLoad(
		const BW::string& resourceID, TerrainSettingsPtr pSettings  )
{
	TerrainBlockCacheEntry* pExisting = this->find( resourceID );
	if (pExisting)
	{
		return pExisting;
	}

	BaseTerrainBlockPtr pNewBlock = BaseTerrainBlock::loadBlock(
				resourceID, Matrix::identity, Vector3::zero(), 
				pSettings );

	if (pNewBlock)
	{
		return this->add( pNewBlock );
	}
	return NULL;
}

/**
 *	This method adds the terrain block into this cache and returns the
 * 	cache entry pointer.
 */
TerrainBlockCacheEntry* TerrainBlockCache::add( BaseTerrainBlockPtr pBlock )
{
	SimpleMutexHolder smh( lock_ );
	TerrainBlockCacheEntry* pEntry = new TerrainBlockCacheEntry( pBlock );
	map_[ pBlock->resourceName() ] = pEntry;
	return pEntry;
}

/**
 *	This method attempts to find an already loaded BaseTerrainBlock and return it.
 *	If the BaseTerrainBlock has not yet been loaded, NULL is returned.
 */
TerrainBlockCacheEntry* TerrainBlockCache::find(
		const BW::string& resourceID )
{
	SimpleMutexHolder smh( lock_ );
	Map::iterator it = map_.find( resourceID );
	return (it != map_.end()) ?  it->second : NULL;
}

/**
 *	ReferenceCountedCacheEntry interface function
 */
void TerrainBlockCache::incRefCacheEntry( const TerrainBlockCacheEntry& entry )
{
	SimpleMutexHolder smh( lock_ );
	entry.ReferenceCount::incRef();
}

/**
 *	ReferenceCountedCacheEntry interface function
 */
void TerrainBlockCache::decRefCacheEntry( const TerrainBlockCacheEntry& entry )
{
	SimpleMutexHolder smh( lock_ );

	if (entry.refCount() == 1)
	{
		map_.erase( const_cast<TerrainBlockCacheEntry&>( entry ).
				pBlock()->resourceName() );
	}

	entry.ReferenceCount::decRef();
}

} // namespace Terrain

BW_END_NAMESPACE

// terrain_block_cache.cpp
