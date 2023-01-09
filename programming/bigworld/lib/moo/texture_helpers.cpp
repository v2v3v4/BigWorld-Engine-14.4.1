#include "pch.hpp"
#include "texture_helpers.hpp"

BW_BEGIN_NAMESPACE

namespace TextureHelpers
{

const char *textureExts[] = 
{ 
	// dds needs to be the first extension in this list
	"dds", "bmp", "tga", "jpg", "png", "hdr", "pfm", "dib", "ppm", NULL
};


BW::StringRef copyNormalisedTextureName( const BW::StringRef & str,
							   char * outPtr, size_t maxSize )
{
	BW_GUARD;

	MF_ASSERT( outPtr && maxSize );
	MF_ASSERT( str.size() < maxSize );
	size_t size = std::min( str.size(), maxSize - 1 );
	// remove slash at the start of the name
	size_t start = (!str.empty() && str[0] == '/') ? 1 : 0;
	for (size_t i = 0; start + i < size; ++i)
	{
		const char ch = str[start + i];
		if (ch == '\\')
		{
			outPtr[i] = '/';
		}
		else
		{
			outPtr[i] = ch;
		}
	}
	outPtr[size] = 0;
	return BW::StringRef( outPtr, size );
}


BW::StringRef copyBaseNameAndExtension( const BW::StringRef & baseName,
	const BW::StringRef & ext, char * outPtr, size_t maxSize )
{
	BW_GUARD;

	MF_ASSERT( outPtr && maxSize );
	MF_ASSERT( !baseName.empty() );
	MF_ASSERT( !ext.empty() && ext[0] != '.' );
	MF_ASSERT( baseName.size() + 1 + ext.size() < maxSize );
	bw_str_copy( outPtr, maxSize, baseName );
	bw_str_append( outPtr, maxSize, ".", 1 );
	bw_str_append( outPtr, maxSize, ext );
	return outPtr;
}

} // namespace TextureHelpers

BW_END_NAMESPACE