
#include "pch.hpp"
#include "graph.hpp"
#include "node.hpp"
#include "edge.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


/**
 *	Constructor.
 */
Graph::Graph()
{
}


/**
 *	Destructor.
 */
Graph::~Graph()
{
}


/**
 *	This method adds a node to the graph.
 *
 *	@param node	Node to add.
 *	@return True if successful.
 */
bool Graph::addNode( const NodePtr & node )
{
	BW_GUARD;

	bool ret = false;

	if (node && nodes_.find( node ) == nodes_.end())
	{
		nodes_.insert( node );
		ret = true;
	}
	else
	{
		ERROR_MSG( "Graph::addNode: Failed to add node to the graph.\n" );
	}

	return ret;
}


/**
 *	This method removes a node from the graph.
 *
 *	@param node	Node to remove.
 *	@return True if successful.
 */
bool Graph::removeNode( const NodePtr & node )
{
	BW_GUARD;

	bool ret = false;

	if (node && nodes_.find( node ) != nodes_.end())
	{
		nodes_.erase( node );
		ret = true;
	}
	else
	{
		ERROR_MSG(
				"Graph::removeNode: Failed to remove node from the graph.\n" );
	}

	return ret;
}


/**
 *	This method add an node to the graph.
 *
 *	@param edge Edge to add.
 *	@return True if successful.
 */
bool Graph::addEdge( const EdgePtr & edge )
{
	BW_GUARD;

	bool ret = false;

	if (edge && edge->start() && edge->end() &&
		nodes_.find( edge->start() ) != nodes_.end() &&
		nodes_.find( edge->end() ) != nodes_.end())
	{
		edges_.insert( edge );
		edgesMap_.insert( std::make_pair( edge->start(), edge ) );
		backEdgesMap_.insert( std::make_pair( edge->end(), edge ) );
		ret = true;
	}
	else
	{
		ERROR_MSG( "Graph::addEdge: Failed to add edge to the graph.\n" );
	}

	return ret;
}


/**
 *	This method removes an edge from the graph.
 *
 *	@param edge Edge to remove.
 *	@return True if successful.
 */
bool Graph::removeEdge( const EdgePtr & edge )
{
	BW_GUARD;

	bool ret = false;

	if (edge && edges_.find( edge ) != edges_.end())
	{
		edges_.erase( edge );
		removeEdgeFromMap( edge, edgesMap_ );
		removeEdgeFromMap( edge, backEdgesMap_ );
		ret = true;
	}
	else
	{
		ERROR_MSG(
				"Graph::removeEdge: Failed to remove edge from the graph.\n" );
	}

	return ret;
}


/**
 *	This method updates the start and end nodes of an edge.
 *
 *	@param edge	Edge to update.
 *	@param start	New start node for the edge.
 *	@param end		New end node for the edge.
 */
void Graph::updateEdge( EdgePtr & edge, const NodePtr & start, const NodePtr & end )
{
	BW_GUARD;

	edge->start( start );
	edge->end( end );
	removeEdge( edge );
	addEdge( edge );
}


/**
 *	This method removes all nodes and edges from the graph.
 */
void Graph::clear()
{
	BW_GUARD;

	nodes_.clear();
	edges_.clear();
	edgesMap_.clear();
	backEdgesMap_.clear();
}


/**
 *	This method returns all the nodes that have edges from the specified node.
 *
 *	@param start	Node used as the start of edges to find adjacent nodes.
 *	@param retNodes	Return param, nodes that are then end of edges from start.
 */
void Graph::adjacentNodes( const NodePtr & start, NodesSet & retNodes ) const
{
	BW_GUARD;

	std::pair< EdgesMap::const_iterator, EdgesMap::const_iterator > range =
												edgesMap_.equal_range( start );

	for (EdgesMap::const_iterator it = range.first; it != range.second; ++it)
	{
		retNodes.insert( (*it).second->end() );
	}
}


/**
 *	This method returns all the nodes that have edges to the specified node.
 *
 *	@param end	Node used as the end of edges to find adjacent nodes.
 *	@param retNodes	Return param, nodes that are then start of edges from end.
 */
void Graph::backAdjacentNodes( const NodePtr & end, NodesSet & retNodes ) const
{
	BW_GUARD;

	std::pair< EdgesMap::const_iterator, EdgesMap::const_iterator > range =
											backEdgesMap_.equal_range( end );

	for (EdgesMap::const_iterator it = range.first; it != range.second; ++it)
	{
		retNodes.insert( (*it).second->start() );
	}
}


/**
 *	This method returns all the edges where the specified node is the start.
 *
 *	@param start	Node used as the start of the edges.
 *	@param retNodes	Return param, edges starting from the specified node.
 */
void Graph::edges( const NodePtr & start, EdgesSet & retEdges ) const
{
	BW_GUARD;

	std::pair< EdgesMap::const_iterator, EdgesMap::const_iterator > range =
												edgesMap_.equal_range( start );

	for (EdgesMap::const_iterator it = range.first; it != range.second; ++it)
	{
		retEdges.insert( (*it).second );
	}
}


/**
 *	This method returns all the edges where the specified node is the end.
 *
 *	@param start	Node used as the end of the edges.
 *	@param retNodes	Return param, edges ending in the specified node.
 */
void Graph::backEdges( const NodePtr & end, EdgesSet & retEdges ) const
{
	BW_GUARD;

	std::pair< EdgesMap::const_iterator, EdgesMap::const_iterator > range =
											backEdgesMap_.equal_range( end );

	for (EdgesMap::const_iterator it = range.first; it != range.second; ++it)
	{
		retEdges.insert( (*it).second );
	}
}


/**
 *	This helper method removes an edge from the specified edges map.
 *
 *	@param edge	Edge to remove.
 *	@param edgesMap	Edges map to remove the edge from.
 */
void Graph::removeEdgeFromMap( const EdgePtr & edge, EdgesMap & edgesMap )
{
	BW_GUARD;

	NodePtr nodes[] = { edge->start(), edge->end() };

	for (int i = 0; i < 2; ++i)
	{
		std::pair< EdgesMap::iterator, EdgesMap::iterator >
								range = edgesMap.equal_range( nodes[ i ] );

		for (EdgesMap::iterator it = range.first; it != range.second; ++it)
		{
			if ((*it).second.get() == edge)
			{
				edgesMap.erase( it );
				break;
			}
		}
	}
}


} // namespace Graph
BW_END_NAMESPACE

