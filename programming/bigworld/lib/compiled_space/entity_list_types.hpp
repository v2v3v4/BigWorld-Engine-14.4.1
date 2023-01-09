#ifndef COMPILED_SPACE_ENTITY_TYPES_HPP
#define COMPILED_SPACE_ENTITY_TYPES_HPP

#include "cstdmf/fourcc.hpp"

namespace BW {
namespace CompiledSpace {

	namespace EntityListTypes
	{
		static const FourCC CLIENT_ENTITIES_MAGIC = FourCC( "CENT");
		static const FourCC SERVER_ENTITIES_MAGIC = FourCC( "SENT");
		static const FourCC UDOS_MAGIC = FourCC( "UDOS");

		static uint32 FORMAT_VERSION = 0x00000001;
	}	
	
} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_ENTITY_TYPES_HPP
