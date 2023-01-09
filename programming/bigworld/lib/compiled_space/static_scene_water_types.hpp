#ifndef STATIC_SCENE_WATER_TYPES_HPP
#define STATIC_SCENE_WATER_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "romp/water.hpp"

#include "string_table_types.hpp"

namespace BW {
namespace CompiledSpace {

	namespace StaticSceneWaterTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWWa" );
		const uint32 FORMAT_VERSION = 0x00000001;

		struct Water
		{
			BW::Water::WaterState state_;
			StringTableTypes::Index	foamTexture_;
			StringTableTypes::Index	waveTexture_;
			StringTableTypes::Index	reflectionTexture_;
			StringTableTypes::Index transparencyTable_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_WATER_TYPES_HPP
