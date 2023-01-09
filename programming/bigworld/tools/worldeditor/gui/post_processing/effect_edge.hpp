#ifndef EFFECT_EDGE_HPP
#define EFFECT_EDGE_HPP


#include "graph/edge.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a Graph Edge for going from Effect to Effect.
 */
class EffectEdge : public Graph::Edge
{
public:
	EffectEdge( const Graph::NodePtr & start, const Graph::NodePtr & end );
};
typedef SmartPointer< EffectEdge > EffectEdgePtr;

BW_END_NAMESPACE

#endif // EFFECT_EDGE_HPP
