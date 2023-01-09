#include "pch.hpp"

#include "texture_reuse_cache.hpp"
#include "math/math_extra.hpp"
#include "texture_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

TextureReuseCache::TextureReuseCache() :
	memoryUsageGPU_( 0 ),
	memoryUsageMain_( 0 )
{

}

TextureReuseCache::DeviceTexturePtr TextureReuseCache::requestTexture(
	uint32 width, uint32 height, int nLevels,
	uint32 usage, DeviceFormat fmt, DevicePool pool, bool recordRequest )
{
	if (nLevels <= 0)
	{
		// The reuse cache needs to know the number of levels expected
		nLevels = BW::log2ceil( max( width, height) ) + 1;
	}

	SimpleMutexHolder smh( lock_ );

	TextureSpecification spec;
	spec.width_ = width;
	spec.height_ = height;
	spec.numLevels_ = nLevels;
	spec.usage_ = usage;
	spec.format_ = fmt;
	spec.pool_ = pool;

	DeviceTexturePtr pTexture;
	for (TextureCollection::reverse_iterator itr = entries_.rbegin(),
		end = entries_.rend(); itr != end; ++itr)
	{
		TextureEntry & entry = *itr;
		if (entry.spec_ == spec)
		{
			pTexture = entry.texture_;

			// Remove the amount of memory
			memoryUsageGPU_ -= entry.memoryUsageGPU_;
			memoryUsageMain_ -= entry.memoryUsageMain_;

			// Erase using a reverse iterator
			entries_.erase( (++itr).base() );

			break;
		}
	}

	uint32 updateType = (recordRequest) ? STAT_REQUEST : 0;
	updateType |= pTexture.hasComObject() ? (STAT_HIT) : (0);
	updateStatistics( spec, updateType );
	
	return pTexture;
}

TextureReuseCache::DeviceTexturePtr TextureReuseCache::requestPartialTexture(
	uint32 width, uint32 height, uint32 nLevels,
	uint32 usage, DeviceFormat fmt, DevicePool pool, 
	uint32* extraMipsAssigned, uint32 extraMipsHigherAllowed )
{
	// This method allows reusing a texture even though it is actually
	// larger than one that we want. This is often useful for staging
	// textures, as we only want a place to push data to temporarily
	// before pushing it further to the GPU. 

	// As a rule, we don't
	// allow partial matches for sizes larger than a given constant
	// defined below:
	const uint32 maxMipForPartialMatch = 11; // (2048 texture)
	uint32 topMipOfRequest = BW::log2ceil( max( width, height ) );
	
	// Check the inputs
	// Make sure extra mips is a value that makes sense
	if (topMipOfRequest > maxMipForPartialMatch)
	{
		extraMipsHigherAllowed = 0;
	}
	else if (extraMipsHigherAllowed == DEFAULT_EXTRA_MIPS ||
		topMipOfRequest + extraMipsHigherAllowed > maxMipForPartialMatch)
	{
		// Handle the default max number of mips we're allowing on
		// this texture. Also set to this if it is above our
		// limit.
		extraMipsHigherAllowed = maxMipForPartialMatch - topMipOfRequest;
	}

	// Loop through the possible larger texture sizes until we find a match:
	for (uint32 extraHigherMips = 0; extraHigherMips <= extraMipsHigherAllowed; ++extraHigherMips)
	{
		uint32 topMip = topMipOfRequest + extraHigherMips;

		// Loop through the possible number of extra lower mips till we
		// find a match
		for (uint32 extraLowerMips = 0; 
			nLevels + extraLowerMips <= topMip + 1; 
			++extraLowerMips)
		{
			// Attempt to find a texture
			const bool isOriginalSpec = extraHigherMips == 0 && extraLowerMips == 0;
			DeviceTexturePtr result = requestTexture(
				width << extraHigherMips, 
				height << extraHigherMips, 
				nLevels + extraHigherMips + extraLowerMips, 
				usage, 
				fmt,
				pool,
				isOriginalSpec);

			if (result.hasComObject())
			{
				if (extraMipsAssigned)
				{
					*extraMipsAssigned = extraHigherMips;
				}
				return result;
			}
		}
	}

	return NULL;
}

