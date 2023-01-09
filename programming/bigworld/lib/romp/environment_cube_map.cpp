#include "pch.hpp"
#include "environment_cube_map.hpp"
#include "enviro_minder.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/draw_context.hpp"

BW_BEGIN_NAMESPACE

static BasicAutoConfig< uint16 > s_ecmSize( "environment/cubeMap/size", 128 );
static BasicAutoConfig< uint8 > s_ecmNumFaceUpdatesPerFrame( "environment/cubeMap/numFaceUpdatesPerFrame", 1 );

//------------------------------------------------------------------------------
/**
 *
 */
class EnvironmentCubeMapSetter : public Moo::EffectConstantValue
{
public:
	EnvironmentCubeMapSetter( Moo::BaseTexturePtr pTexture )
	{
		BW_GUARD;
		this->cubeTexture( pTexture );
	}

	~EnvironmentCubeMapSetter()
	{
		value_ = NULL;
	}

	void cubeTexture( Moo::BaseTexturePtr pTexture )
	{
		value_ = pTexture;
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		HRESULT hr = pEffect->SetTexture( constantHandle, value_->pTexture() );
		return SUCCEEDED( hr );
	}

	Moo::BaseTexturePtr value_;	
};


//------------------------------------------------------------------------------
EnvironmentCubeMap::EnvironmentCubeMap( uint16 textureSize, uint8 numFacesPerFrame ) :
	pCubeRenderTargetEffectConstant_( NULL ),
	environmentCubeMapFace_( 0 ),
	numberOfFacesPerFrame_( numFacesPerFrame )
{	
	BW_GUARD;
	if ( textureSize == 0 )
	{
		textureSize = s_ecmSize.value();
	}

	if ( numFacesPerFrame == 0 )
	{
		numberOfFacesPerFrame_ = Math::clamp( (uint8)1, s_ecmNumFaceUpdatesPerFrame.value(), (uint8)6 );
	}

	static IncrementalNameGenerator cubeMapNames( "EnvironmentCubeMap" );

	pCubeRenderTarget_ = new Moo::CubeRenderTarget( cubeMapNames.nextName() );
	pCubeRenderTarget_->create( textureSize );

	*Moo::rc().effectVisualContext().getMapping( "EnvironmentCubeMap" ) = 
		new EnvironmentCubeMapSetter( pCubeRenderTarget_ );
}

EnvironmentCubeMap::~EnvironmentCubeMap()
{
}

//------------------------------------------------------------------------------
/**
 * This method draws the environment How many faces to draw on this draw call. Max 6, min 0.
 * If 1, it will update each face in turn, one per frame.
 * If -1, it will draw the N faces, where N is configurable
 * below.
 */
void EnvironmentCubeMap::update( float dTime, bool defaultNumFaces,
								uint8 numberOfFaces, uint32 drawSelection )
{
	BW_GUARD;
	MF_ASSERT_DEV( numberOfFaces <= 6 );

	if ( numberOfFaces == 0 )
		return;

	if ( defaultNumFaces )
		numberOfFaces = numberOfFacesPerFrame_;

	ChunkSpacePtr camSpace = ChunkManager::instance().cameraSpace();
	if ( !camSpace )
		return;

	EnviroMinder & enviro = camSpace->enviro();

	const Vector3 & center = Moo::rc().invView().applyToOrigin();
	uint32 ds;
	if (drawSelection <= EnviroMinder::DRAW_ALL)
	{
		ds = drawSelection;
	}
	else
	{
		ds = EnviroMinder::DRAW_SUN_AND_MOON | 
			EnviroMinder::DRAW_SKY_GRADIENT | 
			EnviroMinder::DRAW_SKY_BOXES;
	}

	pCubeRenderTarget_->setupProj();

	for ( int i = 0 ; i < numberOfFaces ; ++i )
	{
		D3DCUBEMAP_FACES face = static_cast< D3DCUBEMAP_FACES >( environmentCubeMapFace_ );
		if ( pCubeRenderTarget_->pushRenderSurface( face ) )
		{
			Moo::rc().device()->Clear(
				0, NULL, D3DCLEAR_ZBUFFER, 0x0, 1.f, 0 );
			pCubeRenderTarget_->setCubeViewMatrix( face, center );
			enviro.drawSkySunCloudsMoon( dTime, ds );
			pCubeRenderTarget_->pop();
		}
		if( ++environmentCubeMapFace_ == 6 )
			environmentCubeMapFace_ = 0;
	}
	pCubeRenderTarget_->restoreProj();
}

//------------------------------------------------------------------------------
/**
 * With this method, you can override the default number of faces per frame 
 * that it draws, which defaults to 1
 */
void EnvironmentCubeMap::numberOfFacesPerFrame( uint8 numberOfFaces )
{
	BW_GUARD;
	MF_ASSERT_DEV( numberOfFaces <= 6 );
	numberOfFacesPerFrame_ = numberOfFaces;
}

BW_END_NAMESPACE

// enviro_cube_map.cpp
