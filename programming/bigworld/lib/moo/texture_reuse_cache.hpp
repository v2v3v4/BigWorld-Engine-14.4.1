#ifndef TEXTURE_REUSE_CACHE_HPP
#define TEXTURE_REUSE_CACHE_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_vector.hpp"
#include "moo_dx.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This cache is designed to attempt to store textures in its cache
 * to save them from being released back to the device when they are
 * going to be recreated in the near future (streaming or updating).
 * it abstracts the strategy for caching these things from the 
 * implementation of a specific device API.
 */
class TextureReuseCache
{
public:

	typedef ComObjectWrap< DX::Texture > DeviceTexturePtr;
	typedef D3DPOOL DevicePool;
	typedef D3DFORMAT DeviceFormat;

	static const uint32 DEFAULT_EXTRA_MIPS = ~0u;

	TextureReuseCache();

	DeviceTexturePtr requestTexture(
		uint32 width, uint32 height, int nLevels,
		uint32 usage, DeviceFormat fmt, DevicePool pool, 
		bool recordRequest = true );
	DeviceTexturePtr requestPartialTexture(
		uint32 width, uint32 height, uint32 nLevels,
		uint32 usage, DeviceFormat fmt, DevicePool pool,
		uint32 * extraMipsAssigned = NULL,
		uint32 extraMipsAllowed = DEFAULT_EXTRA_MIPS );
	void putTexture( const DeviceTexturePtr & pTexture );
	void clear();
	void generateReport( FILE * file ) const;

	size_t size() const;
	size_t memoryUsageGPU() const;
	size_t memoryUsageMain() const;

protected:
	
	enum StatisticsEvent
	{
		STAT_REQUEST = 0x01,
		STAT_PUT = 0x02,
		STAT_HIT = 0x04,
		STAT_DISCARD = 0x08
	};

	struct TextureStatistics
	{
		TextureStatistics();

		// Statistics and tracking
		uint32 requestCount_;
		uint32 hitCount_;
		uint32 currentInstances_;
		uint32 peakInstances_;
		uint32 cachedCount_;
	};

	struct TextureSpecification
	{
		bool operator < (const TextureSpecification & other) const;
		bool operator == (const TextureSpecification & other) const;

		uint32 width_;
		uint32 height_;
		int numLevels_;
		uint32 usage_;
		DevicePool pool_;
		DeviceFormat format_;
	};

	struct TextureEntry
	{
		TextureSpecification spec_;
		DeviceTexturePtr texture_;
		float timeEntered_;
		size_t memoryUsageGPU_;
		size_t memoryUsageMain_;
	};

	typedef BW::list< TextureEntry > TextureCollection;
	typedef BW::map< TextureSpecification, TextureStatistics > TextureStatisticsMap;
	
	void updateStatistics( const TextureSpecification & spec, uint32 updateType );

	static bool isReusable( const TextureSpecification & spec );
	static void extractSpecification( TextureSpecification & spec, 
		const DeviceTexturePtr & pTexture );

	TextureCollection entries_;
	TextureStatisticsMap statistics_;
	size_t memoryUsageGPU_;
	size_t memoryUsageMain_;
	mutable SimpleMutex lock_;
};


} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_REUSE_CACHE_HPP