void TextureReuseCache::putTexture( const DeviceTexturePtr & pTexture )
{
	TextureEntry entry;
	extractSpecification( entry.spec_, pTexture );
	entry.texture_ = pTexture;

	// Check if the cache defines this texture as reusable
	uint32 updateType = STAT_PUT;
	if (isReusable( entry.spec_ ))
	{
		entry.memoryUsageGPU_ = DX::textureSize( entry.texture_.pComObject(), ResourceCounters::MT_GPU );
		entry.memoryUsageMain_ = DX::textureSize( entry.texture_.pComObject(), ResourceCounters::MT_MAIN );
		entry.timeEntered_ = static_cast< float >( TimeStamp::toSeconds( timestamp() ) );
		
		memoryUsageGPU_ += entry.memoryUsageGPU_;
		memoryUsageMain_ += entry.memoryUsageMain_;

		SimpleMutexHolder smh( lock_ );
		entries_.push_back( entry );
	}
	else
	{
		updateType |= STAT_DISCARD;
	}

#if !CONSUMER_CLIENT_BUILD 
	{
		SimpleMutexHolder smh( lock_ );
		updateStatistics( entry.spec_, updateType );
	}
#endif
}

void TextureReuseCache::clear()
{
	SimpleMutexHolder smh( lock_ );

	// Iterate over each bucket's pool and clear textures
	entries_.clear();

	// Iterate over all statistics and update their counts
	// (NOTE: if stats are disabled, the iteration will exit quickly)
	for (TextureStatisticsMap::iterator iter = statistics_.begin(), end = statistics_.end();
		iter != end; ++iter)
	{
		TextureStatistics & stats = iter->second;
		stats.cachedCount_ = 0;
	}
}

void TextureReuseCache::updateStatistics( const TextureSpecification & spec, 
	uint32 updateType )
{
	// Cache statistics only available on development builds
#if !CONSUMER_CLIENT_BUILD 
	TextureStatistics & stats = statistics_[spec];

	if (updateType & STAT_REQUEST)
	{
		++stats.requestCount_;
		++stats.currentInstances_;
		stats.peakInstances_ = max( stats.peakInstances_, stats.currentInstances_ );
	}

	if (updateType & STAT_PUT)
	{
		--stats.currentInstances_;
		++stats.cachedCount_;
	}

	if (updateType & STAT_HIT)
	{
		++stats.hitCount_;
		--stats.cachedCount_;
	}

	if (updateType & STAT_DISCARD)
	{
		--stats.cachedCount_;
	}
#endif
}

