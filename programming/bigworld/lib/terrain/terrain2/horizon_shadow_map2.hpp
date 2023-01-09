#ifndef TERRAIN_HORIZON_SHADOW_MAP2_HPP
#define TERRAIN_HORIZON_SHADOW_MAP2_HPP

#include "../horizon_shadow_map.hpp"
#include "moo/texture_lock_wrapper.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
    class TerrainBlock2;
}

namespace Terrain
{
    /**
     *  This class implements the HorizonShadowMap interface for the second
     *  generation of terrain.
     */
    class HorizonShadowMap2 : public HorizonShadowMap
    {
    public:
        explicit HorizonShadowMap2(TerrainBlock2* block);

//#ifdef EDITOR_ENABLED
		bool create( uint32 size, BW::string *error = NULL );
//#endif

        HorizonShadowPixel shadowAt(int32 x, int32 z) const;

        uint32 width() const;
        uint32 height() const;

        bool lock(bool readOnly);

        ImageType &image();
        ImageType const &image() const;

        bool unlock();

        bool save( DataSectionPtr ) const;
        bool load(DataSectionPtr dataSection, BW::string *error = NULL);

        BaseTerrainBlock &baseTerrainBlock();
        BaseTerrainBlock const &baseTerrainBlock() const;

        DX::Texture *texture() const;

    private:
        TerrainBlock2               *block_;
        ComObjectWrap<DX::Texture>  texture_;
		Moo::TextureLockWrapper		textureLock_;
        uint32                      width_;
        uint32                      height_;
        HorizonShadowImage          image_;
        size_t                      lockCount_;

		size_t						sizeInBytes_;
    };

    typedef SmartPointer<HorizonShadowMap2>     HorizonShadowMap2Ptr;

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_HORIZON_SHADOW_MAP2_HPP
