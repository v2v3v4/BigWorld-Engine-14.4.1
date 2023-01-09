#ifndef STATIC_SCENE_SPEED_TREE_TYPES_HPP
#define STATIC_SCENE_SPEED_TREE_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "math/matrix.hpp"

#include "string_table_types.hpp"


namespace BW {
namespace CompiledSpace {

	namespace StaticSceneSpeedTreeTypes
	{
		static const FourCC FORMAT_MAGIC = FourCC( "SpTr" );
		static const uint32 FORMAT_VERSION = 0x00000001;

		enum Flags
		{
			FLAG_CASTS_SHADOW		= (1<<0),
			FLAG_REFLECTION_VISIBLE	= (1<<1)
		};

		struct SpeedTree
		{
			Matrix worldTransform_;
			StringTableTypes::Index resourceID_;
			uint32 seed_;
			uint32 flags_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_SPEED_TREE_TYPES_HPP
