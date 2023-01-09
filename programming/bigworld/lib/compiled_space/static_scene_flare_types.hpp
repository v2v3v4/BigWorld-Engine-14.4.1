#ifndef STATIC_SCENE_FLARE_TYPES_HPP
#define STATIC_SCENE_FLARE_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "romp/water.hpp"

#include "string_table_types.hpp"

namespace BW {
	namespace CompiledSpace {

		namespace StaticSceneFlareTypes
		{
			const FourCC FORMAT_MAGIC = FourCC( "BWfr" );
			const uint32 FORMAT_VERSION = 0x00000001;

			enum Flags
			{
				FLAG_COLOUR_APPLIED	= (1<<0),
			};

			struct Flare
			{
				StringTableTypes::Index resourceID_;
				float maxDistance_;
				float area_;
				float fadeSpeed_;
				Vector3 position_;
				Vector3 colour_;
				uint32 flags_;
			};
		}

	} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_FLARE_TYPES_HPP
