#include "pch.hpp"
#include "common/bwlockd_connection.hpp"
#include "space_settings.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/misc/sync_mode.hpp"
#include "chunk/chunk_grid_size.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_clean_flags.hpp"
#include "terrain/terrain_data.hpp"

BW_BEGIN_NAMESPACE

SpaceSettings::SpaceSettings( BW::string space )
	: space_( space )
{
	DataSectionPtr pDS =
		BWResource::openSection( space_ + "/" + SPACE_SETTING_FILE_NAME );
	if (pDS == NULL )
	{
		return;
	}
	minX( pDS->readInt("bounds/minX",  1) );
	minY( pDS->readInt("bounds/minY",  1) );
	maxX( pDS->readInt("bounds/maxX", -1) );
	maxY( pDS->readInt("bounds/maxY", -1) );
	gridSize( pDS->readFloat("chunkSize", DEFAULT_GRID_RESOLUTION) );

	if( space != WorldManager::instance().getCurrentSpace() )
	{
		DataSectionPtr pTerrain = ChunkSpace::getTerrainSettingsDataSection( pDS );

		if( pTerrain == NULL )
		{
			return;
		}
		terrainSettings_ = new Terrain::TerrainSettings();
		terrainSettings()->init( gridSize(), pTerrain );
	}
	else
	{
		terrainSettings_ = WorldManager::instance().pTerrainSettings();
	}
}


void SpaceSettings::save() const
{
	DataSectionPtr pDS =
			BWResource::openSection(
				space_ + "/" + SPACE_SETTING_FILE_NAME, true );
	MF_ASSERT( pDS != NULL );

	DataSectionPtr pTerrain =
		ChunkSpace::getTerrainSettingsDataSection( pDS, true );
	MF_ASSERT( pTerrain != NULL );
	terrainSettings()->save( pTerrain );
	pDS->save();
}
BW_END_NAMESPACE

