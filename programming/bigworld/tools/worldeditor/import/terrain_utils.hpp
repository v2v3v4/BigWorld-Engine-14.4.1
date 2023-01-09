#ifndef TERRAIN_UTILS_HPP
#define TERRAIN_UTILS_HPP

#include "chunk/geometry_mapping.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/image.hpp"
#include "terrain/editor_base_terrain_block.hpp"
#include "terrain/terrain_height_map.hpp"

BW_BEGIN_NAMESPACE

namespace TerrainUtils
{
	const float MIN_TERRAIN_HEIGHT = -9950.f;
	const float MAX_TERRAIN_HEIGHT = +9950.f;

    struct TerrainFormat
    {
		TerrainFormat();
        float           poleSpacingX;   // Spacing between poles in metres in x-direction
        float           poleSpacingY;   // Spacing between poles in metres in y-direction
        float           widthM;         // Width of block in metres
        float           heightM;        // Height of block in metres
        uint32          polesWidth;     // Number of poles across entire block in x-direction
        uint32          polesHeight;    // Number of poles across entire block in y-direction 
        uint32          visOffsX;       // X-offset into block for the first visible pixel
        uint32          visOffsY;       // Y-offset into block for the first visible pixel
        uint32          blockWidth;     // The width of the visible portion of a block
        uint32          blockHeight;    // The height of the visible portion of a block
    };

	class TerrainFormatStorage
	{
	public:
		TerrainFormatStorage();

		bool hasTerrainInfo() const;
		const TerrainFormat& getTerrainInfo(
			const GeometryMapping* pMapping,
			Terrain::TerrainSettingsPtr& terrainSettings );
		void setTerrainInfo(
			Terrain::BaseTerrainBlockPtr& block );
		void clear();

	private:
		const Terrain::BaseTerrainBlockPtr forceLoadSomeTerrainBlock(
			const GeometryMapping* pMapping,
			Terrain::TerrainSettingsPtr& terrainSettings ) const;

		bool dirty_;
		TerrainFormat terrainInfo_;
	};

	Terrain::EditorBaseTerrainBlockPtr createDefaultTerrain2Block(
		Terrain::TerrainSettingsPtr& terrainSettings,
		DataSectionPtr& terrainSettingsSection );

    void terrainSize
    (
        BW::string                         const &space,
		float								&gridSize,
        int                                 &gridMinX,
        int                                 &gridMinY,
        int                                 &gridMaxX,
        int                                 &gridMaxY
    );

    struct TerrainGetInfo
    {
        Chunk                               *chunk_;
        BW::string                         chunkName_;
        Terrain::EditorBaseTerrainBlockPtr  block_;
        EditorChunkTerrain                  *chunkTerrain_;

        TerrainGetInfo();
        ~TerrainGetInfo();
    };

    bool getTerrain
    (
        int										gx,
        int										gy,
        Terrain::TerrainHeightMap::ImageType    &terrainImage,
        bool									forceToMemory,
        TerrainGetInfo							*getInfo        = NULL
    );

    void setTerrain
    (
        int										gx,
        int										gy,
        Terrain::TerrainHeightMap::ImageType    const &terrainImage,
        TerrainGetInfo							const &getInfo,
        bool									forceToDisk
    );

	void patchEdges
	(
		int32									gx,
		int32									gy,
		uint32									gridMaxX,
		uint32									gridMaxY,
		TerrainGetInfo							&getInfo,
		Terrain::TerrainHeightMap::ImageType	&terrainImage
	);

    bool isEditable
    (
        int                                 gx,
        int                                 gy
    );

    float heightAtPos
    (
        float                               gx,
        float                               gy,
        bool                                forceLoad   = false
    );

	bool
	rectanglesIntersect
	(
        int  left1, int  top1, int  right1, int  bottom1,
        int  left2, int  top2, int  right2, int  bottom2,
        int &ileft, int &itop, int &iright, int &ibottom		
	);

	bool spaceSettings
	(
		BW::string const &space,
		float		&gridSize,
		uint16      &gridWidth,
		uint16      &gridHeight,
		GridCoord   &localToWorld
	);

	bool sourceTextureHasAlpha
	(
		const BW::string fileName
	);
}

BW_END_NAMESPACE

#endif // TERRAIN_UTILS_HPP
