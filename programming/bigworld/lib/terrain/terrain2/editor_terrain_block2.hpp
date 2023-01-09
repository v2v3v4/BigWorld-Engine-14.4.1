#ifndef TERRAIN_EDITOR_TERRAIN_BLOCK2_HPP
#define TERRAIN_EDITOR_TERRAIN_BLOCK2_HPP

#include "terrain_block2.hpp"
#include "terrain_height_map2.hpp"

#define TERRAINBLOCK2 EditorTerrainBlock2

BW_BEGIN_NAMESPACE

namespace Terrain
{
	class ETB2UnlockCallback;

    class EditorTerrainBlock2 : public TerrainBlock2
    {
    public:
        EditorTerrainBlock2(TerrainSettingsPtr pSettings);
		virtual ~EditorTerrainBlock2();

		bool create( DataSectionPtr settings, BW::string* error = NULL );

        bool load(	BW::string const &filename, 
					Matrix  const &worldTransform,
					Vector3 const &cameraPosition,
					BW::string *error = NULL);

		bool saveLodTexture( DataSectionPtr dataSection ) const;
        bool save(DataSectionPtr dataSection) const;
        bool saveToCData(DataSectionPtr pCDataSection) const;
        bool save(BW::string const &filename) const;

		size_t numberTextureLayers() const;
        size_t maxNumberTextureLayers() const;

        size_t insertTextureLayer(
			uint32 blendWidth = 0, uint32 blendHeight = 0 );
        bool removeTextureLayer(size_t idx);
		TerrainTextureLayer &textureLayer(size_t idx);
        TerrainTextureLayer const &textureLayer(size_t idx) const;

        virtual bool draw( const Moo::EffectMaterialPtr& pMaterial );

		void drawIgnoringHoles(bool setTextures = true) {};

		void setHeightMapDirty();

		void rebuildCombinedLayers(
			bool compressTextures = true, 
			bool generateDominantTextureMap = true,
			bool invalidateLodMap = true);

		void rebuildAOMap( uint32 newSize );
		bool importAOMap(const Moo::Image<uint8>& srcImage, uint top, uint left);
		void rebuildNormalMap( NormalMapQuality normalMapQuality, uint32 newSize = 0 );
		bool rebuildLodTexture( const Matrix& transform, uint32 newSize = 0  );
		bool rebuildVerticesResource( uint32 numVertexLods );
		bool isLodTextureDirty() const { return !pLodMap_; }
		void dirtyLodTexture();

		//-- Note: this methods just changes size of the shadow map but doesn't regenerate it, more
		//--	   over it even doesn't inform EditorTerrainChunk about need to rebuild shadow map.
		//--	   This should be handled manually from the caller code.
		bool resizeShadowMap(uint newSize);

		
		bool resizeHeightMap( uint32 size );

		virtual void stream();

		bool canDrawLodTexture() const;

		static uint32 blendBuildInterval();
		static void blendBuildInterval( uint32 interval );

		static void nextBlendBuildMark();

	private:
		void streamBlends();
		void ensureBlendsLoaded() const;
		bool saveHeightmap( DataSectionPtr dataSection ) const;

		ETB2UnlockCallback* unlockCallback_;

		static uint32 s_nextBlendBuildMark_;

		static uint32 s_blendBuildInterval_;
    };

	/**
	* This is a callback class that allows the EditorTerrainBlock2 to be notified
	* when the height map changes.
	*/
	class ETB2UnlockCallback : public TerrainHeightMap2::UnlockCallback
	{
	public:
		explicit ETB2UnlockCallback( EditorTerrainBlock2& owner )
			: owner_( &owner )
		{}

		virtual bool notify()
		{
			owner_->setHeightMapDirty();
			return true;
		}

	private:
		EditorTerrainBlock2* owner_;
	};

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_EDITOR_TERRAIN_BLOCK2_HPP
