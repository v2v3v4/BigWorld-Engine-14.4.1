#include "pch.hpp"
#include "font_metrics.hpp"
#include "glyph_cache.hpp"
#include "glyph_reference_holder.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Font",0 );


BW_BEGIN_NAMESPACE


// Matches colour tags for colourFormatting mode. Returns the number of characters
// to skip to get passed the tag, otherwise 0 if there was no match.
int FontMetrics::matchColourTag( const BW::wstring& str, size_t offset, uint32& outColour )
{
	BW_GUARD;
	static const wchar_t* colourTagPrefix = L"\\c";
	static const wchar_t* colourTagPostfix = L";";
	static size_t colourTagPrefixLen = wcslen( colourTagPrefix );
	static size_t colourTagPostfixLen = wcslen( colourTagPostfix );
	static size_t colourTagLength = colourTagPrefixLen + wcslen( L"FFFFFFFF" ) + colourTagPostfixLen;

	size_t len = str.length();

	if ( offset > len - colourTagLength )
	{
		return -1;
	}

	if ( wcsncmp( &str[offset], colourTagPrefix, colourTagPrefixLen ) != 0)
	{
		return -1;
	}

	outColour = 0;
	const wchar_t* hexStart = &str[offset + colourTagPrefixLen];
	if ( wcsncmp( hexStart+8, colourTagPostfix, colourTagPostfixLen ) != 0 )
	{
		return -1;
	}

	if ( swscanf( hexStart, L"%x", &outColour ) != 1 )
	{
		return 0;
	}

	if ( offset > 0 && str[offset-1] == L'\\' )
	{
		return 1; // was escaped
	}

	// sscanf reads RRGGBBAA, D3D is AARRGGBB	
	outColour = ((outColour >> 8) & 0xFFFFFF) | ((outColour << 24) & 0xFF000000);
	return int(colourTagLength);
}


//-----------------------------------------------------------------------------
//Section : FontMetrics
//-----------------------------------------------------------------------------
/**
 *	Constructor.
 */
FontMetrics::FontMetrics():
	cache_( new GlyphCache )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
FontMetrics::~FontMetrics()
{
	BW_GUARD;
	bw_safe_delete(cache_);
}


/**
 *	This method breaks a string into a segments for display within a specified
 *	width and height.
 *
 *	This method accepts a long string and the max width, and returns the
 *	result width \& height ( in texels ) of the given string and the
 *	broken string lines.
 *
 *	@param	wstr						The string to break.
 *  @param	w							The desired max width.
 *  @param	h							The desired max height.
 *	@param	wordBreak					The chars that could be used as seperator of word.
 *  @param	punctuation					The chars work as punctuations.
 *  @param	minHyphenWidth				If the width of the rest of line is <= minHyphenWidth, we will use hyphen to break word
 *
 *	@return BW::vector<BW::string>	Broken string lines.
 */
