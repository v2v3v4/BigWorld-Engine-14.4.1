
#include "pch.hpp"
#include "effect_edge.hpp"
#include "graph/node.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param	start	Start node, a Post-processing Effect.
 *	@param	end		End node, a Post-processing Effect.
 */
EffectEdge::EffectEdge( const Graph::NodePtr & start, const Graph::NodePtr & end ) :
	Graph::Edge( start, end )
{
	BW_GUARD;
}

BW_END_NAMESPACE

