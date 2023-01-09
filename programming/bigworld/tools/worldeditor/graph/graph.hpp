#ifndef GRAPH_HPP
#define GRAPH_HPP


#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


// Forward declarations.
class Node;
typedef SmartPointer< Node > NodePtr;


class Edge;
typedef SmartPointer< Edge > EdgePtr;


typedef BW::set< NodePtr > NodesSet;

typedef BW::set< EdgePtr > EdgesSet;

typedef BW::multimap< NodePtr, EdgePtr > EdgesMap;


/**
 *	This class stores info about and manages a set of connected nodes.
 */
class Graph : public ReferenceCount
{
public:
	Graph();
	virtual ~Graph();

	virtual bool addNode( const NodePtr & node );
	virtual bool removeNode( const NodePtr & node );

	virtual bool addEdge( const EdgePtr & edge );
	virtual bool removeEdge( const EdgePtr & edge );

	virtual void updateEdge( EdgePtr & edge, const NodePtr & start, const NodePtr & end );

	virtual void clear();

	virtual void adjacentNodes( const NodePtr & startNode, NodesSet & retNodes ) const;

	virtual void backAdjacentNodes( const NodePtr & endNode, NodesSet & retNodes ) const;

	virtual void edges( const NodePtr & startNode, EdgesSet & retEdges ) const;

	virtual void backEdges( const NodePtr & endNode, EdgesSet & retEdges ) const;

	virtual const NodesSet & allNodes() const { return nodes_; }

	virtual const EdgesSet & allEdges() const { return edges_; }

private:
	Graph( const Graph & other );

	NodesSet nodes_;

	EdgesSet edges_;

	EdgesMap edgesMap_;
	EdgesMap backEdgesMap_;

	void removeEdgeFromMap( const EdgePtr & edge, EdgesMap & edgesMap );
};


typedef SmartPointer< Graph > GraphPtr;


} // namespace Graph

BW_END_NAMESPACE

#endif // GRAPH_VIEW_HPP
