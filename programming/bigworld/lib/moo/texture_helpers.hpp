#ifndef TEXTURE_HELPERS_HPP
#define TEXTURE_HELPERS_HPP

BW_BEGIN_NAMESPACE

namespace TextureHelpers
{

extern const char *textureExts[];
BW::StringRef copyNormalisedTextureName( const BW::StringRef & str,
	char * outPtr, size_t maxSize );
BW::StringRef copyBaseNameAndExtension( const BW::StringRef & baseName,
	const BW::StringRef & ext, char * outPtr, size_t maxSize );

} // namespace TextureHelpers

BW_END_NAMESPACE

#endif // TEXTURE_HELPERS_HPP