#ifndef GLYPH_CACHE_HPP
#define GLYPH_CACHE_HPP

#include "moo/render_target.hpp"
#include "moo/managed_texture.hpp"
#include "resmgr/datasection.hpp"

namespace Gdiplus
{
class Bitmap;
class Brush;
class Font;
class FontFamily;
class Graphics;
} // namespace Gdiplus

BW_BEGIN_NAMESPACE

class GlyphReferenceHolder;

/**
 *	This class manages a cache of glyphs in a render target texture.
 *	
 */
class GlyphCache
{
public:
	GlyphCache();
	~GlyphCache();
	bool load( const BW::string& fontFileName, DataSectionPtr pSection );
	bool save( DataSectionPtr pSection, const BW::string& ddsName, const BW::string& gridName );
	void refillSurface();

	bool dynamicCache() const;

	// Accessors
	Moo::BaseTexturePtr pTexture()			{ return pTexture_; }
	Vector2 mapSize() const					{ return Vector2((float)pTexture_->width(),(float)pTexture_->height()); }
	float mapWidth() const					{ return (float)pTexture_->width(); }
	float mapHeight() const					{ return (float)pTexture_->height(); }
	float heightPixels() const				{ return height_; }
	float maxWidthPixels() const			{ return maxWidth_; }
	float effectsMargin() const				{ return effectsMargin_; }
	float textureMargin() const				{ return textureMargin_; }
	float uvDivisor() const					{ return uvDivisor_; }

	const BW::string& fontName() const;
	const BW::string& fontFileName() const	{ return fontFileName_; }


	// Cache slot support
	struct Slot
	{
		Slot() : x_(0), y_(0) {}
		Slot( uint32 x, uint32 y ):
			x_(x),
			y_(y)
		{};

		uint32	x_;
		uint32	y_;
	};

	void	slotToUV( const Slot& slot, Vector2& ret ) const;
	void	uvToSlot( const Vector2& uv, Slot& ret ) const;
	bool	grabSlot( Slot& slot );
	void	occupied( const Slot& slot, bool state );
	bool	occupied( const Slot& slot ) const;

	// Glyph definition
	class Glyph
	{
	public:
		Glyph() :
			uvWidth_(),
			lastUsed_()
		{
		}

		Vector2	uv_;
		float	uvWidth_;
		uint32	lastUsed_;
	};

	typedef BW::map<wchar_t, Glyph>	Glyphs;
	const Glyphs& glyphs() const			{ return glyphs_; }
	const Glyph& glyph( wchar_t );
	GlyphReferenceHolder& refCounts()		{ return *refCounts_; }

private:
	const Glyph& addGlyphInternal( wchar_t c, const Slot& glyphRegion );
	bool calcGlyphRect( wchar_t c, Vector2& dimensions );
	bool loadLegacyGeneratedSection( DataSectionPtr pSection, const BW::wstring& characterSet );
	bool loadPreGeneratedData( DataSectionPtr pSection );
	void createGridMap( Moo::RenderTarget* pDestRT );
	void preloadGlyphs( const BW::wstring& characterSet );
	void loadCharacterSet( DataSectionPtr pSection, BW::wstring& characterSet );
	void glyphToScreenRect( const Glyph& g, Vector2& tl, Vector2& br );
	bool createFont();
	void clearOccupancy();
	bool expand();
	Glyphs	glyphs_;
	GlyphReferenceHolder* refCounts_;
	bool* occupied_;
	Slot curr_;
	Slot slotBounds_;
	Vector2 slotSize_;

	wchar_t spaceProxyChar_;
	wchar_t widestChar_;
	float effectsMargin_;		//in pixels
	float textureMargin_;		//in pixels
	float maxWidth_;			//in pixels
	float height_;				//in pixels
	BW::string fontFileName_;
	bool proportional_;
	int pointSize_;
	float uvDivisor_;
	BW::wstring defaultCharacterSet_;
	int shadowAlpha_;

	Moo::ManagedTexturePtr	pTexture_;
	bool lastExpandFailed_;

	//GDI+ font rendering
	Gdiplus::Graphics*		graphics_;
	Gdiplus::Brush*			brush_;
	Gdiplus::Brush*			shadowBrush_;
	Gdiplus::Bitmap*		bitmap_;
	uint32					pixelFormat_;
	uint32					gdiSurfacePitch_;

	class SourceFont : public ReferenceCount
	{
	public:
		SourceFont( DataSectionPtr ds, bool bold=false, bool antialias=true, bool dropShadow=false );
		~SourceFont();

		bool createFont( int pointSize );

		bool matchChar( wchar_t c ) const { return c >= rangeMin_ && c <= rangeMax_; }

		const BW::string& fontName() const { return fontName_; }
		uint32 rangeMin() const { return rangeMin_; }
		uint32 rangeMax() const { return rangeMax_; }
		bool antialias() const { return antialias_; }
		bool bold() const { return bold_; }
		bool dropShadow() const { return dropShadow_; }

		bool warnIfMissing() const { return warnIfMissing_; }
		void warnIfMissing( bool b ) { warnIfMissing_ = b; }

		Gdiplus::Font* font() const { return font_; }

	private:
		SourceFont( const SourceFont& o );

		BW::string fontName_;
		uint32 rangeMin_;
		uint32 rangeMax_;
		bool antialias_;
		bool bold_;
		bool dropShadow_;
		bool warnIfMissing_;

		Gdiplus::FontFamily* fontFamily_;
		Gdiplus::Font* font_;
	};

	typedef SmartPointer< SourceFont > SourceFontPtr;

	BW::vector<SourceFontPtr>	sourceFonts_;
	const SourceFontPtr& charToFont( wchar_t c );

	class SortSourceFont
	{
	public:
		bool operator () ( const SourceFontPtr& p1, const SourceFontPtr& p2 )
		{ return p1->rangeMin() < p2->rangeMin(); }
	};
};

BW_END_NAMESPACE

#endif //GLYPH_CACHE_HPP
