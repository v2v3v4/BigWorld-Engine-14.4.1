#include "pch.hpp"
#include "worldeditor/terrain/editor_chunk_terrain_projector.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/vertex_declaration.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "terrain/terrain_settings.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "EditorChunkTerrainProjector", 2 );

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EditorChunkTerrainProjector
// -----------------------------------------------------------------------------

// explicit casting method for float to dword bitwise conversion (ish).
inline DWORD toDWORD( float f )
{
	return *(DWORD*)(&f);
}

EditorChunkTerrainProjector& EditorChunkTerrainProjector::instance()
{
	static EditorChunkTerrainProjector s_projector;

	return s_projector;
}


void EditorChunkTerrainProjector::fini()
{
	if (material_)
	{
		material_ = NULL;
	}
}


/**
 * Constructor.
 */
EditorChunkTerrainProjector::EditorChunkTerrainProjector()
{
	BW_GUARD;

	// Create the effect.
	if (Moo::rc().device())
	{
		const BW::string effectFileName = "resources/materials/terrain_projector.mfm";
		material_ = new Moo::EffectMaterial;
		DataSectionPtr pSect = BWResource::openSection( effectFileName );
		if (pSect && material_->load(pSect))
		{
			ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();						
		}
		else
		{
			CRITICAL_MSG( "EditorChunkTerrainProjector::"
				"EditorChunkTerrainProjector - "
				"couldn't load effect material for terrain2"
				"%s\n", effectFileName.c_str() );			
		}
	}
		
	pDecl_ = Moo::VertexDeclaration::get( "xyznds" );
}


/**
 * This method is an alternative implementation to Terrain::TerrainRenderer::draw.
 * It projects a texture onto n terrain blocks.
 *
 * @param blocks vector of blocks to be rendered.
 */
void EditorChunkTerrainProjector::projectTexture(
	const Moo::BaseTexturePtr& pTexture,
	float scale, float rotation, const Vector3& offset, 
	D3DTEXTUREADDRESS wrapMode,	EditorChunkTerrainPtrs& spEcts, 
	bool showHoles )
{
	BW_GUARD;

	if (!pTexture || !pTexture->pTexture())
		return;

	scale_ = scale;
	rotation_ = rotation;
	offset_ = offset;
	showHoles_ = showHoles;	

	EditorChunkTerrainPtrs::iterator it = spEcts.begin();
	EditorChunkTerrainPtrs::iterator end = spEcts.end();

	BlockVector blocks;
	while ( it != end )
	{
		EditorChunkTerrainPtr spEct = *it++;
		if (spEct->chunk())
		{	// make sure terrain block hasn't disappeared from chunk in the mean time
			blocks.push_back( std::make_pair( spEct->chunk()->transform(), spEct ) );
		}
	}

	setInitialRenderStates(pTexture, wrapMode);
	renderBlocks(blocks);
}

/**
 * This method sets the initial render states, such as common material states etc.
 */
void EditorChunkTerrainProjector::setInitialRenderStates
(
	const Moo::BaseTexturePtr&	pTexture, 
	D3DTEXTUREADDRESS			wrapMode 
)
{
	BW_GUARD;

	Moo::rc().effectVisualContext().initConstants();
	Moo::rc().setVertexDeclaration( pDecl_->declaration() );
	ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();	
	pEffect->SetMatrix( pEffect->GetParameterByName(NULL,"view"), &Moo::rc().view() );
	pEffect->SetMatrix( pEffect->GetParameterByName(NULL,"proj"), &Moo::rc().projection() );
	pEffect->SetInt( pEffect->GetParameterByName(NULL,"wrap"), wrapMode );
	if (pTexture && pTexture->pTexture())
		pEffect->SetTexture( pEffect->GetParameterByName(NULL,"diffuse"), pTexture->pTexture() );
}


/**
 * This method calculates the texture transform for the programmable pipeline.
 */
void EditorChunkTerrainProjector::textureTransform( const Matrix& world, Matrix& ret, const Vector3& offset )
{
	BW_GUARD;

	Vector3 p = offset - Vector3(scale_*.5f, scale_*.5f, scale_*.5f);
	
	// Build a matrix that will rotate the texture around it's centre
	Matrix texrot;
	texrot.setRotateY( rotation_ );
	texrot.preTranslateBy( Vector3( -.5f, -.5f, -.5f ) );
	texrot.postTranslateBy( Vector3( .5f, .5f, .5f ) );
	
	// Build the texture xform matrix	
	ret.setScale( 1.f/scale_, 1.f, 1.f/scale_ );
	ret.postTranslateBy( Vector3( -p.x/scale_, 0, -p.z/scale_ ) );
	ret.postMultiply(texrot);
	ret.transpose();
}


/**
 * This method calculates the texture transform for the fixed function pipeline.
 */
void EditorChunkTerrainProjector::ffTextureTransform( const Matrix& world, Matrix& ret, const Vector3& offset )
{
	BW_GUARD;

	Matrix scalem;
	scalem.setScale( scale_, scale_, scale_ );

	Matrix offsetem;
	offsetem.setTranslate( offset );
	ret.multiply( offsetem, world );
	ret.preMultiply( scalem );
	ret[3] -= Vector3( 0.5f * scale_, 0.f, 0.5f * scale_ );
	ret.postMultiply( Moo::rc().view() );
	ret.invert();

	Matrix rotatem;
	rotatem.setRotateX( DEG_TO_RAD( -90 ) );
	ret.postMultiply( rotatem );
}


/**
 *	This methods renders the block vector passed in.
 *
 *	@param blocks	the blockVector to render.
 */
void EditorChunkTerrainProjector::renderBlocks(BlockVector& blocks)
{
	BW_GUARD;

	ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();
	D3DXHANDLE vp = pEffect->GetParameterByName( NULL,"viewProj" );
	D3DXHANDLE world = pEffect->GetParameterByName( NULL,"world" );	
	D3DXHANDLE fftex = pEffect->GetParameterByName( NULL,"fftextransform" );
	D3DXHANDLE tex = pEffect->GetParameterByName( NULL,"projtextransform" );

	BlockVector::iterator it = blocks.begin();
	BlockVector::iterator end = blocks.end();

	while (it != end)
	{
		Matrix& blockTransform = it->first;
		EditorChunkTerrainPtr& spChunk = it->second;

		Terrain::EditorBaseTerrainBlockPtr pBlock = &spChunk->block();	
		MF_ASSERT( Terrain::BaseTerrainRenderer::instance()->version() ==
			pBlock->getTerrainVersion() );

		Matrix tt;
		Vector3 offset = offset_ - spChunk->chunk()->transform().applyToOrigin();
		
		/* for shader technique */
		this->textureTransform( blockTransform, tt, offset);
		pEffect->SetMatrix( tex, &tt );
		pEffect->SetMatrix( vp, &Moo::rc().viewProjection() );

		/* for texture stage renderer*/			
		pEffect->SetMatrix( world, &blockTransform );			
		this->ffTextureTransform( blockTransform, tt, offset );
		pEffect->SetMatrix( fftex, &tt );

		pEffect->CommitChanges();
		
		Terrain::BaseTerrainRenderer::instance()->drawSingle( 
			Moo::RENDERING_PASS_COLOR, pBlock.getObject(), 
			blockTransform, material_ );

		it->second = NULL;
		it++;
	}

	blocks.clear();
}
BW_END_NAMESPACE