BW::vector<BW::wstring> FontMetrics::breakString( BW::wstring wstr, /* IN OUT */int& w, int& h,
	int minHyphenWidth, const BW::wstring& wordBreak, const BW::wstring& punctuation )
{
	BW_GUARD;
	BW::vector<BW::wstring> result;
	uint maxWidth = 0;
	h = 0;
	BW::wstring::size_type offset1, offset2;
	while( wstr.find( '\r' ) != wstr.npos || wstr.find( '\n' ) != wstr.npos )
	{
		offset1 = wstr.find( '\r' );
		offset2 = wstr.find( '\n' );
		if (offset1 == wstr.npos || (offset2 != wstr.npos && offset2 < offset1))
		{
			offset1 = offset2;
		}
		int W = w, H;
		BW::vector<BW::wstring> temp = breakString( wstr.substr( 0, offset1 ), W, H, minHyphenWidth, wordBreak,
			punctuation );
		if( temp.empty() )
		{
			int width, height;
			stringDimensions( L" ", width, height, false );
			result.push_back( L"" );
			h += height;
		}
		else
		{
			result.insert( result.end(), temp.begin(), temp.end() );
			h += H;
		}
		if( (uint)W >= maxWidth )
			maxWidth = (uint)W;

		wstr.erase( wstr.begin(), wstr.begin() + offset1 );
		if( wstr.size() > 1 && wstr[ 0 ] != wstr[ 1 ] &&
			( wstr[ 1 ] == '\r' || wstr[ 1 ] == '\n' ) )
			wstr.erase( wstr.begin() );
		wstr.erase( wstr.begin() );
	}

	BW::vector<BW::wstring> words;
	while( !wstr.empty() )
	{
		BW::wstring word;
		while( !wstr.empty() && wordBreak.find( wstr[ 0 ] ) != wordBreak.npos )
		{
			wstr.erase( wstr.begin() );
		}

		while( !wstr.empty() && ( punctuation.find( wstr[0] ) == wordBreak.npos &&
			wordBreak.find( wstr[ 0 ] ) == wordBreak.npos ) )
		{
			word += wstr[ 0 ], wstr.erase( wstr.begin() );
		}

		while( !wstr.empty() && ( punctuation.find( wstr[ 0 ] ) != punctuation.npos ||
			wordBreak.find( wstr[ 0 ] ) != wordBreak.npos ) )
		{
			if( wordBreak.find( wstr[ 0 ] ) != wordBreak.npos )
				wstr.erase( wstr.begin() );
			else
				word += wstr[ 0 ], wstr.erase( wstr.begin() );
		}

		words.push_back( word );
	}

	BW::wstring line;
	while( !words.empty() )
	{
		BW::wstring suffix = ( line.empty() || wordBreak.empty() ? L"" : wordBreak.substr( 0, 1 ) ) + words[ 0 ];
		if( stringWidth( line + suffix, false ) <= (uint)w )
		{
			line += suffix;
			words.erase( words.begin() );
		}
		else
		{
			if( stringWidth( line + ( line.empty() || wordBreak.empty() ? L"" : ( wordBreak.substr( 0, 1 ) + L"-" ) ), false )
				< (uint)minHyphenWidth || line.empty() )
			{
				if( !wordBreak.empty() && !line.empty() )
					line += wordBreak[ 0 ];
				while( !words[ 0 ].empty() && stringWidth( line + words[ 0 ].substr( 0, 1 ) + L'-', false ) < (uint)w )
					line += words[ 0 ][ 0 ], words[ 0 ].erase( words[ 0 ].begin() );
				if ( line.empty() )
					break;
				if( !wordBreak.empty() )
					line += L'-';
			}
			int width, height;
			stringDimensions( line, width, height, false );
			if( (uint)width > maxWidth )
				maxWidth += width;
			h += height;
			result.push_back( line );
			line.clear();
		}
	}
	if( !line.empty() )
	{
		int width, height;
		stringDimensions( line, width, height, false );
		if( (uint)width > maxWidth )
			maxWidth += width;
		h += height;
		result.push_back( line );
	}
	w = (int)maxWidth;
	return result;
}


/**
 *	This method returns the width, in texels, of the given string.
 *	@param	str		The string to calculate the width.
 *	@param	bool	Whether or not newline characters should be wrapped. 
 *	@param	bool	Whether or not this string is using formatting tags
 *	@return uint	The width of the string, in texels.
 */
uint FontMetrics::stringWidth( const BW::string& str, bool multiline, bool colourFormatting )
{
	BW_GUARD;
	BW::wstring wstr( str.size(), ' ' );
	bw_snwprintf( &wstr[0], wstr.size(), L"%S\0", str.c_str() );
	return stringWidth( wstr, multiline, colourFormatting );
}


/**
 *	This method returns the width, in texels, of the given string.
 *	@param	str		The string to calculate the width.
 *	@param	bool	Whether or not newline characters should be wrapped. 
 *	@param	bool	Whether or not this string is using formatting tags
 *	@return uint	The width of the string, in texels.
 */
uint FontMetrics::stringWidth( const BW::wstring& str, bool multiline, bool colourFormatting )
{
	BW_GUARD;
	int w,h;
	this->stringDimensionsInternal( str, multiline, colourFormatting, w, h );
	return w;
}


