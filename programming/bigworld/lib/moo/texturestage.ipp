// texturestage.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif


namespace Moo
{


INLINE TextureStage::FilterType TextureStage::minFilter() const
{
	return minFilter_;
}

INLINE void TextureStage::minFilter( TextureStage::FilterType filterType )
{
	minFilter_=filterType;
}


INLINE TextureStage::FilterType TextureStage::magFilter() const
{
	return magFilter_;
}

INLINE void TextureStage::magFilter( TextureStage::FilterType filterType )
{
	magFilter_=filterType;
}


INLINE TextureStage::ColourOperation TextureStage::colourOperation() const
{
	return colourOperation_;
}

INLINE void TextureStage::colourOperation( TextureStage::ColourOperation operation )
{
	colourOperation_=operation;
}

INLINE void TextureStage::colourOperation( TextureStage::ColourOperation operation, TextureStage::ColourArgument argument1, TextureStage::ColourArgument argument2 )
{
	colourOperation_=operation;
	colourArgument1_=argument1;
	colourArgument2_=argument2;
}

INLINE void TextureStage::colourOperation( TextureStage::ColourOperation operation, TextureStage::ColourArgument argument1, TextureStage::ColourArgument argument2, TextureStage::ColourArgument argument3 )
{
	colourOperation_=operation;
	colourArgument1_=argument1;
	colourArgument2_=argument2;
	colourArgument3_=argument3;	
}

INLINE TextureStage::ColourArgument TextureStage::colourArgument1() const
{
	return colourArgument1_;
}

INLINE void TextureStage::colourArgument1( TextureStage::ColourArgument argument )
{
	colourArgument1_=argument;
}


INLINE TextureStage::ColourArgument TextureStage::colourArgument2() const
{
	return colourArgument2_;
}

INLINE void TextureStage::colourArgument2( TextureStage::ColourArgument argument )
{
	colourArgument2_=argument;
}

INLINE TextureStage::ColourArgument TextureStage::colourArgument3() const
{
	return colourArgument3_;
}

INLINE void TextureStage::colourArgument3( TextureStage::ColourArgument argument )
{
	colourArgument3_=argument;
}


INLINE TextureStage::ColourOperation TextureStage::alphaOperation() const
{
	return alphaOperation_;
}

INLINE void TextureStage::alphaOperation( TextureStage::ColourOperation operation )
{
	alphaOperation_=operation;
}

INLINE void TextureStage::alphaOperation( TextureStage::ColourOperation operation, TextureStage::ColourArgument argument1, TextureStage::ColourArgument argument2 )
{
	alphaOperation_=operation;
	alphaArgument1_=argument1;
	alphaArgument2_=argument2;
}

INLINE void TextureStage::alphaOperation( TextureStage::ColourOperation operation, TextureStage::ColourArgument argument1, TextureStage::ColourArgument argument2, TextureStage::ColourArgument argument3 )
{
	alphaOperation_=operation;
	alphaArgument1_=argument1;
	alphaArgument2_=argument2;
	alphaArgument3_=argument3;	
}


INLINE TextureStage::ColourArgument TextureStage::alphaArgument1() const
{
	return alphaArgument1_;
}

INLINE void TextureStage::alphaArgument1( TextureStage::ColourArgument argument )
{
	alphaArgument1_=argument;
}


INLINE TextureStage::ColourArgument TextureStage::alphaArgument2() const
{
	return alphaArgument2_;
}

INLINE void TextureStage::alphaArgument2(TextureStage::ColourArgument argument )
{
	alphaArgument2_=argument;
}

INLINE TextureStage::ColourArgument TextureStage::alphaArgument3() const
{
	return alphaArgument3_;
}

INLINE void TextureStage::alphaArgument3(TextureStage::ColourArgument argument )
{
	alphaArgument3_=argument;
}


INLINE uint32 TextureStage::textureCoordinateIndex() const
{
	return textureCoordinateIndex_;
}

INLINE void TextureStage::textureCoordinateIndex( uint32 i)
{
	textureCoordinateIndex_=i;
}


INLINE TextureStage::WrapMode TextureStage::textureWrapMode() const
{
	return wrapMode_;
}

INLINE void TextureStage::textureWrapMode( TextureStage::WrapMode wm )
{
	wrapMode_ = wm;
}


INLINE bool TextureStage::useMipMapping() const
{
	return useMipMapping_;
}

INLINE void TextureStage::useMipMapping( bool enable )
{
	useMipMapping_ = enable;
}


INLINE BaseTexturePtr TextureStage::pTexture() const
{
	return pTexture_;
}

INLINE void TextureStage::pTexture( BaseTexturePtr pTexture )
{
	pTexture_ = pTexture;
}


}

// texturestage.ipp