#include "pch.hpp"

#include "max_layers_overlay.hpp"

#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/terrain/editor_chunk_terrain_projector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This creates a MaxLayersOverlay.
 *
 *	@param pTerrain		The terrain to draw the overlay on.
 */
/*explicit*/ MaxLayersOverlay::MaxLayersOverlay( EditorChunkTerrain * pTerrain ) :
	pTerrain_( pTerrain )
{
	BW_GUARD;

	pTexture_ = Moo::TextureManager::instance()->get(
		"resources/maps/gizmo/chunkmaxtextures.tga" );
}


/**
 *	This is called to draw the overlay.
 */
void MaxLayersOverlay::render()
{
	BW_GUARD;

	// removes itself after drawing
	WorldManager::instance().removeRenderable( this );

	if (!pTexture_ || !pTerrain_ || !pTerrain_->chunk())
	{
		return;
	}

	EditorChunkTerrainPtrs spChunks;
	spChunks.push_back( pTerrain_ );

	const float gridSize = pTerrain_->chunk()->space()->gridSize();
	EditorChunkTerrainProjector::instance().projectTexture(
		pTexture_,
		gridSize,	// scale
		0.f,				// rotation
		pTerrain_->chunk()->transform().applyToOrigin() +
			Vector3( gridSize/2.0f, 0.f, gridSize/2.0f ),
		D3DTADDRESS_CLAMP,	// wrap mode
		spChunks,			// affected chunks
		false);				// do not draw holes 
}
BW_END_NAMESPACE

