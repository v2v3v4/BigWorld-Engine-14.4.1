#include "pch.hpp"
#include "font_manager.hpp"
#include "font_metrics.hpp"
#include "font.hpp"
#include "glyph_cache.hpp"
#include "glyph_reference_holder.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/zip_section.hpp"
#include "resmgr/auto_config.hpp"
#include "math/colour.hpp"
#include "moo/resource_load_context.hpp"
#include "moo/effect_visual_context.hpp"
#include "time.h"

#ifdef _WIN32
#include "cstdmf/cstdmf_windows.hpp"
#include "Commdlg.h"
#endif

#pragma warning(push)
// member function does not override any base class virtual member function
#pragma warning(disable: 4263)
#include <gdiplus.h>
#pragma warning(pop)
#pragma comment(lib, "gdiplus.lib")

DECLARE_DEBUG_COMPONENT2( "Font",0 );


BW_BEGIN_NAMESPACE

BW_INIT_SINGLETON_STORAGE( FontManager );

static AutoConfigString s_fontManagerFontRoot( "system/fontRoot" );
static AutoConfigString s_fontMFM( "system/fontMFM", "system/materials/font.mfm" );

namespace	//anonymous
{
	BW::string toLower(const BW::string& input)
	{
		BW::string output = input;
		for (uint i = 0; i < output.length(); i++)
		{
			if (output[i] >= 'A' &&
				output[i] <= 'Z')
			{
				output[i] = output[i] - 'A' + 'a';
			}
		}
		return output;
	}
}

// Shader value setters
FontManager::UVDivisorSetter::UVDivisorSetter() :
value_(1.f)
{
}

bool FontManager::UVDivisorSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	pEffect->SetFloat( constantHandle, this->value_ );
	return true;
}


FontManager::FontMapSetter::FontMapSetter() :
	pTexture_(NULL)
{
}


bool FontManager::FontMapSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	pEffect->SetTexture( constantHandle, pTexture_->pTexture() );
	return true;
}

FontManager::FontColourSetter::FontColourSetter() :
value_( Vector4(1,1,1,1) )
{
}

bool FontManager::FontColourSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	pEffect->SetVector( constantHandle, &this->value_ );
	return true;
}

void FontManager::FontColourSetter::value( uint32 v ) 
{ 
	value_ = Colour::getVector4Normalised( v ); 
}

FontManager::FontPixelSnapSetter::FontPixelSnapSetter() :
value_( true )
{
}

bool FontManager::FontPixelSnapSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	pEffect->SetBool( constantHandle, value_ ? TRUE : FALSE );
	return true;
}


// Constructor.
FontManager::FontManager() :
	blendedTechnique_(NULL),
	fontMapSetter_(NULL),
	uvDivisorSetter_(NULL),
	fontColourSetter_(NULL),
	pixelSnapSetter_(NULL),
	gdiPlusToken_(0)

{
	doInit();
}


FontManager::~FontManager()
{
	BW_GUARD;
	doFini();

	//If this asserts, the program has shutdown and
	//FontManager was inited but not fini'd
	MF_ASSERT( !gdiPlusToken_ )
}


bool FontManager::doInit()
{
	BW_GUARD;
	material_ = new Moo::EffectMaterial();
	BW::string mfmName( s_fontMFM.value() );
	material_->load( BWResource::instance().openSection(mfmName) );
	if ( !material_->pEffect() )
	{
		CRITICAL_MSG( "Could not load the font material or fx file %s, exiting\n", mfmName.c_str() );
		exit( EXIT_FAILURE );
	}
	blendedTechnique_ = material_->pEffect()->pEffect()->GetTechniqueByName("FX_BLEND");
	fontMapSetter_ = new FontMapSetter();
	uvDivisorSetter_ = new UVDivisorSetter();
	fontColourSetter_ = new FontColourSetter();
	pixelSnapSetter_ = new FontPixelSnapSetter();

	//Initialise GDI+ members
	Gdiplus::GdiplusStartupInput gdiPlusStartupInput;
	Gdiplus::GdiplusStartup(&gdiPlusToken_, &gdiPlusStartupInput, NULL);

	*Moo::rc().effectVisualContext().getMapping( "FontMap" ) = fontMapSetter_;
	*Moo::rc().effectVisualContext().getMapping( "FontUVDivisor" ) = uvDivisorSetter_;
	*Moo::rc().effectVisualContext().getMapping( "FontColour" ) = fontColourSetter_;
	*Moo::rc().effectVisualContext().getMapping( "FontPixelSnap" ) = pixelSnapSetter_;

	return true;
}

