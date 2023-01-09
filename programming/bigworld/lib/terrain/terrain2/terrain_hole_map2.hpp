#ifndef TERRAIN_TERRAIN_HOLE_MAP2_HPP
#define TERRAIN_TERRAIN_HOLE_MAP2_HPP

#include "../terrain_hole_map.hpp"

#ifndef MF_SERVER
#include "moo/moo_dx.hpp"
#endif


BW_BEGIN_NAMESPACE

namespace Terrain
{
    class CommonTerrainBlock2;
}

namespace Terrain
{
    /**
     *  This is an implementation for TerrainHoleMap for the second generation
     *  of terrain.
     */
    class TerrainHoleMap2 : public TerrainHoleMap
    {
    public:
        explicit TerrainHoleMap2( CommonTerrainBlock2 &block );

        ~TerrainHoleMap2();

        bool noHoles() const;
        bool allHoles() const;

        uint32 width() const;
        uint32 height() const;

        bool lock( bool readOnly );

        ImageType &image();    
        ImageType const &image() const;

        bool unlock();

        bool save(DataSectionPtr) const;
        bool load(DataSectionPtr dataSection, BW::string *error = NULL);

		bool create( uint32 size, BW::string *error = NULL );

        BaseTerrainBlock &baseTerrainBlock();
        BaseTerrainBlock const &baseTerrainBlock() const;

#ifndef MF_SERVER
        DX::Texture *texture() const;
#endif

        uint32 holesMapSize() const;
        uint32 holesSize() const;

		bool		holeAt( float x, float z ) const;

		bool		holeAt( float xs, float zs, float xe, float ze ) const;
    protected:
#ifndef MF_SERVER
		bool createTexture();
        void updateTexture();
#endif
        void recalcHoles();

    private:
        CommonTerrainBlock2         *block_;
		Moo::Image<bool>            image_;
        uint32                      width_;
        uint32                      height_;
        size_t                      lockCount_;
        bool                        readOnly_;
        bool                        allHoles_;
        bool                        noHoles_;
#ifndef MF_SERVER
        ComObjectWrap<DX::Texture>  texture_;
#endif
        uint32                      holesSize_;
        uint32                      holesMapSize_;
    };

    typedef TerrainMapIter<TerrainHoleMap2>     TerrainHoleMap2Iter;
    typedef TerrainMapHolder<TerrainHoleMap2>   TerrainHoleMap2Holder;

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_HOLE_MAP2_HPP
