#ifndef TERRAIN_BLOCK_CACHE_HPP
#define TERRAIN_BLOCK_CACHE_HPP

#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{

class BaseTerrainBlock;
typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
class TerrainSettings;
typedef SmartPointer<TerrainSettings> TerrainSettingsPtr;

/**
 *	An entry in the terrain block cache. This class was created to keep the
 * 	reference count of the BaseTerrainBlock separate from the reference count
 * 	of the cache entry. This is to minimise mutex locking operations since
 * 	every time the cache entry is incRef/decRef, we need to hold lock on the
 * 	terrain cache. Keeping the count separate means that BaseTerrainBlockPtr
 * 	can be used freely without incurring overhead of locking.
 */
class TerrainBlockCacheEntry : public ReferenceCount
{
	BaseTerrainBlockPtr	pBlock_;

public:
	TerrainBlockCacheEntry( Terrain::BaseTerrainBlockPtr pBlock ) :
		pBlock_( pBlock )
	{}

	void incRef() const;
	void decRef() const;

	BaseTerrainBlockPtr pBlock() { return pBlock_; }
};
typedef SmartPointer<TerrainBlockCacheEntry> TerrainBlockCacheEntryPtr;

/**
 *	This class caches terrain blocks, keyed by the resource ID.
 *
 * 	Currently used by the server to save memory when running multiple instances
 * 	of the same space geometry.
 */
class TerrainBlockCache
{
public:
	static TerrainBlockCache& instance();

	TerrainBlockCacheEntryPtr findOrLoad( const BW::string& resourceID, 
		TerrainSettingsPtr pSettings );

	// Interface for TerrainBlockCacheEntry
	void incRefCacheEntry( const TerrainBlockCacheEntry& entry );
	void decRefCacheEntry( const TerrainBlockCacheEntry& entry );

private:
	TerrainBlockCacheEntry* add( BaseTerrainBlockPtr pBlock );
	TerrainBlockCacheEntry* find( const BW::string& resourceID );

	typedef BW::map< BW::string, TerrainBlockCacheEntry* > Map;
	Map map_;

	mutable SimpleMutex lock_;
};

} // namespace Terrain

BW_END_NAMESPACE


#endif /*TERRAIN_BLOCK_CACHE_HPP*/
