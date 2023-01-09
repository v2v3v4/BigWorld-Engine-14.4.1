#include "pch.hpp"
#include "glyph_cache.hpp"
#include "glyph_reference_holder.hpp"
#include "cstdmf/memory_stream.hpp"
#include "math/math_extra.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bin_section.hpp"
#include "moo/geometrics.hpp"
#include "moo/texture_lock_wrapper.hpp"

#pragma warning(push)
// member function does not override any base class virtual member function
#pragma warning(disable: 4263)
#include <gdiplus.h>
#pragma warning(pop)

BW_BEGIN_NAMESPACE

static AutoConfigString s_glyphCacheFontRoot( "system/fontRoot" );

namespace {

const char* gdiplusErrorString( Gdiplus::Status s )
{
	BW_GUARD;
	switch( s )
	{
	case Gdiplus::Ok:
			return "the method call was successful.";
	case Gdiplus::GenericError:
			return "there was an error on the method call, which is identified as something other than those defined by the other elements of this enumeration.";
	case Gdiplus::InvalidParameter:
		return "one of the arguments passed to the method was not valid. ";
	case Gdiplus::OutOfMemory:
		return "the operating system is out of memory and could not allocate memory to process the method call. For an explanation of how constructors use the OutOfMemory status, see the Remarks section at the end of this topic. ";
	case Gdiplus::ObjectBusy:
		return "one of the arguments specified in the API call is already in use in another thread. ";
	case Gdiplus::InsufficientBuffer:
		return "a buffer specified as an argument in the API call is not large enough to hold the data to be received. ";
	case Gdiplus::NotImplemented:
		return "the method is not implemented. ";
	case Gdiplus::Win32Error:
		return "the method generated a Microsoft Win32 error. ";
	case Gdiplus::WrongState:
		return "the object is in an invalid state to satisfy the API call. For example, calling Pen::GetColor from a pen that is not a single, solid color results in a WrongState status. ";
	case Gdiplus::Aborted:
		return "the method was aborted. ";
	case Gdiplus::FileNotFound:
		return "the specified image file or metafile cannot be found. ";
	case Gdiplus::ValueOverflow:
		return "the method performed an arithmetic operation that produced a numeric overflow. ";
	case Gdiplus::AccessDenied:
		return "a write operation is not allowed on the specified file. ";
	case Gdiplus::UnknownImageFormat:
		return "the specified image file format is not known. ";
	case Gdiplus::FontFamilyNotFound:
		return "the specified font family cannot be found. Either the font family name is incorrect or the font family is not installed. ";
	case Gdiplus::FontStyleNotFound:
		return "the specified style is not available for the specified font family. ";
	case Gdiplus::NotTrueTypeFont:
		return "the font retrieved from an HDC or LOGFONT is not a TrueType font and cannot be used with GDI+. ";
	case Gdiplus::UnsupportedGdiplusVersion:
		return "the version of GDI+ that is installed on the system is incompatible with the version with which the application was compiled. ";
	case Gdiplus::GdiplusNotInitialized:
		return "the GDI+API is not in an initialized state. To function, all GDI+ objects require that GDI+ be in an initialized state. Initialize GDI+ by calling GdiplusStartup. ";
	case Gdiplus::PropertyNotFound:
		return "the specified property does not exist in the image. ";
	case Gdiplus::PropertyNotSupported:
		return "the specified property is not supported by the format of the image and, therefore, cannot be set. ";
	/*case Gdiplus::ProfileNotFound:
		return "the color profile required to save an image in CMYK format was not found.";*/
	default:
		return "Unknown Gdiplus error.";
	}
}

/**
 *	Tries to parse the given string as a U+XXXX Unicode character,
 *	otherwise tries parsing it as an integer.
 */
uint32 parseUnicodePoint( const BW::string& s, uint32 defaultValue=0 )
{
	uint32 ret = 0;
	if ( sscanf( s.c_str(), "U+%x", &ret ) != 1 )
	{
		ret = atoi( s.c_str() );
		if (!ret)
		{
			ret = defaultValue;
		}
	}

	return ret;
}

/**
 *	Parse a Unicode range from a string into two integers.
 */
bool parseUnicodeRange( const BW::string& s, uint32& outMin, uint32& outMax )
{
	return sscanf( s.c_str(), "U+%x-U+%x", &outMin, &outMax ) == 2;
}

/**
 *	This function turns a hex character into an integer.
 *
 *	@param c		the hex character
 *	@return uint	integer value represented by c
 */
INLINE uint fromHex( char c )
{
	if ( c >= '0' && c <= '9' )
		return ( c-'0' );
	else if ( c >= 'a' && c <= 'f' )
		return ( 10 + ( c-'a') );
	else if ( c >= 'A' && c <= 'F' )
		return ( 10 + ( c-'A') );
	else
		return 0;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
//Section : SourceFont
//-----------------------------------------------------------------------------
GlyphCache::SourceFont::SourceFont( DataSectionPtr ds, bool bold, 
								    bool antialias, bool dropShadow ):
	rangeMin_( 0 ),
	rangeMax_( std::numeric_limits<uint32>::max() ),
	antialias_( antialias ),
	bold_( bold ),
	dropShadow_( dropShadow ),
	warnIfMissing_( true ),
	fontFamily_( NULL ), 
	font_( NULL )
{
	BW_GUARD;
	MF_ASSERT( ds != NULL );

	fontName_ = ds->readString( "sourceFont" );

	if ( ds->findChild( "unicodeRange" ) )
	{
		if ( !parseUnicodeRange( ds->readString( "unicodeRange" ), rangeMin_, rangeMax_ )
				|| rangeMin_ > rangeMax_ )
		{
			ERROR_MSG( "GlyphCache::SourceFont: Invalid secondary Unicode range specified." );
		}
	}

	antialias_ = ds->readBool( "antialias", antialias_ );
	bold_ = ds->readBool( "bold", bold_ );
	dropShadow_ = ds->readBool( "dropShadow", dropShadow_ );
	warnIfMissing_ = ds->readBool( "warnIfMissing", warnIfMissing_ );
}

GlyphCache::SourceFont::~SourceFont()
{
	BW_GUARD;
	bw_safe_delete(font_);
	bw_safe_delete(fontFamily_);	
}

bool GlyphCache::SourceFont::createFont( int pointSize )
{
	BW_GUARD;
	bw_safe_delete(font_);
	bw_safe_delete(fontFamily_);	


	fontFamily_ = new Gdiplus::FontFamily( bw_utf8tow( fontName_ ).c_str() );
	Gdiplus::Status s = fontFamily_->GetLastStatus();
	if (s == Gdiplus::Ok)
	{
		font_ = new Gdiplus::Font( fontFamily_, (float)(abs(pointSize)), 
							bold_ ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular, Gdiplus::UnitPixel );

		s = font_->GetLastStatus();
		if (s != Gdiplus::Ok)
		{
			ERROR_MSG( "SourceFont::createFont: Font family '%s' is valid, but failed to create font (%d pt). %s\n", 
					fontName_.c_str(), pointSize, gdiplusErrorString(s) );
			bw_safe_delete(fontFamily_);
			bw_safe_delete(font_);
			return false;
		}
	}
	else
	{
		if (warnIfMissing_)
		{
			ERROR_MSG( "SourceFont::createFont: Invalid font family '%s' (%d pt). %s\n", 
						fontName_.c_str(), pointSize, gdiplusErrorString(s) );
		}
		bw_safe_delete(fontFamily_);
		return false;
	}


	return true;
}

//-----------------------------------------------------------------------------
//Section : GlyphCache
//-----------------------------------------------------------------------------
//Constructor.
GlyphCache::GlyphCache() :
	refCounts_( new GlyphReferenceHolder ),
	occupied_( NULL ),
	curr_(0,0),
	slotBounds_(0,0),
	slotSize_(0,0),
	spaceProxyChar_(),
	widestChar_(),
	effectsMargin_(0),
	textureMargin_(0),
	maxWidth_(0),
	height_(0),
	fontFileName_(""),	//e.g. Heading.font
	proportional_( true ),
	pointSize_(),
	uvDivisor_(1.f),
	shadowAlpha_(),
	pTexture_( NULL ),
	lastExpandFailed_( false ),
	graphics_( NULL ),
	brush_( NULL ),
	shadowBrush_(NULL),
	bitmap_( NULL ),
	pixelFormat_( PixelFormat32bppARGB ),
	gdiSurfacePitch_(32)
{
	BW_GUARD;
}


//Destructor.
GlyphCache::~GlyphCache()
{
	BW_GUARD;
	sourceFonts_.clear();
	bw_safe_delete(brush_);
	bw_safe_delete(shadowBrush_);
	bw_safe_delete(bitmap_);
	bw_safe_delete(graphics_);
	bw_safe_delete(refCounts_);
	bw_safe_delete_array(occupied_);
}

/**
 *	This method returns whether or not this cache is being used as
 *	a dynamic cache, or whether it is using pre-generated data.
 *
 *	@return bool	True if dynamic, otherwise false.
 */
bool GlyphCache::dynamicCache() const
{
	BW_GUARD;
	// basically, we're dynamic if we have a Gdiplus font object.
	return !sourceFonts_.empty() && sourceFonts_[0]->font();
}

/**
 *	This method returns the most appropriate SourceFont for the
 *	given character. This works by checking the Unicode ranges
 *	on each SourceFont until a match is found.
 *
 *	@return SourceFontPtr	Pointer to the SourceFont for the given char.
 */
const GlyphCache::SourceFontPtr& GlyphCache::charToFont( wchar_t c )
{
	BW_GUARD;
	// should always have at least one.
	MF_ASSERT( !sourceFonts_.empty() );

	// sorted from lowest range to highest, so go backwards.
	for ( BW::vector<SourceFontPtr>::reverse_iterator it =
		sourceFonts_.rbegin(); it != sourceFonts_.rend(); it++ )
	{
		SourceFontPtr& sf = *it;

		if ( sf->font() && sf->matchChar( c ))
		{
			return sf;
		}
	}

	// always return something
	return sourceFonts_[0];
}

/**
 *	This method returns the glyph for the given wide character.
 *	If the character is not yet in the cache, it adds it synchronously.
 *
 *	@param	c		The character to return the glyph for.
 *	@return Glyph	Glyph information for the given character.
 */
const GlyphCache::Glyph& GlyphCache::glyph( wchar_t c )
{
	BW_GUARD;
	Glyphs::iterator it = glyphs_.find(c);
	if ( it != glyphs_.end() )
	{
		Glyph& g = it->second;
		g.lastUsed_ = Moo::rc().frameTimestamp();
		return g;
	}

	if ( dynamicCache() && graphics_ )
	{
		Slot slot;
		if (this->grabSlot( slot ))
		{
			return this->addGlyphInternal( c,  slot );
		}
		else
		{
			return glyphs_.begin()->second;
		}
	}

	ERROR_MSG( "A glyph (U+%x) was not found in the pre-generated font cache %s\n", (uint32)c, fontFileName_.c_str() );
	return glyphs_.begin()->second;
}


/**
 *	This method calculates a rectangle in pixels, that bounds
 *	the given character.
 *
 *	@param	c		The character to calculate the glyph size for.
 *	@param	dimensions [out] The returned dimensions of the glyph.
 *	@return bool	Success.  Iff true, then dimensions is touched.
 */
bool GlyphCache::calcGlyphRect( wchar_t c, Vector2& dimensions )
{
	BW_GUARD;

	const SourceFontPtr& sourceFont = this->charToFont( c );
	MF_ASSERT( sourceFont != NULL ); // there should _always_ be at least one.

	// This fn should only be called for dynamic glyph caches.
	Gdiplus::Font* font = sourceFont->font();
	MF_ASSERT_DEV( font != NULL );

	wchar_t character = c == L' ' ? spaceProxyChar_ : c;
	Gdiplus::RectF outRect;
	Gdiplus::Status s = graphics_->MeasureString( &character, 1, font,
		Gdiplus::PointF(0.f,0.f), Gdiplus::StringFormat::GenericTypographic(),
		&outRect );

	if ( s == Gdiplus::Ok )
	{
		dimensions.x = ceilf(outRect.Width);
		dimensions.y = ceilf(outRect.Height);
		return true;
	}
	else
	{
		ERROR_MSG( "GlyphCache::calcGlyphRect MeasureString failed (%s)\n",
			gdiplusErrorString(s) );
		return false;
	}

}


/**
 *	This method returns an available slot in the glyph cache map.
 *	If there are no available slots, then grow the cache.
 *
 *	@param Slot reference to output Slot structure
 *
 *	@return bool Whether or not a slot was available
 */
bool GlyphCache::grabSlot( GlyphCache::Slot& slot )
{
	BW_GUARD_PROFILER(GlyphCache_grabSlot_timer);

	//First, quicker loop checks for unoccupied slots, leaving
	//occupied but unused slots around for later reuse.
	size_t nSlots = slotBounds_.x_ * slotBounds_.y_;
	for ( size_t i=0; i < nSlots; i++ )
	{
		if (!occupied( curr_ ))
		{
			slot = curr_;
			return true;
		}
		else
		{
			curr_.x_++;
			if( curr_.x_ >= slotBounds_.x_ )
			{
				curr_.y_ = (curr_.y_ + 1) % slotBounds_.y_;
				curr_.x_ = 0;
			}
		}
	}

	//Entire map is full, run a second, slower loop that
	//finds the first slot that has no outstanding refcount
	GlyphReferenceHolder::GlyphReferenceCountMap::iterator it = refCounts_->begin();
	GlyphReferenceHolder::GlyphReferenceCountMap::iterator en = refCounts_->end();
	while ( it != en )
	{
		bool eraseIt = false;

		if ( it->second == 0 )
		{
			//No permanent refcount holders, but somebody may be using
			//it to display text temporarily, like the consoles (things
			//not using draw into mesh)
			Glyphs::iterator git = glyphs_.find( it->first );
			if (git != glyphs_.end())
			{
				wchar_t c = git->first;
				Glyph& g = git->second;
				//TODO : sort entire glyph list by last used and grab
				//the LRU glyph for replacement.
				if ( g.lastUsed_ != Moo::rc().frameTimestamp() )
				{
					uvToSlot( g.uv_, curr_ );
					occupied( curr_, false );
					glyphs_.erase( git );
					refCounts_->erase( it );
					slot = curr_;
					return true;
				}
			}
			else
			{
				eraseIt = true;
			}
		}

		if (eraseIt)
		{
			it = refCounts_->erase( it );
		}
		else
		{
			++it;
		}
	}

	if (this->expand())
	{
		return grabSlot( slot );
	}
	else
	{
		DEBUG_MSG( "Could not grab space font cache, nor increase the cache size ('%s'). "
			"Too many characters on screen at the same time for glyph height %d px (%d currently in cache). "
			"Continuing, but artifacts will occur!\n",
			fontFileName_.c_str(), int(height_), glyphs_.size() );
		return false;
	}
}


bool GlyphCache::expand()
{
	BW_GUARD_PROFILER( GlyphCache_expand_timer );
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );

	if (lastExpandFailed_)
	{
		return false;
	}

	uint32 oldWidth = pTexture_->width();
	uint32 oldHeight = pTexture_->height();

	//Recreate our render target, make it bigger.
	BW::string rtName;
	rtName = fontFileName_ + "_rt";
	bool result = pTexture_->resize( oldWidth, oldHeight*2, 1, 0, D3DFMT_A8R8G8B8 );
	if (result)
	{
		uvDivisor_ *= 2.f;
		slotBounds_.y_ *= 2;
		bw_safe_delete_array(occupied_);
		occupied_ = new bool[slotBounds_.x_*slotBounds_.y_];
		this->refillSurface();
	}
	else
	{
		DEBUG_MSG( "GlyphCache::expand - failed to expand texture from %dx%d to %dx%d.\n",
			oldWidth, oldHeight, oldWidth, oldHeight*2 );
		lastExpandFailed_ = true;
		return false;
	}

	return true;
}

const BW::string& GlyphCache::fontName() const
{
	BW_GUARD;
	MF_ASSERT_DEV( !sourceFonts_.empty() );

	if (sourceFonts_.empty())
	{
		static BW::string s_empty;
		return s_empty;
	}
	else
	{
		return sourceFonts_[0]->fontName();
	}
}


/**
 *	This method returns the top-left corner of the given slot, in
 *	UV coordinates.
 *
 *	@param	slot	The slot ot calculate the UV coordinates for.
 *	@param	ret		[out] The returned top left UV coordinates.
 */
void GlyphCache::slotToUV( const Slot& slot, Vector2& ret ) const
{
	BW_GUARD;
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );

