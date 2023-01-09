
#include "pch.hpp"
#include "phase_edge_view.hpp"
#include "phase_edge.hpp"
#include "view_skin.hpp"
#include "view_draw_utils.hpp"
#include "graph/node.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param	graphView	Canvas where the view will be drawn.
 *	@param	edge		Phase Graph Edge connecting two Effects.
 */
PhaseEdgeView::PhaseEdgeView( Graph::GraphView & graphView, const PhaseEdgePtr edge ) :
	edge_( edge )
{
	BW_GUARD;

	if (!graphView.registerEdgeView( edge.get(), this ))
	{
		ERROR_MSG( "EffectEdgeView: The edge or its nodes are not in the graph.\n" );
	}
}


/**
 *	This method draws the visual representation of the phase edge.
 *
 *	@param dc		Device context where the draw operations should go.
 *	@param frame	Frame number, ignored.
 *	@param rectStartNode	Rectangle of the start node.
 *	@param rectEndNode		Rectangle of the end node.
 */
void PhaseEdgeView::draw( CDC & dc, uint32 frame, const CRect & rectStartNode, const CRect & rectEndNode )
{
	BW_GUARD;

	ViewDrawUtils::drawBoxConection( dc, rectStartNode, rectEndNode, rect_, ViewSkin::phaseEdgeColour() );
}

BW_END_NAMESPACE

