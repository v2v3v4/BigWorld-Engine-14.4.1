#include "pch.hpp"

#include "render_target.hpp"
#include "render_context.hpp"
#include "effect_visual_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "render_target.ipp"
#endif

namespace Moo
{

///Constructor
RenderTarget::RenderTarget( const BW::string & identitifer ) :
	resourceID_( identitifer ),
	reuseZ_( false ),
	discardDepthStencil_( true ),
	width_( 0 ),
	height_( 0 ),
	origWidth_( 0 ),
	origHeight_( 0 ),
	pixelFormat_( D3DFMT_A8R8G8B8 ),
	depthFormat_( D3DFMT_UNKNOWN ),
	autoClear_( false ),
	clearColour_( (DWORD)0x00000000 ),
	pRT2_( NULL)
{
	this->addToManager();
}


///Destructor
RenderTarget::~RenderTarget()
{
	BW_GUARD;
	this->release();
}


void RenderTarget::destroy() const
{
	// attempt to safety delete the texture
	this->tryDestroy();
}


void RenderTarget::release()
{
	BW_GUARD;
	this->deleteUnmanagedObjects();
}


/**
 *	This method creates the render targets resources.
 *
 *	Note that width and height can be specified either in absolute pixels,
 *	or they can represents multipliers of the screen size.  These dimensions
 *	automatically adjust when the screen size changes.
 *
 *	w,h = 0  : use the screen size
 *	w,h = -1 : screen size / 2
 *	w,h = -2 : screen size / 4
 *	w,h = -n : screen size / pow(2,n)
 *
 *	@param width	the desired width, in pixels, of the surface
 *	@param height	the desired height, in pixels, of the surface
 *	@param reuseMainZBuffer If true, try to use the main z buffer as the Z 
 *							buffer for this render target
 *	@param pixelFormat the desired pixel format of the render target
 *	@param pDepthStencilParent	If provided use the depth stencil surface of
 *                              this parent rather than creating a new one.
 *	@param depthFormatOverride	Format of the depth stencil surface (defaults
 *                              to D3DFMT_UNKNOWN).
 *
 *	@return true if nothing went wrong.
 *	
 */
bool RenderTarget::create( int width, int height, bool reuseMainZBuffer, 
						  D3DFORMAT pixelFormat, RenderTarget* pDepthStencilParent,
						  D3DFORMAT depthFormatOverride, bool discardDepthStencil )
{
	BW_GUARD;

	deleteUnmanagedObjects();
	reuseZ_ = reuseMainZBuffer;
	discardDepthStencil_ = discardDepthStencil;
	origWidth_ = width;
	origHeight_ = height;
	this->calculateDimensions();
	pixelFormat_ = pixelFormat;
	depthFormat_ = depthFormatOverride;
	pDepthStencilParent_ = pDepthStencilParent;
	return true;
}


/**
 *	This method calculates width_ and height_ based on
 *	the values we were originally created with.  If width_
 *	and height_ are lessequal to 0, it means use successive
 *	divisions of the dimensions of the back buffer.
 *	For example, width == -2 means quarter-size width.
 *	while height == 0 means the full height of the backbuffer.
 *	The actual width and height of such render targetrs will 
 *	therefore change when the screen is resized.
 */
void RenderTarget::calculateDimensions()
{
	int32 width = origWidth_;
	int32 height = origHeight_;

	if ( width <= 0 )
	{
		float w = Moo::rc().screenWidth();
		while ( width < 0 )
		{
			w /= 2.f;
			width++;
		}
		width = (int32)w;
	}

	if ( height <= 0 )
	{
		float h = Moo::rc().screenHeight();
		while ( height < 0 )
		{
			h /= 2.f;
			height++;
		}
		height = (int32)h;
	}

	// Creating a zero sized render target is not valid
	// must be at least 1x1 in D3D9, otherwise you get
	// errors from the D3D9 runtime
	width_ = (uint32)max(1, width);
	height_ = (uint32)max(1, height);
}


/**
 *	This method allocates the underlying video resources, if necessary
 *	and returns true if the render target is now in a valid state for use.
 *
 *	@Return true if the render target is now in a valid state for use.
 */
bool RenderTarget::ensureAllocated()
{
	BW_GUARD;

	if ( !pRenderTarget_.hasComObject() && width_ > 0 && height_ > 0 )
	{
		this->allocate();
	}
	return (pRenderTarget_.hasComObject());
}


/**
 *	This method pushes this render target as the current target
 *	for the device.
 *
 *	The current camera, projection matrix and viewport are saved.
 *
 *	returns true if the push was successful
 */
bool RenderTarget::push()
{
	BW_GUARD;

	if ( !ensureAllocated() )
		return false;

	DX::Surface* pDepthTarget = NULL;
	if (pDepthStencilParent_.hasObject())
	{
		pDepthTarget = pDepthStencilParent_->depthBuffer();
	}
	else
	{
		pDepthTarget = pDepthStencilTarget_.pComObject();
	}


	MF_ASSERT_DEV( pRenderTarget_.hasComObject() );
	MF_ASSERT_DEV( pDepthTarget );

	if ( !pRenderTarget_.hasComObject() )
		return false;
	if ( !pDepthTarget )
		return false;

	if (!rc().pushRenderTarget())
		return false;

	//now, push the render target

	//get the top-level surface
	ComObjectWrap<DX::Surface> pSurface;
	HRESULT hr = pRenderTarget_->GetSurfaceLevel( 0, &pSurface );
	if ( FAILED( hr ) )
	{
		rc().popRenderTarget();
		WARNING_MSG( "RenderTarget::push : Could not get surface level 0 for render target texture\n" );
		return false;
	}

	//set the render target
	hr = Moo::rc().setRenderTarget( 0, pSurface );
	if ( FAILED( hr ) )
	{
		WARNING_MSG( "RenderTarget::push : Unable to set render target on device\n" );
		pSurface = NULL;
		rc().popRenderTarget();
		return false;
	}

	hr = Moo::rc().device()->SetDepthStencilSurface( pDepthTarget );
	if ( FAILED( hr ) )
	{
		WARNING_MSG( "RenderTarget::push : Unable to set depth target on device\n" );
		rc().popRenderTarget();
		return false;
	}

	//release the pSurface reference from the GetSurfaceLevel call
	pSurface = NULL;

	Moo::rc().screenWidth( width_ );
	Moo::rc().screenHeight( height_ );

	// Screen resolution changed, update screen constants
	Moo::rc().effectVisualContext().updateSharedConstants( 
		Moo::CONSTANTS_PER_SCREEN );

	if (pRT2_)
	{
		if (!pRT2_->pTexture())
		{
			pRT2_->createUnmanagedObjects();
		}
		IF_NOT_MF_ASSERT_DEV( pRT2_->pTexture() )
		{
			rc().popRenderTarget();
			return false;
		}
		ComObjectWrap<DX::Surface> pSurface2;
		HRESULT hr = ((DX::Texture*)pRT2_->pTexture())->GetSurfaceLevel( 0, &pSurface2 );
		if ( FAILED( hr ) )
		{
			WARNING_MSG( "Failed to get second RT surface\n" );
			rc().popRenderTarget();
			return false;
		}

		//set the render target
		hr = Moo::rc().setRenderTarget( 1, pSurface2 );
		if ( FAILED( hr ) )
		{
			WARNING_MSG( "Failed to set second RT\n" );
			rc().popRenderTarget();
			pSurface2 = NULL;
			return false;
		}
		Moo::rc().setWriteMask( 1, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_ALPHA );
		//release the pSurface2 reference from the GetSurfaceLevel call
		pSurface2 = NULL;
	}
	else
	{
		Moo::rc().setRenderTarget( 1, NULL );
	}

	// Render target objects only support 2 render targets.
	// Set all others to NULL.
	// May introduce a future RenderTargetGroup to support more.
	for (uint32 i = 2; i < MAX_CONCURRENT_RTS; ++i) 
	{ 
		Moo::rc().setRenderTarget( i, NULL ); 
	}

	return true;
}


/**
 *	This method pops the RenderTarget, and restores the
 *	camera, projection matrix and viewport
 */
void RenderTarget::pop()
{
	BW_GUARD;
	rc().popRenderTarget();
	Moo::rc().effectVisualContext().updateSharedConstants( 
		Moo::CONSTANTS_PER_SCREEN );
}


bool RenderTarget::valid()
{
	BW_GUARD;
	return ( width_ > 0 && height_ > 0 );
}


void RenderTarget::deleteUnmanagedObjects( )
{
	BW_GUARD;

	if (pRenderTarget_.hasComObject())
	{
		pRenderTarget_ = NULL;
	}

	if (pDepthStencilTarget_.hasComObject())
	{
		pDepthStencilTarget_ = NULL;
	}
}


void RenderTarget::allocate( )
{
	BW_GUARD;
	if ( width_ <= 0 || height_ <= 0 )
	{
		//no need to set up the render target yet
		return;
	}

	if ( pRenderTarget_.hasComObject() )
	{
		//probably this has been set up already during
		//the render target owner's createUnmanaged call.
		return;
	}

	MF_ASSERT( height_ <= Moo::rc().deviceInfo(0).caps_.MaxTextureHeight )
	MF_ASSERT( width_ <= Moo::rc().deviceInfo(0).caps_.MaxTextureWidth )

	ComObjectWrap<DX::Texture> pTargetTexture;

	//Because the screen may have resized, and because we may have been created
	//as perecentages of the screen size, we need to recalculate our actual
	//dimensions here.
	this->calculateDimensions();

	//The PC can only create standard 32bit colour render targets.
	pTargetTexture =  Moo::rc().createTexture(
		width_, height_, 1, D3DUSAGE_RENDERTARGET, pixelFormat_, D3DPOOL_DEFAULT,
		resourceID_.empty()? "texture/render target/unknown" : ("texture/render target/" + resourceID_).c_str() );

	if (!pTargetTexture)
	{
		WARNING_MSG( "RenderTarget : Could not create render target texture\n" );

		return;
	}

	pRenderTarget_ = pTargetTexture;

	if (!pDepthStencilParent_.hasObject())
	{

		D3DFORMAT depthStencilFormat = Moo::rc().presentParameters().AutoDepthStencilFormat;

		//overriding depth format...
		if (depthFormat_ != D3DFMT_UNKNOWN) 
		{
			depthStencilFormat = depthFormat_;
		}

		DX::Surface* pTargetSurface = NULL;
		HRESULT hr;
		if ( reuseZ_ )
		{
			hr = Moo::rc().device()->GetDepthStencilSurface( &pTargetSurface );
			pDepthStencilTarget_ = pTargetSurface;
			pDepthStencilTarget_->Release();
		}
		else	
		{
			hr = Moo::rc().device()->CreateDepthStencilSurface(
					width_,
					height_,
					depthStencilFormat,
					D3DMULTISAMPLE_NONE, 0, discardDepthStencil_,
					&pTargetSurface, NULL );

			if (!FAILED(hr))
			{
				pDepthStencilTarget_ = pTargetSurface;
#if ENABLE_RESOURCE_COUNTERS
				pDepthStencilTarget_.addAlloc( BW::StringRef( "RenderTarget" ) );
#endif
				pDepthStencilTarget_->Release();
			}
		}

		if (FAILED(hr))
		{
			WARNING_MSG( "RenderTarget : Could not create depth stencil surface.\n" );
			this->deleteUnmanagedObjects();
			return;
		}
	}

	if (pRenderTarget_.hasComObject() != NULL && autoClear_)
	{
		this->push();
		rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, clearColour_, 1, 0 );
		this->pop();
	}
}


