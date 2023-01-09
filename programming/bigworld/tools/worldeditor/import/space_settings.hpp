#ifndef SPACE_SETTINGS_HPP
#define SPACE_SETTINGS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/image.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"

BW_BEGIN_NAMESPACE

/**
	*
	* Convenience class to store all settings for space.settings.file.
	* This is to ensure we don't have multiple implementations all over the space.
	* 
    */
class SpaceSettings
{
public:
	SpaceSettings( BW::string space );
	void save() const;

	void minX( int minX ) { minX_ = minX; }
	void maxX( int maxX ) { maxX_ = maxX; }
	void minY( int minY ) { minY_ = minY; }
	void maxY( int maxY ) { maxY_ = maxY; }

	void gridSize( float gridSize ) { gridSize_ = gridSize; }

	int minX() const { return minX_; }
	int maxX() const { return maxX_; }
	int minY() const { return minY_; }
	int maxY() const { return maxY_; }

	float gridSize() { return gridSize_; }

	Terrain::TerrainSettingsPtr terrainSettings() const
		{ return terrainSettings_; }

private:
	int minX_;
	int minY_;
	int maxX_;
	int maxY_;
	float gridSize_;
	Terrain::TerrainSettingsPtr terrainSettings_;
	BW::string space_;
};

BW_END_NAMESPACE

#endif // SPACE_SETTINGS_HPP
