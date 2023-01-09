/**
 *	Icon Thumbnail Provider (for files without preview, such as prefabs)
 */


#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "icon_thumb_prov.hpp"
#include "ual_manager.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "common/string_utils.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

// Token so this file gets linked in by the linker
int IconThumbProv_token;


// Implement the Icon Thumbnail Provider Factory
IMPLEMENT_THUMBNAIL_PROVIDER( IconThumbProv )


/**
 *	This method reads the list of extensions with generic icons from the
 *	ual_config.xml file.
 */
void IconThumbProv::init()
{
	BW_GUARD;

	inited_ = false;

	BW::string nconfigFile;
	bw_wtoutf8( UalManager::instance().getConfigFile(), nconfigFile );
	DataSectionPtr ds = BWResource::instance().openSection( nconfigFile );
	if ( !ds )
		return;
	
	ds = ds->openSection( "IconThumbnailProvider" );
	if ( !ds )
		return;

	BW::vector<DataSectionPtr> sections;
	ds->openSections( "Thumbnail", sections );
	for( BW::vector<DataSectionPtr>::iterator i = sections.begin();
		i != sections.end(); ++i )
	{
		BW::wstring ext = (*i)->readWideString( "extension" );
		BW::wstring match = (*i)->readWideString( "match" );
		BW::wstring image = (*i)->readWideString( "image" );
		if ( ext.empty() || image.empty() )
			continue;
		// use windows-style slashes when matching directories
		std::replace( match.begin(), match.end(), L'/', L'\\' );
		iconData_.push_back( IconData( ext, match, image ) );
	}

	inited_ = true;
}


/**
 *	Returns the image file name according to the data in iconData_ that matches
 *	criteria with the filename, or an empty string if no element of iconData_
 *	matches.
 *
 *	@param file		Asset requesting the thumbnail
 *	@return			Image filename corresponding to the asset type.
 */
BW::wstring IconThumbProv::imageFile( const BW::wstring& file )
{
	BW_GUARD;

	BW::wstring::size_type dot = (int)file.find_last_of( '.' );
	BW::wstring ext = file.substr( dot + 1 );
	StringUtils::toLowerCaseT( ext );

	for( BW::vector<IconData>::iterator i = iconData_.begin();
		i != iconData_.end(); ++i )
	{
		if ( (*i).extension == ext &&
			( (*i).match.empty() ||
				PathMatchSpec( file.c_str(), (*i).match.c_str() ) ) )
		{
			return (*i).image;
		}
	}
	return L"";
}


/**
 *	This method tells the manager if the asset specified in 'file' can be
 *	handled by the IconThumbProv.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Asset requesting the thumbnail
 *	@return			True if the asset's thumbnail is to be provided by us.
 */
bool IconThumbProv::isValid( const ThumbnailManager& manager, const BW::wstring& file )
{
	BW_GUARD;

	if ( !inited_ )
		init();

	if ( file.empty() )
		return false;

	imageFile_ = imageFile( file );

	return !imageFile_.empty();
}


/**
 *	This method is called to find out if the image for the asset needs to be
 *	created in the background thread, which in our case the answer will always
 *	be "No".
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Asset requesting the thumbnail.
 *	@param thumb	Returns the path to the image file for this asset.
 *	@param size		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool IconThumbProv::needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size )
{
	BW_GUARD;

	if ( file.empty() || thumb.empty() )
		return false; // invalid input params, return false

	// set the thumb filename to the icon's filename, and return false so it
	// loads the thumb directly
	BW::string nconfigFile;
	BW::wstring wthumb;

	bw_wtoutf8( UalManager::instance().getConfigFile(), nconfigFile );
	bw_utf8tow( BWResource::getFilePath( nconfigFile ), wthumb );

	thumb = wthumb +imageFile_;
	return false;
}


/**
 *	This class never needs to prepare the thumbnail in the bg thread.
 *
 *	@param manager	Ignored.
 *	@param file		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool IconThumbProv::prepare( const ThumbnailManager& manager, const BW::wstring& file )
{
	// should never get called
	return false;
}


/**
 *	This class never needs to render the thumbnail.
 *
 *	@param manager	Ignored.
 *	@param file		Ignored.
 *	@param rt		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool IconThumbProv::render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt )
{
	// should never get called
	return false;
}

BW_END_NAMESPACE

