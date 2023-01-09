#ifndef EFFECT_EDGE_VIEW_HPP
#define EFFECT_EDGE_VIEW_HPP


#include "graph/graph_view.hpp"

BW_BEGIN_NAMESPACE

// Forward declarartions
class EffectEdge;
typedef SmartPointer< EffectEdge > EffectEdgePtr;


/**
 *	This class implements a Graph Edge View for displaying edges that go from
 *	Effect to Effect.
 */
class EffectEdgeView : public Graph::EdgeView
{
public:
	EffectEdgeView( Graph::GraphView & graphView, const EffectEdgePtr edge );

	// Graph::NodeView interface
	const CRect & rect() const { return rect_; }

	void draw( CDC & dc, uint32 frame, const CRect & rectStartNode, const CRect & rectEndNode );

private:
	EffectEdgePtr edge_;
	CRect rect_;
};

BW_END_NAMESPACE

#endif // EFFECT_EDGE_VIEW_HPP
