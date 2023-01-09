#include "pch.hpp"
#include "texture_compressor.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "texture_manager.hpp"
#include "texture_lock_wrapper.hpp"
#include "render_context.hpp"
#include "texture_loader.hpp"
#include "cstdmf/string_builder.hpp"
#include "moo/texture_detail_level.hpp"

#define ENABLE_NVTT !(BW_EXPORTER || CONSUMER_CLIENT_BUILD || _MSC_VER >= 1900)

#if ENABLE_NVTT
	#include "nvtt/nvtt.h"
	#ifdef _DEBUG
		#ifdef _M_AMD64
			#pragma message( __FILE__ " Linking nvtt_Debug_noCuda_x64.lib" )
			#pragma comment( lib, "nvtt_Debug_noCuda_x64.lib" )
		#else // _M_AMD64
			#pragma message( __FILE__ " Linking nvtt_Debug_noCuda_Win32.lib" )
			#pragma comment( lib, "nvtt_Debug_noCuda_Win32.lib" )
		#endif // _M_AMD64
	#else // _DEBUG
		#ifdef _M_AMD64
			#pragma message( __FILE__ " Linking nvtt_Release_noCuda_x64.lib" )
			#pragma comment( lib, "nvtt_Release_noCuda_x64.lib" )
		#else // _M_AMD64
			#pragma message( __FILE__ " Linking nvtt_Release_noCuda_Win32.lib" )
			#pragma comment( lib, "nvtt_Release_noCuda_Win32.lib" )
		#endif // _M_AMD64
	#endif // _DEBUG
#endif // ENABLE_NVTT

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "texture_compressor.ipp"
#endif

