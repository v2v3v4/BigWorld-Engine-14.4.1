#include "pch.hpp"
#include "z_attenuation_occluder.hpp"
#include "lens_effect_manager.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/geometrics.hpp"
#include "romp/py_texture_provider.hpp"
#include "romp/texture_feeds.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/material.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/effect_visual_context.hpp"


BW_BEGIN_NAMESPACE

static AutoConfigString s_blackTexture("system/blackBmp");	//as long as the alpha channel is fully white


ZAttenuationOccluder::ZAttenuationOccluder( DataSectionPtr config ):
	handlePool_( MAX_FLARES ),
	testBatch_( D3DPT_POINTLIST ),
	stagingBatch_( D3DPT_POINTLIST ),
	stagingmesh_( D3DPT_TRIANGLESTRIP ),
	transferMesh_( D3DPT_TRIANGLESTRIP )
{
	BW_GUARD;
	//quarter size target to draw flares into, 1/16th of the pixels.
	smallDestTarget_ = new Moo::RenderTarget( "Flare Destination Texture" );
	smallDestTarget_->create( -2, -2, false );

	//2K x 1 staging texture, for intra-frame anti hysteresis
	pStagingTexture_ = new Moo::RenderTarget( "Flare Staging Texture" );
	pStagingTexture_->clearOnRecreate( true, Moo::Colour(0,0,0,0) );
	pStagingTexture_->create( MAX_FLARES, 1, false );

	//PyTextureProviderPtr feed = PyTextureProviderPtr( new PyTextureProvider( NULL, pStagingTexture_ ), true);
	//TextureFeeds::addTextureFeed( "flareStagingTexture",  feed );

	Moo::BaseTexturePtr pBlack = Moo::TextureManager::instance()->get( s_blackTexture.value() );
	pRTSetter_ = new Moo::RenderTargetSetter( pStagingTexture_.get(), pBlack->pTexture() );
	*Moo::rc().effectVisualContext().getMapping( "FlareVisibilityMap" ) = pRTSetter_;
}


ZAttenuationOccluder::~ZAttenuationOccluder()
{
	BW_GUARD;
	*Moo::rc().effectVisualContext().getMapping( "FlareVisibilityMap" ) = NULL;
	pRTSetter_ = NULL;

	freeDrawBatches();
}


