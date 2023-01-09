#ifndef _BINARY_WRITERS_OCTREE_WRITER_HPP
#define _BINARY_WRITERS_OCTREE_WRITER_HPP

#include "binary_format_writer.hpp"
#include <math/loose_octree.hpp>
#include <cstdmf/lookup_table.hpp>

namespace BW {
namespace CompiledSpace {

bool writeOctree( BinaryFormatWriter::Stream * stream, DynamicLooseOctree & tree,
	LookUpTable< BW::vector<DynamicLooseOctree::NodeDataReference> > & treeContents );

} // namespace CompiledSpace
} // namespace BW

#endif // _BINARY_WRITERS_OCTREE_WRITER_HPP
