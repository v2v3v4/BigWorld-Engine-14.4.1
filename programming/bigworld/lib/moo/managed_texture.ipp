// managed_texture.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
uint32 ManagedTexture::textureMemoryUsed( ) const
{
	return textureMemoryUsed_;
}

INLINE
uint32		ManagedTexture::mipSkip() const
{ 
	return mipSkip_;
}

INLINE
void		ManagedTexture::mipSkip( uint32 mipSkip )
{ 
	mipSkip_ = mipSkip;
}

INLINE
uint32 ManagedTexture::width( ) const
{
	return width_;
}

INLINE
uint32 ManagedTexture::height( ) const
{
	return height_;
}

INLINE
D3DFORMAT ManagedTexture::format( ) const
{
	return format_;
}

INLINE
const BW::string& ManagedTexture::resourceID( ) const
{
	return resourceID_;
}

}

// managed_texture.ipp