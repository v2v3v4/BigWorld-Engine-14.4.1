#ifndef TERRAIN_TERRAIN_MAP_ITER_HPP
#define TERRAIN_TERRAIN_MAP_ITER_HPP

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
    /** 
     *  This class provides iteration over a TerrainMap.
     *
     *  The compiler generated copy constructor and assignment are okay.
     */
    template<typename TERRAINMAP>
    class TerrainMapIter
    {
    public:
        typedef typename TERRAINMAP::PixelType PixelType;

        /**
         *  This constructs a terrain map iterator.
         *  
         *  @param terrainMap   The TerrainMap to iterate over.
         *  @param x            The x coordinate.
         *  @param y            The y coordinate.
         *  @param dx           A x-offset applied to coordinates within the
         *                      TerrainMap.
         *  @param dy           A y-offset applied to coordinates within the
         *                      TerrainMap.
         */
        TerrainMapIter
        (
            TERRAINMAP          &terrainMap, 
            int32               x, 
            int32               y, 
            int32               dx  = 0, 
            int32               dy  = 0
        );

        /**
         *  This function returns the x coordinate.
         *
         *  @returns            The x coordinate of the iterator.
         */
        int32 x() const;

        /**
         *  This function returns the x coordinate.
         *
         *  @returns            The x coordinate of the iterator.
         */
        int32 y() const;

        /**
         *  This function moves the iterator by the given amount.
         *
         *  @param dx           The change in x.
         *  @param dy           The change in y.
         */
        void move(int32 dx, int32 dy);

        /**
         *  This returns the value of the pixel under the iterator.
         *
         *  @returns            The pixel under the iterator.
         */
        PixelType get() const;

        /**
         *  This returns the value of the pixel under the iterator.
         *
         *  @param p            The pixel to set.
         */
        void set(PixelType const &p);

        /**
         *  This function returns the TerrainMap that we are iterating over.
         *
         *  @returns            The TerrainMap that we are iterator over.
         */
        TERRAINMAP &terrainMap();

        /**
         *  This function returns the TerrainMap that we are iterating over.
         *
         *  @returns            The TerrainMap that we are iterator over.
         */
        TERRAINMAP const &terrainMap() const;

    private:
        TERRAINMAP      *terrainMap_;
        int32           x_;
        int32           y_;
        int32           dx_;
        int32           dy_;
    };
}

/** 
 *  Implementation of TerrainMapIter
 */ 

template<typename TERRAINMAP>
inline Terrain::TerrainMapIter<TERRAINMAP>::TerrainMapIter
(
    TERRAINMAP          &terrainMap,
    int32               x,
    int32               y,
    int32               dx  /*= 0*/,
    int32               dy  /*= 0*/
) :
    terrainMap_(&terrainMap),
    x_(x),
    y_(y),
    dx_(dx),
    dy_(dy)
{
}


template<typename TERRAINMAP>
inline int32 Terrain::TerrainMapIter<TERRAINMAP>::x() const
{
    return x_;
}


template<typename TERRAINMAP>
inline int32 Terrain::TerrainMapIter<TERRAINMAP>::y() const
{
    return y_;
}


template<typename TERRAINMAP>
inline void Terrain::TerrainMapIter<TERRAINMAP>::move(int32 dx, int32 dy)
{
    x_ += dx; 
    y_ += dy;
}


template<typename TERRAINMAP>
inline typename Terrain::TerrainMapIter<TERRAINMAP>::PixelType Terrain::TerrainMapIter<TERRAINMAP>::get() const
{
    return terrainMap_->image().get(x_ + dx_, y_ + dy_);
}


template<typename TERRAINMAP>
inline void Terrain::TerrainMapIter<TERRAINMAP>::set(PixelType const &p)
{
    terrainMap_->image().set(x_ + dx_, y_ + dy_, p);
}


template<typename TERRAINMAP>
inline TERRAINMAP &Terrain::TerrainMapIter<TERRAINMAP>::terrainMap()
{
    return *terrainMap_;
}


template<typename TERRAINMAP>
inline TERRAINMAP const &Terrain::TerrainMapIter<TERRAINMAP>::terrainMap() const
{
    return *terrainMap_;
}

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_MAP_ITER_HPP
