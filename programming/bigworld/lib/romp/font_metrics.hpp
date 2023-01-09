#ifndef FONT_METRICS_HPP
#define FONT_METRICS_HPP

#include "cstdmf/stdmf.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class GlyphReferenceHolder;
class GlyphCache;


/**
 *	This class can be queried for font based information, including
 *	character width, string width and height.
 *
 *	All values are returned in texels, unless otherwise specified.
 */
class FontMetrics : public ReferenceCount
{
public:
	FontMetrics();
	~FontMetrics();

	BW::vector<BW::wstring> breakString( BW::wstring wstr, /* IN OUT */int& w, int& h,
		int minHyphenWidth = 0, const BW::wstring& wordBreak = L" ",
		const BW::wstring& punctuation = L",<.>/?;:'\"[{]}\\|`~!@#$%^&*()-_=+" );

	void stringDimensions( const BW::string& str, int& w, int& h, bool multiline=false, bool colourFormatting=false );
	uint stringWidth( const BW::string& str, bool multiline=false, bool colourFormatting=false );
	void stringDimensions( const BW::wstring& str, int& w, int& h, bool multiline=false, bool colourFormatting=false );
	uint stringWidth( const BW::wstring& str, bool multiline=false, bool colourFormatting=false );

	float maxWidth() const;
	float clipWidth( wchar_t c );
	float clipHeight() const;
	float heightPixels() const;

	static int matchColourTag( const BW::wstring & str, size_t offset,
		uint32 & outColour );

//private:

	GlyphCache& cache()			{ return *cache_; }
	const GlyphCache& cache() const	{ return *cache_; }
	GlyphCache* cache_;

private:
	void stringDimensionsInternal( const BW::wstring& s, bool multline, bool colourFormatting, int& w, int& h );
	void stringDimensionsInternal( const BW::string& s, bool multline, bool colourFormatting, int& w, int& h );	
};

typedef SmartPointer<FontMetrics>	FontMetricsPtr;

BW_END_NAMESPACE

#endif	//FONT_METRICS_HPP
