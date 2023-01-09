#ifndef EFFECT_NODE_VIEW_HPP
#define EFFECT_NODE_VIEW_HPP


#include "graph/graph_view.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
class EffectNode;
typedef SmartPointer< EffectNode > EffectNodePtr;


/**
 *	This class implements an Effect node view, which is responsible for
 *	displaying an EffectNode on the GraphView canvas.
 */
class EffectNodeView : public Graph::NodeView
{
public:
	EffectNodeView( Graph::GraphView & graphView, const EffectNodePtr node );
	~EffectNodeView();

	// Graph::NodeView interface
	const CRect & rect() const { return rect_; }

	int zOrder() const;

	int alpha() const;

	void position( const CPoint & pos );

	void draw( CDC & dc, uint32 frame, STATE state );

	void leftClick( const CPoint & pt );

	void doDrag( const CPoint & pt );
	void endDrag( const CPoint & pt, bool canceled = false );

private:
	EffectNodePtr node_;
	CRect rect_;
};

BW_END_NAMESPACE

#endif // EFFECT_NODE_VIEW_HPP
