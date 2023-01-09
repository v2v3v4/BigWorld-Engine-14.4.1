#include "pch.hpp"
#include "texture_lock_wrapper.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
/**
 *	Texture lock wrapper to make code IDirect3DDevice9Ex device compatible
 */
TextureLockWrapper::TextureLockWrapper( const ComObjectWrap<DX::Texture>& tex ):
lockedMask_( 0 ),
dirtyMask_( 0 ),
dirtyRectsMask_( 0 ),
texture_ ( NULL )
{
	init ( tex );
}

TextureLockWrapper::~TextureLockWrapper()
{
	if (rc().device())
	{
		// flush all changes
		flush();
		// destroy staging texture
		fini();
	}
}

/**
 *	Init texture lock wrapper. Create staging texture if necessary
 *	Make sure all old data have been cleared
 */
void	TextureLockWrapper::init( const ComObjectWrap<DX::Texture>& tex )
{
	// make sure old texture doesn't have anything locked
	MF_ASSERT( lockedMask_ == 0 );
	MF_ASSERT( dirtyMask_ == 0 );
	MF_ASSERT( dirtyRectsMask_ == 0 );

	texture_ = tex;
	lockedMask_ = 0;
	dirtyMask_ = 0;
	dirtyRectsMask_ = 0;

	// do nothing if source texture is NULL
	if (!texture_.hasComObject())
	{
		return;
	}
	// source texture should have at least one level
	MF_ASSERT (tex->GetLevelCount() > 0 );

	D3DSURFACE_DESC desc;
	texture_->GetLevelDesc( 0, &desc );

	// create staging texture if source texture isn't lockable
	bool notLockable = ( desc.Pool == D3DPOOL_DEFAULT && !(desc.Usage & D3DUSAGE_DYNAMIC) );
	if (notLockable)
	{
		MF_ASSERT( !stagingTexture_.hasComObject() );
		stagingTexture_ = rc().createTexture( desc.Width, desc.Height, texture_->GetLevelCount(),
											desc.Usage, desc.Format, D3DPOOL_SYSTEMMEM );
	}
}

/**
 *	Emulates IDirect3DTexture9 LockRect behaviour for Ex and non Ex devices
 *	It will use staging texture and propagate changes back to original texture
 *  if IDirect3DDevice9Ex device is in use and original texture isn't lockable
 *  If original texture is lockable just call LockRect on it and return result
 */
HRESULT	TextureLockWrapper::lockRect( UINT level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD flags )
{
	// check lock level in storage range
	MF_ASSERT( level < MAX_LEVELS );
	// make sure this level isn't locked
	MF_ASSERT( ( lockedMask_ & ( 1 << level ) ) == 0 );
	// original texture must exist
	MF_ASSERT( texture_ );

	// Here we might want to check if this level is dirty (i.e pending a flush)
	// and actually perform the flush on the target texture. Currently
	// we just overwrite the update for this level regardless of if flushed.

	HRESULT hr;
	if (stagingTexture_.hasComObject())
	{
		// we're in Ex mode, lock staging texture
		hr =  stagingTexture_->LockRect( level, pLockedRect, pRect, flags );
	}
	else
	{
		// non ex mode, just forward lockRect to source texture
		hr = texture_->LockRect( level, pLockedRect, pRect, flags );
	}

	if (SUCCEEDED( hr ))
	{
		// set lock flag
		lockedMask_ |= 1 << level;
		// set dirty flag
		// we assume if something locks region it's gonna be changed
		// and we need to propagate this change to the original texture
		dirtyMask_ |= 1 << level;
		if (pRect)
		{
			// Make sure a dirty rect for this level doesn't already exist.
			// Otherwise, we can potentially lose rect data for a pending flush.
			// Possible enhancements are to maintain a queue of 
			// flush actions so that more than one action can be performed per
			// level during a flush of the staging texture.
			MF_ASSERT( ( dirtyRectsMask_ & (1 << level) ) == 0 );
			dirtyRects_[level] = *pRect;
			dirtyRectsMask_ |= 1 << level;
		}
	}
	return hr;
}


