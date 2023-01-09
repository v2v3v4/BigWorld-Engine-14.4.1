#ifndef TERRAIN_HOLE_MAP_HPP
#define TERRAIN_HOLE_MAP_HPP

#include "terrain_map.hpp"
#include "terrain_map_holder.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
    /**
     *  This class allows access to hole data of a terrain.
     */
    class TerrainHoleMap : public TerrainMap<bool>
    {
    public:
		TerrainHoleMap( float blockSize ) : TerrainMap<bool>( blockSize ) {};
        /**
         *  This function returns whether there are no holes in the terrain.
         *
         *  @return true if there are no holes in the terrain.
         */
        virtual bool noHoles() const = 0;

        /**
         *  This function returns whether the entire terrain is holes.
         *
         *  @return true if the entire block is a hole.
         */
        virtual bool allHoles() const = 0;

        /**
         *  This function returns whether there is a hole at the specified x/z
		 *	location
		 *
		 *	@param x the local x position in the terrain block in meters
		 *	@param z the local x position in the terrain block in meters
         *
         *  @return true if there is a hole at the specified location
         */
		virtual bool holeAt( float x, float z ) const = 0;

		virtual bool holeAt( float xs, float zs, float xe, float ze ) const = 0;
    };


    /**
     *  A TerrainHoleMapIter can be used to iterate over a TerrainHoleMap.
     */
    typedef TerrainMapIter<TerrainHoleMap>      TerrainHoleMapIter;


    /**
     *  A TerrainHoleMapHolder can be used to lock/unlock a TerrainHoleMap.
     */
    typedef TerrainMapHolder<TerrainHoleMap>    TerrainHoleMapHolder;
}

BW_END_NAMESPACE

#endif // TERRAIN_HOLE_MAP_HPP
