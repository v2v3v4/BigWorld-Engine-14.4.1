#include "pch.hpp"

#include "brush_thumb_provider.hpp"

#include "worldeditor/terrain/terrain_paint_brush.hpp"

#include "common/string_utils.hpp"

BW_BEGIN_NAMESPACE

int BrushThumbProvider_token;


// Brush Provider implementation
IMPLEMENT_THUMBNAIL_PROVIDER( BrushThumbProvider )


/*
 *	This tests whether the given file is something that this provider
 *	is responsible for, in this case any file with the extension
 *	"brush".
 *
 *	See ThumbnailProvider::isValid for more details.
 */
/*virtual*/ bool BrushThumbProvider::isValid( 
	const ThumbnailManager &		manager, 
	const BW::wstring &				file )
{
	BW_GUARD;

	if (file.empty())
	{
		return false;
	}

	BW::wstring ext = bw_utf8tow( BWResource::getExtension( bw_wtoutf8( file ) ).to_string() );
	return wcsicmp( ext.c_str(), L"brush" ) == 0;
}


/*
 *	This prepares for rendering a brush thumbnail for the UAL.  Here we extract
 *	the texture used by the brush and use a ImageThumbProv to do the actual
 *	preparing.
 *
 *	See BrushThumbProvider::prepare and ImageThumbProv::prepare for more 
 *	details.
 */
/*virtual*/ bool BrushThumbProvider::prepare( 
	const ThumbnailManager &		manager, 
	const BW::wstring &				file )
{
	BW_GUARD;

	BW::wstring textureFile = getTextureFileForBrush( file );
	return imageProvider_.prepare( manager, textureFile );
}


/*
 *	This renders a brush thumbnail for the UAL.  Here we extract the texture 
 *	used by the brush and use a ImageThumbProv to do the actual preparing.
 *
 *	See BrushThumbProvider::prepare and ImageThumbProv::prepare for more 
 *	details.
 */
/*virtual*/ bool BrushThumbProvider::render( 
	const ThumbnailManager &		manager, 
	const BW::wstring &				file, 
	Moo::RenderTarget *				rt )
{
	BW_GUARD;

	BW::wstring textureFile = getTextureFileForBrush( file );
	return imageProvider_.render( manager, textureFile, rt );
}


/**
 *	This takes a brush file and returns the absolute location of the texture
 *	that it uses.
 *
 *	@param file			The brush file.
 *	@return				The texture that the brush uses.
 */
BW::wstring BrushThumbProvider::getTextureFileForBrush( 
	const BW::wstring &			file 
) const
{
	BW_GUARD;

	DataSectionPtr pBrushDataSection = BWResource::openSection( bw_wtoutf8( file ) );

	// If the file does not exist return the empty string:
	if (pBrushDataSection)
	{
		BW::string texFile = TerrainPaintBrush::texture( pBrushDataSection );
		return bw_utf8tow( BWResource::resolveFilename( texFile ) );
	}
	else
	{
		return BW::wstring();
	}
		
}
BW_END_NAMESPACE