/**
 *	Emulates IDirect3DTexture9 UnlockRect behaviour for Ex and non Ex devices
 */
HRESULT	TextureLockWrapper::unlockRect( UINT level )
{
	// check lock level in storage range
	MF_ASSERT(level < MAX_LEVELS );
	// check if this level was locked before
	MF_ASSERT( lockedMask_ & (1 << level) );
	// original texture must exist
	MF_ASSERT( texture_ );

	HRESULT hr;
	if (stagingTexture_.hasComObject())
	{
		// we're in Ex mode, unlock staging texture
		hr =  stagingTexture_->UnlockRect( level );
	}
	else
	{
		// non ex mode, just forward unlockRect to source texture
		hr = texture_->UnlockRect( level );
	}

	if (SUCCEEDED( hr ))
	{
		// clear lock flag
		lockedMask_ ^= 1 << level;
	}
	return hr;

}


/**
 *	Copies changes from staging texture back to main texture
 *  Does nothing if there are no changes or no staging texture 
 */
void TextureLockWrapper::flush()
{
	// check if everything is unlocked
	MF_ASSERT( lockedMask_ == 0 );

	// make sure we have dirty surfaces and staging texture exists
	if (texture_.hasComObject() && stagingTexture_.hasComObject() && dirtyMask_)
	{
		DWORD numSrcLevels = texture_->GetLevelCount();

		// check if all levels are dirty and no rects are stored, then we can use UpdateTexture
		if (( 1 << numSrcLevels ) - 1 == dirtyMask_ && dirtyRectsMask_ == 0 )
		{
			HRESULT hr;

			// update all levels at once
			hr = rc().device()->UpdateTexture( stagingTexture_.pComObject(), texture_.pComObject() );
			MF_ASSERT( SUCCEEDED( hr ) );
		}
		else
		{
			// update dirty surfaces one by one, based on available stored locked rect information
			for (uint level = 0; level < numSrcLevels; ++level)
			{
				if (dirtyMask_ & (1 << level ))
				{
					// this surface is dirty, update it
					HRESULT hr;

					// collect source and destination surfaces
					IDirect3DSurface9*	srcSurf;
					hr = stagingTexture_->GetSurfaceLevel( level, &srcSurf );
					MF_ASSERT( SUCCEEDED( hr ) );

					IDirect3DSurface9*	dstSurf;
					hr = texture_->GetSurfaceLevel( level, &dstSurf );
					MF_ASSERT( SUCCEEDED( hr) );

					if (dirtyRectsMask_ & (1 << level))
					{
						// copy data using stored rect
						const RECT & srcRect = dirtyRects_[level];
						POINT destPoint;
						destPoint.x = srcRect.left;
						destPoint.y = srcRect.top;
						hr = rc().device()->UpdateSurface( srcSurf, &srcRect, dstSurf, &destPoint);
					}
					else
					{
						// just copy the entire surface level
						hr = rc().device()->UpdateSurface( srcSurf, NULL, dstSurf, NULL );
					}
					MF_ASSERT( SUCCEEDED( hr) );

					dstSurf->Release();
					srcSurf->Release();
				}
			}
		}
	}
	// clear dirty mask
	dirtyMask_ = 0;
	dirtyRectsMask_ = 0;
}

/**
 *  Destroys staging texture
 */
void	TextureLockWrapper::fini()
{
	// make sure everything is unlocked
	MF_ASSERT( lockedMask_ == 0 );

	// everything should be clean by now
	// if something is dirty this means explicit fini call without
	// calling flush first
	MF_ASSERT( dirtyMask_ == 0 );
	MF_ASSERT( dirtyRectsMask_ == 0 );

	// put staging texture to the reuse list
	if (stagingTexture_.hasComObject())
	{
		rc().putTextureToReuseList( stagingTexture_ );
		stagingTexture_ = NULL;
	}
	// drop reference to the source texture
	texture_ = NULL;
}

