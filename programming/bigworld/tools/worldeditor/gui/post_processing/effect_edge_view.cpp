
#include "pch.hpp"
#include "effect_edge_view.hpp"
#include "effect_edge.hpp"
#include "view_skin.hpp"
#include "view_draw_utils.hpp"
#include "graph/node.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param	graphView	Canvas where the view will be drawn.
 *	@param	edge		Effect Graph Edge connecting two Effects.
 */
EffectEdgeView::EffectEdgeView( Graph::GraphView & graphView, const EffectEdgePtr edge ) :
	edge_( edge )
{
	BW_GUARD;

	if (!graphView.registerEdgeView( edge.get(), this ))
	{
		ERROR_MSG( "EffectEdgeView: The edge or its nodes are not in the graph.\n" );
	}
}


/**
 *	This method draws the visual representation of the effect edge.
 *
 *	@param dc		Device context where the draw operations should go.
 *	@param frame	Frame number, ignored.
 *	@param rectStartNode	Rectangle of the start node.
 *	@param rectEndNode		Rectangle of the end node.
 */
void EffectEdgeView::draw( CDC & dc, uint32 frame, const CRect & rectStartNode, const CRect & rectEndNode )
{
	BW_GUARD;

	ViewDrawUtils::drawBoxConection( dc, rectStartNode, rectEndNode, rect_, ViewSkin::effectEdgeColour() );
}

BW_END_NAMESPACE

