#ifndef TERRAIN_COMMON_TERRAIN_BLOCK2_HPP
#define TERRAIN_COMMON_TERRAIN_BLOCK2_HPP

#if defined( EDITOR_ENABLED )
#include "terrain/editor_base_terrain_block.hpp"
#else
#include "terrain/base_terrain_block.hpp"
#endif // defined( EDITOR_ENABLED )

#include "../base_terrain_block.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
	class TerrainHeightMap2;
	class TerrainHoleMap2;
	class DominantTextureMap2;
	class TerrainTextureLayer;

	typedef SmartPointer<TerrainHeightMap2>		TerrainHeightMap2Ptr;
	typedef SmartPointer<TerrainHoleMap2>		TerrainHoleMap2Ptr;
	typedef SmartPointer<DominantTextureMap2>	DominantTextureMap2Ptr;
	typedef SmartPointer<TerrainTextureLayer>	TerrainTextureLayerPtr;
}

namespace Terrain
{
#ifdef EDITOR_ENABLED
	typedef EditorBaseTerrainBlock		CommonTerrainBlock2Base;
#else
	typedef BaseTerrainBlock			CommonTerrainBlock2Base;
#endif // EDITOR_ENABLED

    /**
     *  This class implements the BaseTerrainBlock interface for the
	 *	second generation of terrain.
     */
    class CommonTerrainBlock2 : public CommonTerrainBlock2Base
    {
    public:
        CommonTerrainBlock2(TerrainSettingsPtr pSettings);
        virtual ~CommonTerrainBlock2();

        bool load(	BW::string const &filename, 
					Matrix  const &worldTransform,
					Vector3 const &cameraPosition,
					BW::string *error = NULL );

        TerrainHeightMap &heightMap();
        TerrainHeightMap const &heightMap() const;

        TerrainHoleMap &holeMap();
        TerrainHoleMap const &holeMap() const;
		uint32 holesMapSize() const;
		uint32 holesSize() const;

		DominantTextureMapPtr dominantTextureMap();
        DominantTextureMapPtr const dominantTextureMap() const;

        virtual BoundingBox const &boundingBox() const;

        virtual bool collide
        (
            Vector3                 const &start, 
            Vector3                 const &end,
            TerrainCollisionCallback *callback
        ) const;

        virtual bool collide
        (
            WorldTriangle           const &start, 
            Vector3                 const &end,
            TerrainCollisionCallback *callback
        ) const;

		virtual float heightAt(float x, float z) const;
        virtual Vector3 normalAt(float x, float z) const;

		const BW::string dataSectionName() const { return "terrain2"; };

		const TerrainSettingsPtr& settings() const { return settings_; }

		DominantTextureMap2Ptr dominantTextureMap2() const;
		void dominantTextureMap2(DominantTextureMap2Ptr dominantTextureMap);

		virtual TerrainHeightMap2Ptr heightMap2() const;

		static void stripUnusedHeightSections( DataSectionPtr terrainDs,
			uint32 heightMapLodToPreserve );

	protected:
		TerrainSettingsPtr	settings_;

        bool internalLoad(
			BW::string const &filename, 
			DataSectionPtr terrainDS,
			Matrix  const &worldTransform,
			Vector3 const &cameraPosition,
			uint32 heightMapLod,
			BW::string *error = NULL);

		void heightMap2( TerrainHeightMap2Ptr heightMap2 );
		void holeMap2( TerrainHoleMap2Ptr holeMap2 );
		TerrainHoleMap2Ptr holeMap2() const;

		// data
		mutable BoundingBox         bb_;
    private:
	    TerrainHeightMap2Ptr        pHeightMap_;
        TerrainHoleMap2Ptr          pHoleMap_;
		DominantTextureMap2Ptr		pDominantTextureMap_;
    };
} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_COMMON_TERRAIN_BLOCK2_HPP
