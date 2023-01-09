#ifndef COMPILED_SPACE_STATIC_GEOMETRY_HPP
#define COMPILED_SPACE_STATIC_GEOMETRY_HPP

#include "compiled_space/forward_declarations.hpp"
#include "compiled_space/binary_format.hpp"

#include "static_geometry_types.hpp"
#include "string_table.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class ReferenceCount;

BW_END_NAMESPACE

namespace BW {
namespace Moo {
	class Vertices;
	typedef SmartPointer<Vertices> VerticesPtr;
}

namespace CompiledSpace {

class COMPILED_SPACE_API StaticGeometry
{
public:
	StaticGeometry();
	~StaticGeometry();

	bool read( BinaryFormat& reader );
	void close();

	bool isValid() const;
	size_t size() const;
	bool empty() const;

	void loadGeometry();

	float percentLoaded() const;

private:

	BinaryFormat* pReader_;
	BinaryFormat::Stream* pStream_;

	uint32 numEntriesPopulated_;

	StringTable stringTable_;
	ExternalArray<StaticGeometryTypes::Entry> entries_;
	ExternalArray<StaticGeometryTypes::StreamEntry> streams_;
	ExternalArray<StaticGeometryTypes::BufferEntry> bufferEntries_;

	BW::vector<Moo::VerticesPtr> loadedResources_;

private:
	class Detail;
};

} // namespace CompiledSpace
} // namespace BW

#endif // COMPILED_SPACE_STATIC_GEOMETRY_HPP
