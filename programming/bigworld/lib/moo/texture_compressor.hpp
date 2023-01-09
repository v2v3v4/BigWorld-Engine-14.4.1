#ifndef MOO_TEXTURE_COMPRESSOR_HPP
#define MOO_TEXTURE_COMPRESSOR_HPP

/**
 *	This class has the ability to compress a texture to any given D3DFORMAT.
 *	This class is typically used with limited scope.
 */
#include "com_object_wrap.hpp"
#include "moo_dx.hpp"
#include "resmgr/binary_block.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

class TextureDetailLevel;

class TextureCompressor
{
public:
	TextureCompressor( DX::Texture*	srcTexture );
	TextureCompressor( const BW::StringRef&	srcTextureName );

	bool save( const BW::StringRef & dstFilename, D3DFORMAT format,
				uint numMips = 0, uint maxExtent = 0 );
	bool save( const BW::StringRef & dstFilename, 
				const SmartPointer< TextureDetailLevel >& dstInfo,
				const bool bCompressed );
	bool stow( DataSectionPtr pSection, const BW::StringRef & childTag,
				D3DFORMAT format, uint numMips = 0, uint maxExtent = 0);
	BinaryPtr compressToMemory( D3DFORMAT format, uint numMips,
								uint maxExtent = 0, D3DXIMAGE_FILEFORMAT imgFormat = D3DXIFF_DDS );
	BinaryPtr compressToMemory( const SmartPointer< TextureDetailLevel >& dstInfo,
		const bool bCompressed );

	ComObjectWrap<DX::Texture> compress( D3DFORMAT format,
					uint numMips = 0, uint maxExtent = 0 ) const;
	void compress( ComObjectWrap<DX::Texture>& dstTexture );

	static D3DXIMAGE_FILEFORMAT getD3DImageFileFormat( const BW::StringRef& fileName,
													D3DXIMAGE_FILEFORMAT defaultVal );

private:
	TextureCompressor( const TextureCompressor& );
	TextureCompressor& operator=( const TextureCompressor& );
	ComObjectWrap<DX::Texture>	srcTexture_;
	BW::string					srcTextureName_;
};

} // namespace Moo

#ifdef CODE_INLINE
#include "texture_compressor.ipp"
#endif

BW_END_NAMESPACE

#endif // MOO_TEXTURE_COMPRESSOR_HPP
