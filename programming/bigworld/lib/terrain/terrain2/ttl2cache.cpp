#include "pch.hpp"
#ifdef EDITOR_ENABLED
#include "ttl2cache.hpp"
#include "../manager.hpp"
#include "terrain_texture_layer2.hpp"
#include "cstdmf/concurrency.hpp"

DECLARE_DEBUG_COMPONENT2("Moo", 0)


BW_BEGIN_NAMESPACE

using namespace Terrain;

namespace
{
	SimpleMutex		cacheMutex;
}

/**
 *	This gets the TTL2Cache from the Manager.
 */
/*static*/ TTL2Cache *TTL2Cache::instance()
{
	if ( Manager::pInstance() == NULL )
		return NULL;

	return &Manager::instance().ttl2Cache();
}


/**
 *	This is the default constructor for TTL2Cache as needed by the compiler.
 */
TTL2Cache::TTL2Cache()
{
}


/**
 *	This method clears the cache lists.
 */
void TTL2Cache::clear()
{
	layers_.clear();
}


/**
 *	This is called then a TerrainTextureLayer is being locked.  We make
 *	sure that it is online and uncompressed, ready for reading and editing.
 *
 *	@param layer	The layer being locked.
 *  @param readOnly	Is the layer only being read?
 */
void TTL2Cache::onLock(TerrainTextureLayer2 *layer, bool /*readOnly*/)
{
	BW_GUARD;
	SimpleMutexHolder holder(cacheMutex);

	// If the layer was already in the cache remove it.
	LayerList::iterator it = std::find(layers_.begin(), layers_.end(), layer);
	if (it != layers_.end())
		layers_.erase(it);

	// Add the to front of the cache:
	layers_.push_front(layer);

	// Make sure that the new layer is decompressed:
	if ((layer->state() & TerrainTextureLayer2::BLENDS) == 0)
		layer->decompressBlend();

	// If the cache has grown too large remove the last element:
	if (layers_.size() > CACHE_SIZE)
	{
		TerrainTextureLayer2 *last = layers_.back();
		last->compressBlend(); 
		layers_.pop_back();
	}
}


/**
 *	This is called when a TerrainTextureLayer2 is unlocked.
 *
 *  @param layer	The unlocked layer.
 */
void TTL2Cache::onUnlock(TerrainTextureLayer2 * /*layer*/)
{
}


/**
 *	This is called when a TerrainTextureLayer2 is deleted.  We simply
 *  remove if from the cache.
 *
 *  @param layer	The layer being deleted.
 */
void TTL2Cache::delTextureLayer(TerrainTextureLayer2 *layer)
{
	BW_GUARD;
	SimpleMutexHolder holder(cacheMutex);

	LayerList::iterator it = std::find(layers_.begin(), layers_.end(), layer);
	if (it != layers_.end())
		layers_.erase(it);
}

BW_END_NAMESPACE

#endif // EDITOR_ENABLED
