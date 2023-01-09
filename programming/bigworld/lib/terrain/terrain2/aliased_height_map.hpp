#ifndef TERRAIN_ALIASED_HEIGHT_MAP_HPP
#define TERRAIN_ALIASED_HEIGHT_MAP_HPP

BW_BEGIN_NAMESPACE

namespace Terrain
{
    class TerrainCollisionCallback;
    class TerrainHeightMap2;
    typedef SmartPointer<TerrainHeightMap2> TerrainHeightMap2Ptr;
}

namespace Terrain
{
    /**
     *	This class implements an aliased heightmap that allows sampling of a 
     *  lower resolution of a parent TerrainHeightMap2.
     */
    class AliasedHeightMap 
    {
    public:
        AliasedHeightMap(uint32 level, TerrainHeightMap2Ptr pParent);

        float height( uint32 x, uint32 z ) const;

    private:
        uint32                  level_;
	    TerrainHeightMap2Ptr	pParent_;        
    };

    typedef SmartPointer<AliasedHeightMap> AliasedHeightMapPtr;
}

BW_END_NAMESPACE

#endif // TERRAIN_ALIASED_HEIGHT_MAP_HPP
