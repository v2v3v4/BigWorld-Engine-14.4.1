#include "pch.hpp"
#include "moo/render_context.hpp"
#include "moo/visual_manager.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/mrt_support.hpp"

#include "distortion.hpp"
#include "water.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "cstdmf/debug.hpp"

//#define DEBUG_DISTORTION

DECLARE_DEBUG_COMPONENT2( "Romp", 0 );


BW_BEGIN_NAMESPACE

static TextureSetter *s_mapSetter_ = NULL;
Moo::RenderTargetPtr Distortion::s_pRenderTexture_ = NULL;

bool TextureSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	if (map_.hasComObject())
		pEffect->SetTexture( constantHandle, map_.pComObject() );
	else
		pEffect->SetTexture( constantHandle, NULL );
	return true;
}

void TextureSetter::map(DX::BaseTexture* pTexture )
{
	BW_GUARD;
	map_ = pTexture;
}

ComObjectWrap<DX::BaseTexture> TextureSetter::map()
{
	return map_;
}

// -----------------------------------------------------------------------------
// Section: class Distortion
// -----------------------------------------------------------------------------
Distortion::Distortion():
	inited_(false),
	watcherEnabled_(true)
#ifdef EDITOR_ENABLED
	,editorEnabled_(true)
#endif
{
	BW_GUARD;
	{
		MF_WATCH( "Client Settings/fx/Distortion/enable",
			watcherEnabled_,
			Watcher::WT_READ_WRITE,
			"Enable the distortion channel." );
	}
}


Distortion::~Distortion()
{
	BW_GUARD;
	this->finz();
}


bool Distortion::isSupported()
{
	BW_GUARD;
	if (Moo::rc().vsVersion() < 0x101)
	{
		INFO_MSG( "Distortion is not supported because the vertex shader version is not sufficient\n" );
		return false;
	}
	if (Moo::rc().psVersion() < 0x101)
	{
		INFO_MSG( "Distortion is not supported because the pixel shader version is not sufficient\n" );
		return false;
	}
	return true;
}


bool Distortion::init()
{
	BW_GUARD;
	if ( inited_ )
		return true;

	if ( !Distortion::isSupported() )
		return false;

	// These will get destroyed in EffectConstantValue::fini
	if (s_mapSetter_ == NULL)
		s_mapSetter_ = new TextureSetter();

	*Moo::rc().effectVisualContext().getMapping( "DistortionBuffer" ) = s_mapSetter_;

	if (s_pRenderTexture_ == NULL)
		s_pRenderTexture_ = new Moo::RenderTarget("DistortionRT");
	else
		s_pRenderTexture_->release();

	s_pRenderTexture_->create((uint32)Moo::rc().screenWidth(), (uint32)Moo::rc().screenHeight(), true);

	inited_ = true;
	return true;
}


void Distortion::finz()
{
	BW_GUARD;
	if ( !inited_ )
		return;	

	if (s_mapSetter_)
	{
		s_mapSetter_->map( NULL );
		s_mapSetter_ = NULL;
	}	

	s_pRenderTexture_ = NULL;

	inited_ = false;
}


/**
 *
 */
//TODO: fix the editor enabled / graphics settings
bool Distortion::isEnabled()
{
	BW_GUARD;
	bool enabled = watcherEnabled_ && (Moo::EffectManager::instance().PSVersionCap() >= 1);
#ifdef EDITOR_ENABLED
	enabled &= editorEnabled_;
#endif
	return enabled;
}

bool Distortion::recreateForD3DExDevice() const
{
	return true;
}

void Distortion::deleteUnmanagedObjects()
{
	BW_GUARD;
	finz();
	inited_ = false;	
}


bool Distortion::begin()
{
	BW_GUARD;
	if (this->isEnabled() && this->drawCount() > 0)
	{
		this->copyBackBuffer();
		return this->pushRT();
	}
	return false;
}


void Distortion::end()
{
	BW_GUARD;
	this->drawMasks();
	this->popRT();
	this->drawScene();
}


void Distortion::copyBackBuffer()
{
	BW_GUARD;
	if ( !inited_ && !init())
		return;


	// get the device
	DX::Device* pDev = Moo::rc().device();

	// save the current back buffer to our render target
	ComObjectWrap<DX::Surface> pSrc = NULL;
	ComObjectWrap<DX::Surface> pDest = NULL;
	if (!pDev || !s_pRenderTexture_->valid())
	{
		INFO_MSG( "Distortion buffer failed to copy.\n" );
		return;
	}
	pSrc = Moo::rc().getRenderTarget( 0 );
	if (!pSrc.hasComObject())
	{
		INFO_MSG(	"Distortion buffer failed to copy."
					" Unable to obtain source texture.\n");
		return;
	}

	HRESULT res = ((IDirect3DTexture9*)s_pRenderTexture_->pTexture())->GetSurfaceLevel(0,&pDest);
	if (FAILED(res))
	{
		pSrc = NULL;
		INFO_MSG(	"Distortion buffer failed to copy."
					" Unable to obtain destination texture.\n");
		return;
	}
	res = pDev->StretchRect( pSrc.pComObject(), NULL, pDest.pComObject(), NULL, D3DTEXF_NONE );
	if (FAILED(res))
	{
		INFO_MSG(	"Distortion buffer failed to copy."
					" Invalid call to StretchRect.\n");
	}
	pSrc  = NULL;
	pDest = NULL;

	//Need to remove our render target as the DistortionBuffer semantic.
	//Because in the next step of the Distortion (in draw masks) some shaders
	//Will use it, and you can have a texture bound to a sampler and a render
	//target at the same time.
	s_mapSetter_->map( NULL );
}


void Distortion::drawMasks()
{
	BW_GUARD;
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );

	Waters::instance().drawMasks();

	this->drawDistortionChannel( false );

	Moo::rc().setRenderState( 
		D3DRS_COLORWRITEENABLE, 
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE );
}


void Distortion::drawScene()
{
	BW_GUARD;
	//bind depth texture, if available
	Moo::rc().MRTSupport().bind();
	//bind distortion texture
	s_mapSetter_->map( s_pRenderTexture_->pTexture() );

	Moo::rc().setRenderState( 
		D3DRS_COLORWRITEENABLE, 
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE );	

	Waters::instance().drawDrawList( dTime_ );

	this->drawDistortionChannel();

	Moo::rc().MRTSupport().unbind();
}

uint Distortion::drawCount() const
{
	BW_GUARD;
	return Waters::instance().drawCount();
}

void Distortion::tick( float dTime )
{
	BW_GUARD;
	dTime_ = dTime;
}


void Distortion::drawDistortionChannel( bool clear )
{
	BW_GUARD;
	MF_ASSERT( isEnabled() );
}


void Distortion::setViewport()
{
	BW_GUARD;
	DX::Viewport viewport;
	viewport.Width = (DWORD)Moo::rc().screenWidth();
	viewport.Height = (DWORD)Moo::rc().screenHeight();
	viewport.MinZ = 0.f;
	viewport.MaxZ = 1.f;
	viewport.X = 0;
	viewport.Y = 0;
	Moo::rc().setViewport( &viewport );
}

BW_END_NAMESPACE

// distortion.cpp