void ZAttenuationOccluder::update( LensEffectsMap& le )
{
	BW_GUARD;
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

	if ( !pBBCopy_ )
	{
		//Full-size target to perform visibility checks
		Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get( "backBufferCopy", false, true, false );
		if ( pTex.get() )
		{
			pBBCopy_ = reinterpret_cast<Moo::RenderTarget*>( pTex.get() );
			INFO_MSG( "ZAttenuationOccluder - sharing PostProcessing render target\n" );
		}
		else
		{
			pBBCopy_ = new Moo::RenderTarget( "Flare WOM Texture" );
			pBBCopy_->create( 0, 0, true );
			INFO_MSG( "ZAttenuationOccluder - using own render target\n" );
		}
	}

	testBatch_.clear();
	stagingBatch_.clear();

	this->clearDrawBatches();

	Moo::VertexXYZ testVertex;
	Moo::VertexTUV stagingVertex;

	Vector3 camPosition( Moo::rc().invView().applyToOrigin() );

	float halfRTWidth, halfRTHeight;
	if ( smallDestTarget_.get() )
	{
		halfRTWidth = smallDestTarget_->width() / 2.f;
		halfRTHeight = smallDestTarget_->height() / 2.f;
	}
	else
	{
		halfRTWidth = Moo::rc().screenWidth() / 2.f;
		halfRTHeight = Moo::rc().screenHeight() / 2.f;
	}

	//1. For each lens effect visible on screen, add a 3x3 rectangle to our batch
	//and assign it a location in the staging texture.
	LensEffectsMap::iterator it = le.begin();
	LensEffectsMap::iterator en = le.end();
	while ( it != en )
	{
		LensEffect& le = it->second;
		le.ageBy( 0.5f );

		float dist = Vector3(le.position() - camPosition).length();
		if (dist <= le.maxDistance())
		{
			Vector4 projPos( le.position(), 1.f );
			Moo::rc().viewProjection().applyPoint( projPos, projPos );

			if (projPos.w > 0.f)
			{
				float oow = 1.f / projPos.w;
				projPos.x *= oow;
				projPos.y *= oow;
				projPos.z *= oow;
				projPos.w = oow;

				//projPos now from ( -1,-1,0 .. 1,1,1 ) if on screen.
				if ( fabsf(projPos.x) <= 1.f && fabsf(projPos.y) <= 1.f && projPos.z <= 1.f )
				{
					HandlePool::Handle h = handlePool_.handleFromId( le.id() );
					DrawBatch* drawBatch = this->getDrawBatch(le);
					const FlareData& flareData = le.occlusionLevels().begin()->second;

					//first store the actual flare for later drawing
					float vizCoord = ((float)h + 0.5f) / (float)MAX_FLARES;
					Moo::VertexTLUV2 vertex[ 4 ];
					flareData.createMesh( projPos, 1.f, le.distanceToAlpha(dist), le.colour(), vertex, vizCoord, halfRTWidth, halfRTHeight );
					drawBatch->push_back(vertex[0]);
					drawBatch->push_back(vertex[1]);
					drawBatch->push_back(vertex[3]);
					drawBatch->push_back(vertex[0]);
					drawBatch->push_back(vertex[3]);
					drawBatch->push_back(vertex[2]);

					//testBatch_ contains a point list of 2x2 quads that don't need
					//any uvs, they just draw solid colours but need to use the
					//z-buffer

					//Move flare position towards camera by the zCullBias., to account for
					//geometry the lens flare may be residing within.
					Vector3 dir( le.position() - camPosition  );
					dir.normalise();
					dir *= flareData.zCullBias();
					Vector3 testPos = le.position() - dir;

					testVertex.pos_ = testPos;
					testBatch_.push_back ( testVertex );

					//Reproject the tested position, we need to know the screen coords
					//for sampling
					projPos = Vector4( testPos, 1.f );
					Moo::rc().viewProjection().applyPoint( projPos, projPos );
					//I'm guessing the adjusted vert could now sit directly on the
					//camera plane but ignoring for now...
					//MF_ASSERT( projPos.w != 0 )
					float oow = 1.f / projPos.w;
					projPos.x *= oow;
					projPos.y *= oow;
					projPos.z *= oow;
					projPos.w = oow;

					//stagingBatch_ contains a triangle list of 1x1 pixel quads.  However
					//for each destination position, there are 4 pixels, these get the
					//alpha value at each of the 4 pixels written in the step above.
					//
					//the handle value is used directly as the pixel position in the
					//staging bitmap.
					stagingVertex.pos_.set( (float)h + 0.5f, 0.5f, 0.5f, 1.f );

					//convert proj pos ( -1..+1 ) into uvs ( 0..1 )
					projPos.x = projPos.x / 2.f + 0.5f;
					projPos.y = projPos.y / 2.f + 0.5f;
					projPos.y = 1.f - projPos.y;

					//back buffer texel size
					Vector2 bbtexel;
					bbtexel.x = 1.f / Moo::rc().screenWidth();
					bbtexel.y = 1.f / Moo::rc().screenHeight();
					bbtexel *= 0.5f;	//we want sample offsets of half a texel

					//note too we are adding half a texel offset in x and y, to take into account
					//the directX rasterization / texture sampling rules.

					stagingVertex.uv_.set( bbtexel.x + projPos.x - bbtexel.x, bbtexel.y + projPos.y - bbtexel.y );
					stagingBatch_.push_back(stagingVertex);

					stagingVertex.uv_.set( bbtexel.x + projPos.x - bbtexel.x, bbtexel.y + projPos.y + bbtexel.y );
					stagingBatch_.push_back(stagingVertex);

					stagingVertex.uv_.set( bbtexel.x + projPos.x + bbtexel.x, bbtexel.y + projPos.y - bbtexel.y );
					stagingBatch_.push_back(stagingVertex);

					stagingVertex.uv_.set( bbtexel.x + projPos.x + bbtexel.x, bbtexel.y + projPos.y + bbtexel.y );
					stagingBatch_.push_back(stagingVertex);
				}
			}
		}

		++it;
	}

	if ( testBatch_.empty() )
		return;

	//Set up common render states
	Moo::Material::setVertexColour();
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR );
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, D3DZB_FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSCALEENABLE,  FALSE );
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );

	//2. Z-Read Disable : Draw the test batch to the alpha channel of the main buffer,
	//entirely black
	this->setPointSize(3.f);
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
	Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, 0x00000000 );
	testBatch_.draw();

	//3. Z-Read Enable : Draw the test batch to the alpha channel again, but in white.
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, 0x40ffa080 );
	testBatch_.draw();

	//4. Copy the alpha results into the staging texture using the staging batch.
	//we use additive blending to accumulate the results into the existing
	//attenuation buffer.
	ComObjectWrap<DX::Surface> pBB = Moo::rc().getRenderTarget(0);
	ComObjectWrap<DX::Surface> pDest;
	if (SUCCEEDED(pBBCopy_->pSurface(pDest)))
	{
		HRESULT hr = Moo::rc().device()->StretchRect(
			pBB.pComObject(), NULL, pDest.pComObject(), NULL, D3DTEXF_POINT );
	}

	if ( pStagingTexture_->push() )
	{
		//Darken render target.  fade out existing flares gradually
		Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
		Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT );	//dest - src
		Moo::rc().setRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_REVSUBTRACT );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, 0x80808080 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
			D3DCOLORWRITEENABLE_RED | 
			D3DCOLORWRITEENABLE_GREEN | 
			D3DCOLORWRITEENABLE_BLUE |
			D3DCOLORWRITEENABLE_ALPHA );
		stagingmesh_.clear();
		Geometrics::createRectMesh(Vector2(0.f,0.f),Vector2((float)MAX_FLARES,1.f),0xffffffff,false,stagingmesh_);
		stagingmesh_.draw();
		Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
		Moo::rc().setRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );

		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE );
		Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, 0xffffffff );
		this->setPointSize(1.f);
		Moo::rc().setTexture( 0, pBBCopy_->pTexture() );
		stagingBatch_.draw();
		pStagingTexture_->pop();
	}

	//5. Optional : Repeat step 2 to reset the main alpha channel to black.  This
	//may be required for the following heat-shimmer pass.
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR );
	Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, 0x00000000 );
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );
	this->setPointSize(3.f);
	testBatch_.draw();

	//Now draw them
	bool smallDestTargetPushed = false;

	if ( smallDestTarget_ )
	{
		if (smallDestTarget_->push())
		{
			Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00, 1, 0 );
			smallDestTargetPushed = true;
		}
	}

	pRTSetter_->renderTarget( pStagingTexture_.get() );

	DrawBatchMap::iterator dbmit = materialBatchList_.begin();
	DrawBatchMap::iterator dbmen = materialBatchList_.end();
	while ( dbmit != dbmen )
	{
		const BW::string& matName = dbmit->first;
		DrawBatch* drawBatch = dbmit->second;
		Moo::EffectMaterialPtr material = 
			LensEffectManager::instance().getMaterial( matName );
		if (material && material->begin())
		{
			material->beginPass(0);
			drawBatch->drawEffect();
			material->endPass();
			material->end();
		}
		++dbmit;
	}

	pRTSetter_->renderTarget( NULL );

	if ( smallDestTarget_ )
	{
		if ( smallDestTargetPushed )
		{
			smallDestTarget_->pop();
		}

		Moo::Material::setVertexColour();
		Moo::rc().setTexture( 0, smallDestTarget_->pTexture() );
		Moo::rc().setTexture( 1, NULL );
		Moo::rc().setTexture( 2, NULL );
		Moo::rc().setTexture( 3, NULL );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
		Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
		Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );

		transferMesh_.clear();
		Geometrics::createRectMesh(Vector2(0.f,0.f),Vector2(Moo::rc().screenWidth(),Moo::rc().screenHeight()),0xffffffff,false,transferMesh_);
		transferMesh_.draw();
	}

	//Set back some good defaults
	float defaultPointSize = 1.f;
	Moo::rc().setRenderState( D3DRS_POINTSIZE_MIN, *((DWORD*)&defaultPointSize));
	defaultPointSize = 64.f;
	Moo::rc().setRenderState( D3DRS_POINTSIZE_MAX, *((DWORD*)&defaultPointSize));
}


