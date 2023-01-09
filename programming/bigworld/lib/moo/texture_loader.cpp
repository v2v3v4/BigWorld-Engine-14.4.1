#include "pch.hpp"
#include "dds.h"
#include "texture_loader.hpp"
#include "resmgr/file_streamer.hpp"
#include "resmgr/file_system.hpp"
#include "render_context.hpp"
#include "moo/texture_lock_wrapper.hpp"
#include "resmgr/multi_file_system.hpp"


BW_BEGIN_NAMESPACE

/**
 *	The DDSHelper namespace contains helper functions for dds loading
 */
namespace DDSHelper
{
	const D3DFORMAT FOURCC_ATI1N = ((D3DFORMAT)MAKEFOURCC('A', 'T', 'I', '1'));
	const D3DFORMAT FOURCC_ATI2N = ((D3DFORMAT)MAKEFOURCC('A', 'T', 'I', '2'));

	bool operator == (const DDS_PIXELFORMAT& pf1, const DDS_PIXELFORMAT& pf2 )
	{
		if (pf1.dwFlags == DDS_FOURCC && pf2.dwFlags == DDS_FOURCC)
			return pf1.dwFourCC == pf2.dwFourCC;
		return pf1.dwFlags == pf2.dwFlags &&
			pf1.dwRGBBitCount == pf2.dwRGBBitCount &&
			pf1.dwRBitMask == pf2.dwRBitMask &&
			pf1.dwGBitMask == pf2.dwGBitMask &&
			pf1.dwBBitMask == pf2.dwBBitMask &&
			pf1.dwABitMask == pf2.dwABitMask;
	}

	bool isFlatTexture( const DDS_HEADER& header )
	{
		return header.dwCubemapFlags == 0 &&
			(header.dwHeaderFlags & DDS_HEADER_FLAGS_VOLUME) == 0;
	}

