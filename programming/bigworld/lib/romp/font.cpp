#include "pch.hpp"
#include "font.hpp"
#include "font_metrics.hpp"
#include "font_manager.hpp"
#include "glyph_cache.hpp"
#include "glyph_reference_holder.hpp"
#include "moo/texture_manager.hpp"
#include "moo/render_target.hpp"
#include "moo/custom_mesh.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/string_utils.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "time.h"

DECLARE_DEBUG_COMPONENT2( "Font",0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "font.ipp"
#endif


//-----------------------------------------------------------------------------
//Section : Font
//-----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Font::Font( FontMetrics& fm )
:scale_( 1.f, 1.f ),
 metrics_( fm ),
 fitToScreen_( false ),
 numCharsXY_( 120, 40 ),
 colour_( 0xffffffff )
{
	BW_GUARD;
}


/**
 *	This method draws the given string into a mesh.  The mesh contains
 *	vertices in clip coordinates, and is anchored at the top left.
 *
 *	Note the text is added to the mesh, so make sure you clear the mesh
 *	first, if that is what you require.
 *
 *  @param	mesh	The mesh to draw into.
 *	@param	str		The string to draw.
 *	@param	clipX	The starting X coordinate in clip space.
 *	@param	clipY	The starting Y coordinate in clip space.
 *	@param	retW	The final X position (NOT width).
 *	@param	retH	The height of the string.
 *
 *	@return	float	The width of the string.
 */
float Font::drawIntoMesh(
	VectorNoDestructor<Moo::VertexXYZDUV2>& mesh,
	const BW::wstring& str,
	float clipX,
	float clipY,
	float* retW,
	float* retH,
	bool multiline,
	bool colourFormatting )
{
	BW_GUARD;
	float initialX = clipX;	
	float initialY = clipY;

	if ( str.size() == 0 )
	{
		return 0.f;
	}

	size_t base = mesh.size();
	size_t n = str.size();
	mesh.resize( base + n*6);
	Vector2 pos( clipX, clipY );

	float halfx = Moo::rc().halfScreenWidth();
	float halfy = Moo::rc().halfScreenHeight();
	float effectsWidthInClip = metrics_.cache().effectsMargin() / halfx;

	int lines = 1;
	int rowColumns = 0;

	size_t i = 0;
	size_t ichr = 0;
	while( i < n )
	{
		uint32 colour;
		if (colourFormatting)
		{
			int res = FontMetrics::matchColourTag( str, i, colour );
			if ( res > 0 )
			{
				if (res > 1)
				{
					this->colour( colour );
				}

				i += res;
				continue;
			}			
		}

		if ( multiline && str[i] == '\n' )
		{
			if ( rowColumns == 0 )
			{
				// If this line has no actual characters, stick in a blank
				// space. This is to make sure our mesh bounds are correct for
				// the number of lines.
				this->makeCharacter( &mesh[base+ichr*6], L' ', pos );
				++ichr;
			}

			pos.x = clipX;
			pos.y -= metrics_.heightPixels() / halfy;
			lines += 1;
			rowColumns = 0;
		}
		else
		{
			pos.x += this->makeCharacter( &mesh[base+ichr*6], str[i], pos );
			pos.x -= effectsWidthInClip * scale_.x;
			++ichr;
			++rowColumns;
		}

		++i;		
	}

	if (mesh.size() != base+ichr*6 )
	{
		mesh.resize( base+ichr*6 );
	}

	pos.x += effectsWidthInClip;

	if ( retW )
	{
		*retW = pos.x;
	}

	if ( retH )
	{
		*retH = lines * (metrics_.clipHeight()) * scale_.y;
	}

	return pos.x - initialX;
}


/**
 *	This method draws the given string into a mesh.  The mesh contains
 *	vertices in clip coordinates, and is anchored at the top left.  The
 *	required dimensions are passed in, and the string is resized to
 *	exactly fit within the area.
 *
 *  @param	mesh	The mesh to draw into.
 *	@param	str		The string to draw.
 *	@param	clipX	The starting X coordinate in clip space.
 *	@param	clipY	The starting Y coordinate in clip space.
 *	@param	w		The desired width.
 *	@param	h		The desired height.
 *	@param	retW	The width being used is returned in this variable (if provided).
 *	@param	retH	The height being used is returned in this variable (if provided).
 */
void Font::drawIntoMesh(
	VectorNoDestructor<Moo::VertexXYZDUV2>& mesh,
	const BW::wstring& str,
	float clipX,
	float clipY,
	float w,
	float h,
	float* retW,
	float* retH,
	bool multiline,
	bool colourFormatting)
{
	BW_GUARD;
	float width,height;
	size_t base = mesh.size();
	this->drawIntoMesh(mesh,str,clipX,clipY,&width,&height,multiline,colourFormatting);

	//now resize the vertices to fit the given box.
	if (w!=0.f || h!=0.f)
	{		
		//if w or h is 0, we keep the other dimension explicit,
		//and set the width/height such that the correct aspect
		//ratio is maintained.
		if (w == 0.f && h != 0.f)
			w = h * (width/height);	
		else if (h == 0.f && w != 0.f)
			h = w * (height/width);	

		Vector2 scale( w/width, h/height );
		for (size_t i=base; i<mesh.size(); i++)
		{
			mesh[i].pos_.x *= scale.x;
			mesh[i].pos_.y *= scale.y;
		}	
	}
	else
	{
		//if passed in (desired) width, height are 0,
		//then we leave the mesh at the optimal size
		w = width;
		h = height;
	}

	if (retW) *retW = w;
	if (retH) *retH = h;
}


/**
 * This method returns the fonts character size which can be used
 * to determine the offsets for each character to be printed on the
 * screen.
 */
Vector2 Font::screenCharacterSize() const
{
	BW_GUARD;
	int charSizePx = int(metrics_.cache().maxWidthPixels());
	int charSizePy = int(metrics_.cache().heightPixels());
	int effectsWidthInPixels = int(metrics_.cache().effectsMargin());

	Vector2 size( float(charSizePx - effectsWidthInPixels), float(charSizePy) );

	if ( fitToScreen_ )
	{
		//calculate the difference between desired num chars and how big the font is on the screen
		float desiredPx = numCharsXY_.x * (float)charSizePx;
		float actualPx = Moo::rc().screenWidth();
		float desiredPy = numCharsXY_.y * (float)charSizePy;
		float actualPy = Moo::rc().screenHeight();
		size.x *= actualPx / desiredPx;
		size.y *= actualPy / desiredPy;		
	}

	return size;
}


/**
 *	This method draws the string directly onto the screen, using the
 *	current material settings.
 *
 *	@param	str		The string to draw.
 *	@param	col		Column.
 *	@param	row		Row.
 *	@param	x		The x coordinate, in character blocks.
 *	@param	y		The y coordinate, in character blocks.
 */
void Font::drawConsoleString( const BW::string& str, int col, int row, int x, int y )
{
	BW_GUARD;
	BW::wstring wstr;
	bw_ansitow( str, wstr );
	this->drawWConsoleString( wstr, col, row, x, y );
}

/**
 *	This method draws the string directly onto the screen, using the
 *	current material settings.
 *
 *	@param	str		The string to draw.
 *	@param	x		The x coordinate, in character blocks.
 *	@param	y		The y coordinate, in character blocks.
 */
void Font::drawWConsoleString( const BW::wstring& str, int col, int row, int x, int y )
{
	BW_GUARD;
	int charSizePx = int(metrics_.cache().maxWidthPixels());
	int charSizePy = int(metrics_.cache().heightPixels());
	int effectsWidthInPixels = int(metrics_.cache().effectsMargin());
	int px = col * (charSizePx - effectsWidthInPixels) + x;
	int py = row * charSizePy + y;
	Vector2 savedScale = scale_;

	if ( fitToScreen_ )
	{
		//calculate the difference between desired num chars and how big the font is on the screen
		float desiredPx = numCharsXY_.x * (float)charSizePx;
		float actualPx = Moo::rc().screenWidth();
		float desiredPy = numCharsXY_.y * (float)charSizePy;
		float actualPy = Moo::rc().screenHeight();
		scale_.x = actualPx / desiredPx;
		scale_.y = actualPy / desiredPy;
	}

	this->drawWString( str, px, py );

	scale_ = savedScale;
}


/**
 *	This method draws the string directly onto the screen, using the
 *	current material settings.
 *
 *	@param	str		The string to draw.
 *	@param	x		The x coordinate, in pixels.
 *	@param	y		The y coordinate, in pixels.
 *	@param	align	The alignment offset to use when drawing the string.
 */
void Font::drawString( const BW::string& str, int x, int y )
{
	BW_GUARD;
	//convert string to unicode
	wchar_t buf[256];
	MF_ASSERT( str.size() < 256 );
	bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%S\0", str.c_str() );
	this->drawWString( buf, x, y );
}


/**
 *	This method draws the string directly onto the screen, using the
 *	current material settings.
 *
 *	@param	str		The string to draw.
 *	@param	x		The x coordinate, in pixels.
 *	@param	y		The y coordinate, in pixels.
 *	@param	align	The alignment offset to use when drawing the string.
 */
void Font::drawWString( const BW::wstring& str, int x, int y )
{
	BW_GUARD;
	if ( !str.size() )
		return;

	//convert pixels to clip
	Vector3 pos(0, 0, 0);
	float halfx = Moo::rc().halfScreenWidth();
	float halfy = Moo::rc().halfScreenHeight();
	pos.x = ((float)x - halfx) / halfx;
	pos.y = (halfy - (float)y) / halfy;

	this->drawStringInClip( str, pos );
}


/**
 *	This method draws the string directly onto the screen with width and height limit, using the
 *	current material settings.
 *
 *	@param	wstr			The string to draw.
 *	@param	x				The x coordinate, in pixels.
 *	@param	y				The y coordinate, in pixels.
 *	@param	w				The max width, in pixels.
 *	@param	h				The max height, in pixels.
 *	@param	minHyphenWidth	If width is smaller than it, we will break the next word by hyphen.
 *	@param	wordBreak		Glyphs act as word breaks.
 *	@param	punctuation		Glyphs act as word punctuations.
 *	@return	int				The height of the real string output in pixels
 */
int Font::drawString( BW::wstring wstr, int x, int y, int w, int h, int minHyphenWidth,
	const BW::wstring& wordBreak, const BW::wstring& punctuation )
{
	BW_GUARD;
	int H;
	int yOff = 0;
	BW::vector<BW::wstring> wstrs = metrics().breakString( wstr, w, H, minHyphenWidth, wordBreak, punctuation );
	for( BW::vector<BW::wstring>::iterator iter = wstrs.begin(); iter != wstrs.end(); ++iter )
	{
		int linew, lineh;
		metrics().stringDimensions( *iter, linew, lineh );
		if( yOff + lineh > h )
			break;
		drawWString( *iter, x, y + yOff );
		yOff += lineh;
	}
	return yOff;
}

void Font::drawStringInClip( const BW::string& str, const Vector3 & position )
{
	BW_GUARD;
	//convert string to unicode
	wchar_t buf[256];
	MF_ASSERT( str.size() < 256 );
	bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%S\0", str.c_str() );
	this->drawStringInClip( buf, position );
}

void Font::drawStringInClip( const BW::wstring& wstr, const Vector3 & position )
{
	BW_GUARD;
	typedef Moo::VertexXYZDUV2 VertexType;

	static CustomMesh<VertexType> mesh;
	mesh.clear();

	//then draw
	float widthInClip = this->drawIntoMesh( mesh, wstr.c_str(), position.x, position.y ); 

	if ( !mesh.size() )
		return;

	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

 	if ( SUCCEEDED( Moo::rc().setFVF( VertexType::fvf() ) ) )
 	{
		// Now rendering with DynamicVertexBuffer....
		int vertexSize = sizeof(VertexType);
		Moo::DynamicVertexBufferBase2& vb = 
			Moo::DynamicVertexBufferBase2::instance( vertexSize );
		uint32 lockIndex = 0;
		const uint32 meshSize = static_cast<uint32>( mesh.size() );
		if ( vb.lockAndLoad( &mesh.front(), meshSize, lockIndex ) &&
			 SUCCEEDED(vb.set( 0 )) )
		{
			Moo::rc().drawPrimitive( D3DPT_TRIANGLELIST, lockIndex, meshSize / 3);
			vb.unset( 0 );
		}
	}

	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
}


/**
 *	This method puts a character into the vertex mesh.
 *	Six vertices will be made, so make sure that there is room enough.
 *	Vertices are generated ready for render with drawPrimitiveUP.
 *
 *	@param	vert		Pointer to six vertices to be filled.
 *	@param	c			The character to draw into the vertex array.
 *	@param	pos			The clip position to make the character at.
 *
 *	@return	width		The width of the character, in clip coordinates
 *						This includes the effects margin.
 */
float Font::makeCharacter( Moo::VertexXYZDUV2* vert, wchar_t c, const Vector2& pos )
{
	BW_GUARD;
	const GlyphCache::Glyph& g = this->metrics_.cache().glyph(c);
	float uvWidth = g.uvWidth_;
	float sourceMapHeight = this->metrics_.cache().mapHeight() / this->metrics_.cache().uvDivisor();
	float uvHeight = this->metrics_.heightPixels() / sourceMapHeight;
	float halfx = Moo::rc().halfScreenWidth();
	float halfy = Moo::rc().halfScreenHeight();
	Vector2 texToClip( metrics_.cache().mapWidth() / halfx, sourceMapHeight / halfy );
	float clipWidth = uvWidth*texToClip.x*scale_.x;
	float clipHeight = uvHeight*texToClip.y*scale_.y;

	static float s_offsetAmount = -1000.f;
	if ( s_offsetAmount < 0.f )
	{
		//Note - pixel snapping is now done in the shader, so we don't want
		//to offset here.  Retained watcher for debugging purposes only.
		s_offsetAmount = 0.f;

		MF_WATCH(	"Render/Font Offset", s_offsetAmount, Watcher::WT_READ_WRITE, 
					"Offset added to the characters in the font texture so "
					"that each texel get mapped to a pixel on the screen"	);
	}
	float clipOffsetX = -s_offsetAmount / halfx;
	float clipOffsetY = -s_offsetAmount / halfy;

	// setup a four-vertex list with the char's quad coords
	Moo::VertexXYZDUV2 tmpvert[4];
	tmpvert[0].pos_.x	= pos.x;
	tmpvert[0].pos_.y	= pos.y;
	tmpvert[0].pos_.z	= 0.0f;
	tmpvert[0].uv_		= g.uv_;
	tmpvert[0].uv_.y	= tmpvert[0].uv_.y;
	tmpvert[0].uv2_		= vert[0].uv2_;
	tmpvert[0].colour_	= colour_;

	tmpvert[1].pos_.x	=  pos.x+clipWidth;
	tmpvert[1].pos_.y	= pos.y;
	tmpvert[1].pos_.z	= 0.0f;
	tmpvert[1].uv_.x	= tmpvert[0].uv_.x + uvWidth;
	tmpvert[1].uv_.y	= tmpvert[0].uv_.y;
	tmpvert[1].uv2_		= vert[1].uv2_;
	tmpvert[1].colour_	= colour_;

	tmpvert[2].pos_.x	= pos.x+clipWidth;
	tmpvert[2].pos_.y	= pos.y-clipHeight;
	tmpvert[2].pos_.z	= 0.0f;
	tmpvert[2].uv_.x	= tmpvert[1].uv_.x;
	tmpvert[2].uv_.y	= tmpvert[1].uv_.y + uvHeight;
	tmpvert[2].uv2_		= vert[2].uv2_;
	tmpvert[2].colour_	= colour_;

	tmpvert[3].pos_.x	= pos.x;
	tmpvert[3].pos_.y	= pos.y-clipHeight;
	tmpvert[3].pos_.z	= 0.0f;
	tmpvert[3].uv_.x	= tmpvert[0].uv_.x;
	tmpvert[3].uv_.y	= tmpvert[2].uv_.y;
	tmpvert[3].uv2_		= vert[3].uv2_;
	tmpvert[3].colour_	= colour_;

	//adjust so everything looks nice and crisp
	for ( int i=0; i<4; i++ )
	{
		tmpvert[i].pos_.x += clipOffsetX;
		tmpvert[i].pos_.y += clipOffsetY;
	}

	// Return the tmp vertices as six, ordered, ready-to-render vertices
	vert[0] = tmpvert[ 0 ];
	vert[1] = tmpvert[ 1 ];
	vert[2] = tmpvert[ 3 ];
	vert[3] = tmpvert[ 2 ];
	vert[4] = tmpvert[ 3 ];
	vert[5] = tmpvert[ 1 ];

	return clipWidth;
}


Moo::BaseTexturePtr Font::pTexture()
{
	return metrics_.cache().pTexture();
}


//-----------------------------------------------------------------------------
//Section : CachedFont
//-----------------------------------------------------------------------------


//Constructor.
CachedFont::CachedFont( FontMetrics& fm ):
	Font( fm ),
	glyphRefHolder_( new GlyphReferenceHolder )
{
	BW_GUARD;
}


//Destructor.
CachedFont::~CachedFont()
{
	BW_GUARD;
	bw_safe_delete(glyphRefHolder_);
}


/**
 *	This method overrides Font::makeCharacter, and adds a reference to each
 *	character made by the cached font.  If you are using a cached font, and
 *	are going to replace an existing string with a new string, then release the
 *	references before making new characters.
 *
 *	@param	vert		Pointer to six vertices to be filled.
 *	@param	c			The character to draw into the vertex array.
 *	@param	pos			The clip position to make the character at.
 *
 *	@return	width		The width of the character, in clip coordinates
 *						This includes the effects margin.
 */
float CachedFont::makeCharacter( Moo::VertexXYZDUV2* vert, wchar_t c, const Vector2& pos )
{
	BW_GUARD;
	glyphRefHolder_->incRef(c);
	metrics_.cache().refCounts().incRef(c);
	return Font::makeCharacter( vert, c, pos );
}


/**
 *	This method releases currently held references.
 */
void CachedFont::releaseRefs()
{
	BW_GUARD;
	metrics_.cache().refCounts().releaseRefs( *glyphRefHolder_ );
	glyphRefHolder_->reset();
}

BW_END_NAMESPACE

// font.cpp
