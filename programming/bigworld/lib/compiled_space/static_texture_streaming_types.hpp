#ifndef STATIC_TEXTURE_STREAMING_TYPES_HPP
#define STATIC_TEXTURE_STREAMING_TYPES_HPP

#include "string_table_types.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "math/sphere.hpp"
#include "scene/scene_type_system.hpp"

namespace BW {
namespace CompiledSpace {

namespace StaticTextureStreamingTypes
{
	const FourCC FORMAT_MAGIC = FourCC( "BWTS" );
	const uint32 FORMAT_VERSION = 0x00000001;

	struct Usage
	{
		Sphere bounds_;
		StringTableTypes::Index resourceID_;
		float worldScale_;
		float uvDensity_;
	};
}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_TEXTURE_STREAMING_TYPES_HPP
