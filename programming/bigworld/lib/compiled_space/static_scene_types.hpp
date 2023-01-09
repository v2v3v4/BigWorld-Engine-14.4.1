#ifndef STATIC_SCENE_TYPES_HPP
#define STATIC_SCENE_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"

#include "string_table_types.hpp"

#include "scene/scene_type_system.hpp"

#define STATIC_SCENE_TYPES			\
	X( DEFAULT, "BWSS" )		\
	X( VLO, "VLOS" )


namespace BW {
namespace CompiledSpace {

	namespace StaticSceneTypes
	{
#define X( type, fcc ) e##type,
		enum Type { STATIC_SCENE_TYPES };		
#undef X

#define X( type, fcc ) FourCC( fcc ),
		static const FourCC FORMAT_MAGIC[] = { STATIC_SCENE_TYPES };
#undef X

		static const uint32 FORMAT_VERSION = 0x00000001;

		struct TypeHeader
		{
			SceneTypeSystem::RuntimeTypeID typeID_;
			uint32 numObjects_;
		};
		
		struct StaticObjectType
		{
			enum Value
			{
				STATIC_MODEL,
				SPEED_TREE,
				WATER,
				FLARE,
				DECAL,

				USER_TYPES_BEGIN
			};
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_TYPES_HPP
