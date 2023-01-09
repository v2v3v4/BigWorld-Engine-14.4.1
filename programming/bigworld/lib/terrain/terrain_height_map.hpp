#ifndef TERRAIN_TERRAIN_HEIGHT_MAP_HPP
#define TERRAIN_TERRAIN_HEIGHT_MAP_HPP

#include "terrain_map.hpp"
#include "terrain_map_holder.hpp"
#include "terrain_settings.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{

// This is the default value for unit scale. It is the default precision
// for height maps stored as uint16 in memory.
const float DEFAULT_UNIT_SCALE = 0.01f;

    /**
     *  This class allows access to height data of a terrain.
     */
    class TerrainHeightMap : public TerrainMap<float>
	{
    public:
        /**
         *  This function returns an iterator over the map.
         *  This function should only be called within a lock/unlock pair.
         *
         *  @param x            The x coordinate.
         *  @param y            The y coordinate.
         *  @returns            An iterator that can be used to access the
         *                      underlying image.
         */
        /*virtual*/ Iterator iterator(int32 x, int32 y);

        /**
         *  This function returns the spacing between each sample.
         *
         *  @returns            The spacing between each sample in the 
         *                      x-direction.
         */
        virtual float spacingX() const = 0;

        /**
         *  This function returns the spacing between each sample.
         *
         *  @returns            The spacing between each sample in the 
         *                      z-direction.
         */
        virtual float spacingZ() const = 0;

        /**
         *  This function returns the width of the visible part of the 
         *  TerrainMap.
         *
         *  @returns            The width of the visible part of the 
         *                      TerrainMap.
         */
        virtual uint32 blocksWidth() const = 0;

        /**
         *  This function returns the height of the visible part of the 
         *  TerrainMap.
         *
         *  @returns            The height of the visible part of the 
         *                      TerrainMap.
         */
        virtual uint32 blocksHeight() const = 0;

        /**
         *  This function returns the number of vertices wide of the visible 
         *  part of the TerrainMap.
         *
         *  @returns            The width of the visible part of the 
         *                      TerrainMap in terms of vertices.
         */
        virtual uint32 verticesWidth() const = 0;

        /**
         *  This function returns the number of vertices high of the visible 
         *  part of the TerrainMap.
         *
         *  @returns            The height of the visible part of the 
         *                      TerrainMap in terms of vertices.
         */
        virtual uint32 verticesHeight() const = 0;

        /**
         *  This function returns the width of the TerrainMap, including 
         *  non-visible portions.
         *
         *  @returns            The width of the TerrainMap, including 
         *                      non-visible portions.
         */
        virtual uint32 polesWidth() const = 0;

        /**
         *  This function returns the height of the TerrainMap, including 
         *  non-visible portions.
         *
         *  @returns            The height of the TerrainMap, including 
         *                      non-visible portions.
         */
        virtual uint32 polesHeight() const = 0;

        /**
         *  This function returns the x-offset of the visible portion of the 
         *  HeightMap.
         *
         *  @returns            The x-offset of the first visible column.
         */
        virtual uint32 xVisibleOffset() const = 0;

        /**
         *  This function returns the z-offset of the visible portion of the 
         *  HeightMap.
         *
         *  @returns            The z-offset of the first visible row.
         */
        virtual uint32 zVisibleOffset() const = 0;

        /**
         *  This function returns the minimum height in the height map.
         *
         *  @returns            The minimum height in the height map.
         */
        virtual float minHeight() const = 0;

        /**
         *  This function returns the maximum height in the height map.
         *
         *  @returns            The maximum height in the height map.
         */
        virtual float maxHeight() const = 0;

		/**
		 *  This function determines the height at the given point.
		 *
		 *  @param x            The x coordinate to get the height at.
		 *  @param z            The z coordinate to get the height at.
		 *  @returns            The height at x, z.
		 */
		virtual float heightAt(int x, int z) const = 0;

        /**
         *  This function determines the height at the given point.
         *
         *  @param x            The x coordinate to get the height at.
         *  @param z            The z coordinate to get the height at.
         *  @returns            The height at x, z.
         */
        virtual float heightAt(float x, float z) const;

        /**
         *  This function determines the normal at the given point.
         *
         *  @param x            The x coordinate to get the normal at.
         *  @param z            The z coordinate to get the normal at.
         *  @returns            The normal at x, z.
         */ 
        virtual Vector3 normalAt(int x, int z) const = 0;

        /**
         *  This function determines the normal at the given point.
         *
         *  @param x            The x coordinate to get the normal at.
         *  @param z            The z coordinate to get the normal at.
         *  @returns            The normal at x, z.
         */ 
        virtual Vector3 normalAt(float x, float z) const = 0;

        /**
         *  This function determines the slope at the given point.
         *
         *  @param x            The x coordinate to get the slope at.
         *  @param z            The z coordinate to get the slope at.
         *  @returns            The angle in degrees at the given point.
         */
        float slopeAt(int x, int z) const;

        /**
         *  This function determines the slope at the given point.
         *
         *  @param x            The x coordinate to get the slope at.
         *  @param z            The z coordinate to get the slope at.
         *  @returns            The angle in degrees at the given point.
         */
        float slopeAt(float x, float z) const;

		/**
		 * Convert to and from compressed heights, this is always in a "relative"
		 * sense, and does not take into account minHeight_.
		 */
		inline float convertHeight2Float( int32 height ) const;
		inline int32 convertFloat2Height( float height ) const;

	protected:
		TerrainHeightMap(float blockSize, float unitScale = DEFAULT_UNIT_SCALE): 
			 TerrainMap<float>( blockSize ),
			 unitScale_(unitScale) 
		{
		}
		float	unitScale_;

		/** Get the height at grid position - this will return absolute height in
		 * editor or client.
		 */
		inline float getAbsoluteHeightAt( uint32 x, uint32 y ) const;
    };

    typedef SmartPointer<TerrainHeightMap>      TerrainHeightMapPtr;
    typedef TerrainMapHolder<TerrainHeightMap>  TerrainHeightMapHolder;

	// inline definitions

	inline float TerrainHeightMap::convertHeight2Float( int32 height ) const
	{
		return float(height) * unitScale_;
	}

	inline int32 TerrainHeightMap::convertFloat2Height( float height ) const
	{
		return int32(
			floorf( ( height + 0.5f * unitScale_ ) / unitScale_ ) );
	}

	inline float TerrainHeightMap::getAbsoluteHeightAt( uint32 x, uint32 y ) const
	{
		return image().get( x, y );
	}
}

BW_END_NAMESPACE

#endif // TERRAIN_HEIGHT_MAP_HPP