	ret.x = (slot.x_ * slotSize_.x) / pTexture_->width();
	ret.y = uvDivisor_ * (slot.y_ * slotSize_.y) / pTexture_->height();
}


/**
 *	This method calculates which slot contains the given UV coordinates.
 *
 *	@param	uv		The UV coordinates to search for.
 *	@param	ret		[out] The returned slot containing the UV coordinates.
 */
void GlyphCache::uvToSlot( const Vector2& uv, Slot& ret ) const
{
	BW_GUARD;
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );

	ret.x_ = (uint32)floorf( uv.x * pTexture_->width() / slotSize_.x + 0.5f );
	ret.y_ = (uint32)floorf( (uv.y/uvDivisor_) * pTexture_->height() / slotSize_.y + 0.5f );

	MF_ASSERT_DEV( ret.x_ <= slotBounds_.x_ );
	MF_ASSERT_DEV( ret.y_ <= slotBounds_.y_ );
}


/**
 *	This method returns whether or not a given slot is occupied or not.
 *
 *	@param	slot	The slot in question.
 *	@return bool	Whether or not the slot is currently occupied by a glyph.
 */
bool GlyphCache::occupied( const Slot& slot ) const
{
	BW_GUARD;
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );
	return occupied_[slot.x_ + slot.y_ * slotBounds_.x_];
}


