
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE DX::BaseTexture*	RenderTarget::pTexture( )
{
	if (ensureAllocated())
	{
		return pRenderTarget_.pComObject();
	}
	else
	{
		return NULL;
	}	
}

INLINE
bool	RenderTarget::recreateForD3DExDevice() const
{
	return true;
}

INLINE DX::Surface*	RenderTarget::depthBuffer( )
{
	if (ensureAllocated())
	{
		return pDepthStencilTarget_.pComObject();
	}
	else
	{
		return NULL;
	}
}

INLINE uint32 RenderTarget::width( ) const
{
	return width_;
}

INLINE uint32 RenderTarget::height( ) const
{
	return height_;
}

INLINE D3DFORMAT RenderTarget::format( ) const
{
	return pixelFormat_;
}

INLINE const BW::string& RenderTarget::resourceID( ) const
{
	return resourceID_;
}

}

/*render_target.ipp*/
