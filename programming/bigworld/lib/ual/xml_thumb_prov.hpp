/**
 *	XML File Thumbnail Provider (particles, lights, etc)
 */


#ifndef XML_THUMB_PROV_HPP
#define XML_THUMB_PROV_HPP

#include "thumbnail_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	XML File Provider
 */
class XmlThumbProv : public ThumbnailProvider
{
public:
	XmlThumbProv() {};
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size );
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt  );

private:
	DECLARE_THUMBNAIL_PROVIDER()

	bool isParticleSystem( const BW::wstring& file );
	bool isLight( const BW::wstring& file );
	BW::wstring particleImageFile();
	BW::wstring lightImageFile();
};

BW_END_NAMESPACE

#endif // XML_THUMB_PROV_HPP