/**
 *	This method sets the occupied-ness of a given slot.
 *
 *	@param	slot	The slot in question.
 *	@param	state	Desired occupied-ness of the slot.
 */
void GlyphCache::occupied( const Slot& slot, bool state )
{
	BW_GUARD;
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );
	occupied_[slot.x_ + slot.y_ * slotBounds_.x_] = state;
}


/**
 *	This private method adds a character in the given slot position.
 *	It draws the glyph into the cache render target, and returns the
 *	newly created glyph information object.
 *
 *	@param	c		The character to add to the cache.
 *	@param	slot	The slot in which to draw the glyph.  This must be free.
 *	@return	Glyph	Newly created glyph information.
 */
const GlyphCache::Glyph& GlyphCache::addGlyphInternal( wchar_t c, const GlyphCache::Slot& slot )
{
	BW_GUARD_PROFILER( GlyphCache_addGlyphInternal_timer );
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );
	MF_ASSERT_DEV( !occupied(slot) );

	Glyph glyph;
	this->slotToUV( slot, glyph.uv_ );
	glyph.uvWidth_ = maxWidth_ / this->mapSize().x;
	glyph.lastUsed_ = Moo::rc().frameTimestamp();

	const SourceFontPtr& sourceFont = this->charToFont( c );
	if( sourceFont && sourceFont->font() && pTexture_->pTexture())
	{
		Gdiplus::Font* font = sourceFont->font();

		graphics_->SetTextRenderingHint( sourceFont->antialias() ? 
			Gdiplus::TextRenderingHintAntiAliasGridFit : Gdiplus::TextRenderingHintSingleBitPerPixelGridFit );	

		graphics_->SetCompositingMode( Gdiplus::CompositingModeSourceCopy );
		//debugging..
		//graphics_->Clear(Gdiplus::Color(80,255,slot.x_ * 8, slot.y_ * 64));
		graphics_->Clear(Gdiplus::Color(1,255,255,255));	//must have 1 alpha or it smartly decides to not do anything at all
		graphics_->SetCompositingMode( Gdiplus::CompositingModeSourceOver );

		Vector2 dimensions;
		this->calcGlyphRect( c, dimensions );
		float pixelWidth = proportional_ ? dimensions.x + effectsMargin_ : maxWidth_;
		float pixelHeight = dimensions.y + (uint)effectsMargin_;
		glyph.uvWidth_ = pixelWidth / this->mapSize().x;

		//INFO_MSG( "%s - %d - %0.1f, %0.1f\t\tpixel width and height\n", fontFileName_.c_str(), (uint32)c, pixelWidth, pixelHeight );

		//Draw GDI Glyph into top-left of bitmap_ system memory surface
		Gdiplus::Status s;
		if ( sourceFont->dropShadow() )
		{
			s = graphics_->DrawString( &c, 1, font, Gdiplus::PointF(1,1),
				Gdiplus::StringFormat::GenericTypographic(), shadowBrush_ );
			if ( s != Gdiplus::Ok )
			{
				ERROR_MSG( "GlyphCache::addGlyphInternal could not draw string (%s)",
					gdiplusErrorString(s) );
			}
		}

		s = graphics_->DrawString( &c, 1, font, Gdiplus::PointF(0,0),
			Gdiplus::StringFormat::GenericTypographic(), brush_ );
		if ( s != Gdiplus::Ok )
		{
			ERROR_MSG( "GlyphCache::addGlyphInternal could not draw string (%s)",
				gdiplusErrorString(s) );
		}

		//Get the pixels into a system memory buffer
		RECT srcRect = { 0,0, (LONG)slotSize_.x, (LONG)slotSize_.y };
		RECT dstRect( srcRect );
		::OffsetRect(&dstRect, slot.x_ * (uint32)slotSize_.x, slot.y_ * (uint32)slotSize_.y);

		//INFO_MSG( "%s - %d - %0.1f, %0.1f\t\tslot size\n", fontFileName_.c_str(), (uint32)c, slotSize_.x, slotSize_.y );
		//INFO_MSG( "%s - %d - slot %d, %d\trect %d, %d, %d, %d\tglyph uv %0.3f, %0.3f, %0.3f, %0.3f\n", fontFileName_.c_str(), (uint32)c, slot.x_, slot.y_, dstRect.left, dstRect.top, dstRect.right, dstRect.bottom, glyph.uv_.x, glyph.uv_.y, glyph.uvWidth_, slotSize_.y / this->mapHeight() );

		Gdiplus::BitmapData bmpData;
		Gdiplus::Rect gdiRect( srcRect.left, srcRect.top, srcRect.right, srcRect.bottom );
		s = bitmap_->LockBits( &gdiRect, Gdiplus::ImageLockModeRead, pixelFormat_, &bmpData );

		if ( s == Gdiplus::Ok )
		{
			HRESULT hr;
			unsigned char* cbits = (unsigned char*)(bmpData.Scan0);

			//Copy bits into system memory texture, and flag the driver to update the vid mem one whenever it wants
			D3DLOCKED_RECT lr;
			lr.pBits = 0;
			lr.Pitch = 0;
			DX::Texture* pTex = (DX::Texture*)pTexture_->pTexture();

			Moo::TextureLockWrapper textureLock( pTex );
			hr = textureLock.lockRect( 0, &lr, &dstRect, 0 );
			if ( SUCCEEDED(hr) )
			{
				uint32 * pDestBits = (uint32*)lr.pBits;
				uint32 * pSrcBits = (uint32*)cbits;

				for ( int32 y=0; y<srcRect.bottom; y++ )
				{
					for ( int32 x=0; x<srcRect.right; x++ )
					{
						pDestBits[x] = pSrcBits[x];
					}
					pDestBits += lr.Pitch / 4;
					pSrcBits += gdiSurfacePitch_;
				}
				textureLock.unlockRect( 0 );
			}
			else
			{
				ERROR_MSG( "GlyphCache::addGlyphInternal - could not get lock texture surface rectangle (%s)", DX::errorAsString(hr).c_str() );
			}


			bitmap_->UnlockBits(&bmpData);
		}
		else
		{
			ERROR_MSG( "GlyphCache::addGlyphInternal - could not lock bitmap (%s)", gdiplusErrorString(s) );
		}
	}

	glyphs_.insert( std::make_pair(c, glyph) );
	this->occupied( slot, true );
	return glyphs_[c];
}


