#ifndef COMPILED_SPACE_SETTINGS_TYPES_HPP
#define COMPILED_SPACE_SETTINGS_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"
#include "math/boundbox.hpp"

namespace BW {
namespace CompiledSpace {

	namespace CompiledSpaceSettingsTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWCS" );
		const uint32 FORMAT_VERSION = 0x00000001;

		struct Header
		{
			AABB spaceBounds_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_SETTINGS_TYPES_HPP
