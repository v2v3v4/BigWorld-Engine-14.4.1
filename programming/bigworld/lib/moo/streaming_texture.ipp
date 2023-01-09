// streaming_texture.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

	INLINE void StreamingTexture::streamingTextureHandle( 
		StreamingTextureHandle handle )
	{
		streamingTextureHandle_ = handle;
	}

	INLINE StreamingTexture::StreamingTextureHandle 
		StreamingTexture::streamingTextureHandle() const
	{
		return streamingTextureHandle_;
	}

}

// streaming_texture.ipp