/**
 *	This method recreates the texture map based on all the existing
 *	reference counted glyphs in the cache.
 *
 *	If the cache was initialised from a pre-generated texture, it copies
 *	the glyph out of the source texture map.
 */
void GlyphCache::refillSurface()
{
	BW_GUARD;
	//this fn should only be called for dynamic glyph caches
	MF_ASSERT_DEV( dynamicCache() );
	//INFO_MSG( "Recreating %s_%d font cache\n", this->fontName_.c_str(), pointSize_ );

	this->clearOccupancy();

	Glyphs::iterator it = glyphs_.begin();
	Glyphs::iterator en = glyphs_.end();
	while ( it != en )
	{
		wchar_t chr = it->first;
		Glyph& glyph = it->second;
		if ( refCounts_->refCount(chr) > 0 )
		{
			Slot slot(0,0);
			this->uvToSlot( glyph.uv_, slot );
			//DEBUG_MSG( "uv : %0.3f, %0.3f, slot %d, %d\n", glyph.uv_.x, glyph.uv_.y, slot.x_, slot.y_ );
			this->addGlyphInternal( chr, slot );
			++it;
		}
		else
		{
			//valid to do this during iteration on MS stl
			it = glyphs_.erase(it);
			refCounts_->erase(chr);
		}
	}
}


