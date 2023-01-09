#ifndef STRING_TABLE_TYPES_HPP
#define STRING_TABLE_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"


namespace BW {
namespace CompiledSpace {

	namespace StringTableTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWST" );
		const uint32 FORMAT_VERSION = 0x00000001;

		typedef uint32 Index;

		const Index INVALID_INDEX = (Index)-1;

		struct Header
		{
			Index numEntries_;
		};

		struct Entry
		{
			uint32 offset_; // From beginning of string data
			uint32 length_; // Not including null terminator
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STRING_TABLE_TYPES_HPP
