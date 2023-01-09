/**
 *	Image Thumbnail Provider
 */


#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "image_thumb_prov.hpp"
#include "moo/render_context.hpp"
#include "common/string_utils.hpp"

BW_BEGIN_NAMESPACE

// Token so this file gets linked in by the linker
int ImageThumbProv_token;


// Implement the Image Thumbnail Provider Factory
IMPLEMENT_THUMBNAIL_PROVIDER( ImageThumbProv )


/**
 *	This method tells the manager if the asset specified in 'file' can be
 *	handled by the ImageThumbProv.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Asset requesting the thumbnail
 *	@return			True if the asset is an image file, false otherwise.
 */
bool ImageThumbProv::isValid( const ThumbnailManager& manager, const BW::wstring& file )
{
	BW_GUARD;

	if ( file.empty() )
		return false;

	BW::wstring::size_type dot = file.find_last_of( '.' );
	BW::wstring ext = file.substr( dot + 1 );
	StringUtils::toLowerCaseT( ext );

	return ext == L"bmp"
		|| ext == L"png"
		|| ext == L"jpg"
		|| ext == L"ppm"
		|| ext == L"dds"
		|| ext == L"tga"
		|| ext == L"dib"
		|| ext == L"hdr"
		|| ext == L"pfm";
}


/**
 *	This method is called from the bg thread to prepare the thumbnail, so here
 *	we just load the image from disc into a DX testure.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Image file asset requesting the thumbnail.
 *	@return			True if the thumbnail was created, false otherwise.
 */
bool ImageThumbProv::prepare( const ThumbnailManager& manager, const BW::wstring& file )
{
	BW_GUARD;

	if ( file.empty() || !PathFileExists( file.c_str() ) )
		return false;

	//load an image into a texture
	D3DXIMAGE_INFO imgInfo;
	if ( !SUCCEEDED( D3DXGetImageInfoFromFile( file.c_str(), &imgInfo ) ) )
		return false;

	int s = manager.size();

	int origW = imgInfo.Width;
	int origH = imgInfo.Height;
	manager.recalcSizeKeepAspect( s, s, origW, origH );
	origW = ( origW >> 2 ) << 2; // align to 4-pixel boundary
	origH = ( origH >> 2 ) << 2; // align to 4-pixel boundary

	size_.x = ( ( s - origW ) >> 3 ) << 2;
	size_.y = ( ( s - origH ) >> 3 ) << 2;

	return !!( pTexture_ = Moo::rc().createTextureFromFileEx(
			file.c_str(),
			origW, origH,
			1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM,
			D3DX_FILTER_TRIANGLE,
			D3DX_DEFAULT,
			0, NULL, NULL ) );
}


/**
 *	This method is called from the main thread to render the prepared image
 *	into the passed-in render target of the thumbnail manager.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Image file asset requesting the thumbnail.
 *	@param rt		Render target to render the prepared thumbnail to.
 *	@return			True if the thumbnail was rendered, false otherwise.
 */
bool ImageThumbProv::render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt )
{
	BW_GUARD;

	if ( !pTexture_ )
		return false;
	Moo::rc().device()->Clear( 0, NULL,
		D3DCLEAR_TARGET, RGB( 255, 255, 255 ), 1, 0 );

	DX::BaseTexture* pBaseTex = rt->pTexture();
	DX::Texture* pDstTexture;
	if( pBaseTex && pBaseTex->GetType() == D3DRTYPE_TEXTURE )
	{
		pDstTexture = (DX::Texture*)pBaseTex;
		pDstTexture->AddRef();
	}
	else
		return false;
	
	DX::Surface* pSrcSurface;
	if ( !SUCCEEDED( pTexture_->GetSurfaceLevel( 0, &pSrcSurface ) ) )
	{
		pDstTexture->Release();
		return false;
	}

	DX::Surface* pDstSurface;
	if ( !SUCCEEDED( pDstTexture->GetSurfaceLevel( 0, &pDstSurface ) ) )
	{
		pSrcSurface->Release();
		pDstTexture->Release();
		return false;
	}

	// divide dest point by two and align to 4-pixel boundary
	Moo::rc().device()->UpdateSurface(
		pSrcSurface, NULL,
		pDstSurface, &size_ );

	pDstSurface->Release();
	pSrcSurface->Release();

	pDstTexture->Release();

	pTexture_ = 0;
	return true;
}

BW_END_NAMESPACE