bool FontManager::doFini()
{
	BW_GUARD;
	//Before shutting down GDIplus, we need to free all GDIplus resources,
	//thus force all GlyphCaches to be destroyed.
	StringFontMetricsMap::iterator it = fonts_.begin();
	StringFontMetricsMap::iterator end = fonts_.end();

	while ( it != end )
	{
		FontMetricsPtr fm = it->second;
		bw_safe_delete(fm->cache_);
		++it;
	}

	fonts_.clear();

	Gdiplus::GdiplusShutdown(gdiPlusToken_);
	gdiPlusToken_ = 0;
	return true;
}

/**
 *	This method returns a CachedFont for the given font resource.
 *	A Cached Font holds onto references of every character drawn with it,
 *	until the user explicitly releases the references.
 *
 *	@param	resourceName		name of the font resource
 *	@return	CachedFontPtr		cached font object.
 */
CachedFontPtr FontManager::getCachedFont(  const BW::string& resourceName )
{
	BW_GUARD;
	FontPtr pFont = this->get( resourceName, true );
	if ( pFont.hasObject() )
	{
		return dynamic_cast<CachedFont*>(pFont.get());
	}
	else
	{
		return NULL;
	}
}


/**
 *	This method retrieves a new font pointer.
 *
 *	@param	resourceName	The name of the font resource.
 *	@param	cached			Whether the font caches character glyphs.  If not
 *							the font can draw strings onto the screen, but the
 *							glyphs used are only guaranteed to be around for a
 *							single frame.  If you request a cached font, the
 *							font will remember any strings it produces and the
 *							last drawn string will keep a reference to the
 *							glyphs it requires.  These glyphs will be released
 *							when the font ptr goes out of scope, or when you
 *							explicitly release them via cachedFont->releaseGlyphRefs()
 *
 *	@return	FontPtr				A reference counted font object.
 */
FontPtr	FontManager::get( const BW::string& resourceName, bool cached )
{
	BW_GUARD;

	// Lock the cache
	SimpleMutexHolder mutexHolder( mutex_ );

	Moo::ScopedResourceLoadContext resLoadCtx( BWResource::getFilename( resourceName ) );

	FontMetricsPtr res;
	BW::string niceResourceName = toLower(resourceName);

	//Check the cache
	StringFontMetricsMap::iterator it = fonts_.find( niceResourceName );

	if ( it != fonts_.end() )
	{
		res = it->second;
	}
	else
	{
		DataSectionPtr pSection = BWResource::instance().openSection(
			s_fontManagerFontRoot.value() + resourceName );
		if ( pSection.hasObject() )
		{
			res = new FontMetrics();
			if (res->cache().load( resourceName, pSection ))
			{
				fonts_[niceResourceName] = res;
			}
			else
			{
				ERROR_MSG( "Error loading font %s\n", resourceName.c_str() );
				return NULL;
			}
		}
		else
		{
			ERROR_MSG( "Font resource %s does not exist\n",
						resourceName.c_str() );
			return NULL;
		}
	}

	if ( res != NULL )
	{
		return ( cached ? new CachedFont( *res ):
						  new Font( *res ) );	
	}

	return NULL;
}


/**
 *	This method finds the font name for a given font pointer.
 *	Do not use this method all the time, cause it performs a map find.
 *
 *	@param	pFont		smart pointer to the font to retrieve the name for.
 *	@return	BW::string	returned font name.
 */
const BW::string& FontManager::findFontName( const Font& font )
{
	BW_GUARD;
	static BW::string s_fontName = "font not found.";

	StringFontMetricsMap::iterator it = fonts_.begin();
	StringFontMetricsMap::iterator end = fonts_.end();

	while ( it != end )
	{
		FontMetricsPtr fm = it->second;
		if ( fm == &font.metrics() )
		{
			s_fontName = it->first;
			return s_fontName;
		}
		it++;
	}

	return s_fontName;
}


/**
 *	This method sets up a material for the given font, but
 *	does not start the begin/end pair.
 *
 *	@param	Font&	The font to associate with the standard font material.
 *	@param	D3DXHANDLE Optional effect material technique handle.
 */
