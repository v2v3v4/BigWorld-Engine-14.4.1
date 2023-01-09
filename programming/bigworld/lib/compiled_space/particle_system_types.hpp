#ifndef STATIC_PARTICLE_SYSTEM_TYPES_HPP
#define STATIC_PARTICLE_SYSTEM_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "romp/water.hpp"

#include "string_table_types.hpp"

namespace BW {
namespace CompiledSpace {

	namespace ParticleSystemTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWPs" );
		const uint32 FORMAT_VERSION = 0x00000001;

		enum Flags
		{
			FLAG_REFLECTION_VISIBLE	= (1<<0),
		};

		struct ParticleSystem
		{
			Matrix worldTransform_;
			StringTableTypes::Index resourceID_;
			uint32 flags_;
			float seedTime_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_PARTICLE_SYSTEM_TYPES_HPP
