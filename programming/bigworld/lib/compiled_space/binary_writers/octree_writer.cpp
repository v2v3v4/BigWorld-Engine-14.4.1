#include "pch.hpp"
#include "octree_writer.hpp"

#include "compiled_space/forward_declarations.hpp"

#include "binary_format_writer.hpp"
#include <math/loose_octree.hpp>
#include <cstdmf/lookup_table.hpp>

namespace BW {
namespace CompiledSpace {

bool writeOctree( BinaryFormatWriter::Stream * stream, DynamicLooseOctree & tree,
                  LookUpTable< BW::vector<DynamicLooseOctree::NodeDataReference> > & treeContents )
{
	// Build a set of data spans
	typedef BW::vector<DynamicLooseOctree::NodeDataReference> NodeContents;
	LookUpTable< DataSpan > treeSceneContentSpans;
	NodeContents sceneContents;
	for (uint32 nodeIndex = 0; nodeIndex < treeContents.size(); 
		++nodeIndex)
	{
		DataSpan span;
		span.first( static_cast<DataSpan::value_type>(sceneContents.size()) );

		// Add all contents of this node to the scene contents
		const NodeContents & nodeContents = treeContents[nodeIndex];
		sceneContents.insert( sceneContents.end(), 
			nodeContents.begin(), nodeContents.end() );

		span.last( static_cast<DataSpan::value_type>(sceneContents.size() - 1) );

		treeSceneContentSpans[nodeIndex] = span;
	}

	// Output
	stream->write( tree );
	stream->write( treeSceneContentSpans.storage() );
	stream->write( sceneContents );

	return true;
}

} // namespace CompiledSpace
} // namespace BW