void FontMetrics::stringDimensionsInternal( const BW::wstring& str, 
										   bool multiline, bool colourFormatting,
										   int& w, int& h )
{
	BW_GUARD;
	w = h = 0;
	size_t s = str.size();

	//Note about this calculation.
	//
	//Firstly, the charWidth includes the effects margin ( e.g. drop shadow )
	//However, when drawing many characters into a string, the effects margin
	//is overwritten by the next character.
	//
	//We probably should the very last character's effects margin back on,
	//so that the width is exactly right for the string.
	//
	//However, there are two probable ways of using string width.
	//1) for single words in the gui.  in this case, the calculated width will
	//be one pixel too small, meaning any centering calculations will still be ok.
	//2) for single words in the gui, if wrapping the word in a border, then one
	//would expect a margin to be applied anyway, in which case the small
	//inaccuracy won't matter.
	//3) *most importantly* the string width is used to lay out text fields, and
	//is used incrementally ( i.e. calculate the incremental width of a sentence )
	//in this case, we don't want to add the effects margin on at all, because if
	//we did the accumulated addition of effects margins ( that won't be drawn )
	//would end up reported a far greater width than is correct.
	//
	//Therefore, we choose the lowest probable error and don't add on the final
	//margin.
	//
	//TODO : maybe we could just have two functions, one for incremental calcs, and
	//one for single use calcs.

	int lines = 1;
	int linew = 0;

	size_t i = 0;
	while( i < s )
	{
		uint32 colour;
		if ( colourFormatting )
		{
			// Skip colour tags when calculating size.
			int res = matchColourTag( str, i, colour );
			if ( res > 0 )
			{
				i += res;
				continue;
			}			
		}

		if ( multiline && str[i] == '\n' )
		{
			w = std::max( w, linew );
			linew = 0;
			lines += 1;
		}
		else
		{
			GlyphCache::Glyph g = cache_->glyph(str[i]);
			float uvWidth = g.uvWidth_;
			linew += (int)(uvWidth*cache_->mapWidth()-cache_->effectsMargin());
		}

		++i;
	}

	float chh = this->heightPixels();

	w = std::max( w, linew );
	h = int(lines * chh);
}


void FontMetrics::stringDimensionsInternal( const BW::string& s, bool multiline,
										    bool colourFormatting, int& w, int& h )
{
	BW_GUARD;
	static wchar_t buf[1024];
	bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%S\0", s.c_str() );
	this->stringDimensionsInternal( buf, multiline, colourFormatting, w, h );
}


/**
 *	This method returns the height of the font.  It is assumed
 *	all characters in the font set have the same height.
 *
 *	@return	float		The height of the font.
 */
float FontMetrics::heightPixels() const
{
	BW_GUARD;
	return cache_->heightPixels();
}


/**
 *	This method returns the height of the font, in clip coordinates.
 *	It is calculated each time, because texel->clip can change.
 *
 *	@return	float		The height of the font, in clip coordinates.
 */
float FontMetrics::clipHeight() const
{
	BW_GUARD;
	return cache_->heightPixels() / (Moo::rc().halfScreenHeight());
}


/**
 *	This method returns the width of a character, in clip coordinates.
 *	It is calculated each time, because texel->clip can change.
 *
 *	@param	c			The character for the uv width calculation.
 *	@return	float		The width of c, in clip coordinates.
 */
float FontMetrics::clipWidth( wchar_t c )
{
	BW_GUARD;
	const GlyphCache::Glyph& g = cache_->glyph(c);
	return (g.uvWidth_ * cache_->mapWidth()) / (Moo::rc().halfScreenWidth());
}


/**
 *	This method returns the width and height of a string, in texels.
 *
 *	@param	str		The string for dimension calculation.
 *	@param	w		The returned width
 *	@param	h		The returned height
 *	@param	bool	Whether or not newline characters should be wrapped.
 *	@param	bool	Whether or not colour formatting tags should be parsed.
 */
void FontMetrics::stringDimensions( const BW::string& str, int& w, int& h, 
											bool multiline, bool colourFormatting )
{
	BW_GUARD;
	this->stringDimensionsInternal( str, multiline, colourFormatting, w, h );
}

/**
 *	This method returns the width and height of a string, in texels.
 *
 *	@param	str		The string for dimension calculation.
 *	@param	w		The returned width
 *	@param	h		The returned height
 *	@param	bool	Whether or not newline characters should be wrapped.
 *	@param	bool	Whether or not colour formatting tags should be parsed. 
 */
void FontMetrics::stringDimensions( const BW::wstring& str, int& w, int& h, 
											bool multiline, bool colourFormatting )
{
	BW_GUARD;
	this->stringDimensionsInternal( str, multiline, colourFormatting, w, h );
}

/**
 *	This method returns the width of the widest character, in texels.
 * 
 *	@return float	Width in texels of the widest character in the font.
 */
float FontMetrics::maxWidth() const
{
	BW_GUARD;
	return cache_->maxWidthPixels();
}

BW_END_NAMESPACE

// font_metrics.cpp
