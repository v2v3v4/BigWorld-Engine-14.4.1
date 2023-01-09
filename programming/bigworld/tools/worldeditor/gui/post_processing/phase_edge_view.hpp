#ifndef PHASE_EDGE_VIEW_HPP
#define PHASE_EDGE_VIEW_HPP


#include "graph/graph_view.hpp"

BW_BEGIN_NAMESPACE

// Forward declarartions
class PhaseEdge;
typedef SmartPointer< PhaseEdge > PhaseEdgePtr;


/**
 *	This class implements a Graph Edge View for displaying edges that go to
 *	a Phase node.
 */
class PhaseEdgeView : public Graph::EdgeView
{
public:
	PhaseEdgeView( Graph::GraphView & graphView, const PhaseEdgePtr edge );

	// Graph::NodeView interface
	const CRect & rect() const { return rect_; }

	void draw( CDC & dc, uint32 frame, const CRect & rectStartNode, const CRect & rectEndNode );

private:
	PhaseEdgePtr edge_;
	CRect rect_;
};

BW_END_NAMESPACE

#endif // PHASE_EDGE_VIEW_HPP