/**
 *	CubeTexture lock wrapper to make code IDirect3DDevice9Ex device compatible
 */
CubeTextureLockWrapper::CubeTextureLockWrapper( const ComObjectWrap<DX::CubeTexture>& tex ):
texture_ ( NULL )
{
	for (int i = 0; i < NUM_CUBE_FACES; ++i)
	{
		lockedMask_[i] = 0;
		dirtyMask_[i] = 0;
	}

	init ( tex );
}

CubeTextureLockWrapper::~CubeTextureLockWrapper()
{
	if (rc().device())
	{
		// flush all changes
		flush();
		// destroy staging texture
		fini();
	}
}

/**
 *	Init texture lock wrapper. Create staging texture if necessary
 *	Make sure all old data have been cleared
 */
void	CubeTextureLockWrapper::init( const ComObjectWrap<DX::CubeTexture>& tex )
{
	for (int i = 0; i < NUM_CUBE_FACES; ++i)
	{
		// make sure old texture doesn't have anything locked
		MF_ASSERT( lockedMask_[i] == 0 );
		lockedMask_[i] = 0;
		dirtyMask_[i] = 0;
	}
	texture_ = tex;

	// do nothing if source texture is NULL
	if (!texture_.hasComObject())
	{
		return;
	}

	// source texture should have at least one level
	MF_ASSERT (tex->GetLevelCount() > 0 );

	D3DSURFACE_DESC desc;
	texture_->GetLevelDesc( 0, &desc );
	MF_ASSERT( desc.Height == desc.Width );

	// create staging texture if source texture isn't lockable
	bool notLockable = ( desc.Pool == D3DPOOL_DEFAULT && !(desc.Usage & D3DUSAGE_DYNAMIC) );
	if (notLockable)
	{
		MF_ASSERT( !stagingTexture_.hasComObject() );
		stagingTexture_ = rc().createCubeTexture( desc.Width, texture_->GetLevelCount(),
											desc.Usage, desc.Format, D3DPOOL_SYSTEMMEM );
	}
}

/**
 *	Emulates IDirect3DTexture9 LockRect behaviour for Ex and non Ex devices
 *	It will use staging texture and propagate changes back to original texture
 *  if IDirect3DDevice9Ex device is in use and original texture isn't lockable
 *  If original texture is lockable just call LockRect on it and return result
 */
HRESULT	CubeTextureLockWrapper::lockRect( D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT* pLockedRect, DWORD flags )
{
	// we only allow 31 lock levels
	MF_ASSERT( level < 32 );
	// make sure this level isn't locked
	MF_ASSERT( ( lockedMask_[face] & ( 1 << level ) ) == 0 );
	// original texture must exist
	MF_ASSERT( texture_ );

	HRESULT hr;
	if (stagingTexture_.hasComObject())
	{
		// we're in Ex mode, lock staging texture
		hr =  stagingTexture_->LockRect( face, level, pLockedRect, NULL, flags );
	}
	else
	{
		// non ex mode, just forward lockRect to source texture
		hr = texture_->LockRect( face, level, pLockedRect, NULL, flags );
	}

	if (SUCCEEDED( hr ))
	{
		// set lock flag
		lockedMask_[face] |= 1 << level;
		// set dirty flag
		// we assume if something locks region it's gonna be changed
		// and we need to propagate this change to the original texture
		dirtyMask_[face] |= 1 << level;
	}
	return hr;
}


/**
 *	Emulates IDirect3DTexture9 UnlockRect behaviour for Ex and non Ex devices
 */
