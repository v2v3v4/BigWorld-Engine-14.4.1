#ifndef MOO_TEXTURESTAGE_HPP
#define MOO_TEXTURESTAGE_HPP


#include "cstdmf/stdmf.hpp"
#include "base_texture.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements a utility wrapper for the
 *	direct3D texture stage concept.
 *
 *	TextureStage objects are most commonly used in
 *	conjunction with Moo::Material objects.
 */
class TextureStage
{
public:
    TextureStage();
    ~TextureStage();


	enum ColourOperation
	{
		DISABLE = 1,
		SELECTARG1,
		SELECTARG2,
		MODULATE,
		MODULATE2X,
		MODULATE4X,
		ADD,
		ADDSIGNED,
		ADDSIGNED2X,
		SUBTRACT,
		ADDSMOOTH,
		BLENDDIFFUSEALPHA,
		BLENDTEXTUREALPHA,
		BLENDFACTORALPHA,
		BLENDTEXTUREALPHAPM,
		BLENDCURRENTALPHA,
		PREMODULATE,
		MODULATEALPHA_ADDCOLOR,
		MODULATECOLOR_ADDALPHA,
		MODULATEINVALPHA_ADDCOLOR,
		MODULATEINVCOLOR_ADDALPHA,
		BUMPENVMAP,
		BUMPENVMAPLUMINANCE,
		DOTPRODUCT3,
		MULTIPLYADD,
		LERP,
		LAST_COLOROP = 0x7FFFFFFF
   	};

	enum FilterType
	{
		POINT=1,          // valid for mag&min filters
		LINEAR,			  // valid for mag&min filters
		ANISOTROPIC,      // valid for mag&min filters
		FLATCUBIC,		  // valid for mag filter
		GAUSSIANCUBIC,    // valid for mag filter
		LAST_FILTER = 0x7FFFFFFF
	};


	enum ColourArgument
	{
		CURRENT=0,
		DIFFUSE,
		TEXTURE,
		TEXTURE_FACTOR,
		TEXTURE_ALPHA,
		TEXTURE_INVERSE,
		TEXTURE_ALPHA_INVERSE,
		DIFFUSE_ALPHA,
		DIFFUSE_INVERSE,
		DIFFUSE_ALPHA_INVERSE,
		LAST_COLORARG = 0x7FFFFFFF
	};

	enum WrapMode
	{
		REPEAT=1,
		MIRROR,
		CLAMP,
		LAST_REPEAT_TYPE = 0x7FFFFFFF
	};

	FilterType minFilter() const;
	void minFilter( FilterType filterType );
	FilterType magFilter() const;
	void magFilter( FilterType filterType );

	ColourOperation colourOperation() const;
	void colourOperation( ColourOperation operation );
	void colourOperation( ColourOperation operation, ColourArgument argument1, ColourArgument argument2 );
	void colourOperation( ColourOperation operation, ColourArgument argument1, ColourArgument argument2, ColourArgument argument3 );
	ColourArgument colourArgument1() const;
	void colourArgument1( ColourArgument argument );
	ColourArgument colourArgument2() const;
	void colourArgument2( ColourArgument argument );
	ColourArgument colourArgument3() const;
	void colourArgument3( ColourArgument argument );

	ColourOperation alphaOperation() const;
	void alphaOperation( ColourOperation operation );
	void alphaOperation( ColourOperation operation, ColourArgument argument1, ColourArgument argument2 );
	void alphaOperation( ColourOperation operation, ColourArgument argument1, ColourArgument argument2, ColourArgument argument3 );
	ColourArgument alphaArgument1() const;
	void alphaArgument1( ColourArgument argument );
	ColourArgument alphaArgument2() const;
	void alphaArgument2(ColourArgument argument );
	ColourArgument alphaArgument3() const;
	void alphaArgument3(ColourArgument argument );

	uint32 textureCoordinateIndex() const;
	void textureCoordinateIndex( uint32 i);

	WrapMode textureWrapMode() const;
	void textureWrapMode( WrapMode wm );

	bool useMipMapping() const;
	void useMipMapping( bool enable );

	BaseTexturePtr pTexture() const;
	void pTexture( BaseTexturePtr pTexture );

private:

	FilterType minFilter_;
	FilterType magFilter_;
	ColourOperation colourOperation_;
	ColourArgument colourArgument1_;
	ColourArgument colourArgument2_;
	ColourArgument colourArgument3_;
	ColourOperation alphaOperation_;
	ColourArgument alphaArgument1_;
	ColourArgument alphaArgument2_;
	ColourArgument alphaArgument3_;
	uint32 textureCoordinateIndex_;
	WrapMode wrapMode_;
	bool useMipMapping_;

	BaseTexturePtr	pTexture_;

	friend bool operator == (const TextureStage& ts1, const TextureStage& ts2 );
};

} // namespace Moo

#ifdef CODE_INLINE
    #include "texturestage.ipp"
#endif

BW_END_NAMESPACE

#endif // MOO_TEXTURESTAGE_HPP