void ZAttenuationOccluder::setPointSize( float size )
{
	BW_GUARD;
	Moo::rc().setRenderState( D3DRS_POINTSIZE_MIN, *((DWORD*)&size ));
	Moo::rc().setRenderState( D3DRS_POINTSIZE_MAX, *((DWORD*)&size ));
	Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&size ));
}


void ZAttenuationOccluder::clearDrawBatches()
{
	BW_GUARD;
	DrawBatchList::iterator dbit = drawBatches_.begin();
	DrawBatchList::iterator dben = drawBatches_.end();
	while (dbit != dben)
	{
		(*dbit)->clear();
		++dbit;
	}
}


ZAttenuationOccluder::DrawBatch* ZAttenuationOccluder::getDrawBatch( const LensEffect& le )
{
	BW_GUARD;
	const BW::string& matName = le.occlusionLevels().begin()->second.material();
	BW::map<BW::string, DrawBatch*>::iterator it = materialBatchList_.find( matName );
	if (it != materialBatchList_.end())
		return it->second;
	DrawBatch* newOne = new DrawBatch;
	drawBatches_.push_back( newOne );
	materialBatchList_[matName] = newOne;
	return newOne;
}


void ZAttenuationOccluder::freeDrawBatches()
{
	BW_GUARD;
	DrawBatchList::iterator dbit = drawBatches_.begin();
	DrawBatchList::iterator dben = drawBatches_.end();
	while (dbit != dben)
	{
		bw_safe_delete(*dbit);
		++dbit;
	}
	drawBatches_.clear();
	materialBatchList_.clear();
}

BW_END_NAMESPACE

// z_attenuation_occluder.cpp