/**
 *	This method loads a character set range from a data section.
 *
 *	@param	pSection	DataSection containing the character set range.
 *	@param	characterSet [out] returned character string.
 */
void GlyphCache::loadCharacterSet( DataSectionPtr pSection, BW::wstring& characterSet )
{
	BW_GUARD;
	uint charStart, charEnd;

	// ACP character range
	charStart = parseUnicodePoint( pSection->readString( "startChar" ) );
	charEnd   = parseUnicodePoint( pSection->readString( "endChar" ) );
	if (charEnd >0)
	{
		characterSet.reserve( (charEnd - charStart) + 1 );
		for (uint i = charStart; i <= charEnd; i++)
		{
			char character = char(i);
			wchar_t wideCharacter;
			MultiByteToWideChar( CP_ACP, 0, &character, 1, &wideCharacter, 1 );
			characterSet.push_back( wideCharacter );
		}
	}

	// Explicit unicode characters
	BW::vector<DataSectionPtr> codePoints;
	pSection->openSections( "unicodeChar", codePoints );
	for( size_t i = 0; i < codePoints.size(); i++ )
	{
		uint codePoint = parseUnicodePoint( codePoints[i]->asString() );
		if (!codePoint)
		{
			WARNING_MSG( "loadCharacterSet: invalid unicodeChar tag encountered: '%s'\n",
					codePoints[i]->asString().c_str() );
		}
	}

	// Explicit unicode ranges
	BW::vector<DataSectionPtr> codePointRanges;
	pSection->openSections( "unicodeRange", codePointRanges );
	for( size_t i = 0; i < codePointRanges.size(); i++ )
	{
		uint start, end;
		if ( parseUnicodeRange( codePointRanges[i]->asString(), start, end ) )
		{
			characterSet.reserve( characterSet.size() + (end - start) + 1 );

			for( uint k = start; k <= end; k++ )
			{
				characterSet.push_back( wchar_t(k) );
			}
		}
		else
		{
			WARNING_MSG( "loadCharacterSet: invalid unicodeRange tag encountered: '%s'\n",
					codePointRanges[i]->asString().c_str() );
		}
	}
}


/**
 *	This method loads the font information from an xml file, and optionally
 *	precaches some glyphs
 *
 *	For information aboout the XML font file grammar, please see the File
 *	Grammar Guide.
 *
 *	@param	fontFileName Name of the source font, e.g. Heading.font
 *	@param	pSection	The data section from a font file.
 *	@return bool		True if the font data was loaded successfully.
 */
