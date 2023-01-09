#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

/**
 *	This method returns a pointer to the d3d texture of this cubemap.
 */
INLINE DX::BaseTexture* CubeRenderTarget::pTexture( )
{
	return reinterpret_cast< DX::BaseTexture* >( &*pRenderTarget_ );
}

/**
 *	This method returns the width of one cubemap surface.
 */
INLINE uint32 CubeRenderTarget::width( ) const
{
	return cubeDimensions_;
}

/**
 *	This method returns the height of one cubemap surface
 */
INLINE uint32 CubeRenderTarget::height( ) const
{
	return cubeDimensions_;
}

INLINE D3DFORMAT CubeRenderTarget::format( ) const
{
	return pixelFormat_;
}

INLINE uint32 CubeRenderTarget::textureMemoryUsed( ) const
{
	//we use A8R8G8B8 format, with one surface level
	return cubeDimensions_ * cubeDimensions_ * 4 * 6;
}

INLINE const BW::string& CubeRenderTarget::resourceID( ) const
{
	return identifier_;
}

}

/*cube_render_target.ipp*/
