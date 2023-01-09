#ifndef TEXTURE_LOADER_HPP
#define TEXTURE_LOADER_HPP

#include "moo/com_object_wrap.hpp"
#include "moo/moo_dx.hpp"
#include "dds.h"

BW_BEGIN_NAMESPACE

namespace DDSHelper
{
	D3DFORMAT getFormat( const DDS_PIXELFORMAT & ddspf );
	void getSurfaceInfo( uint32 width, uint32 height, D3DFORMAT fmt, 
		uint32 * pNumBytes, uint32 * pRowBytes, uint32 * pNumRows );
	bool isDXTFormat( D3DFORMAT fmt );
	bool isFlatTexture( const DDS_HEADER & header );
}

namespace Moo
{

class TextureLoader
{
public:
	enum	SourceMipmapsType
	{
		SOURCE_NO_MIPMAPS,
		SOURCE_HORIZONTAL_MIPMAPS,
		SOURCE_VERTICAL_MIPMAPS,
	};

	struct InputOptions
	{
		InputOptions();
		void				reset();

		uint				numMipsToLoad_;		// default 0 means load all mips
		uint				mipSkip_;			// default 0 means do not skip
		D3DPOOL				pool_;				// default D3DPOOL_MANAGED
		D3DFORMAT			format_;			// default D3DFMT_UNKNOWN
		DWORD				mipFilter_;
		SourceMipmapsType	sourceMipsType_;	// default SOURCE_NO_MIPMAPS
		bool				upscaleToNextPow2_; // default true
	};

	struct	Output
	{
		Output();
		void				reset();
		// texture width or 0 if loading failed
		uint				width_;			
		// texture height or 0 if loading failed
		uint				height_;			
		// texture format or D3DFMT_UNKNOWN if loading failed
		D3DFORMAT			format_;			
		// only one of those will be valid, another will always be NULL
		// both might be NULL if texture loading has failed
		ComObjectWrap< DX::Texture >		texture2D_;
		ComObjectWrap< DX::CubeTexture >	cubeTexture_;
	};

	TextureLoader();

	// load texture from disk
	bool			loadTextureFromFile(
						const BW::string& resourceId, 
						const InputOptions& input,
						const BW::string& allocator = "texture/unknown managed texture" );
	// load texture from file in memory
	bool			loadTextureFromMemory(
						const void* data, const uint length, 
						const InputOptions& input,
						const BW::string& allocator = "texture/unknown managed texture" );
	// access output data
	const Output&	output() const			{ return output_; }

	static size_t	calcDDSSize( D3DFORMAT format, uint width, uint height, uint numMips );

private:
	void	loadCubeTextureFromMemory(
						const void* data, const uint length, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator );

	void	loadTexture2DFromMemory(
						const void* data, const uint length, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator );

	// loader output
	Output				output_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TEXTURE_LOADER_HPP
