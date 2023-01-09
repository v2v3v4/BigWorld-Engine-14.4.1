#ifndef SPACE_HEIGHT_MAP_HPP
#define SPACE_HEIGHT_MAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "terrain/terrain_height_map.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class implements a virtual height map the size of the space.
 */
class SpaceHeightMap : public ReferenceCount
{
public:
	typedef Terrain::TerrainHeightMap::ImageType Image;
	typedef Terrain::TerrainHeightMap::PixelType Pixel;

	SpaceHeightMap();
	~SpaceHeightMap();

	void prepare( const TerrainUtils::TerrainFormat &format );

	bool prepared() const;

	void clear();

	void add( float x, float y, Image& img );

	Pixel get( float x, float y ) const;

	bool getRect( float x, float y, float width, float height, Image& outImg ) const;

private:
	float invGridResolution_;
	float invPoleSpacingX_;
	float invPoleSpacingY_;
	uint32 numPolesX_;
	uint32 numPolesY_;
	uint32 visOffsX_;
	uint32 visOffsY_;
	float gridSize_;
	int minGridX_;
	int maxGridX_;
	int minGridY_;
	int maxGridY_;
	bool prepared_;

	typedef std::pair<int, int> Key;
	typedef BW::map<Key, Image> Heights;
	mutable Heights heights_;

	int gridCoordX( float spaceCoord ) const;
	int gridCoordY( float spaceCoord ) const;

	int mapCoordX( float spaceCoord, int gridCoord ) const;
	int mapCoordY( float spaceCoord, int gridCoord ) const;

	const Image* getImage( const Key& key, float x, float y ) const;
};


typedef SmartPointer<SpaceHeightMap> SpaceHeightMapPtr;

BW_END_NAMESPACE

#endif // SPACE_HEIGHT_MAP_HPP