/**
 *	This method tells the render target whether or not to 
 *	clear its surface automatically upon a device recreation.
 *
 *	By default, render targets are left uninitialised after
 *	changing the screen size or switching to/from fullscreen.
 *
 *	@param	enable		True to enable auto-clear
 *	@param	col			Colour to clear the colour buffer to
 */
void RenderTarget::clearOnRecreate( bool enable, const Colour& col )
{
	autoClear_ = enable;
	clearColour_ = col;
}


/**
 *	This method retrieves the render target drawing surface.
 *
 *	@param	ret			returned surface pointer, wrapped up
 *						safely using ComObjectWrap
 *	@return	HRESULT		directX return code for GetSurfaceLevel
 */
HRESULT RenderTarget::pSurface( ComObjectWrap<DX::Surface>& ret )
{
	HRESULT hr = E_FAIL;

	if ( ensureAllocated() )
	{
		hr = pRenderTarget_->GetSurfaceLevel( 0, &ret );
	}

	if (FAILED(hr))
	{
		ERROR_MSG( "RenderTarget::pSurface - Error getting surface level 0 "
			": %s\n", DX::errorAsString( hr ).c_str() );
	}

	return hr;
}


/**
 *	This method returns the video memory used by the render target
 *	this includes any memory used by the depth buffer if the render target
 *	has a unique depth buffer
 */
