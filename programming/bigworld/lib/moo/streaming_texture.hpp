// streaming_texture.hpp

#ifndef STREAMING_TEXTURE_HPP
#define STREAMING_TEXTURE_HPP

#include "base_texture.hpp"
#include "managed_texture.hpp"
#include "texture_manager.hpp"
#include "texture_streaming_manager.hpp"
#include "streaming_manager.hpp"

BW_BEGIN_NAMESPACE

class IFileStreamer;

namespace Moo
{
class TextureStreamingManager;
class TextureDetailLevel;

namespace StreamingTextureLoader
{
	struct HeaderInfo
	{
		HeaderInfo();

		bool valid_;
		uint32 width_;
		uint32 height_;
		uint32 numMipLevels_;
		D3DFORMAT format_;
		bool isFlatTexture_;
	};

	struct LoadOptions
	{
		LoadOptions();

		uint32 mipSkip_;
		uint32 numMips_;
	};

	struct LoadResult
	{
		ComObjectWrap< DeviceTexture2D > texture2D_;
	};

	bool createTexture( const HeaderInfo & header, const LoadOptions & options, 
		LoadResult & output );
	bool loadHeaderFromStream( IFileStreamer * pStream, HeaderInfo & header );
	bool loadTextureFromStream( IFileStreamer * pStream, 
		const HeaderInfo & header, const LoadOptions & options, 
		LoadResult & result );
	bool copyMipChain( DeviceTexture2D * pDstTex, uint32 dstBeginLevel, 
		DeviceTexture2D * pSrcTex, uint32 srcBeginLevel, uint32 numLevels );
};

typedef SmartPointer< class StreamingTexture > StreamingTexturePtr;

class StreamingTexture : public BaseTexture
{
public:
	typedef TextureStreamingManager::StreamingTextureHandle 
		StreamingTextureHandle;

	StreamingTexture( TextureDetailLevel * detail,
		const BW::StringRef & resourceID, const FailurePolicy & notFoundPolicy,
		D3DFORMAT format = D3DFMT_UNKNOWN,
		int mipSkip = 0, bool noResize = false, bool noFilter = false,
		const BW::StringRef & allocator = "texture/unknown managed texture" );


	virtual DX::BaseTexture * pTexture();
	virtual TextureType type() const;
	
	virtual void reload();
	virtual void reload( const BW::StringRef & resourceID );

	void assignTexture( DeviceTexture * pTex );
	void assignDebugTexture( DeviceTexture * pTex );

	void streamingTextureHandle( StreamingTextureHandle handle );
	StreamingTextureHandle streamingTextureHandle() const;

	TextureDetailLevel * detail();
	DeviceTexture * assignedTexture();

	virtual uint32 width() const;
	virtual uint32 height() const;
	virtual D3DFORMAT format() const;
	virtual uint32 textureMemoryUsed() const;
	virtual const BW::string & resourceID() const;
	virtual uint32 maxMipLevel() const;
	virtual uint32 mipSkip() const;
	virtual void mipSkip( uint32 mipSkip );

private:
	virtual void destroy() const;
	virtual ~StreamingTexture();

	TextureStreamingManager * streamingManager() const;
	
	StreamingTextureHandle streamingTextureHandle_;
	TextureDetailLevelPtr detail_;
	DeviceTexture * activeTexture_;
	ComObjectWrap< DeviceTexture > curTexture_;
	BW::string resourceID_;
	uint32 mipSkip_;
	float mipLodBias_;
	uint32 textureMemoryUsed_;

	friend void TextureStreamingManager::tryDestroy( const StreamingTexture * );
};



} // namespace Moo

#ifdef CODE_INLINE
#include "streaming_texture.ipp"
#endif

BW_END_NAMESPACE

#endif
// streaming_texture.hpp
 
