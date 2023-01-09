#include "pch.hpp"
#include "worldeditor/undo_redo/merge_graphs_operation.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "chunk/station_graph.hpp"
#include "chunk/chunk_stationnode.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This is the MergeGraphsOperation constructor.
 *
 *  @param graph1Id     The id of the source graph.
 *  @param graph2id     The id of the destination graph.
 */
MergeGraphsOperation::MergeGraphsOperation
(
    UniqueID        const &graph1Id,
    UniqueID        const &graph2Id
) :
    UndoRedo::Operation(size_t(typeid(MergeGraphsOperation).name())),
    graph1Id_(graph1Id),
    graph2Id_(graph2Id)
{
	BW_GUARD;

    StationGraph *graph1 = StationGraph::getGraph(graph1Id);
    StationGraph *graph2 = StationGraph::getGraph(graph2Id);

    BW::vector<ChunkStationNode*> nodes = graph2->getAllNodes();
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        nodes_.push_back(nodes[i]->id());
    }
}


/**
 *  This does an undo.
 */
/*virtual*/ void MergeGraphsOperation::undo()
{
	BW_GUARD;

	// First add the current state of the link to the undo/redo list
	UndoRedo::instance().add
    (
        new MergeGraphsOperation(graph2Id_, graph1Id_, nodes_)
    );

    // Swap the nodes from graph1 to graph2:
    StationGraph *graph1 = StationGraph::getGraph(graph1Id_);
    StationGraph *graph2 = StationGraph::getGraph(graph2Id_);

    // We transfer the nodes to the new graph and keep a record of which ones
    // have changed.  We cannot update the dirtiness until the end because
    // the graph is invalid during the movement of nodes and saving the node
    // would fail.
    BW::vector<ChunkStationNode *> dirtyNodes;
    for (size_t i = 0; i < nodes_.size(); ++i)
    {
        ChunkStationNode *node = graph1->getNode(nodes_[i]);
        if (node != NULL)
        {
            graph1->deregisterNode(node);
            graph2->registerNode(node, node->chunk());
            node->graph(graph2);
            dirtyNodes.push_back(node);         
        }
    }
    for (size_t i = 0; i < dirtyNodes.size(); ++i)
        dirtyNodes[i]->makeDirty();
}


/**
 *  This compares this with oth.
 *
 *  @param oth          The MergeGraphsOperation to compare against.
 * 
 *  @return             True if the operations are the same.
 */
/*virtual*/ bool 
MergeGraphsOperation::iseq( UndoRedo::Operation const &oth ) const
{
	BW_GUARD;

    MergeGraphsOperation const *other = 
        reinterpret_cast<MergeGraphsOperation const *>(&oth);
    if (nodes_.size() != other->nodes_.size())
        return false;
    for (size_t i = 0; i < other->nodes_.size(); ++i)
    {
        if (nodes_[i] != other->nodes_[i])
            return false;
    }
    return other->graph1Id_ == graph1Id_ && other->graph2Id_ == graph2Id_;
}


/**
 *  This is a private constructor for undo.
 * 
 *  @param graph1Id     The id of the source graph.
 *  @param graph2id     The id of the destination graph.
 *  @param nodes        The nodes to put back into the source graph.
 */
MergeGraphsOperation::MergeGraphsOperation
(
    UniqueID                const &graph1Id,
    UniqueID                const &graph2Id,
    BW::vector<UniqueID>   const &nodes
) :
    UndoRedo::Operation(size_t(typeid(MergeGraphsOperation).name())),
    graph1Id_(graph1Id),
    graph2Id_(graph2Id),
    nodes_(nodes)
{
}

BW_END_NAMESPACE