void FontManager::prepareMaterial(Font& font, uint32 colour, bool pixelSnap, D3DXHANDLE technique)
{
	BW_GUARD;
	/*if (font.fitToScreen())
	{	
		ts.useMipMapping( true );
		ts.minFilter( Moo::TextureStage::LINEAR );
		ts.magFilter( Moo::TextureStage::LINEAR );
	}
	else
	{
		ts.useMipMapping( false );
		ts.minFilter( Moo::TextureStage::POINT );
		ts.magFilter( Moo::TextureStage::POINT );
	}*/

	if ( technique == 0 )
	{
		technique = blendedTechnique_;
	}
	material_->hTechnique( technique );

	fontMapSetter_->pTexture( font.metrics().cache().pTexture() );
	uvDivisorSetter_->value( font.metrics().cache().uvDivisor() );
	fontColourSetter_->value( colour );
	pixelSnapSetter_->value( pixelSnap );
}


/**
 *	This method sets up a material for the given font, just in case
 *	the font user doesn't want to use it's own materials.  ( Useful
 *	for the console and generic font drawing routines )
 *
 *	endDraw must be called at the end.
 *
 *	@param	Font&	The font to associate with the standard font material.
 *	@param	D3DXHANDLE Optional effect material technique handle.
 */
bool FontManager::begin( Font& font, D3DXHANDLE technique, bool pixelSnap )
{
	BW_GUARD;
	// We're going to be rendering fonts in clip space, so
	// we need to set all our transforms to identity.
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	origView_ = Moo::rc().view();
	origProj_ = Moo::rc().projection();

	Moo::rc().view( Matrix::identity );
	Moo::rc().projection( Matrix::identity );
	Moo::rc().updateViewTransforms();

	this->prepareMaterial( font, 0xffffffff, pixelSnap, technique );
	material_->begin();
	return material_->beginPass(0);
}


/**
 *	This method ends a (set of) draw call(s).  It must be called in pairs
 *	with FontManager::begin.
 */
void FontManager::end()
{
	BW_GUARD;
	material_->endPass();
	material_->end();

	Moo::rc().pop();

	Moo::rc().view( origView_ );
	Moo::rc().projection( origProj_ );
	Moo::rc().updateViewTransforms();
}

void FontManager::setUVDivisor( const Font& font )
{
	BW_GUARD;
	uvDivisorSetter_->value( font.metrics().cache().uvDivisor() );
}


BW_END_NAMESPACE
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
BW_BEGIN_NAMESPACE

static void saveFontCacheMap( const BW::string& fontName )
{
	BW_GUARD;
	FontPtr pFont = FontManager::instance().get( fontName );
	if ( pFont )
	{
		BW::string baseName = s_fontManagerFontRoot.value() + fontName;
		BW::string metricsFilename = baseName + ".generated";
		BW::string cacheFilename = baseName + ".dds";
		BW::string gridFilename = baseName + ".grid.dds";

		DataSectionPtr pFile = BWResource::instance().rootSection()->openSection( metricsFilename, true, ZipSection::creator() );
		if (pFile)
		{
			pFile->delChildren();
			pFont->metrics().cache().save(pFile, cacheFilename, gridFilename);
			pFile->save();
		}
		else
		{
			PyErr_Format( PyExc_ValueError, "BigWorld.saveFontCacheMap() "
					"could not open or create resource '%s'",
				metricsFilename.c_str() );
		}
	}
	else
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.saveFontCacheMap() "
				"font resource '%s' does not exist", fontName.c_str() );
	}
}


/*~ function BigWorld.saveFontCacheMap
 *	@components{ client, tools }
 *
 *	This saves the current cache texture map for the given font out to disk as 
 *	a .dds file with the same name as the font file.
 *
 *  Additionally it saves out a .generated file of the same name, which is a
 *	binary file with the dimensions and positions of each glyph in that map 
 *	(note that the font will continue to operate dynamically until it is next
 *	loaded).
 *
 *	Finally it saves another DDS file that has colour-coded blocks representing
 *	glyph dimensions, the effect margin and texture margin to aid artists with
 *	generating their own font texture maps.
 *
 *	Please see the client programming guide for more information.
 *
 *	@param	identifier	A string label to identify the font
*/
PY_AUTO_MODULE_FUNCTION( RETVOID, saveFontCacheMap, ARG( BW::string, END ), BigWorld )

BW_END_NAMESPACE

// font_manager.cpp
