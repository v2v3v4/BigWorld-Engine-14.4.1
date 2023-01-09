/**
 *	Image Thumbnail Provider
 */


#ifndef IMAGE_THUMB_PROV_HPP
#define IMAGE_THUMB_PROV_HPP

#include "thumbnail_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements an Image Thumbnail Provider, used for image files.
 */
class ImageThumbProv : public ThumbnailProvider
{
public:
	ImageThumbProv() : pTexture_( 0 ) {};
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool render( const ThumbnailManager& manager,
		const BW::wstring& file, Moo::RenderTarget* rt  );

private:
	ComObjectWrap<DX::Texture> pTexture_;
	POINT size_;

	DECLARE_THUMBNAIL_PROVIDER()
};

BW_END_NAMESPACE

#endif // IMAGE_THUMB_PROV_HPP