bool GlyphCache::load( const BW::string& fontFileName, DataSectionPtr pSection )
{
	BW_GUARD;
	if ( !pSection.hasObject() )
	{
		ERROR_MSG( "Font file missing completely\n" );
		return false;
	}

	DataSectionPtr pLegacyGenerated = pSection->openSection( "generated" );
	pSection = pSection->openSection( "creation" );

	if ( !pSection.hasObject() )
	{
		ERROR_MSG( "Font file missing <creation> section\n" );
		return false;
	}

	sourceFonts_.push_back( new SourceFont( pSection ) );

	fontFileName_ = fontFileName;
	Vector2 mapDimensions = pSection->readVector2( "mapDimensions", Vector2(512,32) );
	pointSize_ = pSection->readInt( "sourceFontSize" );
	proportional_ = !pSection->readBool( "fixedWidth", false );	
	widestChar_ = parseUnicodePoint( pSection->readString( "widestChar" ), L'W' );
	spaceProxyChar_ = pSection->readInt( "spaceProxyChar", 105 ); // i
	shadowAlpha_ = pSection->readInt( "shadowAlpha", 255 );
	textureMargin_ = pSection->readFloat( "textureMargin", 0.f );
	if ( sourceFonts_[0]->dropShadow() )
	{
		effectsMargin_ = 1.f;
	}
	effectsMargin_ = pSection->readFloat( "effectsMargin",effectsMargin_ );

	BW::vector<DataSectionPtr> secondarySections;
	pSection->openSections( "secondary", secondarySections );
	for (size_t i = 0; i < secondarySections.size(); i++)
	{
		sourceFonts_.push_back( 
			new SourceFont( secondarySections[i], 
							sourceFonts_[0]->bold(), 
							sourceFonts_[0]->antialias(),
							sourceFonts_[0]->dropShadow() )
		);
	}

	std::sort( sourceFonts_.begin(), sourceFonts_.end(), SortSourceFont() );

	BW::string preGeneratedName = s_glyphCacheFontRoot.value() + fontFileName_ + ".generated";
	DataSectionPtr pGenerated = BWResource::instance().openSection( preGeneratedName );

	loadCharacterSet( pSection, defaultCharacterSet_ );

	if ( pGenerated.hasObject() )
	{
		this->loadPreGeneratedData( pGenerated );
	}
	else if ( pLegacyGenerated.hasObject() )
	{
		this->loadLegacyGeneratedSection( pLegacyGenerated, defaultCharacterSet_ );
	}
	else
	{
		if ( !this->createFont() )
		{
			return false;
		}

		slotSize_.set( maxWidth_ + textureMargin_, height_ + textureMargin_ );

		if (slotSize_.x > mapDimensions.x)
		{
			ERROR_MSG( "Horizontal map size %d is not big enough for point size %d (needs to be at least %d). "
				"Specify a larger mapDimensions in '%s'.\n",
				(int)mapDimensions.x, pointSize_, (int)fabsf(slotSize_.x),
				fontFileName.c_str() );
			return false;
		}

		//Make sure the texture contain at least one line of glyphs
		if (mapDimensions.y<slotSize_.y)
		{
			mapDimensions.y = (float)BW::largerPow2( (uint32)(slotSize_.y) );
			if ( mapDimensions.y > (float)gdiSurfacePitch_ )
			{
				gdiSurfacePitch_ = BW::largerPow2( (uint32)(slotSize_.y) );
				bw_safe_delete(bitmap_);
				bw_safe_delete(graphics_);
				bitmap_ = new Gdiplus::Bitmap( gdiSurfacePitch_, gdiSurfacePitch_, pixelFormat_ );
				graphics_ = new Gdiplus::Graphics(bitmap_);
			}
		}

		slotBounds_.x_ = (uint32)floorf( mapDimensions.x / slotSize_.x );
		slotBounds_.y_ = (uint32)floorf( mapDimensions.y / slotSize_.y );

		//There must be, by now, room for at least 1 row and 1 column in the cache.
		MF_ASSERT_DEV( slotBounds_.x_ > 0 )
		MF_ASSERT_DEV( slotBounds_.y_ > 0 )

		occupied_ = new bool[slotBounds_.x_*slotBounds_.y_];
		this->clearOccupancy();

		BW::string mapName;
		mapName = fontFileName_ + ".dds";
		INFO_MSG( "Created Font Texture Map %s [%d x %d]\n", mapName.c_str(), (int)mapDimensions.x, (int)mapDimensions.y );

		pTexture_ = new Moo::ManagedTexture( mapName, (int)mapDimensions.x, (int)mapDimensions.y, 1, 0, D3DFMT_A8R8G8B8, "texture/glyph cache" );
		this->preloadGlyphs( defaultCharacterSet_ );
	}

	return true;
}


/**
 *	This method supports the legacy <generated> section of font files.
 *	It initialises the glyph cache from the previously generated data.
 *
 *	@param	pSection	<generated> section of a legacy font file.
 *	@param	characterSet string of characters to initialise the cache with.
 *	@return	bool		success of the operation.
 */
bool GlyphCache::loadLegacyGeneratedSection( DataSectionPtr pSection, const BW::wstring& characterSet )
{
	BW_GUARD;
	glyphs_.clear();
	refCounts_->reset();
	size_t numChars = characterSet.size();
	BW::string mapName = pSection->readString( "map" );
	Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get( mapName, false, true );
	pTexture_ = dynamic_cast<Moo::ManagedTexture*>(pTex.getObject());

	if ( !pTexture_ )
	{
		ASSET_MSG( "Generated section exists, but could not load the generated texture map %s\n", mapName.c_str() );
		return false;
	}

	Vector2 mapDimensions( (float)pTexture_->width(), (float)pTexture_->height() );
	slotBounds_.x_ = (uint32)floorf( mapDimensions.x / slotSize_.x );
	slotBounds_.y_ = (uint32)floorf( mapDimensions.y / slotSize_.y );

	INFO_MSG( "Loaded Legacy Font Texture Map %s [%d x %d]\n", mapName.c_str(), (int)mapDimensions.x, (int)mapDimensions.y );

	float sourceHeight = pSection->readFloat( "height", 0.f );
	if ( sourceHeight == 0.f )
	{
		ERROR_MSG( "Generated section exists, but does not contain the glyph height\n" );
		return false;
	}
	height_ = sourceHeight + effectsMargin_;

	BW::string positions = pSection->readString( "uvs", "" );
	BW::string widths = pSection->readString( "widths", "" );
	if ( positions.length() < numChars * 8 )
	{
		ERROR_MSG( "FontMetrics::load failed because numChars did not match the position info string\n" );
		return false;
	}
	if ( widths.length() < numChars * 4 )
	{
		ERROR_MSG( "FontMetrics::load failed because numChars did not match the width info string\n" );
		return false;
	}

	//The positions string is a long hex string containing position data in the bitmap.
	//There should be 4 hex characters per font character, being xxyy(xpos)(ypos)
	//The widths string is similar, but has only a 2-character hex code per string.
	maxWidth_ = 0;
	for ( size_t i = 0; i < numChars; i++ )
	{
		size_t posIdx = i*8;	//posIdx goes up by 8, 
		size_t widIdx = i*4;	//widths go up by 4
		char temp[9];
		temp[0] = positions[posIdx];
		temp[1] = positions[posIdx+1];
		temp[2] = positions[posIdx+2];
		temp[3] = positions[posIdx+3];
		temp[4] = positions[posIdx+4];
		temp[5] = positions[posIdx+5];
		temp[6] = positions[posIdx+6];
		temp[7] = positions[posIdx+7];
		temp[8] = (char)0;
		Vector2 sourceUV((float)((fromHex(positions[posIdx])<<12) +
							(fromHex(positions[posIdx+1])<<8) +
							(fromHex(positions[posIdx+2])<<4) +
							fromHex(positions[posIdx+3])),
					(float)((fromHex(positions[posIdx+4])<<12) +
							(fromHex(positions[posIdx+5])<<8) +
							(fromHex(positions[posIdx+6])<<4) +
							fromHex(positions[posIdx+7])) );		
		sourceUV += Vector2(0,1);
		sourceUV.x /= mapDimensions.x;
		sourceUV.y /= mapDimensions.y;
		float sourceWidth = (float)((fromHex(widths[widIdx])<<12) + 
							(fromHex(widths[widIdx+1])<<8) +
							(fromHex(widths[widIdx+2])<<4) +
							fromHex(widths[widIdx+3]));

		MF_ASSERT( sourceWidth >= 0.f );
		MF_ASSERT( sourceWidth <= 1024.f );	//that's a big character.

		maxWidth_ = max(sourceWidth,maxWidth_);

		Vector2 sourceSize( sourceWidth/mapDimensions.x, sourceHeight/mapDimensions.y );			

		//Add a glyph to the cache
		Glyph glyph;		
		glyph.lastUsed_ = Moo::rc().frameTimestamp();
		glyph.uvWidth_ = sourceSize.x;
		glyph.uv_ = sourceUV;
		glyphs_.insert( std::make_pair(characterSet[i], glyph) );
		this->refCounts().incRef(characterSet[i]);
	}

	maxWidth_ += effectsMargin_;

	return true;
}