	// Helper macro for checking whether the texture is of a
	// supported non-fourcc format
	#define CHECK_UNCOMPRESSED_FORMAT(ddsp, FORMAT) \
		else if ( ddspf == DDSPF_##FORMAT ) \
		{\
			return D3DFMT_##FORMAT;\
		}

	D3DFORMAT getFormat( const DDS_PIXELFORMAT& ddspf )
	{
		if (ddspf.dwFlags & DDS_FOURCC)
		{
			return D3DFORMAT(ddspf.dwFourCC);
		}
		CHECK_UNCOMPRESSED_FORMAT(ddspf, A8R8G8B8)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, A1R5G5B5)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, A4R4G4B4)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, R8G8B8)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, R5G6B5)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, A8)
		CHECK_UNCOMPRESSED_FORMAT(ddspf, L8)

		return D3DFMT_UNKNOWN;
	}

	uint32 bitsPerPixel( D3DFORMAT fmt )
	{
		uint32 fmtU = ( uint32 )fmt;
		switch( fmtU )
		{
			case D3DFMT_A32B32G32R32F:
				return 128;

			case D3DFMT_A16B16G16R16:
			case D3DFMT_Q16W16V16U16:
			case D3DFMT_A16B16G16R16F:
			case D3DFMT_G32R32F:
				return 64;

			case D3DFMT_A8R8G8B8:
			case D3DFMT_X8R8G8B8:
			case D3DFMT_A2B10G10R10:
			case D3DFMT_A8B8G8R8:
			case D3DFMT_X8B8G8R8:
			case D3DFMT_G16R16:
			case D3DFMT_A2R10G10B10:
			case D3DFMT_Q8W8V8U8:
			case D3DFMT_V16U16:
			case D3DFMT_X8L8V8U8:
			case D3DFMT_A2W10V10U10:
			case D3DFMT_D32:
			case D3DFMT_D24S8:
			case D3DFMT_D24X8:
			case D3DFMT_D24X4S4:
			case D3DFMT_D32F_LOCKABLE:
			case D3DFMT_D24FS8:
			case D3DFMT_INDEX32:
			case D3DFMT_G16R16F:
			case D3DFMT_R32F:
				return 32;

			case D3DFMT_R8G8B8:
				return 24;

			case D3DFMT_A4R4G4B4:
			case D3DFMT_X4R4G4B4:
			case D3DFMT_R5G6B5:
			case D3DFMT_L16:
			case D3DFMT_A8L8:
			case D3DFMT_X1R5G5B5:
			case D3DFMT_A1R5G5B5:
			case D3DFMT_A8R3G3B2:
			case D3DFMT_V8U8:
			case D3DFMT_CxV8U8:
			case D3DFMT_L6V5U5:
			case D3DFMT_G8R8_G8B8:
			case D3DFMT_R8G8_B8G8:
			case D3DFMT_D16_LOCKABLE:
			case D3DFMT_D15S1:
			case D3DFMT_D16:
			case D3DFMT_INDEX16:
			case D3DFMT_R16F:
			case D3DFMT_YUY2:
				return 16;

			case D3DFMT_R3G3B2:
			case D3DFMT_A8:
			case D3DFMT_A8P8:
			case D3DFMT_P8:
			case D3DFMT_L8:
			case D3DFMT_A4L4:
				return 8;

			case D3DFMT_DXT1:
				return 4;

			case D3DFMT_DXT2:
			case D3DFMT_DXT3:
			case D3DFMT_DXT4:
			case D3DFMT_DXT5:
				return  8;

				// From DX docs, reference/d3d/enums/d3dformat.asp
				// (note how it says that D3DFMT_R8G8_B8G8 is "A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, and so on)")
			case D3DFMT_UYVY:
				return 16;

			// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directshow/htm/directxvideoaccelerationdxvavideosubtypes.asp
			case MAKEFOURCC( 'A', 'I', '4', '4' ):
			case MAKEFOURCC( 'I', 'A', '4', '4' ):
				return 8;

			case MAKEFOURCC( 'Y', 'V', '1', '2' ):
				return 12;

			case FOURCC_ATI1N:
				return 4;
			case FOURCC_ATI2N:
				return 8;

	#if !defined(D3D_DISABLE_9EX)
			case D3DFMT_D32_LOCKABLE:
				return 32;

			case D3DFMT_S8_LOCKABLE:
				return 8;

			case D3DFMT_A1:
				return 1;
	#endif // !D3D_DISABLE_9EX

			default:
				MF_ASSERT( 0 ); // unhandled format
				return 0;
		}
	}

	bool isDXTFormat( D3DFORMAT fmt )
	{
		return fmt == D3DFMT_DXT1 ||
			fmt == D3DFMT_DXT2 ||
			fmt == D3DFMT_DXT3 ||
			fmt == D3DFMT_DXT4 ||
			fmt == D3DFMT_DXT5;
	}

	void getSurfaceInfo( uint32 width, uint32 height, D3DFORMAT fmt, uint32* pNumBytes, uint32* pRowBytes, uint32* pNumRows )
	{
		uint32 numBytes = 0;
		uint32 rowBytes = 0;
		uint32 numRows = 0;

		// From the DXSDK docs:
		//
		//     When computing DXTn compressed sizes for non-square textures, the 
		//     following formula should be used at each mipmap level:
		//
		//         max(1, width / 4) x max(1, height / 4) x 8(DXT1) or 16(DXT2-5)
		//
		//     The pitch for DXTn formats is different from what was returned in 
		//     Microsoft DirectX 7.0. It now refers the pitch of a row of blocks. 
		//     For example, if you have a width of 16, then you will have a pitch 
		//     of four blocks (4*8 for DXT1, 4*16 for DXT2-5.)"

		bool blockCompressed =	isDXTFormat(fmt) ||
								fmt == FOURCC_ATI1N || fmt == FOURCC_ATI2N;
		// handle block compressed formats
		// block size is 4 x 4
		if (blockCompressed )
		{
			int numBlocksWide = 0;
			if( width > 0 )
				numBlocksWide = std::max<uint32>( 1, width / 4 );
			int numBlocksHigh = 0;
			if( height > 0 )
				numBlocksHigh = std::max<uint32>( 1, height / 4 );

			uint32 numBytesPerBlock = (fmt == D3DFMT_DXT1 || fmt == FOURCC_ATI1N) ? 8 : 16;
			rowBytes = numBlocksWide * numBytesPerBlock;
			numRows = numBlocksHigh;
		}
		else
		{
			uint32 bpp = bitsPerPixel( fmt );
			rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
			numRows = height;
		}
		numBytes = rowBytes * numRows;
		if( pNumBytes != NULL )
			*pNumBytes = numBytes;
		if( pRowBytes != NULL )
			*pRowBytes = rowBytes;
		if( pNumRows != NULL )
			*pNumRows = numRows;
	}

	bool isPow2( DWORD value )
	{
		DWORD mask = 1;

		while (mask != value)
		{
			if ((mask & ~value) == 0)
				return true;
			mask <<= 1;
		}
		return false;
	}


	bool isPow2( const DDS_HEADER& header )
	{
		return isPow2( header.dwWidth ) && isPow2( header.dwHeight );
	}

	/**
	 *	This method tries to load the texture directly as a dds without using d3dx
	 *	@return Texture object if successfull
	 */
	ComObjectWrap< DX::Texture > loadTextureAsDDS(const BW::string &resourceId, D3DPOOL pool, uint32 mipSkip)
	{
		FileStreamerPtr pFileStreamer = 
			BWResource::instance().fileSystem()->streamFile( resourceId );
		if (!pFileStreamer)
		{
			return NULL;
		}
		
		// Read DDS header.
		DWORD magic;
		pFileStreamer->read( sizeof( magic ), &magic );
		if (magic != DDS_MAGIC)
		{
			// this is not a dds file or it has a corrupted signature
			// return unknown so calling code can fall back to d3dx
			return NULL;
		}

		DDS_HEADER header;
		pFileStreamer->read( sizeof( header ), &header );

		const Moo::DeviceInfo& di = Moo::rc().deviceInfo( Moo::rc().deviceIndex() );
		
		if (di.caps_.TextureCaps & D3DPTEXTURECAPS_POW2 &&
			!DDSHelper::isPow2(header))
		{
			// device doesn't support non power2 texture dimensions
			// return NULL so calling code can fall back to d3dx..
			return NULL;
		}

		// Volume textures and cubemaps are loaded in different code path.
		if ( !isFlatTexture(header) )
		{
			return NULL;
		}

		D3DFORMAT fmt = D3DFMT_UNKNOWN;
		fmt = DDSHelper::getFormat( header.ddspf );

		if (fmt == D3DFMT_UNKNOWN)
		{
			// unknown dds format,  fall back to d3dx
			ASSET_MSG( "Unsupported DDS format in texture %s\n", resourceId.c_str() );
			return NULL;
		}

		// double check device format before attempting to create a texture
		// with an unsupported format.
		if (!Moo::rc().supportsTextureFormat(fmt))
		{
			// TODO: Currently there is no supported fallback path for GPUs which do not support ATI1N or ATI2N
			// for ATI2n we need to do runtime conversion to ARGB8 and drop 1 more mip level to keep memory usage identical
			// for ATI1n we need to do runtime conversion to DXT1
			// According to  http://aras-p.info/texts/D3D9GPUHacks.html#3dc
			// ATI1N is supported on GeForce 8+, Radeon X1300+, Intel G45+.
			// ATI2N is supported on GeForce 7+,  Radeon 9500+, Intel G45+.
			// NVIDIA exposing it a while ago and Intel exposing it recently (drivers 15.17 or higher).

			ERROR_MSG("Video card driver failed to create %s texture "
				"because texture format is not supported.\n"
				"Try to update drivers for your video card.\n", 
				resourceId.c_str());
			return NULL;
		}

		// read dds data
		uint32 w = header.dwWidth;
		uint32 h = header.dwHeight;

		// Calculate the amount of data to skip in the file depending on the number of mipmaps
		// we want to skip
		uint32 fileSkip = 0;
		for (uint32 i = 0; i < mipSkip && i < header.dwMipMapCount; i++)
		{
			UINT skipBytes = 0;
			DDSHelper::getSurfaceInfo( w, h, fmt, &skipBytes, NULL, NULL );
			w = std::max<uint32>(1, w >> 1);
			h = std::max<uint32>(1, h >> 1);

			fileSkip += skipBytes;
		}

		if (fileSkip > 0)
		{
			pFileStreamer->skip( fileSkip );
		}

		uint32 nMips = mipSkip < header.dwMipMapCount ? header.dwMipMapCount - mipSkip : 1;

		/* Special case for hacked DX9 formats
		from http://aras-p.info/texts/D3D9GPUHacks.html#3dc

		Thing to keep in mind: when DX9 allocates the mip chain, they check if the format is a known compressed format
		and allocate the appropriate space for the smallest mip levels.
		
		For example, a 1x1 DXT1 compressed level actually takes up 8 bytes, as the block size is fixed at 4x4 texels.
		This is true for all block compressed formats.
		
		Now when using the hacked formats DX9 doesn't know it's a block compression format and
		will only allocate the number of bytes the mip would have taken, if it weren't compressed.
		
		For example a 1x1 ATI1n format will only have 1 byte allocated.
		What you need to do is to stop the mip chain before the size of the either dimension shrinks
		below the block dimensions otherwise you risk having memory corruption.
		*/
		if (fmt == FOURCC_ATI1N || fmt == FOURCC_ATI2N)
		{
			uint32 numTailMipsToDrop = nMips;
			uint32 tWidth = w;
			uint32 tHeight = h;
			// continue until we hit mip with width or hight less than 4 texels
			while (tWidth >= 4 && tHeight >= 4 && numTailMipsToDrop > 0)
			{
				numTailMipsToDrop--;
				tWidth /= 2;
				tHeight /= 2;
			}
			nMips -= numTailMipsToDrop;

			if (nMips == 0)
			{
				// we should only hit this branch if given ATI1N or ATI2N texture dimensions
				// are less than 4x4 bytes after we applied mipskip
				// if this is the case then we can't use direct dds loading
				// due to possible memory corruption issues which are described above
				// fall back to d3dx which is will throw an asset error
				return NULL;
			}
		}


		uint32 tWidth = w;
		uint32 tHeight = h;

		// Ensure texture dimensions for DXT formats are multiple of 4
		if (DDSHelper::isDXTFormat( fmt ))
		{
			tWidth = (tWidth + 3) & 0xfffffffc;
			tHeight = (tHeight + 3) & 0xfffffffc;
		}
		// create a d3d texture	
		ComObjectWrap< DX::Texture > tex = Moo::rc().createTexture( tWidth, tHeight,
														nMips, 0, fmt, pool, "DDSLOADER" );
	
		if (!tex.hasComObject())
		{
			// failed to create a texture, must be out of VA space
			return NULL;
		}		
		
		Moo::TextureLockWrapper texLock( tex );
		// Load up the mipmaps
		for (uint32 i = 0; i < nMips; i++)
		{
			UINT mipBytes = 0;
			UINT rowSize = 0;
			UINT rowCount = 0;
			D3DLOCKED_RECT lockedRect;

			DDSHelper::getSurfaceInfo( w, h, fmt, &mipBytes, &rowSize, &rowCount );
			
			texLock.lockRect( i, &lockedRect, NULL, 0 );

			bool bSuccessRead = true;
			if (lockedRect.Pitch * rowCount == mipBytes)
			{
				bSuccessRead &= ( mipBytes == pFileStreamer->read( mipBytes, lockedRect.pBits ) );
			}
			else
			{
				uint8* dest = (uint8*)lockedRect.pBits;
				for (uint32 j = 0; j < rowCount && bSuccessRead; j++)
				{
					bSuccessRead &= ( rowSize == pFileStreamer->read( rowSize, dest ) );
					dest += lockedRect.Pitch;
				}
			}

			texLock.unlockRect( i );

			w = std::max<uint32>( 1, w >> 1 );
			h = std::max<uint32>( 1, h >> 1 );

			if (!bSuccessRead)
			{
				// dds file is corrupted or just ended too early ?
				return NULL;
			}
		}

		// all good, return texture
		return tex;
	}
} // DDSHelper


