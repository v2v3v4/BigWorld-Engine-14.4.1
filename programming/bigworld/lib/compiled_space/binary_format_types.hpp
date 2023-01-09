#ifndef BINARY_FORMAT_TYPES_HPP
#define BINARY_FORMAT_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"

namespace BW {
namespace CompiledSpace {

	namespace BinaryFormatTypes
	{
		static const FourCC FORMAT_MAGIC = FourCC( "BWTB" );
		static const uint32 FORMAT_VERSION = 0x00000001;

		#pragma pack(push,1)
		struct Header
		{
			FourCC magic_;
			uint32 version_;
			uint32 headerSize_;
			uint64 fileSize_;
			uint32 numSections_;
		};
		#pragma pack(pop)


		#pragma pack(push,1)
		struct SectionHeader
		{
			FourCC magic_;
			uint32 version_;
			uint64 offset_;
			uint64 length_;
		};
		#pragma pack(pop)

		const size_t MAX_SECTIONS = 32;
	}

} // namespace CompiledSpace
} // namespace BW


#endif // COMPILED_SPACE_BINARY_HPP
