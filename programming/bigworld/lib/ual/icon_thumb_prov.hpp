/**
 *	Icon Thumbnail Provider (for files without preview, such as prefabs)
 */


#ifndef ICON_THUMB_PROV_HPP
#define ICON_THUMB_PROV_HPP

#include "thumbnail_manager.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements an Icon Thumbnail Provider, for files without
 *	preview thumbnail, such as prefabs.
 */
class IconThumbProv : public ThumbnailProvider
{
public:
	IconThumbProv() : inited_( false ) {};
	virtual bool isValid( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size );
	virtual bool prepare( const ThumbnailManager& manager, const BW::wstring& file );
	virtual bool render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt  );

private:
	bool inited_;
	BW::wstring imageFile_;
	/**
	 *	This internal struct is used to match a file type with an icon.
	 */
	struct IconData
	{
		IconData(
			const BW::wstring& e, const BW::wstring& m, const BW::wstring& i
			) :
		extension( e ),
		match( m ),
		image( i )
		{};
		BW::wstring extension;
		BW::wstring match;
		BW::wstring image;
	};
	BW::vector<IconData> iconData_;

	DECLARE_THUMBNAIL_PROVIDER()

	void init();
	BW::wstring imageFile( const BW::wstring& file );
};

BW_END_NAMESPACE

#endif // ICON_THUMB_PROV_HPP
