#ifndef PHASE_EDGE_HPP
#define PHASE_EDGE_HPP


#include "graph/edge.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a Graph Edge for going to a Phase node.
 */
class PhaseEdge : public Graph::Edge
{
public:
	PhaseEdge( const Graph::NodePtr & start, const Graph::NodePtr & end );
};
typedef SmartPointer< PhaseEdge > PhaseEdgePtr;

BW_END_NAMESPACE

#endif // PHASE_EDGE_HPP
