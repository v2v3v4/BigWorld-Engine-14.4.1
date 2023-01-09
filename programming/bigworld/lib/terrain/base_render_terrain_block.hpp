#ifndef TERRAIN_BASE_RENDER_TERRAIN_BLOCK_HPP
#define TERRAIN_BASE_RENDER_TERRAIN_BLOCK_HPP

#ifndef MF_SERVER

#include "moo/forward_declarations.hpp"
#include "math/vector3.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class Matrix;

namespace Terrain
{
	class EffectMaterial;
	class HorizonShadowMap;
}

namespace Moo
{
	class EffectMaterial;
}

namespace Terrain
{
	/**
	 *	This class is the base class for terrain blocks used in the client and
	 *	tools.  The server uses BaseTerrainBlock as the base class.  This class
	 *	implements rendering interfaces and terrain hole map interfaces.
	 */
	class BaseRenderTerrainBlock : public SafeReferenceCount
	{
	public:
        /**
         *  Render this terrain block immediately.
         *
         *  @param pMaterial The material to render with.
         */
        virtual bool draw(const Moo::EffectMaterialPtr& pMaterial ) = 0;

        /**
         *  This function returns the horizon shadow map for this block of 
         *  terrain.
		 *
		 *  @returns				The HorizonShadowMap or the terrain.
         */
        virtual HorizonShadowMap &shadowMap() = 0;

        /**
         *  This function returns the horizon shadow map for this block of 
         *  terrain as a const reference.
		 *
 		 *  @returns				The HorizonShadowMap or the terrain.
         */
        virtual HorizonShadowMap const &shadowMap() const = 0;


	};
}

BW_END_NAMESPACE

#endif // MF_SERVER

#endif // TERRAIN_BASE_RENDER_TERRAIN_BLOCK_HPP
