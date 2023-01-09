#ifndef LEGACY_MATERIAL_DEFS_HPP_
#define LEGACY_MATERIAL_DEFS_HPP_

#include "texturestage.hpp"


BW_BEGIN_NAMESPACE

/**
 * Ye olde material definitions (fixed function tex/blend operations and such)
 */
class LegacyMaterialDefs
{
public:

	enum BlendType
	{
		ZERO=0,
		ONE,
		SRC_COLOUR,
		INV_SRC_COLOUR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_ALPHA,
		INV_DEST_ALPHA,
		DEST_COLOUR,
		INV_DEST_COLOUR,
		SRC_ALPHA_SAT,
		BOTH_SRC_ALPHA,
		BOTH_INV_SRC_ALPHA,
		LAST_BLENDTYPE = 0x7FFFFFFF
	}BlendType;


	enum ColourOperation
	{
		DISABLE = 0,
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

	static Moo::TextureStage::ColourOperation translateColourOp( ColourOperation lop )
	{
		using Moo::TextureStage;
		switch (lop)
		{
			case DISABLE:					return TextureStage::DISABLE;
			case SELECTARG1:				return TextureStage::SELECTARG1;
			case SELECTARG2:				return TextureStage::SELECTARG2;
			case MODULATE:					return TextureStage::MODULATE;
			case MODULATE2X:				return TextureStage::MODULATE2X;
			case MODULATE4X:				return TextureStage::MODULATE4X;
			case ADD:						return TextureStage::ADD;
			case ADDSIGNED:					return TextureStage::ADDSIGNED;
			case ADDSIGNED2X:				return TextureStage::ADDSIGNED2X;
			case SUBTRACT:					return TextureStage::SUBTRACT;
			case ADDSMOOTH:					return TextureStage::ADDSMOOTH;
			case BLENDDIFFUSEALPHA:			return TextureStage::BLENDDIFFUSEALPHA;
			case BLENDTEXTUREALPHA:			return TextureStage::BLENDTEXTUREALPHA;
			case BLENDFACTORALPHA:			return TextureStage::BLENDFACTORALPHA;
			case BLENDTEXTUREALPHAPM:		return TextureStage::BLENDTEXTUREALPHAPM;
			case BLENDCURRENTALPHA:			return TextureStage::BLENDCURRENTALPHA;
			case PREMODULATE:				return TextureStage::PREMODULATE;
			case MODULATEALPHA_ADDCOLOR:	return TextureStage::MODULATEALPHA_ADDCOLOR;
			case MODULATECOLOR_ADDALPHA:	return TextureStage::MODULATECOLOR_ADDALPHA;
			case MODULATEINVALPHA_ADDCOLOR:	return TextureStage::MODULATEINVALPHA_ADDCOLOR;
			case MODULATEINVCOLOR_ADDALPHA:	return TextureStage::MODULATEINVCOLOR_ADDALPHA;
			case BUMPENVMAP:				return TextureStage::BUMPENVMAP;
			case BUMPENVMAPLUMINANCE:		return TextureStage::BUMPENVMAPLUMINANCE;
			case DOTPRODUCT3:				return TextureStage::DOTPRODUCT3;
			case MULTIPLYADD:				return TextureStage::MULTIPLYADD;
			case LERP:						return TextureStage::LERP;
			case LAST_COLOROP:				break;
		}
		return TextureStage::SELECTARG1;
	}

	enum FilterType
	{
		POINT=0,          // valid for mag&min filters
		ANISOTROPIC,      // valid for mag&min filters
		LINEAR,			  // valid for mag&min filters
		FLATCUBIC,		  // valid for mag filter
		GAUSSIANCUBIC,    // valid for mag filter
		LAST_FILTER = 0x7FFFFFFF
	};

	static Moo::TextureStage::FilterType translateFilterType( FilterType ft )
	{
		using Moo::TextureStage;
		switch (ft)
		{
			case POINT:			return TextureStage::POINT;
			case LINEAR:		return TextureStage::LINEAR;
			case ANISOTROPIC:	return TextureStage::ANISOTROPIC;
			case FLATCUBIC:		return TextureStage::FLATCUBIC;
			case GAUSSIANCUBIC:	return TextureStage::GAUSSIANCUBIC;
			case LAST_FILTER:	break;
		}
		return TextureStage::LINEAR;
	}


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
		REPEAT=0,
		MIRROR,
		CLAMP,
		LAST_REPEAT_TYPE = 0x7FFFFFFF
	};
};

BW_END_NAMESPACE

#endif