/**
 *	This method supports loading from pre generated font files.  These may be
 *	artist modified, or simply a copy of the original glyph cache.
 *
 *	@param	pSection	a font.generated file.
 *	@return	bool		success of the operation.
 */
bool GlyphCache::loadPreGeneratedData( DataSectionPtr pSection )
{
	BW_GUARD;

	BW::string mapName = s_glyphCacheFontRoot.value() + fontFileName_ + ".dds";
	Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get( mapName, false, true );
	pTexture_ = dynamic_cast<Moo::ManagedTexture*>(pTexture.getObject());
	if ( !pTexture_ )
	{
		ASSET_MSG( "Generated section exists, but could not load the generated texture map %s\n", mapName.c_str() );
		return false;
	}

	Vector2 mapDimensions( (float)pTexture_->width(), (float)pTexture_->height() );

	INFO_MSG( "Loaded pre-generated font texture map %s [%d x %d]\n", mapName.c_str(), (int)mapDimensions.x, (int)mapDimensions.y );

	DataSectionPtr binChild = pSection->openSection( "glyphCache", true, BinSection::creator() );
	BinaryPtr pBinData = binChild->asBinary();
	MemoryIStream str( pBinData->cdata(), pBinData->len() );

	BW::string headerString;
	str >> headerString;

	if ( headerString != "GlyphCache_v1" )
	{
		ERROR_MSG( "Generated section exists, but the header (%s) does not match the expected version string (GlyphCache_v1).\n", headerString.c_str() );
		return false;
	}

	glyphs_.clear();
	refCounts_->reset();
	str >> maxWidth_;
	str >> height_;

	while ( str.remainingLength() )
	{
		wchar_t c;
		Glyph g;
		str >> c;
		str >> g.uv_;
		str >> g.uvWidth_;
		g.lastUsed_ = 0;
		glyphs_[c] = g;
		refCounts_->incRef(c);
	}	

	return true;
}


/**
 *	This method outputs a font metrics grid, displaying in the colour channels:
 *
 *		  red : raw glyph extent
 *		green : glyph + effects margin
 *		 blue : glyph + effects margin + texture margin
 *		alpha : glyph alpha
 */
void GlyphCache::createGridMap( Moo::RenderTarget* pDestRT )
{
	BW_GUARD;
	//The positions string is a long hex string containing position data in the bitmap.
	//There should be 4 hex characters per font character, being xxyy(xpos)(ypos)
	//The widths string is similar, but has only a 2-character hex code per string.
	Moo::rc().beginScene();
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
				D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );

	MF_ASSERT( pDestRT->width() == pTexture_->width() )
	MF_ASSERT( pDestRT->height() == pTexture_->height() )

	if ( pDestRT->push() )
	{
		const uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | (Moo::rc().stencilAvailable() ? D3DCLEAR_STENCIL : 0);
			Moo::rc().device()->Clear( 0, NULL, clearFlags, 0x00000000, 1.0f, 0 );

		//We now have the uv top left & width in uv coordinates for the
		//source map.  copy that into our glyph region.
		Geometrics::setVertexColour(0xffffffff);
		CustomMesh< Moo::VertexTLUV > mesh( D3DPT_TRIANGLESTRIP );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setTexture( 0, pTexture_->pTexture() );

		Glyphs::iterator it = glyphs_.begin();
		Glyphs::iterator en = glyphs_.end();
		while ( it != en )
		{
			Glyph& g = it->second;
			Vector2 tl,br;
			Vector2 sourceUV( g.uv_.x, g.uv_.y / uvDivisor_ );
			Vector2 sourceSize( g.uvWidth_, height_ / this->mapHeight() );
			glyphToScreenRect( g, tl, br );

			//Glyph area plus Effect margin in the green and alpha channel
			mesh.clear();
			Geometrics::createRectMesh(tl, br, sourceUV, sourceUV + sourceSize, 0xFF00FF00, mesh);
			mesh.draw();

			//Glyph area (minus effects margin) in the red channel
			br.x -= effectsMargin_;
			br.y -= effectsMargin_;
			mesh.clear();
			Geometrics::createRectMesh(tl, br, sourceUV, sourceUV + sourceSize, 0x00FF0000, mesh);			
			mesh.draw();

			//Glyph area plus Effect margin plus Texture margin in the blue channel
			br.x += effectsMargin_;
			br.y += effectsMargin_;
			br.x += textureMargin_;
			br.y += textureMargin_;
			mesh.clear();
			Geometrics::createRectMesh(tl, br, sourceUV, sourceUV + sourceSize, 0x000000FF, mesh);
			mesh.draw();

			++it;
		}

		pDestRT->pop();
	}

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
				D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
	Moo::rc().endScene();
}