//**  Texture loader
namespace Moo
{

TextureLoader::InputOptions::InputOptions()
{
	reset();
}

void TextureLoader::InputOptions::reset()
{
	numMipsToLoad_ = 0;
	mipSkip_ = 0;
	pool_ = D3DPOOL_MANAGED;
	format_ = D3DFMT_UNKNOWN;
	mipFilter_ = D3DX_FILTER_BOX | D3DX_FILTER_MIRROR;
	sourceMipsType_ = SOURCE_NO_MIPMAPS;
	upscaleToNextPow2_ = true;
}

TextureLoader::TextureLoader()
{
}


TextureLoader::Output::Output()
{
	reset();
}

void TextureLoader::Output::reset()
{
	width_ = 0;
	height_ = 0;
	format_ = D3DFMT_UNKNOWN;
	texture2D_ = NULL;
	cubeTexture_ = NULL;
}


/**
 *	This function loads texture from disk.
 *  It supports regular 2d textures and cubemaps.
 *  It uses direct dds loading and falls back to d3dx for
 *  non-dds textures and cubemaps.
 *
 *	@param	resourceId	name of the resource on disk
 *	@param	inputOptions	input options structure
 *	@param	outputOptions	output options structure
 *
 *	@return	true if loading is successfull and false otherwise
 */
bool	TextureLoader::loadTextureFromFile( 
						const BW::string& resourceId, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator )
{
	BinaryPtr bin = NULL;

	// check if d3d device is ready 
	// it might be NULL if something is trying to create a texture too early
	// or after shutdown
	if( Moo::rc().device() == NULL )
	{
		return false;
	}

	output_.reset();

	if (resourceId.substr( 0, 4 ) != "////")
	{	
		// try to load texture resouce as dds texture
		
		// TODO: add cubemap support to direct dds loading codepath

		// TODO: this codepath doesn't support following input options
		// uint				numMipsToLoad_;		// default 0 means load all mips
		// D3DFORMAT			format_;			// default D3DFMT_UNKNOWN
		// DWORD				mipFilter_;
		// SourceMipmapsType	sourceMipsType_;	// default SOURCE_NO_MIPMAPS
		// bool				upscaleToNextPow2_; // default true
		output_.texture2D_ = DDSHelper::loadTextureAsDDS( resourceId, input.pool_, input.mipSkip_ );
	}
	else
	{
		// WTF is this for ? passing binary texture data as a string ????
		bin = 
			new BinaryBlock
			(
				resourceId.data() + 4, 
				resourceId.length() - 4,
				"BinaryBlock/ManagedTexture"
			);
	}

	if (output_.texture2D_)
	{
		D3DSURFACE_DESC desc;
		output_.texture2D_->GetLevelDesc( 0, &desc );
		output_.format_ = desc.Format;
		output_.width_ = desc.Width;
		output_.height_ = desc.Height;
		output_.cubeTexture_ = NULL;
	}
	else if (!bin)
	{
		// we get here only if direct dds loading failed
		// or if we need to load cubemap
		bin = BWResource::instance().rootSection()->readBinary( resourceId );
	}
	if (bin)
	{
		this->loadTextureFromMemory( bin->data(), bin->len(),
												input, allocator );
	}

    const bool loadSuccess = (output_.texture2D_ || output_.cubeTexture_);
    if (!loadSuccess)
	{
		ASSET_MSG( "Can't open file %s\n", resourceId.c_str() );
	}
	return loadSuccess;
}

void	TextureLoader::loadTexture2DFromMemory(
						const void* data, const uint length, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator )
{
	// NOTE: this codepath doesn't support following input options
	//	SourceMipmapsType	sourceMipsType_;	// default SOURCE_NO_MIPMAPS
	//	bool				upscaleToNextPow2_; // default true

	MF_ASSERT( length > 4 );

	uint32 sizeFlag = D3DX_DEFAULT;
	if (!input.upscaleToNextPow2_)
	{
		sizeFlag = D3DX_DEFAULT_NONPOW2;
	}
	// 0 means load full mip chain
	uint32 mipLevels = input.numMipsToLoad_;
	if (mipLevels == 0)
	{
		mipLevels = D3DX_DEFAULT;
	}
	// default mip filter is D3DX_FILTER_BOX | D3DX_FILTER_MIRROR

	// currently the only way we can get here if we are loading non dds texture
	// this means we cannot use D3DX_SKIP_DDS_MIP_LEVELS because
	// it doesn't work for textures which don't have pre-generated miplevels
	// ( e.g. bmp, png, tga )
	// we gonna create a simple texture with one mip level in it, create a
	// second texture with
	// desired width, height and full mip chain and copy data from temp texture
	// into it
	// it's not very fast, but game should be loading non dds textures only if
	// conversion in progress
	// or if it's a special occasion.

	// Create the texture in system memory if we are using the Ex device
	// and need to copy data from the temp texture
	const bool doingMipSkip = (input.mipSkip_ > 0);
	D3DPOOL tempTexPool = input.pool_;
	if (Moo::rc().usingD3DDeviceEx() && doingMipSkip)
	{
		tempTexPool = D3DPOOL_SYSTEMMEM;
	}

	D3DFORMAT format = input.format_;
	DWORD* head = (DWORD*)data;
	if (head[0] == DDS_MAGIC && head[1] == 124 )
	{			
		DDS_HEADER* ddsHeader = (DDS_HEADER*)&head[1];
		D3DFORMAT fmt = DDSHelper::getFormat( ddsHeader->ddspf );
		if (fmt != D3DFMT_UNKNOWN)
		{
			format = fmt;
		}
	}

	ComObjectWrap< DX::Texture > tempTex =
		Moo::rc().createTextureFromFileInMemoryEx( 
		data, length,
		sizeFlag, sizeFlag, D3DX_DEFAULT , 0, format,
		tempTexPool,
		D3DX_FILTER_BOX | D3DX_FILTER_MIRROR, 
		input.mipFilter_,
		0,
		NULL,
		NULL
#if ENABLE_RESOURCE_COUNTERS
		, allocator.c_str()
#endif
		);

	if (!tempTex)
	{
		ERROR_MSG( "TextureLoader::loadTexture2DFromMemory: could not create "
			"temporary texture\n" );
		return;
	}

	DX::Surface* surface = NULL;
	if (FAILED( tempTex->GetSurfaceLevel( 0, &surface ) ))
	{
		ERROR_MSG( "TextureLoader::loadTexture2DFromMemory: failed to get"
			"surface\n" );
		return;
	}

	// Fill output data if everything is ok
	D3DSURFACE_DESC desc;
	surface->GetDesc( &desc );

	output_.width_ = std::max<uint>( desc.Width >> input.mipSkip_, 1 );
	output_.height_ = std::max<uint>( desc.Height >> input.mipSkip_, 1 );
	output_.format_ = desc.Format;

	surface->Release();
	surface = NULL;

	if (!doingMipSkip)
	{
		output_.texture2D_ = tempTex;
		return;
	}

	// Since loading non dds textures doesn't support mipskip we have
	// to do it manually
	// DirectX driver isn't really happy about very small textures encoded as DXT1,DXT3 or DXT5 textures
	// it might work or might crash. simple workaround would be to use A8R8G8B8 format in this case
	if (output_.width_ < 4 || output_.height_ < 4)
	{
		output_.format_ = D3DFMT_A8R8G8B8;
	}
	ComObjectWrap< DX::Texture > tex = Moo::rc().createTexture(
		output_.width_,
		output_.height_,
		input.numMipsToLoad_,
		0,
		output_.format_,
		input.pool_
#if ENABLE_RESOURCE_COUNTERS
		, allocator.c_str()
#endif
		);

	if (tex)
	{
		// Copy mip levels
		const uint srcMipLevel = input.mipSkip_;
		const uint numSrcLevels = tempTex->GetLevelCount();
		for (uint iLevel = input.mipSkip_; iLevel < numSrcLevels; ++iLevel)
		{
			DX::Surface* psurfSrc = NULL;
			DX::Surface* psurfDest = NULL;

			HRESULT hr;
			hr = tempTex->GetSurfaceLevel( iLevel, &psurfSrc );
			MF_ASSERT_DEV( SUCCEEDED( hr ) );

			const uint32 destLevel = iLevel - input.mipSkip_;
			hr = tex->GetSurfaceLevel( destLevel, &psurfDest );
			MF_ASSERT_DEV( SUCCEEDED( hr ) );

			hr = D3DXLoadSurfaceFromSurface(
				psurfDest, NULL, NULL,
				psurfSrc, NULL, NULL,
				D3DX_FILTER_NONE,
				0 );
			MF_ASSERT_DEV( SUCCEEDED( hr ) );

			psurfSrc->Release();
			psurfDest->Release();
		}

		// Set result to output
		output_.texture2D_ = tex;
	}
	else
	{
		ERROR_MSG( "TextureLoader::loadTexture2DFromMemory: could not create "
			"texture mips when loading texture from memory\n" );

		// Change from system memory back to default pool
		if (Moo::rc().usingD3DDeviceEx())
		{
			tempTex = NULL;
			tempTex = Moo::rc().createTextureFromFileInMemoryEx( 
				data, length,
				sizeFlag, sizeFlag, D3DX_DEFAULT , 0, input.format_,
				input.pool_,
				D3DX_FILTER_BOX | D3DX_FILTER_MIRROR, 
				input.mipFilter_,
				0,
				NULL,
				NULL
#if ENABLE_RESOURCE_COUNTERS
				, allocator.c_str()
#endif
				);
		}

		output_.texture2D_ = tempTex;
	}
}

void	TextureLoader::loadCubeTextureFromMemory(
						const void* data, const uint length, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator )
{
	// NOTE: this codepath doesn't support following input options
	//	uint				numMipsToLoad_;		// default 0 means load all mips
	//	uint				mipSkip_;			// default 0 means do not skip
	//	D3DFORMAT			format_;			// default D3DFMT_UNKNOWN
	//	SourceMipmapsType	sourceMipsType_;	// default SOURCE_NO_MIPMAPS
	//	bool				upscaleToNextPow2_; // default true

	MF_ASSERT( length > 4 );
	// Cubemaps do not use the mip skip functionality - it was causing problems.
	output_.cubeTexture_ = Moo::rc().createCubeTextureFromFileInMemoryEx( 
		data, length,
		D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, 
		input.pool_, D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 
		input.mipFilter_, 0, NULL, NULL
#if ENABLE_RESOURCE_COUNTERS
		, allocator.c_str()
#endif
		);

	if (output_.cubeTexture_)
	{
		// fill output data if everything is ok
		DX::Surface* surface = NULL;
		if( SUCCEEDED( output_.cubeTexture_->GetCubeMapSurface(
						D3DCUBEMAP_FACE_POSITIVE_X,  0, &surface ) ) )
		{
			D3DSURFACE_DESC desc;
			surface->GetDesc( &desc );

			output_.width_ = desc.Width;
			output_.height_ = desc.Height;
			output_.format_ = desc.Format;
			surface->Release();
			surface = NULL;
		}
		else
		{
			output_.cubeTexture_ = NULL;
		}
	}
}

/**
 *	This function loads texture from the provided memory block.
 *  It supports regular 2d textures and cubemaps.
 *
 *	@param	data	pointer to the memory block
 *	@param	lenght	lenght of the provided memory block
 *	@param	inputOptions	input options structure
 *	@param	outputOptions	output options structure
 *
 *	@return	true if loading is successfull and false otherwise
 */
bool	TextureLoader::loadTextureFromMemory( 
						const void* data, const uint length, 
						const TextureLoader::InputOptions& input,
						const BW::string& allocator )
{
	output_.reset();
	// check if d3d device is ready 
	// it might be NULL if something is trying to create a texture too early
	// or after shutdown
	if( Moo::rc().device() == NULL )
	{
		return false;
	}

	MF_ASSERT( length > 8 );

	bool cubemap = false;
	// detect if we have a cubemap texture
	DWORD* head = (DWORD*)data;
	if (head[0] == DDS_MAGIC && head[1] == 124 )
	{			
		DDS_HEADER* ddsHeader = (DDS_HEADER*)&head[1];
		// we have a DDS file....
		if ( ddsHeader->dwCubemapFlags & DDS_CUBEMAP_ALLFACES )
		{
			// and we have a cube map...
			cubemap = true;
		}
	}
	if ( cubemap )
	{
		this->loadCubeTextureFromMemory( data, length, input, allocator );
	}
	else // 2d texture
	{
		this->loadTexture2DFromMemory( data, length, input, allocator );
	}
	return (output_.texture2D_ || output_.cubeTexture_);
}

size_t	TextureLoader::calcDDSSize( D3DFORMAT format, uint width,
									uint height, uint numMips )
{
	size_t reqSize = sizeof(DWORD) + sizeof(DDS_HEADER);
	for (uint i = 0; i < numMips; ++i)
	{
		uint32 numBytes = 0;
		DDSHelper::getSurfaceInfo( 
							std::max<uint>(1, width >> i),
							std::max<uint>(1, height >> i),
							format, &numBytes, NULL, NULL );
		reqSize += numBytes;
	}
	return reqSize;
	
}

} // namespace Moo

BW_END_NAMESPACE

// texture_loader.cpp

