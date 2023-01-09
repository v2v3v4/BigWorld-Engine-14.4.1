#ifndef FONT_HPP
#define FONT_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"
#include "math/vector2.hpp"
#include "moo/render_context.hpp"
#include "moo/render_target.hpp"
#include "moo/material.hpp"
#include "moo/base_texture.hpp"
#include "moo/vertex_formats.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class GlyphReferenceHolder;
class GlyphCache;
class FontMetrics;


/**
 *	This class represents an instance of a font that can draw
 *	immediately to the screen, or into a mesh.
 *
 *	The glyphs used by the standard Font class are guaranteed
 *	only to exist for one frame.  If you are using the drawIntoMesh
 *	methods and want the glyphs to stay around for as long as your
 *	mesh is in use, then use the CachedFont class instead.
 *
 *	The font class includes all font metrics, and can be drawn
 *	proportionally or fixed.
 *	The only states a Font has is scale, and colour. ( and perhaps
 *	indirectly a font metrics cache )
 */
class Font : public ReferenceCount
{
public:
	//creates a mesh in clip coordinates, ignoring the font scale
	//when using font meshes, simply prepend a matrix transformation.
	virtual float drawIntoMesh(
		VectorNoDestructor<Moo::VertexXYZDUV2>& mesh,
		const BW::wstring& str,
		float clipX = 0.f,
		float clipY = 0.f,
		float* w = NULL,
		float* h = NULL,
		bool multiline = false,
		bool colourFormatting = false );
	//creates a mesh in clip coordinates, exactly within the box
	//specified
	virtual void drawIntoMesh(
		VectorNoDestructor<Moo::VertexXYZDUV2>& mesh,
		const BW::wstring& str,
		float clipX,
		float clipY,
		float w,
		float h,
		float* retW = NULL,
		float* retH = NULL,
		bool multiline = false,
		bool colourFormatting = false );
	//draws immediately to the screen at the griven character block
	void drawConsoleString( const BW::string& str, int col, int row, int x = 0, int y = 0 );
	//draws immediately to the screen at the griven character block
	void drawWConsoleString( const BW::wstring& str, int col, int row, int x = 0, int y = 0 );
	//draws immediately to the screen at the given pixel position
	virtual void drawString( const BW::string& str, int x, int y );
	//draws immediately to the screen at the given pixel position
	virtual void drawWString( const BW::wstring& wstr, int x, int y );
	//draws immediately to the screen at the given pixel position with maxWidth
	virtual int drawString( BW::wstring wstr, int x, int y, int w, int h,
		int minHyphenWidth = 0, const BW::wstring& wordBreak = L" ",
		const BW::wstring& punctuation = L",<.>/?;:'\"[{]}\\|`~!@#$%^&*()-_=+"	);
	//draws immediately to the screen at the given clip position
	void drawStringInClip( const BW::string& wstr, const Vector3 & position );
	//draws immediately to the screen at the given clip position
	void drawStringInClip( const BW::wstring& wstr, const Vector3 & position );

	void colour( uint32 col )	{ colour_ = col; }
	uint32 colour() const			{ return colour_; }
	void scale( const Vector2& s )	{ scale_ = s; }
	const Vector2& scale() const	{ return scale_; }

	Moo::BaseTexturePtr pTexture();

	void fitToScreen( bool state, const Vector2& numCharsXY );
	bool fitToScreen() const;

	Vector2 screenCharacterSize() const;

	FontMetrics& metrics();
	const FontMetrics& metrics() const;
protected:
	Font( FontMetrics& fm );
	virtual float	makeCharacter( Moo::VertexXYZDUV2* vert, wchar_t c, const Vector2& pos );

	FontMetrics& metrics_;

	Vector2 scale_;
	uint32	colour_;
	//static FontIndices s_fontIndices;

	//this should go in ConsoleFont
	bool	fitToScreen_;
	Vector2	numCharsXY_;

	//friend used so constructor may be private, ensuring
	//fonts are only created via the FontManager.
	friend class FontManager;
};

typedef SmartPointer<Font>	FontPtr;


/**
 *	This class looks like Font, but it holds a reference to every character
 *	that it makes.  It is up to the user to release references they no longer
 *	require via the releaseRefs() method.
 */
class CachedFont : public Font
{
public:
	void releaseRefs();		
private:
	CachedFont( FontMetrics& fm );
	~CachedFont();
	virtual float	makeCharacter( Moo::VertexXYZDUV2* vert, wchar_t c, const Vector2& pos );
	GlyphReferenceHolder*	glyphRefHolder_;

	//friend used so constructor may be private, ensuring
	//fonts are only created via the FontManager.
	friend class FontManager;
};

typedef SmartPointer<CachedFont>	CachedFontPtr;


#ifdef CODE_INLINE
#include "font.ipp"
#endif

BW_END_NAMESPACE

#endif