/**
 *	This method determines the screen rect (cache render target, pixel
 *	coordinates) for the given glyph.
 *
 *	@param	g		The Glyph in question.
 *	@param	tl		[out] top left pixel coordinates of the glyph.
 *	@param	br		[out] bottom right pixel coordinates of the glyph.
 */
void GlyphCache::glyphToScreenRect( const Glyph& g, Vector2& tl, Vector2& br )
{
	tl.set( g.uv_.x * (float)pTexture_->width(), (g.uv_.y/uvDivisor_) * (float)pTexture_->height() );
	br.set( tl.x + g.uvWidth_ * (float)pTexture_->width(), tl.y + height_ );
}


/**
 *	This method preloads the glyph cache with the given characters.
 *
 *	@param	characterSet string of characters to initialise the cache with.
 */
void GlyphCache::preloadGlyphs( const BW::wstring& characterSet )
{
	BW_GUARD;
	//Now precache any characters as specified
	size_t numChars = characterSet.size();

	//Access each glyph in the character set, and artifically raise its reference count in
	//order to leak that glyph for the duration
	for ( size_t i = 0; i < numChars; i++ )
	{
		this->glyph( characterSet[i] );
		this->refCounts().incRef(characterSet[i]);
	}
	//this->refCounts().report();
}


/**
 *	This method saves the font cache object to the supplied data section.
 *	It is not yet implemented.
 *
 *	@param	dataSection DataSection to save the metrics information to.
 *	@param	ddsName		Name for the dds file containing the glyph cache.
 *	@param	gridName	Name for the dds file containing the alignment grid.
 */
bool GlyphCache::save( DataSectionPtr pSection, const BW::string& ddsName, const BW::string& gridName )
{
	BW_GUARD;
	DataSectionPtr binChild = pSection->openSection( "glyphCache", true, BinSection::creator() );

	MemoryOStream str;

	str << "GlyphCache_v1";
	str << maxWidth_;
	str << height_;

	Glyphs::iterator it = glyphs_.begin();
	Glyphs::iterator en = glyphs_.end();
	while ( it != en )
	{
		wchar_t c = it->first;
		Glyph& g = it->second;
		str << c;
		Vector2 normalisedUV( g.uv_.x, g.uv_.y / uvDivisor_ );
		str << normalisedUV;
		str << g.uvWidth_;
		++it;
	}

	BinaryPtr pData = new BinaryBlock( str.data(), str.size(), "GlyphCache" );
	BinSectionPtr pBinSection = new BinSection( "generated", pData );
	binChild->setBinary( pData );

	Moo::TextureManager::writeDDS( this->pTexture()->pTexture(), ddsName, D3DFMT_A8R8G8B8, 1 );
	INFO_MSG( "Font cache map for %s saved to %s\n", fontFileName_.c_str(), ddsName.c_str() );

	Moo::RenderTargetPtr pGridMap = new Moo::RenderTarget( "temporaryFontGridMap" );
	pGridMap->create( pTexture_->width(), pTexture_->height(), false );
	this->createGridMap( pGridMap.get() );
	Moo::TextureManager::writeDDS( pGridMap->pTexture(), gridName, D3DFMT_A8R8G8B8, 1 );
	INFO_MSG( "Font cache grid map for %s saved to %s\n", fontFileName_.c_str(), gridName.c_str() );

	return true;
}


/**
 *	This method clears the occupancy table for the glyph cache.
 */
void GlyphCache::clearOccupancy()
{
	memset( occupied_, 0, sizeof(bool) * (slotBounds_.x_*slotBounds_.y_) );
}


/**
 *	This method creates the underlying True Type Font resource used to copy
 *	glyphs into our cache.
 *
 *	@return	bool	success of the operation.
 */
bool GlyphCache::createFont()
{
	BW_GUARD;
	MF_ASSERT_DEV( !sourceFonts_.empty() );

	if (sourceFonts_.empty())
	{
		return false;
	}

	// always warn if the primary font fails to load
	sourceFonts_[0]->warnIfMissing( true );

	for( size_t i = 0; i < sourceFonts_.size(); i++ )
	{
		sourceFonts_[i]->createFont( pointSize_ );
	}

	// Make sure we successfully created at least the first source font.
	// If others failed, they'll just fall back to this.
	if (sourceFonts_[0]->font() == NULL)
	{
		return false;
	}

	bitmap_ = new Gdiplus::Bitmap( gdiSurfacePitch_, gdiSurfacePitch_, pixelFormat_ );
	graphics_ = new Gdiplus::Graphics(bitmap_);

	brush_ = new Gdiplus::SolidBrush( Gdiplus::Color(255,255,255,255) );
	shadowBrush_ = new Gdiplus::SolidBrush( Gdiplus::Color((BYTE)(shadowAlpha_*255.f),0,0,0) );
	Vector2 dimensions;

	//Try a couple of glyphs to ascertain the maximum size
	calcGlyphRect( widestChar_, dimensions );
	maxWidth_ = dimensions.x + effectsMargin_;
	height_ = dimensions.y + effectsMargin_;

	//95 is the 'underscore' unicode character
	calcGlyphRect( 95, dimensions );
	height_ = max( height_, dimensions.y + effectsMargin_ );

	//0x2588 is the 'full block' unicode character
	calcGlyphRect( 0x2588, dimensions );
	height_ = max( height_, dimensions.y + effectsMargin_ );

	//INFO_MSG( "%s : %0.1f * %0.1f\n", fontName_.c_str(), maxWidth_, height_ );
	return true;
}

BW_END_NAMESPACE

// glyph_cache.cpp
