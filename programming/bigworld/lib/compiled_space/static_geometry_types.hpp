#ifndef STATIC_GEOMETRY_TYPES_HPP
#define STATIC_GEOMETRY_TYPES_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/fourcc.hpp"

#include "moo/primitive_file_structs.hpp"

namespace BW {
namespace CompiledSpace {

	namespace StaticGeometryTypes
	{
		const FourCC FORMAT_MAGIC = FourCC( "BWSG" );
		const uint32 FORMAT_VERSION = 0x00000001;

		struct Entry
		{
			uint32 resourceID_;
			uint32 beginStreamIndex_;
			uint32 endStreamIndex_;
			uint32 numVertices_;
			uint32 originalVertexFormat_;
		};

		struct StreamEntry
		{
			// Configuration info
			uint32 streamIndex_;
			uint32 stride_;
			uint32 size_;
			
			// Buffer information
			uint32 hostBufferID_;
			uint32 hostBufferOffset_;
		};

		struct BufferEntry
		{
			uint32 size_;
		};
	}

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_GEOMETRY_TYPES_HPP
