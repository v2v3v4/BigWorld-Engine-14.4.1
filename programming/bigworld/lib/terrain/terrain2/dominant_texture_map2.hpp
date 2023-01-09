#ifndef TERRAIN_DOMINANT_TEXTURE_MAP2_HPP
#define TERRAIN_DOMINANT_TEXTURE_MAP2_HPP

#include "../dominant_texture_map.hpp"


BW_BEGIN_NAMESPACE

namespace Terrain
{
    /**
     *  This class allows access to material data of terrain blocks.
     */
    class DominantTextureMap2 : public DominantTextureMap
    {
    public:
		DominantTextureMap2( float blockSize );

		DominantTextureMap2(
			float				blockSize,
			TextureLayers&		layerData,
			float               sizeMultiplier = 0.5f );

#ifdef EDITOR_ENABLED
		virtual bool save(DataSectionPtr) const;
#endif
		virtual bool load(DataSectionPtr dataSection, BW::string *error = NULL);

    };

} // namespace Terrain

BW_END_NAMESPACE

#endif // TERRAIN_DOMINANT_TEXTURE_MAP2_HPP