HRESULT	CubeTextureLockWrapper::unlockRect( D3DCUBEMAP_FACES face, UINT level )
{
	// we only allow 31 lock levels
	MF_ASSERT(level < 32 );
	// check if this level was locked before
	MF_ASSERT( lockedMask_[face] & (1 << level) );
	// original texture must exist
	MF_ASSERT( texture_ );

	HRESULT hr;
	if (stagingTexture_.hasComObject())
	{
		// we're in Ex mode, unlock staging texture
		hr =  stagingTexture_->UnlockRect( face, level );
	}
	else
	{
		// non ex mode, just forward unlockRect to source texture
		hr = texture_->UnlockRect( face, level );
	}

	if (SUCCEEDED( hr ))
	{
		// clear lock flag
		lockedMask_[face] ^= 1 << level;
	}
	return hr;

}


/**
 *	Copies changes from staging texture back to main texture
 *  Does nothing if there are no changes or no staging texture 
 */
void CubeTextureLockWrapper::flush()
{
	// check that everything is unlocked
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_POSITIVE_X] == 0 );
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_NEGATIVE_X] == 0 );
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_POSITIVE_Y] == 0 );
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_NEGATIVE_Y] == 0 );
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_POSITIVE_Z] == 0 );
	MF_ASSERT( lockedMask_[D3DCUBEMAP_FACE_NEGATIVE_Z] == 0 );

	// make sure we have dirty surfaces and staging texture exists
	if (texture_.hasComObject() && stagingTexture_.hasComObject() && dirtyMask_)
	{
		uint32 numSrcLevels = texture_->GetLevelCount();
		bool allDirty = true;
		uint32 requiredBits = ( 1 << numSrcLevels ) - 1;
		for (int face = 0; face < NUM_CUBE_FACES; ++face)
		{
			allDirty &= requiredBits == dirtyMask_[face];
		}
		if (allDirty)
		{
			// update all levels at once
			rc().device()->UpdateTexture( stagingTexture_.pComObject(), texture_.pComObject() );
		}
		else
		{
			// update dirty surfaces one by one
			for (int face = 0; face < NUM_CUBE_FACES; ++face)
			{
				for (uint level = 0; level < numSrcLevels; ++level)
				{
					if (dirtyMask_[face] & (1 << level ))
					{
						// this surface is dirty, update it
						HRESULT hr;
						// collect source and destination surfaces
						IDirect3DSurface9*	srcSurf;
						hr = stagingTexture_->GetCubeMapSurface( (D3DCUBEMAP_FACES)face, level, &srcSurf );
						MF_ASSERT( SUCCEEDED( hr ) );

						IDirect3DSurface9*	dstSurf;
						hr = texture_->GetCubeMapSurface( (D3DCUBEMAP_FACES)face, level, &dstSurf );
						MF_ASSERT( SUCCEEDED( hr) );
						// copy data
						hr = rc().device()->UpdateSurface( srcSurf, NULL, dstSurf, NULL );
						MF_ASSERT( SUCCEEDED( hr) );

						dstSurf->Release();
						srcSurf->Release();
					}
				}
			}
		}
	}
	// clear dirty mask
	for (int face = 0; face < NUM_CUBE_FACES; ++face)
	{
		dirtyMask_[face] = 0;
	}
}

/**
 *  Destroys staging texture
 */
void	CubeTextureLockWrapper::fini()
{
	// clear dirty mask
	for (int face = 0; face < NUM_CUBE_FACES; ++face)
	{
		// make sure everything is unlocked
		MF_ASSERT( lockedMask_[face] == 0 );
		// everything should be clean by now
		// if something is dirty this means explicit fini call without
		// calling flush first
		MF_ASSERT( dirtyMask_[face] == 0 );
	}

	// we aren't reusing cube map textures yet
	stagingTexture_ = NULL;
	// drop reference to the source texture
	texture_ = NULL;

}

} // namespace Moo

BW_END_NAMESPACE

// texture_lock_wrapper.cpp
