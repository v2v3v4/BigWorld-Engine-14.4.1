#include "pch.hpp"

#include "streaming_texture.hpp"
#include "texture_manager.hpp"
#include "texture_streaming_manager.hpp"
#include "resmgr\file_streamer.hpp"
#include "dds.h"
#include "texture_lock_wrapper.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "streaming_texture.ipp"
#endif

namespace Moo
{

StreamingTexture::StreamingTexture( TextureDetailLevel * detail,
	const BW::StringRef & resourceID, const FailurePolicy & notFoundPolicy,
	D3DFORMAT format,
	int mipSkip, bool noResize, bool noFilter,
	const BW::StringRef & allocator ) : 
		BaseTexture( allocator ), 
		streamingTextureHandle_( ), 
		detail_( detail ), 
		resourceID_( resourceID.to_string() ), 
		mipSkip_( mipSkip ), 
		mipLodBias_( 0.0f ), 
		activeTexture_( NULL ), 
		textureMemoryUsed_( 0 )
{
	status( STATUS_READY );
}


StreamingTexture::~StreamingTexture()
{
	// at this point, texture streaming manager should already be locked
	if (TextureStreamingManager * tsManager = streamingManager())
	{
		tsManager->unregisterTexture( this ); 
	}
	assignTexture( NULL );
}


void StreamingTexture::destroy() const
{
	if (TextureStreamingManager * tsManager = streamingManager())
	{
		// this will ensure that tsManager is locked before ref is checked
		tsManager->tryDestroy( this );
	}
	else
	{
		// fall back to default behaviour
		this->tryDestroy();
	}
}

BaseTexture::TextureType StreamingTexture::type() const
{
	return STREAMING;
}

void StreamingTexture::assignTexture( DeviceTexture * pTex )
{
	// If the currently active texture is the assigned texture,
	// then make the active texture this new texture.
	// Otherwise we would be removing any debug textures currently assigned.
	if (activeTexture_ == curTexture_.pComObject())
	{
		activeTexture_ = pTex;
	}

	// let the texture manager know original texture is unused
	// reuse it if reuse flag is set
	if (curTexture_.hasComObject())
	{
		// Check if streaming manager is still around.
		// if it is, then the render context should be too.
		if (streamingManager())
		{
			if (curTexture_->GetType() == D3DRTYPE_TEXTURE)
			{
				DeviceTexture2D * p2DTexture = 
					reinterpret_cast< DeviceTexture2D * >( curTexture_.pComObject() );
				rc().putTextureToReuseList( p2DTexture );
			}
		}
	}

	curTexture_ = pTex;

	textureMemoryUsed_ = BaseTexture::textureMemoryUsed( pTex );
}

void StreamingTexture::assignDebugTexture( DeviceTexture * pTex )
{
	if (pTex == NULL)
	{
		activeTexture_ = curTexture_.pComObject();
	}
	else
	{
		activeTexture_ = pTex;
	}
}

DX::BaseTexture * StreamingTexture::pTexture( )
{
	return activeTexture_;
}

DeviceTexture * StreamingTexture::assignedTexture()
{
	return curTexture_.pComObject();
}

TextureDetailLevel * StreamingTexture::detail()
{
	return detail_.get();
}

TextureStreamingManager * StreamingTexture::streamingManager() const
{
	TextureManager * pManager = TextureManager::instance();
	if (pManager)
	{
		return pManager->streamingManager();
	}
	else
	{
		return NULL;
	}
}

uint32 StreamingTexture::width() const
{
	return streamingManager()->getTextureWidth( this );
}

uint32 StreamingTexture::height() const
{
	return streamingManager()->getTextureHeight( this );
}

D3DFORMAT StreamingTexture::format() const
{
	return streamingManager()->getTextureFormat( this );
}

uint32 StreamingTexture::textureMemoryUsed() const
{
	return textureMemoryUsed_;
}

const BW::string& StreamingTexture::resourceID() const
{
	return resourceID_;
}

uint32 StreamingTexture::maxMipLevel() const
{
	return streamingManager()->getTextureMaxMipLevel( this );
}

uint32 StreamingTexture::mipSkip() const
{
	return mipSkip_;
}

void StreamingTexture::mipSkip( uint32 mipSkip )
{
	if (mipSkip_ == mipSkip)
	{
		return;
	}

	mipSkip_ = mipSkip;
	streamingManager()->updateTextureMipSkip( this, mipSkip );
}

void StreamingTexture::reload()
{
	streamingManager()->reloadTexture( this );
}

void StreamingTexture::reload( const BW::StringRef & resourceID )
{
	resourceID_ = resourceID.to_string();
	reload();
}

// ==========================
// Streaming texture loader
// ==========================

bool StreamingTextureLoader::createTexture( const HeaderInfo & header, 
	const LoadOptions & options, LoadResult & output )
{
	D3DPOOL pool = D3DPOOL_MANAGED;

	uint32 width = std::max< uint32 >( header.width_ >> options.mipSkip_, 1 );
	uint32 height = std::max< uint32 >( header.height_ >> options.mipSkip_, 1 );

	// Ensure texture dimensions for DXT formats are multiple of 4
	if (DDSHelper::isDXTFormat( header.format_ ))
	{
		width = (width + 3) & 0xfffffffc;
		height = (height + 3) & 0xfffffffc;
	}

	output.texture2D_ = Moo::rc().createTexture( width, height, 
		options.numMips_, 0, header.format_, pool, "DDSLOADER" );

	if (!output.texture2D_.hasComObject())
	{
		// TODO: Currently there is no supported fallback path for GPUs which
		// do not support ATI1N or ATI2N for ATI2n we need to do runtime 
		// conversion to ARGB8 and drop 1 more mip level to keep memory usage 
		// identical for ATI1n we need to do runtime conversion to DXT1
		// According to  http://aras-p.info/texts/D3D9GPUHacks.html#3dc
		// ATI1N is supported on GeForce 8+, Radeon X1300+, Intel G45+.
		// ATI2N is supported on GeForce 7+,  Radeon 9500+, Intel G45+.
		// NVIDIA exposing it a while ago and Intel exposing it recently
		// (drivers 15.17 or higher).

		// double check device format to make sure we failed because driver 
		// rejected unsupported texture format
		if (!Moo::rc().supportsTextureFormat( header.format_ ))
		{
			ERROR_MSG( "Video card driver failed to create texture because"
				"texture format is not supported.\n"
				"Try to update drivers for your video card.\n" );
		}
		// failed to create a texture, must be out of VA space
		return false;
	}
	else
	{
		return true;
	}
}

bool StreamingTextureLoader::loadHeaderFromStream( 
	IFileStreamer * pStream, HeaderInfo & header )
{
	size_t streamBegin = pStream->getOffset();

	// Read DDS header.
	DWORD magic;
	pStream->read( sizeof( magic ), &magic );
	if (magic != DDS_MAGIC)
	{
		// this is not a dds file or it has a corrupted signature
		// return unknown so calling code can fall back to d3dx
		return false;
	}

	DDS_HEADER ddsHeader;
	pStream->read( sizeof( ddsHeader ), &ddsHeader );

	// Copy this data into the target structure
	header.valid_ = true;
	header.width_ = ddsHeader.dwWidth;
	header.height_ = ddsHeader.dwHeight;
	header.numMipLevels_ = std::max< uint32 >( ddsHeader.dwMipMapCount, 1 );
	header.format_ = DDSHelper::getFormat( ddsHeader.ddspf );
	header.isFlatTexture_ = DDSHelper::isFlatTexture( ddsHeader );

	/* Special case for hacked DX9 formats
	from http://aras-p.info/texts/D3D9GPUHacks.html#3dc

	Thing to keep in mind: when DX9 allocates the mip chain, they check if the 
	format is a known compressed format and allocate the appropriate space for
	the smallest mip levels.
		
	For example, a 1x1 DXT1 compressed level actually takes up 8 bytes, as the 
	block size is fixed at 4x4 texels. This is true for all block compressed 
	formats.
		
	Now when using the hacked formats DX9 doesn't know it's a block 
	compression format and will only allocate the number of bytes the mip 
	would have taken, if it weren't compressed.
		
	For example a 1x1 ATI1n format will only have 1 byte allocated.
	What you need to do is to stop the mip chain before the size of the 
	either dimension shrinks below the block dimensions otherwise you risk 
	having memory corruption.
	*/
	if (header.format_ == ((D3DFORMAT)MAKEFOURCC( 'A', 'T', 'I', '1' )) || 
		header.format_ == ((D3DFORMAT)MAKEFOURCC( 'A', 'T', 'I', '2' )))
	{
		uint32 numTailMipsToDrop = header.numMipLevels_;
		uint32 tWidth = header.width_;
		uint32 tHeight = header.height_;
		
		// continue until we hit mip with width or hight less than 4 texels
		while (tWidth >= 4 && 
			tHeight >= 4 && 
			numTailMipsToDrop > 0)
		{
			numTailMipsToDrop--;
			tWidth /= 2;
			tHeight /= 2;
		}
		header.numMipLevels_ -= numTailMipsToDrop;

		if (header.numMipLevels_ == 0)
		{
			// we should only hit this branch if given ATI1N or ATI2N texture 
			// dimensions are less than 4x4 bytes after we applied mipskip
			// if this is the case then we can't use direct dds loading
			// due to possible memory corruption issues which are described
			// above fall back to d3dx which is will throw an asset error
			header.valid_ = false;
			return false;
		}
	}

	pStream->setOffset( streamBegin );

	return true;
}

bool StreamingTextureLoader::loadTextureFromStream(
	IFileStreamer * pStream, 
	const HeaderInfo & header, 
	const LoadOptions & options, 
	LoadResult & result )
{
	MF_ASSERT( header.valid_ );
	MF_ASSERT( result.texture2D_.hasComObject() );

	// Must not request to load nothing! (calling function has the header and
	// destination texture, and should have adjusted for this)
	MF_ASSERT( options.mipSkip_ < header.numMipLevels_ );

	// Calculate the amount of data to skip in the file depending on the 
	// number of mipmaps we want to skip
	uint32 w = header.width_;
	uint32 h = header.height_;

	// Magic number + DDS header
	uint32 fileSkip = sizeof(DWORD) + sizeof(DDS_HEADER); 
	for (uint32 i = 0; i < options.mipSkip_ && i < header.numMipLevels_; i++)
	{
		UINT skipBytes = 0;
		DDSHelper::getSurfaceInfo( w, h, header.format_, &skipBytes, NULL, 
			NULL );
		w = std::max< uint32 >( 1, w >> 1 );
		h = std::max< uint32 >( 1, h >> 1 );

		fileSkip += skipBytes;
	}

	if (fileSkip > 0)
	{
		pStream->skip( fileSkip );
	}
	
	// Calculate the number of mips to load
	uint32 numMipsToLoad = options.mipSkip_ < header.numMipLevels_ ? 
		header.numMipLevels_ - options.mipSkip_ : 1;
	numMipsToLoad = std::min< uint32 >( numMipsToLoad, options.numMips_ );

	Moo::TextureLockWrapper texLock( result.texture2D_ );

	// Load up the mipmaps
	for (uint32 i = 0; i < numMipsToLoad; i++)
	{
		UINT mipBytes = 0;
		UINT rowSize = 0;
		UINT rowCount = 0;
		D3DLOCKED_RECT lockedRect;

		DDSHelper::getSurfaceInfo( w, h, header.format_, &mipBytes, &rowSize, 
			&rowCount );
			
		texLock.lockRect( i, &lockedRect, NULL, 0 );

		bool bSuccessRead = true;
		if (lockedRect.Pitch * rowCount == mipBytes)
		{
			size_t bytesRead = pStream->read( mipBytes, lockedRect.pBits );
			bSuccessRead = bSuccessRead && (mipBytes == bytesRead);
		}
		else
		{
			uint8 * dest = static_cast< uint8 * >( lockedRect.pBits );
			for (uint32 j = 0; j < rowCount && bSuccessRead; j++)
			{
				size_t bytesRead = pStream->read( rowSize, dest );
				bSuccessRead = bSuccessRead && (rowSize == bytesRead);
				dest += lockedRect.Pitch;
			}
		}

		texLock.unlockRect( i );

		w = std::max< uint32 >( 1, w >> 1 );
		h = std::max< uint32 >( 1, h >> 1 );

		if (!bSuccessRead)
		{
			// dds file is corrupted or just ended too early ?
			return false;
		}
	}

	// all good, return texture
	return result.texture2D_.hasComObject();
}

bool StreamingTextureLoader::copyMipChain( DeviceTexture2D * pDstTex, 
	uint32 dstBeginLevel, DeviceTexture2D * pSrcTex, 
	uint32 srcBeginLevel, uint32 numLevels )
{
	for (UINT i = 0; i < numLevels; ++i)
	{
		DX::Surface * pSrcSurf = NULL;
		DX::Surface * pDstSurf = NULL;

		// get this level's original surface
		pSrcTex->GetSurfaceLevel( i + srcBeginLevel, &pSrcSurf );

		// get this level's new surface
		pDstTex->GetSurfaceLevel( i + dstBeginLevel, &pDstSurf );

		// Verify that the surfaces are correct for eachother
		D3DSURFACE_DESC srcSurfDesc;
		pSrcSurf->GetDesc( &srcSurfDesc );

#if !CONSUMER_CLIENT_BUILD
		D3DSURFACE_DESC dstSurfDesc;
		pDstSurf->GetDesc( &dstSurfDesc );

		MF_ASSERT( srcSurfDesc.Width == dstSurfDesc.Width );
		MF_ASSERT( srcSurfDesc.Height == dstSurfDesc.Height );
		MF_ASSERT( srcSurfDesc.Format == dstSurfDesc.Format );
		MF_ASSERT( srcSurfDesc.Pool == dstSurfDesc.Pool );
#endif

		// Copy between the surfaces
		HRESULT hr;
		if (srcSurfDesc.Pool == D3DPOOL_DEFAULT)
		{
			hr = Moo::rc().deviceEx()->StretchRect( pSrcSurf, NULL, pDstSurf, 
				NULL, D3DTEXF_NONE );
		}
		else
		{
			hr = D3DXLoadSurfaceFromSurface(
				pDstSurf, 
				0, 
				0, 
				pSrcSurf, 
				0, 
				0, 
				D3DX_DEFAULT, 
				0);
		}
		
		pSrcSurf->Release();
		pDstSurf->Release();

		if (FAILED( hr ))
		{
			return false;
		}
	}

	return true;
}

StreamingTextureLoader::HeaderInfo::HeaderInfo() : 
	valid_( false ),
	width_( 0 ), 
	height_( 0 ),
	numMipLevels_( 0 ),
	format_( D3DFMT_UNKNOWN ),
	isFlatTexture_( false )
{

}

StreamingTextureLoader::LoadOptions::LoadOptions() : 
	mipSkip_( 0 ),
	numMips_( uint32(-1) )
{

}

} // namespace Moo

BW_END_NAMESPACE

// streaming_texture.cpp