INLINE uint32 RenderTarget::textureMemoryUsed( ) const
{
	uint32 memoryUsed = 0;

	if( pRenderTarget_.hasComObject() )
	{
		// Get memory used by render target
		D3DSURFACE_DESC desc;
		HRESULT hr = pRenderTarget_->GetLevelDesc( 0, &desc );
		if (SUCCEEDED(hr))
		{
			memoryUsed += DX::surfaceSize(desc);
		}

		// If we have our own depth buffer include the memory used by it
		if (!reuseZ_ && pDepthStencilTarget_.hasComObject())
		{
			hr = pDepthStencilTarget_->GetDesc( &desc );
			if (SUCCEEDED(hr))
			{
				memoryUsed += DX::surfaceSize(desc);
			}
		}
	}

	return memoryUsed;
}


/**
 *	This method uses StretchRect to copy the top surface from the given texture
 *	into the render target surface.
 *
 *	Currently no mip-map levels are supported.
 *
 *	@param	pTexture		The texture to copy.
 *	@return	bool			Success status.
 */
bool RenderTarget::copyTexture( Moo::BaseTexturePtr pTexture )
{
	HRESULT hr;
	ComObjectWrap<DX::Surface> pSrc;
	ComObjectWrap<DX::Surface> pDest;
	hr = this->pSurface(pDest);
	if (SUCCEEDED(hr))
	{
		if ( pTexture->pTexture()->GetType() == D3DRTYPE_TEXTURE )
		{
			ComObjectWrap<DX::Texture> pTex;
			pTexture->pTexture()->QueryInterface( IID_IDirect3DTexture9, (void**)&pTex );
			hr = pTex->GetSurfaceLevel(0, &pSrc);
			if (SUCCEEDED(hr))
			{
				hr = Moo::rc().device()->StretchRect(
					pSrc.pComObject(), NULL, pDest.pComObject(), NULL, D3DTEXF_LINEAR );
				if (SUCCEEDED(hr))
				{
					return true;
				}
				else
				{
					WARNING_MSG( "RenderTarget::copyTexture - could not perform stretch rect ( error %lx:%s )\n", hr, DX::errorAsString(hr).c_str() );
				}
			}
			else
			{
				WARNING_MSG( "RenderTarget::copyTexture - failed to GetSurfaceLevel 0 for source render target ( error %lx:%s )\n", hr, DX::errorAsString(hr).c_str() );
			}
		}
		else
		{
			WARNING_MSG( "RenderTarget::copyTexture - secondary texture is not type Texture (probably of type cube or volume texture) ( error %lx:%s )\n", hr, DX::errorAsString(hr).c_str() );
		}
	}
	else
	{
		WARNING_MSG( "RenderTarget::copyTexture - failed to GetSurfaceLevel 0 for dest render target ( error %lx:%s )\n", hr, DX::errorAsString(hr).c_str() );
	}

	return false;
}


/**
 * RenderTargetSetter is an effect constant binding that holds a reference
 * to a render target and sets its texture on effects on demand.
 */
RenderTargetSetter::RenderTargetSetter( RenderTarget* rt, DX::BaseTexture* b ):
  pRT_( rt ),
  backup_( b )
{
}


bool RenderTargetSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	if ( pRT_ )
	{
		HRESULT hr = pEffect->SetTexture( constantHandle, pRT_->pTexture() );
		return ( hr == S_OK );
	}
	else if ( backup_ )
	{
		HRESULT hr = pEffect->SetTexture( constantHandle, backup_.pComObject() );
		return ( hr == S_OK );
	}
	return false;
}


void RenderTargetSetter::renderTarget( RenderTarget* rt )
{
	pRT_ = rt;
}


} // namespace Moo

BW_END_NAMESPACE

// render_target.cpp
