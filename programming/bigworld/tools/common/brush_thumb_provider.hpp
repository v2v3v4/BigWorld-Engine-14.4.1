#ifndef BRUSH_THUMB_PROVIDER_HPP
#define BRUSH_THUMB_PROVIDER_HPP


#include "ual/image_thumb_prov.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class helps generate thumbnails for .brush files.
 */
class BrushThumbProvider : public ThumbnailProvider
{
public:
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt );

protected:
	BW::wstring getTextureFileForBrush( const BW::wstring & file ) const;

private:
	DECLARE_THUMBNAIL_PROVIDER()

	ImageThumbProv		imageProvider_;
};

BW_END_NAMESPACE

#endif // BRUSH_THUMB_PROVIDER_HPP
