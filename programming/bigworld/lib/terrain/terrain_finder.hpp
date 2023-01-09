#ifndef TERRAIN_TERRAIN_FINDER_HPP
#define TERRAIN_TERRAIN_FINDER_HPP

#include "math/boundbox.hpp"
#include "math/matrix.hpp"

BW_BEGIN_NAMESPACE

namespace Terrain
{
    class BaseTerrainBlock;
}

namespace Terrain
{
    /**
     *  This class is used to find terrain at the given position.
     */
    class TerrainFinder
    {
    public:
        /**
         *  This is the return result of findOutsideBlock.
         */
        struct Details
        {
            BaseTerrainBlock        *pBlock_;
            Matrix                  const *pMatrix_;
            Matrix                  const *pInvMatrix_;

            Details();
        };

	    virtual ~TerrainFinder();

	    virtual Details findOutsideBlock(Vector3 const &pos) = 0;
    };
}

BW_END_NAMESPACE

#endif // TERRAIN_TERRAIN_FINDER_HPP