void TextureReuseCache::generateReport( FILE * f ) const
{
	if (!f)
	{
		return;
	}
	
	// Output a list of all textures in the reuse cache, along with statistics 
	// on each one.

	fprintf( f, "\nCached Texture Type Statistics:\n" );
	fprintf( f, "Pool;Usage;Format;Width;Height;NumMips;RequestCount;HitCount;MissCount;CurrentInst;PeakInst;Count;\n" );

	// Iterate over each bucket's pool and clear textures
	for (TextureStatisticsMap::const_iterator iter = statistics_.begin(), end = statistics_.end();
		iter != end; ++iter)
	{
		const TextureStatistics & stats = iter->second;
		const TextureSpecification & spec = iter->first;

		fprintf( f, "%d;%d;%s;%d;%d;%d;%d;%d;%d;%d;%d;%d;\n",
			spec.pool_,
			spec.usage_,
			TextureManager::getFormatName( spec.format_ ).c_str(),
			spec.width_,
			spec.height_,
			spec.numLevels_,
			stats.requestCount_,
			stats.hitCount_,
			stats.requestCount_ - stats.hitCount_,
			stats.currentInstances_,
			stats.peakInstances_,
			stats.cachedCount_
			);
	}

	// Output info about all textures in the list
	fprintf( f, "\nCached Textures:\n" );
	fprintf( f, "TimeInCache(s);Pool;Usage;Format;Width;Height;NumMips;GpuMemory;SysMemory;\n" );

	size_t totalGPUMemory = 0;
	size_t totalSysMemory = 0;
	size_t totalTextureTypes[4] = { 0, 0, 0, 0 };

	float curTime = (float)TimeStamp::toSeconds( timestamp() );

	for (TextureCollection::const_iterator iter = entries_.begin();
		iter != entries_.end();
		++iter)
	{
		const TextureEntry & entry = *iter;
		const TextureSpecification & spec = entry.spec_;

		fprintf( f, "%f;%d;%d;%s;%d;%d;%d;%" PRIzu ";%" PRIzu ";\n",
			curTime - entry.timeEntered_,
			spec.pool_,
			spec.usage_,
			TextureManager::getFormatName( spec.format_ ).c_str(),
			spec.width_,
			spec.height_,
			spec.numLevels_,
			entry.memoryUsageGPU_,
			entry.memoryUsageMain_
			);

		// Update totals
		MF_ASSERT( spec.pool_ < ARRAY_SIZE(totalTextureTypes) );
		totalTextureTypes[spec.pool_]++;
	}

	// Output total memory used
	fprintf( f, "\nCached Texture Totals:;%" PRIzu "\n", entries_.size());
	fprintf( f, "Total GPU memory:;%" PRIzu "\n", memoryUsageGPU_ );
	fprintf( f, "Total System memory:;%" PRIzu "\n", memoryUsageMain_ );
	fprintf( f, "Total default textures:;%" PRIzu "\n",
		totalTextureTypes[D3DPOOL_DEFAULT] );
	fprintf( f, "Total managed textures:;%" PRIzu "\n",
		totalTextureTypes[D3DPOOL_MANAGED] );
	fprintf( f, "Total system textures:;%" PRIzu "\n",
		totalTextureTypes[D3DPOOL_SYSTEMMEM] );
	fprintf( f, "Total scratch textures:;%" PRIzu "\n",
		totalTextureTypes[D3DPOOL_SCRATCH] );
}

size_t TextureReuseCache::size() const
{
	return entries_.size();
}

size_t TextureReuseCache::memoryUsageGPU() const
{
	return memoryUsageGPU_;
}

size_t TextureReuseCache::memoryUsageMain() const
{
	return memoryUsageMain_;
}

void TextureReuseCache::extractSpecification( 
	TextureSpecification & spec, const DeviceTexturePtr & pTexture )
{
	D3DSURFACE_DESC desc;
	pTexture->GetLevelDesc( 0, &desc );

	spec.width_ = desc.Width;
	spec.height_ = desc.Height;
	spec.format_ = desc.Format;
	spec.usage_ = desc.Usage;
	spec.numLevels_ = pTexture->GetLevelCount();
	spec.pool_ = desc.Pool;
}

bool TextureReuseCache::isReusable( const TextureSpecification & spec )
{
#define ENABLE_TEXTURE_REUSE 0
#if ENABLE_TEXTURE_REUSE
	return true;
#else
	return false;
#endif
}

TextureReuseCache::TextureStatistics::TextureStatistics() : 
	requestCount_( 0 ), 
	hitCount_( 0 ), 
	currentInstances_( 0 ), 
	peakInstances_( 0 ), 
	cachedCount_( 0 )
{

}

bool TextureReuseCache::TextureSpecification::operator < (
	const TextureSpecification & other) const
{
	if (width_ != other.width_)
	{
		return width_ < other.width_;
	}

	if (height_ != other.height_)
	{
		return height_ < other.height_;
	}

	if (numLevels_ != other.numLevels_)
	{
		return numLevels_ < other.numLevels_;
	}

	if (usage_ != other.usage_)
	{
		return usage_ < other.usage_;
	}

	if (pool_ != other.pool_)
	{
		return pool_ < other.pool_;
	}

	if (format_ != other.format_)
	{
		return format_ < other.format_;
	}

	// Otherwise we're equal!
	return false;
}

bool TextureReuseCache::TextureSpecification::operator == (
	const TextureSpecification & other) const
{
	return 
		other.width_ == width_ && 
		other.height_ == height_ && 
		other.numLevels_ == numLevels_ &&
		other.usage_ == usage_ && 
		other.format_ == format_ && 
		other.pool_ == pool_;
}

} // namespace Moo

BW_END_NAMESPACE

// texture_reuse_cache.cpp
