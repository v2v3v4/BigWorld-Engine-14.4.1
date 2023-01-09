#include "pch.hpp"

#include "forced_lod_overlay.hpp"

#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/terrain/editor_chunk_terrain_projector.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This creates a ForcedLodOverlay.
 *
 *	@param terrain		The terrain to draw the overlay on.
 */
ForcedLodOverlay::ForcedLodOverlay( EditorChunkTerrain * terrain ) :
	terrain_(terrain)
{
	BW_GUARD;

	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_0.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_1.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_2.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_3.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_4.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_5.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_6.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_7.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_8.bmp"));
	textures_.push_back(Moo::TextureManager::instance()->get("resources/maps/gizmo/lod_lock_level_9.bmp"));
}


/**
 *	This is called to draw the overlay.
 */
void ForcedLodOverlay::render()
{
	BW_GUARD;

	// removes itself after drawing
	WorldManager::instance().removeRenderable(this);

	if (textures_.empty() || !terrain_ || !terrain_->chunk())
	{
		return;
	}

	const int forcedLod = min(terrain_->getForcedLod(), static_cast<int>(textures_.size()));
	if (forcedLod < 0)
	{
		return;
	}

	EditorChunkTerrainPtrs chunks;
	chunks.push_back(terrain_);

	const float gridSize = terrain_->chunk()->space()->gridSize();
	EditorChunkTerrainProjector::instance().projectTexture(
		textures_[forcedLod],
		gridSize,	// scale
		0.f,				// rotation
		terrain_->chunk()->transform().applyToOrigin() +
			Vector3( gridSize/2.0f, 0.f, gridSize/2.0f ),
		D3DTADDRESS_CLAMP,	// wrap mode
		chunks,			// affected chunks
		false);				// do not draw holes 
}

BW_END_NAMESPACE
