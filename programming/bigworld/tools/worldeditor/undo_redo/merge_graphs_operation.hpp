#ifndef MERGE_GRAPHS_OPERATION_HPP
#define MERGE_GRAPHS_OPERATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/undoredo.hpp"
#include "cstdmf/unique_id.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This class undos/redos the merge of two graphs.
 */
class MergeGraphsOperation : public UndoRedo::Operation
{
public:
    MergeGraphsOperation
    (
        UniqueID                const &graph1Id,
        UniqueID                const &graph2Id
    );

	/*virtual*/ void undo();

	/*virtual*/ bool iseq( const UndoRedo::Operation & oth ) const;

protected:
    MergeGraphsOperation
    (
        UniqueID                const &graph1Id,
        UniqueID                const &graph2Id,
        BW::vector<UniqueID>   const &nodes
    );

protected:
    UniqueID                    graph1Id_;
    UniqueID                    graph2Id_;
    BW::vector<UniqueID>       nodes_;
};

BW_END_NAMESPACE

#endif // MERGE_GRAPHS_OPERATION_HPP
