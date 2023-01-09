#ifndef TERRAIN_TERRAIN_TEXTURE_LAYER2_HPP
#define TERRAIN_TERRAIN_TEXTURE_LAYER2_HPP

#include "../terrain_texture_layer.hpp"
#include "moo/image.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
    class CommonTerrainBlock2;
}

namespace Terrain
{
    /**
     *  This implements the TerrainTextureLayer interface for the second
     *  generation terrain.
     */
    class TerrainTextureLayer2 : public TerrainTextureLayer
    {
    public:
        explicit TerrainTextureLayer2(
			CommonTerrainBlock2 &terrainBlock, bool loadBumpMaps = false,
			uint32 blendWidth = 0, uint32 blendHeight = 0
			);
		virtual ~TerrainTextureLayer2();

        BW::string textureName() const;
        bool textureName(BW::string const &filename);

		BW::string bumpTextureName() const;
		bool bumpTextureName(const BW::string& fileName);

#ifndef MF_SERVER
		const Moo::BaseTexturePtr& texture() const { return pTexture_;  }
		void texture(Moo::BaseTexturePtr texture, BW::string const &textureName);  

		const Moo::BaseTexturePtr& bumpTexture() const { return pBumpTexture_; }
		void bumpTexture(Moo::BaseTexturePtr texture, const BW::string& textureName);
#endif

        bool hasUVProjections() const;
        Vector4 const &uProjection() const; 
        void uProjection(Vector4 const &u);
        Vector4 const &vProjection() const;
        void vProjection(Vector4 const &v);

        uint32 width() const;
        uint32 height() const;

#ifdef EDITOR_ENABLED
		bool lock(bool readOnly);
		ImageType &image();
		bool unlock();
		bool save(DataSectionPtr) const;
#endif
        ImageType const &image() const;
        bool load(DataSectionPtr dataSection, BW::string *error = NULL);

		void onLoaded();

	    BaseTerrainBlock &baseTerrainBlock();
        BaseTerrainBlock const &baseTerrainBlock() const;	

	protected:
#ifdef EDITOR_ENABLED
		enum State
		{
			LOADING			= 1,		// Still loading
			COMPRESSED		= 2,		// Loaded, compressedBlend_ has blends
			BLENDS			= 4			// Loaded, blends_ has blends
		};

		void compressBlend();
		void decompressBlend();
		State state();

		friend class TTL2Cache;
#endif

    private:
        CommonTerrainBlock2             *terrainBlock_;
	    BW::string				        textureName_;
		BW::string						bumpTextureName_;
        TerrainTextureLayer2::ImageType blends_;
		uint32							width_;
		uint32							height_;
	    Vector4					        uProjection_;
	    Vector4					        vProjection_;
        size_t                          lockCount_;

#ifndef MF_SERVER
		bool							loadBumpMaps_;
		Moo::BaseTexturePtr				pBumpTexture_;
		Moo::BaseTexturePtr				pTexture_;
#endif

#ifdef EDITOR_ENABLED
		State							state_;
		BinaryPtr						compressedBlend_;
#endif
    };

    typedef SmartPointer<TerrainTextureLayer2>  TerrainTextureLayer2Ptr;
} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_TEXTURE_LAYER2_HPP
