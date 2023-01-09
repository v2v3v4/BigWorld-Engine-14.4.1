#ifndef GLYPH_REFERENCE_HOLDER_HPP
#define GLYPH_REFERENCE_HOLDER_HPP

BW_BEGIN_NAMESPACE

/**
 *	This class stores a reference count for a collection of glyphs.
 *	One is held by the base font metrics resource, and is used to
 *	determine available texture cache slots.
 *
 *	Others are held by CachedFonts, so any particular font object
 *	can hold onto references of strings in use.
 */
class GlyphReferenceHolder
{
public:
	typedef BW::map<wchar_t, uint32> GlyphReferenceCountMap;

	void incRef( wchar_t c );
	int decRef( wchar_t c, int num = 1 );
	int refCount( wchar_t c ) const;
	GlyphReferenceCountMap::iterator begin();
	GlyphReferenceCountMap::iterator end();
	void erase( wchar_t c );
	GlyphReferenceCountMap::iterator erase( GlyphReferenceCountMap::iterator it );
	void releaseRefs( const GlyphReferenceHolder& other );
	void reset();
	void report() const;
private:
	GlyphReferenceCountMap refCounts_;
};

BW_END_NAMESPACE

#endif //GLYPH_REFERENCE_HOLDER_HPP
