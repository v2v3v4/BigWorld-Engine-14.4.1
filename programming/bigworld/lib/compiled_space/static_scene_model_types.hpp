#ifndef STATIC_SCENE_MODEL_TYPES_HPP
#define STATIC_SCENE_MODEL_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "math/matrix.hpp"

#include "string_table_types.hpp"


namespace BW {
namespace CompiledSpace {

	namespace StaticSceneModelTypes
	{
		static const FourCC FORMAT_MAGIC = FourCC( "BWSM" );
		static const uint32 FORMAT_VERSION = 0x00000001;

		enum Flags
		{
			FLAG_CASTS_SHADOW		= (1<<0),
			FLAG_REFLECTION_VISIBLE	= (1<<1),
			FLAG_IS_SHELL			= (1<<2)
		};

		struct IndexRange
		{
			uint16 first_;
			uint16 last_;
		};

		struct Model
		{
			Matrix worldTransform_;
			IndexRange resourceIDs_;
			StringTableTypes::Index animationName_;
			float animationMultiplier_;
			uint32 flags_;
			IndexRange tintDyes_; // (tint1, dye1, tint2, dye2, ...)
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_MODEL_TYPES_HPP