namespace Moo
{

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// -----------------------------------------------------------------------------
// Section: Texture compressor helper functions
// -----------------------------------------------------------------------------

namespace
{

int	calcMipChainLength( uint width, uint height )
{
	int numMips = 0;
	// caclulate number of mips in the full mip chain
	uint size = std::max( width, height );
	while (size > 0)
	{
		size /= 2;
		numMips++;
	}
	return numMips;
}

bool	canReuseSource(const ComObjectWrap<DX::Texture>&	texture,
			D3DFORMAT format, uint numMips, uint dstWidth, uint dstHeight)
{
	MF_ASSERT( texture );

	uint numLevels = texture->GetLevelCount();

	D3DSURFACE_DESC desc;
	HRESULT hr = texture->GetLevelDesc( 0, &desc );
	MF_ASSERT( SUCCEEDED( hr ) );

	if (numMips == 0)
	{
		numMips = calcMipChainLength( dstWidth, dstHeight );
	}
	//  return false if source texture is not lockable
	bool bLockable = (	desc.Usage & D3DUSAGE_DYNAMIC || 
						desc.Pool != D3DPOOL_DEFAULT);
	bool bParamMatch = (numLevels > 0 && numLevels == numMips &&
		desc.Format == format);

	return (bLockable && bParamMatch);
}

#if ENABLE_NVTT
/**
*	This function converts source texture to in-memory dds file using
*   NVidia texture tools library. 
*
*	@returns			Pointer to created binary block, NULL otherwise
*	
*/
bool	isFormatNVTTSupported( D3DFORMAT format )
{
	return (format == D3DFMT_A8R8G8B8 ||
			format == D3DFMT_DXT1 ||
			format == D3DFMT_DXT3 ||
			format == D3DFMT_DXT5);
}

BinaryPtr compressToMemoryNVTT(
	const BW::string& srcTextureName,
	const SmartPointer<TextureDetailLevel>& dstInfo,
	D3DFORMAT format)
{
	BW_GUARD;

	if (!rc().device())
	{
		return NULL;
	}

	// setup nvtt options
	nvtt::Format nvttFormat;
	switch (format)
	{
		case D3DFMT_A8R8G8B8:	nvttFormat = nvtt::Format_RGBA; break;
		case D3DFMT_DXT1:		nvttFormat = nvtt::Format_DXT1; break;
		case D3DFMT_DXT3:		nvttFormat = nvtt::Format_DXT3; break;
		case D3DFMT_DXT5:		nvttFormat = nvtt::Format_DXT5; break;
		default:
		{
			// NVTT does not support destination format
			return NULL;
		}
	}

	ComObjectWrap<DX::Texture> srcTexture;
	// load source texture
	BinaryPtr bin = 
		BWResource::instance().rootSection()->readBinary( srcTextureName );

	D3DXIMAGE_INFO srcInfo;

	if (bin)
	{
		// Create the DX texture, using D3DX to support legacy formats
		// tell d3dx not to upscale non power 2 textures
		// this way we can use nvtt for upscaling if required
		srcTexture = rc().createTextureFromFileInMemoryEx(
			bin->data(), bin->len(),
			D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_SCRATCH, D3DX_FILTER_NONE,
			D3DX_FILTER_NONE, 0, &srcInfo, NULL );
	}

	if (!srcTexture.hasComObject())
	{
		// failed to load source texture
		ASSET_MSG("compressToMemoryNVTT - failed to load source texture %s\n",
					srcTextureName.c_str() );
		return NULL;
	}


	// setup nvtt options
	nvtt::InputOptions inputOptions;
	nvtt::CompressionOptions compressionOptions;
	nvtt::OutputOptions outputOptions;

	inputOptions.setTextureLayout( nvtt::TextureType_2D, srcInfo.Width,
								srcInfo.Height, 1 );

	if (dstInfo->maxExtent() > 0)
	{
		inputOptions.setMaxExtents( dstInfo->maxExtent() );
	}

	if (!dstInfo->noResize())
	{
		// upscale to the next power of 2 to support old hardware
		inputOptions.setRoundMode( nvtt::RoundMode_ToNextPowerOfTwo ); 
	}
	else if ((nvttFormat == nvtt::Format_DXT1 || 
			nvttFormat == nvtt::Format_DXT3 ||
			nvttFormat == nvtt::Format_DXT5 ) &&
			((srcInfo.Width & 0x3) || (srcInfo.Height & 0x3)))
	{
		WARNING_MSG( "TextureCompressor: DXT format only support width/height \
				to be multiple of 4. Using uncompressed format instead.\n" );
		nvttFormat = nvtt::Format_RGBA;
	}


	nvtt::AlphaMode alphaMode = nvtt::AlphaMode_None;
	if (nvttFormat == nvtt::Format_RGBA || nvttFormat == nvtt::Format_DXT3 ||
		nvttFormat == nvtt::Format_DXT5)
	{
		alphaMode = nvtt::AlphaMode_Transparency;
	}
	inputOptions.setAlphaMode( alphaMode );

	if (dstInfo->normalMap())
	{
		// we have some data in the alpha channel
		// what we gonna do is to move it to the blue channel
		inputOptions.setNormalMap( true );
		inputOptions.setAlphaMode( nvtt::AlphaMode_None );
		if (nvttFormat == nvtt::Format_DXT1)
		{
			compressionOptions.setColorWeights( 1.0f, 1.0f, 0.0f );
		}
		else if (nvttFormat == nvtt::Format_DXT5)
		{
			nvttFormat = nvtt::Format_DXT5n;
		}
	}

	compressionOptions.setFormat( nvttFormat );
	compressionOptions.setQuality( nvtt::Quality_Normal );

	// fill binary data
	TextureLockWrapper textureLock( srcTexture );
	D3DLOCKED_RECT lockedRect;

	HRESULT hr = textureLock.lockRect( 0, &lockedRect, NULL, 0 );
	if (FAILED( hr ))
	{
		ASSET_MSG("compressToMemoryNVTT - failed to lock source texture %s\n",
					srcTextureName.c_str() );
		return NULL;
	}

	D3DSURFACE_DESC realTexDesc;
	srcTexture->GetLevelDesc( 0, &realTexDesc );

	// real texture width and hight might be bigger
	// if source texture does not have power 2 dimensions
	// we need to check this case and extract part of the source texture

	// get desired number of mip levels
	// it might be too big, we still need to calculate real number
	int numMips = (int)dstInfo->mipCount();
	if (dstInfo->sourceMipmaps() == TextureDetailLevel::SOURCE_NO_MIPMAPS)
	{
		inputOptions.setTextureLayout( nvtt::TextureType_2D, srcInfo.Width,
								srcInfo.Height, 1 );
		// set the whole image as mip 0
		inputOptions.setMipmapDataRect( lockedRect.pBits, 0, 0, 
									srcInfo.Width, srcInfo.Height,
									realTexDesc.Width, realTexDesc.Height, 1, 0, 0 );

		int numRealMips = calcMipChainLength( srcInfo.Width, srcInfo.Height );
		if (numMips == 0 || numMips > numRealMips)
		{
			// generate full mipchain
			numMips = numRealMips;
		}
	}
	else
	{
		// we have prebaked mipmap chain
		// read all of them and fill nvtt mipmap data
		// we assume that image is square and the rest of it is a mipchain
		uint32 mipSize = 0;
		int chainSpace = 0;
		bool horizontal = ( dstInfo->sourceMipmaps() ==
						  TextureDetailLevel::SOURCE_HORIZONTAL_MIPMAPS );

		if (horizontal)
		{
			// horizontal mipchain
			mipSize = srcInfo.Height;
			MF_ASSERT( srcInfo.Width >= srcInfo.Height &&
					   srcInfo.Height * 2 >= srcInfo.Width);
			chainSpace = srcInfo.Width;
		}
		else
		{
			// vertical mipchain
			mipSize = srcInfo.Width;
			MF_ASSERT( srcInfo.Height >= srcInfo.Width &&
					   srcInfo.Width * 2 >= srcInfo.Height);
			chainSpace = srcInfo.Height;
		}
		inputOptions.setTextureLayout( nvtt::TextureType_2D, mipSize,
								mipSize, 1 );

		uint32 x0 = 0;
		uint32 y0 = 0;
		int numRealMips = 0;
		// supply nvtt with the mipchain data
		// at the end of the loop mipSize might be bigger than 0
		// which will indicate that we have incomplete mipchain
		while (mipSize >= 1 && chainSpace > 0)
		{
			inputOptions.setMipmapDataRect( lockedRect.pBits, x0, y0,
						x0 + mipSize, y0 + mipSize, realTexDesc.Width, realTexDesc.Height,
						1, 0, numRealMips );
			if (horizontal)
			{
				x0 += mipSize;
			}
			else
			{
				y0 += mipSize;
			}
			numRealMips++;
			chainSpace -= mipSize;
			mipSize /= 2;
		}

		// update number of mipmaps
		// we only update number of mipmaps if provided number is too big
		if (numRealMips < numMips || numMips == 0)
		{
			numMips = numRealMips;
		}
	}

	hr = textureLock.unlockRect( 0 );
	textureLock.flush();
	textureLock.fini();
	MF_ASSERT( SUCCEEDED( hr ) );

	srcTexture = NULL;

	// nvtt won't regenerate mip level if we supplied data for it
	// setup up number mipmaps required, filter and alpha mode
	inputOptions.setMipmapGeneration(true, numMips );
	inputOptions.setMipmapFilter( nvtt::MipmapFilter_Kaiser );

	// output memory handler
	struct MemoryOutputHandler : nvtt::OutputHandler
	{
		// preallocate memory
		MemoryOutputHandler( size_t reqSize )
		{
			binary_.reserve( reqSize );
		}
		/// Indicate the start of a new compressed image
		// that's part of the final texture.
		virtual void beginImage( int size, int width, int height,
			int depth, int face, int miplevel )
		{	// ignore
		}

		/// Output data. Compressed data is output as soon
		// as it's generated to minimize memory allocations.
		virtual bool writeData( const void * data, int size )
		{
			MF_ASSERT( size > 0 );
			size_t oldSize = binary_.size();
			binary_.resize( oldSize + size );
			memcpy( &binary_[0] + oldSize, data, size );
			return true;
		}

		BW::vector<uint8>		binary_;
	};

	// Error handler
	struct ErrorReporter : nvtt::ErrorHandler
	{
		virtual void error(nvtt::Error e)
		{
			ERROR_MSG("compressToMemoryNVTT - NVTT:%s\n",
					nvtt::errorString( e ) );
		}
	};

	ErrorReporter		errorReporter;
	size_t reqDDSMemory = TextureLoader::calcDDSSize( 
					format, srcInfo.Width, srcInfo.Height, numMips );
	MemoryOutputHandler	memoryOutputHandler( reqDDSMemory );

	// setup output options
	outputOptions.setOutputHandler( &memoryOutputHandler );
	outputOptions.setErrorHandler( &errorReporter );

	nvtt::Compressor compressor;
	bool bSuccess = compressor.process( inputOptions, compressionOptions,
										outputOptions );
	if (!bSuccess)
	{
		// nvtt error should be reported by ErrorReporter
		ASSET_MSG("TextureManager::convertToDDS- Failed to convert texture %s\n",
			srcTextureName.c_str() );
		return NULL;
	}

	BinaryPtr outputBin = new BinaryBlock( &memoryOutputHandler.binary_[0],
				memoryOutputHandler.binary_.size(), "BinaryBlock/TextureCompressor" );

	return outputBin;
}

#endif // CONSUMER_CLIENT_BUILD
/**
 *	This function loads texture using d3dx functionality
 *  It supports prebaked horizontal or vertical mipmaps
 *
 *	@param	string	Texture name
 *	@param	TextureDetailLevel	Texture detail level
*	@param	output uint	number of mipmaps
 *
 *	@return DX::Texture
 */
ComObjectWrap<DX::Texture> loadD3DXTexture(
	const BW::string& srcTextureName,
	const SmartPointer<TextureDetailLevel>& dstInfo,
	uint* pNumMips)
{
	BW_GUARD;

	if (!rc().device())
	{
		return NULL;
	}

	*pNumMips = 0;
	// failed to read texture data
	BinaryPtr inputBin = BWResource::instance().rootSection()->readBinary( srcTextureName );
	if (!inputBin)
	{
		return NULL;
	}

	ComObjectWrap<DX::Texture> outputTexture;
	if (dstInfo->mipCount() == 0 && !dstInfo->noResize())
	{
		// Create the texture and the mipmaps.
		outputTexture = rc().createTextureFromFileInMemoryEx( 
			inputBin->data(), inputBin->len(),
			D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM, D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 
			D3DX_FILTER_BOX|D3DX_FILTER_MIRROR, 0, NULL, NULL );
	}
	else
	{
		// create temporary texture and process horizontal or vertical mipmaps
		ComObjectWrap<DX::Texture> tex = rc().createTextureFromFileInMemoryEx( 
			inputBin->data(), inputBin->len(),
			D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_A8R8G8B8,
			D3DPOOL_SCRATCH, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, NULL, NULL );

		if (tex)
		{
			D3DSURFACE_DESC desc;
			tex->GetLevelDesc( 0, &desc );

			uint32 width = desc.Width;
			uint32 height = desc.Height;
			uint32 newWidth = width;
			uint32 newHeight = height;
			uint32 mipBase = dstInfo->mipSize();

			if (dstInfo->sourceMipmaps() == TextureDetailLevel::SOURCE_HORIZONTAL_MIPMAPS)
			{
				newWidth = mipBase != 0 
					? mipBase 
					: newWidth / 2;
				mipBase = newWidth;
			}
			else
			{
				newHeight = mipBase != 0 
					? mipBase 
					: newHeight / 2;
				mipBase = newHeight ;
			}

            uint32 realMipCount = 0;
			uint32 levelWidth = newWidth;
			uint32 levelHeight = newHeight;
			uint32 levelOffset = 0;
			uint32 levelSize = mipBase;
			uint32 levelMax = 
				dstInfo->sourceMipmaps() == TextureDetailLevel::SOURCE_HORIZONTAL_MIPMAPS ? width : height;
			uint32 mipCount = dstInfo->mipCount() > 0 ?  dstInfo->mipCount() : 0xffffffff;
			while (levelWidth > 0 &&
				levelHeight > 0 &&
				realMipCount < dstInfo->mipCount() &&
				(levelSize + levelOffset) <= levelMax)
			{
				realMipCount++;
				levelHeight = levelHeight >> 1;
				levelWidth = levelWidth >> 1;
				levelOffset += levelSize;
				levelSize = levelSize >> 1;
			}

			outputTexture = rc().createTexture( newWidth, newHeight, realMipCount, 0, D3DFMT_A8R8G8B8,
				D3DPOOL_SCRATCH );
			if (outputTexture)
			{
				uint32 levelWidth = newWidth;
				uint32 levelHeight = newHeight;
				uint32 levelBase = 0;
				bool horizontal = 
					dstInfo->sourceMipmaps() == TextureDetailLevel::SOURCE_HORIZONTAL_MIPMAPS;

				DX::Surface* pSurfSrc = NULL;
				tex->GetSurfaceLevel(0, &pSurfSrc);
				RECT srcRect;
				SimpleMutexHolder smh(rc().getD3DXCreateMutex());
				for (uint32 iLevel = 0; iLevel < realMipCount; iLevel++)
				{
					srcRect.top = horizontal ? 0 : levelBase;
					srcRect.bottom = srcRect.top + levelHeight;
					srcRect.left = horizontal ? levelBase : 0;
					srcRect.right = srcRect.left + levelWidth;
					levelBase += mipBase;
					levelWidth = levelWidth >> 1;
					levelHeight = levelHeight >> 1;
					mipBase = mipBase >> 1;
					DX::Surface* pSurfDest = NULL;
					outputTexture->GetSurfaceLevel(iLevel, &pSurfDest);
					HRESULT hr = D3DXLoadSurfaceFromSurface(pSurfDest, NULL, NULL,
						pSurfSrc, NULL, &srcRect, D3DX_FILTER_NONE, 0);
					MF_ASSERT( SUCCEEDED( hr ) );
					pSurfDest->Release();
				}
				pSurfSrc->Release();
				*pNumMips = realMipCount;
			}
		}
	}
	return outputTexture;
}


/**
 *	This method copies data from the source texture to destination texture
 *  completing the mipchain if necessary
 *
 *	@param dstTexture	Destination texture. Can't be empty.
 */
static void copyTexture( ComObjectWrap<DX::Texture>& dstTexture,
	const ComObjectWrap<DX::Texture>& srcTexture )
{
    BW_GUARD;
	// both source and destination textures must exist
	MF_ASSERT( srcTexture.hasComObject() );
	MF_ASSERT ( dstTexture.hasComObject() );

	DX::Texture*	pmiptexSrc = srcTexture.pComObject();
	DX::Texture*	pmiptexDest = dstTexture.pComObject();
	uint32			numSrcMipLevels = srcTexture->GetLevelCount();
	uint32			numDstMipLevels = dstTexture->GetLevelCount();

	HRESULT hr;
	DWORD srcMipMapLevel = 0;
	D3DSURFACE_DESC desc;
	
		
	DWORD	currMipLevel;

	// if we have fewer destination mipmap levels than the source, 
	//  it may be because we have specified a very low maxextent (dimensions)
	//	of the destination texture, which means only the smallest mipmaps 
	//	from the source texture will fit the maxextent requirements.
	if (numDstMipLevels < numSrcMipLevels)
	{
		// find the largest source mipmap level that corresponds to the max dimensions
		//  required for the destination texture, and set that as the starting mipmap
		//	level to copy into the destination.

		// get the dimensions of the destination mipmaps
		DX::Surface* pOriginalDestination = NULL;
		hr = pmiptexDest->GetSurfaceLevel( 0, &pOriginalDestination );
		MF_ASSERT( pOriginalDestination ); // assert if we cant get the destination DX::surface
		hr = pOriginalDestination->GetDesc( &desc );
		UINT largestDstMipMapDimension = max<UINT>( desc.Width,desc.Height );
		
		SAFE_RELEASE( pOriginalDestination ); // reduce reference count on this dx resource so it can be freed
		
		// get the dimensions of the largest source mipmap
		DX::Surface* pOriginalSource = NULL;
		hr = pmiptexSrc->GetSurfaceLevel( 0, &pOriginalSource );
		MF_ASSERT( pOriginalSource ); // assert if we cant get the source DX::surface
		hr = pOriginalSource->GetDesc( &desc );
		UINT largestSrcMipMapDimension = max<UINT>( desc.Width, desc.Height );
		
		SAFE_RELEASE( pOriginalSource ); // reduce reference count on this dx resource so it can be freed
		
		UINT currDimension = largestSrcMipMapDimension;
		MF_ASSERT( currDimension ); // assert if largest source tex mip size is zero.
		currMipLevel = 0;
		while (currDimension > largestDstMipMapDimension)
		{
			currDimension /= 2;
			++currMipLevel;
			if ((currDimension < 1) || (currMipLevel > numSrcMipLevels))
			{
				WARNING_MSG( "Not enough mip maps in source texture to copy into destination of correct dimensions" );
				currMipLevel = 0; // start src mipmap copying from the largest one
				break;
			}
		}
		
		srcMipMapLevel = currMipLevel;
	}
	else 
	// If we have more destination than source mip levels, don't copy the last 
	// mip level in the source chain, as this one will be copied to scratch 
	// memory before being used as the source for further mip creation
	if (numDstMipLevels > numSrcMipLevels)
	{
		--numSrcMipLevels;
	}

	DWORD iLevel = 0;
	
	SimpleMutexHolder smh(rc().getD3DXCreateMutex());
		
	// copy the existing src mip levels that fit into the dest mip levels
	for (; iLevel < numDstMipLevels; ++iLevel)
	{
		DX::Surface* psurfSrc = NULL;
		DX::Surface* psurfDest = NULL;
		hr = pmiptexSrc->GetSurfaceLevel( srcMipMapLevel, &psurfSrc );
		MF_ASSERT( SUCCEEDED( hr ) );
		hr = pmiptexDest->GetSurfaceLevel( iLevel, &psurfDest );
		MF_ASSERT( SUCCEEDED( hr ) );
		hr = D3DXLoadSurfaceFromSurface( psurfDest, NULL, NULL,
			psurfSrc, NULL, NULL, D3DX_FILTER_TRIANGLE, 0 );
		MF_ASSERT( SUCCEEDED( hr ) );
		SAFE_RELEASE( psurfSrc );
		SAFE_RELEASE( psurfDest );
		++srcMipMapLevel;

		// if we have run out of src mipmap levels (because we had to start half way through 
		//	the source mip levels due to max dimension requirements of the destination texture,
		//	and I guess that not all mip levels were created for the source texture)
		//  stop copying them.
		if (srcMipMapLevel >= numSrcMipLevels) 
			break; 

		// if we have more source mip levels than we need in the dest tex, dont copy anymore
		if (iLevel >= numDstMipLevels)
			break;
	}


	// Now, create further desired mip map levels
	if ( numDstMipLevels > numSrcMipLevels )
	{
		DX::Surface * psurfSrc[2] = { NULL, NULL };

		hr = D3D_OK;

		DX::Surface *pOriginalSource = NULL;
		
		
		hr = pmiptexSrc->GetSurfaceLevel( iLevel, &pOriginalSource );
		hr = pOriginalSource->GetDesc( &desc );

		D3DFORMAT intermediateFormat = desc.Format;

		// If our source format is one of the compressed formats, go via
		// an uncompressed format when creating the mipmaps
		if (intermediateFormat == D3DFMT_DXT1 || intermediateFormat == D3DFMT_DXT2 ||
			intermediateFormat == D3DFMT_DXT3 || intermediateFormat == D3DFMT_DXT4 ||
			intermediateFormat == D3DFMT_DXT5 )
		{
			intermediateFormat = D3DFMT_A8R8G8B8;
		}

		// Create the source surface to use in scratch memory,
		// and load it with the desired mipmap
		hr = rc().device()->CreateOffscreenPlainSurface( desc.Width, 
			desc.Height, intermediateFormat, D3DPOOL_SYSTEMMEM, &psurfSrc[0], NULL );
		MF_ASSERT( SUCCEEDED( hr ) );

		hr = rc().device()->CreateOffscreenPlainSurface( desc.Width >> 1, 
			desc.Height >> 1, intermediateFormat, D3DPOOL_SYSTEMMEM, &psurfSrc[1], NULL );
		MF_ASSERT( SUCCEEDED( hr ) );

		hr = D3DXLoadSurfaceFromSurface(psurfSrc[0], NULL, NULL,
			pOriginalSource, NULL, NULL, D3DX_FILTER_NONE, 0);
		MF_ASSERT( SUCCEEDED( hr ) );
		SAFE_RELEASE( pOriginalSource );

		RECT curRect = { 0, 0, (LONG)desc.Width, (LONG)desc.Height };

		int curSurf = 0;

		// Generate the mipmaps from the lowest known mipmap in the source texture
		for ( uint32 level = iLevel; level < numDstMipLevels; level++ )
		{
			DX::Surface* psurfDest = NULL;
			hr = pmiptexDest->GetSurfaceLevel(level, &psurfDest);
			hr = D3DXLoadSurfaceFromSurface(psurfDest, NULL, NULL,
				psurfSrc[ curSurf ], NULL, &curRect, D3DX_FILTER_NONE, 0);
			MF_ASSERT( SUCCEEDED( hr ) );

			if (curRect.right >= 2 && curRect.bottom >= 2)
			{
				RECT nextRect = { 0, 0, curRect.right >> 1, curRect.bottom >> 1 };
				int nextSurf = 1 - curSurf;

				hr = D3DXLoadSurfaceFromSurface(psurfSrc[ nextSurf ], NULL, &nextRect,
					psurfSrc[ curSurf ], NULL, &curRect, D3DX_FILTER_BOX | D3DX_FILTER_MIRROR, 0);
				MF_ASSERT( SUCCEEDED( hr ) );

				curRect = nextRect;
				curSurf = nextSurf;
			}

			SAFE_RELEASE( psurfDest );
		}

		SAFE_RELEASE( psurfSrc[0] );
		SAFE_RELEASE( psurfSrc[1] );
	}
}

} // end of unnamed namespace

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

TextureCompressor::TextureCompressor( DX::Texture*	srcTexture ):
srcTexture_( srcTexture ),
srcTextureName_( "internal" )
{
}

TextureCompressor::TextureCompressor( const BW::StringRef&	srcTextureName ):
srcTexture_( NULL ),
srcTextureName_( srcTextureName.data(), srcTextureName.size() )
{
}

/*static*/D3DXIMAGE_FILEFORMAT TextureCompressor::getD3DImageFileFormat( const BW::StringRef& fileName, D3DXIMAGE_FILEFORMAT defaultVal )
{
	D3DXIMAGE_FILEFORMAT fileFormat = defaultVal;
	BW::StringRef ext = BWResource::getExtension( fileName );
	if ( ext.equals_lower( "bmp" ) )
	{
		fileFormat = D3DXIFF_BMP;
	}
	else if ( ext.equals_lower( "jpg" ) )
	{
		fileFormat = D3DXIFF_JPG;
	}
	else if ( ext.equals_lower( "tga" ) )
	{
		fileFormat = D3DXIFF_TGA;
	}
	else if ( ext.equals_lower( "png" ) )
	{
		fileFormat = D3DXIFF_PNG;
	}
	return fileFormat;
}

/**
 *	This method compressed texture into the memory and returns
 *  result as a binary block
 *
 *	@param	TextureDetailLevel	Texture detail level information
 *
 *	@return BinaryBlock pointer
 */
BinaryPtr TextureCompressor::compressToMemory( 
	const SmartPointer< TextureDetailLevel>& dstInfo, const bool bCompressed )
{
	BW_GUARD;

	MF_ASSERT(dstInfo);
	MF_ASSERT( srcTexture_.pComObject() == NULL );
	MF_ASSERT( srcTextureName_ != "internal" );

	D3DFORMAT format;
	if ( bCompressed && dstInfo->isCompressed() )
	{
		format = dstInfo->formatCompressed();
	}
	else
	{
		format = dstInfo->format();
	}
	// try to use nvtt first if we are not in consumer config
	BinaryPtr bin;
#if ENABLE_NVTT
	if (isFormatNVTTSupported(format))
	{
		bin = compressToMemoryNVTT( srcTextureName_, dstInfo, format );
		if (!bin)
		{
			ASSET_MSG( "TextureCompressor failed to do nvtt compression for %s, fall back to d3dx\n", srcTextureName_.c_str() );
		}
	}
#endif
	if (!bin)
	{
		// fallback to default d3dx conversion
		uint numMips = 0;
		srcTexture_ = loadD3DXTexture( srcTextureName_, dstInfo, &numMips );
		if (srcTexture_)
		{
			bin = this->compressToMemory( format, numMips, dstInfo->maxExtent() ); 
		}
	}
	return bin;
}

//***********************************************************************************************

/**
 *	This method converts the given source texture into the given
 *	texture format, and saves it to the given relative filename.
 *
 *	@param	D3DFORMAT	The required destination format
 *	@param	uint		The number of MIP levels to convert 
 *						(optional, defaults to full mip chain)
 *
 *	@return BinaryBlock pointer
 */

BinaryPtr TextureCompressor::compressToMemory( D3DFORMAT format, uint numMips,
											uint maxExtent, D3DXIMAGE_FILEFORMAT imgFormat )
{
	BW_GUARD;

	BinaryPtr bin;
	// default d3dx compression
	ComObjectWrap<DX::Texture> texture = this->compress( format, numMips, maxExtent );

	if ( texture )
	{
		SimpleMutexHolder smh(rc().getD3DXCreateMutex());
		ID3DXBuffer* pBuffer = NULL;
		HRESULT hr = D3DXSaveTextureToFileInMemory( &pBuffer,
			imgFormat/*D3DXIFF_DDS*/, texture.pComObject(), NULL );

		if ( SUCCEEDED( hr ) )
		{
			bin = new BinaryBlock( pBuffer->GetBufferPointer(),
				pBuffer->GetBufferSize(), "BinaryBlock/TextureCompressor" );

			pBuffer->Release();
		}
	}

	return bin;
}

/**
 *	This method converts the given source texture into the given
 *	texture format, and saves it to the given relative filename.
 *
 *	@param	filename	an absolute path or a resource tree relative filename.
*	@param	D3DFORMAT	destination texture format
*	@param	uint		distination number of mip levels
 *
 *	@return True if there were no problems.
 */
bool TextureCompressor::save( const BW::StringRef & dstFilenameRef,
								D3DFORMAT format, uint numMips, uint maxExtent )
{
	bool bResult = false;
	D3DXIMAGE_FILEFORMAT fileFormat =
				 TextureCompressor::getD3DImageFileFormat( dstFilenameRef, D3DXIFF_DDS );
	BinaryPtr bin = this->compressToMemory( format, numMips, maxExtent, fileFormat );
	if (bin)
	{
		if (BWUtil::isAbsolutePath( dstFilenameRef ))
		{
			BW::string dstFilename( dstFilenameRef.data(), dstFilenameRef.size() );
			MF_ASSERT( dstFilenameRef.length() <= MAX_PATH );
			FILE * pFile = bw_fopen( dstFilename.c_str(), "wb" );

			if (pFile && !ferror(pFile))
			{					
				if (bin->len())
				{
					if (fwrite( bin->data(), bin->len(), 1, pFile ) == 1)
					{
						bResult = true;
					}
					else
					{
						CRITICAL_MSG( "TextureCompressor::save: "
								"Error writing to %s. Disk full?\n",
							dstFilename.c_str() );
					}
				}
				fclose( pFile );
			}
		}
		else
		{
			bResult = BWResource::instance().fileSystem()->writeFile(
				dstFilenameRef, bin, true );
		}
	}
	if (!bResult)
	{
		ASSET_MSG( "TextureCompressor: Failed to save file %s\n", dstFilenameRef.to_string().c_str() );
	}
	return bResult;
}

/**
 *	This method converts the given source texture into the given
 *	texture format, and saves it to the given relative filename.
 *
 *	@param	filename	a resource tree relative filename.
*	@param	TextureDetailLevelPtr	destination texture info
 *
 *	@return True if there were no problems.
 */
bool TextureCompressor::save( const BW::StringRef & dstFilename,
							const TextureDetailLevelPtr& dstInfo,
							const bool bCompressed )
{
	bool bResult = false;
	BinaryPtr bin = this->compressToMemory( dstInfo, bCompressed );
	if (bin)
	{
		bResult = BWResource::instance().fileSystem()->writeFile(
				dstFilename, bin, true );
	}
	if (!bResult)
	{
		ASSET_MSG( "TextureCompressor: Failed to save file %s\n", dstFilename.to_string().c_str() );
	}
	return bResult;
}

/**
 *	This method converts the given source texture into the given
 *	texture format, and stows it in the given data section.
 *
 *	This method does not save the data section to disk.
 *
 *	@param	pSection	a Data Section smart pointer.
 *	@param	childTag	tag within the given section.
 *
 *	@return True if there were no problems.
 */
bool TextureCompressor::stow( DataSectionPtr pSection, const BW::StringRef & childTag,
							D3DFORMAT format, uint numMips, uint maxExtent )
{
	BW_GUARD;

	BinaryPtr binaryBlock = this->compressToMemory( format, numMips, maxExtent );

	//Stick the DDS into a binary section, but don't save the file to disk.
	if (!childTag.empty())
	{
		pSection->delChild( childTag );
		DataSectionPtr textureSection = pSection->openSection( childTag, true );
		textureSection->setParent(pSection);
		textureSection->save();
		textureSection->setParent( NULL );

		if ( !pSection->writeBinary( childTag, binaryBlock ) )
		{
			CRITICAL_MSG( "TextureCompressor::stow: error while writing \
							BinSection in \"%s\"\n", childTag.to_string().c_str());
			return false;
		}
	}
	else
	{
		pSection->setBinary(binaryBlock);
	}	

	return true;
}


/**
*  This method converts source texture to destination texture. 
*  Destination texture is created in SYSTEMMEMORY pool.
*
*	@param D3DFORMAT	format of destination texture.
*	@param uint			number of mip levels in destination texture.
*	@returns			converted texture or NULL if operation not succeeded.
*	
*/
ComObjectWrap<DX::Texture> TextureCompressor::compress( 
						D3DFORMAT dstFormat, uint numMips, uint maxExtent ) const	
{
	BW_GUARD;
	//  D3DX conversion only

	MF_ASSERT( srcTexture_ );

	// check format and dimensions compatibility
	if (!rc().supportsTextureFormat( dstFormat ))
	{
		WARNING_MSG( "TextureCompressor: This device does not support "
			"the desired compressed texture format: %d ('%c%c%c%c')\n", dstFormat,
			char(dstFormat>>24), char(dstFormat>>16), char(dstFormat>>8), 
			char(dstFormat) );
		dstFormat = D3DFMT_A8R8G8B8;
	}

	D3DSURFACE_DESC desc;
	HRESULT hr = srcTexture_->GetLevelDesc( 0, &desc );
	MF_ASSERT( SUCCEEDED( hr ) );

	uint width = desc.Width;
	uint height = desc.Height;
	uint maxd = std::max( width, height );
	if (maxExtent != 0 && maxd > maxExtent)
	{
		float koef = (float)maxd / maxExtent;
		// downscale keeping aspect ratio
		width = std::max<uint>( 1, (uint)(width / koef) );
		height = std::max<uint>( 1, (uint)(height / koef) );
	}

	if (((width & 0x3) || (height & 0x3)) &&
		(dstFormat == D3DFMT_DXT1 || dstFormat == D3DFMT_DXT2 || 
		dstFormat == D3DFMT_DXT3 || dstFormat == D3DFMT_DXT4 ||
		dstFormat == D3DFMT_DXT5) )
	{
		WARNING_MSG( "TextureCompressor: DXT format only support width/height \
				to be multiple of 4. Using uncompressed format instead.\n" );
		dstFormat = D3DFMT_A8R8G8B8;
	}

	if (canReuseSource( srcTexture_, dstFormat, numMips, width, height ))
	{
		return srcTexture_;
	}

	// create destination texture
	ComObjectWrap<DX::Texture> dstTexture = rc().createTexture( width,
		height, numMips, 0, dstFormat, D3DPOOL_SYSTEMMEM );
	// copy data into it
	if (dstTexture)
	{
		copyTexture( dstTexture, srcTexture_ );
	}
	else
	{
		ERROR_MSG( "TextureCompressor: failed to create destination texture \
					 %d x %d %d mips\n", width, height, numMips );
	}
	return dstTexture;

}

void TextureCompressor::compress( ComObjectWrap<DX::Texture>& dstTexture )
{
	BW_GUARD;
	// d3dx compression only
	copyTexture( dstTexture, srcTexture_ );
}

} // namespace Moo

BW_END_NAMESPACE

// texture_compressor.cpp
