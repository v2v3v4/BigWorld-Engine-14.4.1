#ifndef STATIC_SCENE_DECAL_TYPES
#define STATIC_SCENE_DECAL_TYPES

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "romp/water.hpp"

#include "string_table_types.hpp"

namespace BW {
namespace CompiledSpace {

	namespace StaticSceneDecalTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "WGSD" );
		const uint32 FORMAT_VERSION = 0x00000001;

		struct Decal
		{
			Matrix worldTransform_;
			StringTableTypes::Index diffuseTexture_;
			StringTableTypes::Index bumpTexture_;
			uint8 priority_;
			uint8 influenceType_;
			uint8 materialType_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_DECAL_TYPES
