#include "pch.hpp"
#include "terrain_photographer.hpp"

#ifdef EDITOR_ENABLED

#include "terrain_renderer2.hpp"

#include "../terrain_data.hpp"
#include "../base_terrain_renderer.hpp"
#include "../base_terrain_block.hpp"
#include "moo/texture_compressor.hpp"
#include "moo/render_target.hpp"
#include "moo/effect_visual_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Terrain", 0 )


BW_BEGIN_NAMESPACE

using namespace Terrain;

class SurfaceHelper
{
public:
	typedef ComObjectWrap<DX::Surface> Surface;

	bool initOffscreenPlain( uint32 width, uint32 height, D3DFORMAT format, 
		D3DPOOL pool )
	{
		BW_GUARD;
		pSurface_ = NULL;
		HRESULT hr = Moo::rc().device()->CreateOffscreenPlainSurface( width, height,
			format, pool, &pSurface_, NULL );
		return SUCCEEDED( hr );
	}

	bool initRenderTarget( uint32 width, uint32 height, D3DFORMAT format )
	{
		BW_GUARD;
		pSurface_ = NULL;

		HRESULT hr = Moo::rc().device()->CreateRenderTarget( width, height, format,
			D3DMULTISAMPLE_NONE, 0, false, &pSurface_, NULL );

		return SUCCEEDED( hr );
	}

	bool initFromMooRenderTarget( Moo::RenderTargetPtr pTarget )
	{
		BW_GUARD;
		bool res = false;
		pSurface_ = NULL;
		DX::Texture* pTexture = (DX::Texture*)pTarget->pTexture();
		if (pTexture)
		{
			HRESULT hr = pTexture->GetSurfaceLevel( 0, &pSurface_ );
			if (SUCCEEDED(hr))
			{
				res = true;
			}
		}
		return res;
	}

	operator DX::Surface*()
	{
		return pSurface_.pComObject();
	}

	DX::Surface* operator ->()
	{
		return pSurface_.pComObject();
	}

private:
	ComObjectWrap<DX::Surface> pSurface_;
};

// -----------------------------------------------------------------------------
// Section: TerrainPhotographer
// -----------------------------------------------------------------------------

TerrainPhotographer::TerrainPhotographer(TerrainRenderer2& drawer) : drawer_(drawer)
{

}

TerrainPhotographer::~TerrainPhotographer()
{
	// dereference smart pointer
	pBasePhoto_ = NULL;
}


bool TerrainPhotographer::init( uint32 basePhotoSize )
{
	BW_GUARD;
	bool ret = true;

	// re-create base photo if its the wrong size or doesn't exist.
	if ( !pBasePhoto_ || ( pBasePhoto_->width() != basePhotoSize ) )
	{
		// pBasePhoto_ is a smart pointer, just re-assign.
		pBasePhoto_ = new Moo::RenderTarget("terrain photographer");
		ret = pBasePhoto_->create( basePhotoSize, basePhotoSize );

		if (!ret)
		{
			pBasePhoto_ = NULL;
		}
	}

	return ret;
}

bool TerrainPhotographer::photographBlock( BaseTerrainBlock*	pBlock,
										   const Matrix&		transform )
{
	BW_GUARD;
	bool res = false;
	if (pBasePhoto_)
	{
		//-- push color writing states.
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE1 );
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE2 );
		Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE3 );

		Moo::rc().setWriteMask( 0, 0xFF );
		Moo::rc().setWriteMask( 1, 0 );
		Moo::rc().setWriteMask( 2, 0 );
		Moo::rc().setWriteMask( 3, 0 );

		if (pBasePhoto_->push())
		{
			Moo::rc().device()->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0);

			// Save old texture filter setting
 			Moo::GraphicsSetting::GraphicsSettingPtr textureQualitySettingPtr = 
 				Moo::GraphicsSetting::getFromLabel( "TEXTURE_QUALITY" );
			MF_ASSERT( textureQualitySettingPtr && "Missing texture quality option" );
 			int oldTextureQualityOption = textureQualitySettingPtr->activeOption();

			// set new to highest supported option
			textureQualitySettingPtr->selectOption( 0 );

			float imageSize = pBlock->blockSize();

			Matrix projection;
			projection.orthogonalProjection( imageSize, imageSize, -5000, 5000 );
			Matrix view;
			view.lookAt( Vector3( imageSize * 0.50375f, 0, imageSize * 0.49625f ) + transform.applyToOrigin(), 
				Vector3( 0, -1, 0), Vector3( 0, 0, 1 ) );

			Moo::rc().projection( projection );	
			Moo::rc().view( view );
			Moo::rc().updateViewTransforms();

			// hide overlays.
			bool isOverlaysEnabled = Terrain::BaseTerrainRenderer::instance()->overlaysEnabled();
			Terrain::BaseTerrainRenderer::instance()->enableOverlays(false);

			//-- update shader constants.
			Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

			//-- render
			Moo::rc().push();
			res = drawer_.editorGenerateLodTexture(pBlock, transform);
			Moo::rc().pop();

			Terrain::BaseTerrainRenderer::instance()->enableOverlays(isOverlaysEnabled);

			//-- update shader constants.
			Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);

			// Reset old texture filter option
 			textureQualitySettingPtr->selectOption( oldTextureQualityOption );

			pBasePhoto_->pop();
		}

		// restore color write state.
		Moo::rc().popRenderState();
		Moo::rc().popRenderState();
		Moo::rc().popRenderState();
		Moo::rc().popRenderState();
	}

	return res;
}

/**
 *	This method copies the photograph to a texture. If supplied texture is empty, then
 *	new one is created with given format.
 *
 *	@param pDestTexture		An empty texture or an existing texture to reuse.
 *	@param destImageFormat	Format of new texture (if needed).
 *
 */
bool TerrainPhotographer::output(	ComObjectWrap<DX::Texture>&	pDestTexture,
									D3DFORMAT					destImageFormat)
{
	if (!pBasePhoto_->pTexture())
	{
		return false;
	}
	Moo::TextureCompressor	tc( (DX::Texture*)pBasePhoto_->pTexture() ); 
	if (pDestTexture.hasComObject())
	{
		tc.compress( pDestTexture );
	}
	else
	{
		pDestTexture = tc.compress( destImageFormat );
	}

	return pDestTexture.hasComObject();
}

BW_END_NAMESPACE

#endif //-- EDITOR_ENABLED

// terrain_photographer.cpp
