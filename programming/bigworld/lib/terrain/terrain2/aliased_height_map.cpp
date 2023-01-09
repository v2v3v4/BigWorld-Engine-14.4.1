#include "pch.hpp"
#include "aliased_height_map.hpp"

#include "terrain_height_map2.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

using namespace Terrain;

/**
 *  This is the AliasedHeightMap constructor.
 *
 *  @param level        The level that this AliasedHeightMap will represent.
 *  @param pParent      The parent TerrainHeightMap2.
 */
AliasedHeightMap::AliasedHeightMap(uint32 level, TerrainHeightMap2Ptr pParent):
    level_(level),
    pParent_(pParent)
{
}


/**
 *  This function samples the underlying TerrainHeightMap2 at this level.
 *
 *  @param x            The x coordinate.
 *  @param z            The z coordinate.
 *  @returns            The height in the parent TerrainHeightMap2 at the
 *                      given coordinates, taking into account the level of
 *                      this mip-map.
 */
float AliasedHeightMap::height( uint32 x, uint32 z ) const
{
    return pParent_->heightAt((int)(x << level_), (int)(z << level_));
}

BW_END_NAMESPACE

// aliased_height_map.cpp
