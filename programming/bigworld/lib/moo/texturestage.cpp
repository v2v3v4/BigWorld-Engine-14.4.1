#include "pch.hpp"

#include "texturestage.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
    #include "texturestage.ipp"
#endif

namespace Moo
{

TextureStage::TextureStage()
:minFilter_(LINEAR),
 magFilter_(LINEAR),
 colourOperation_(DISABLE),
 colourArgument1_(TEXTURE),
 colourArgument2_(DIFFUSE),
 colourArgument3_(CURRENT),
 alphaOperation_(DISABLE),
 alphaArgument1_(TEXTURE),
 alphaArgument2_(DIFFUSE),
 alphaArgument3_(CURRENT),
 textureCoordinateIndex_(0),
 wrapMode_( REPEAT ),
 useMipMapping_( true )
{

}



TextureStage::~TextureStage()
{
}


bool operator == (const TextureStage& ts1, const TextureStage& ts2)
{
	if ((ts1.minFilter_					==	ts2.minFilter_) &&
		(ts1.magFilter_					==	ts2.magFilter_) &&
		(ts1.colourOperation_			==	ts2.colourOperation_) &&
		(ts1.colourArgument1_			==	ts2.colourArgument1_) &&
		(ts1.colourArgument2_			==	ts2.colourArgument2_) &&
		(ts1.colourArgument3_			==	ts2.colourArgument3_) &&
		(ts1.alphaOperation_			==	ts2.alphaOperation_) &&
		(ts1.alphaArgument1_			==	ts2.alphaArgument1_) &&
		(ts1.alphaArgument2_			==	ts2.alphaArgument2_) &&
		(ts1.alphaArgument3_			==	ts2.alphaArgument3_) &&
		(ts1.textureCoordinateIndex_	==	ts2.textureCoordinateIndex_) &&
		(ts1.wrapMode_					==	ts2.wrapMode_) &&
		(ts1.useMipMapping_				==	ts2.useMipMapping_) &&
		(ts1.pTexture_					==	ts2.pTexture_))
	{
		return true;
	}
	return false;
}

} // namespace Moo

BW_END_NAMESPACE

// texturestage.cpp
