#ifndef GRAPH_VIEW_HPP
#define GRAPH_VIEW_HPP


#include "graph_item_views.hpp"
#include "controls/memdc.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace Graph
{


/**
 *	This class implements the visual representation of a Graph.
 */
class GraphView : public CStatic
{
public:
	static const int CANVAS_WIDTH_MARGIN = 60;
	static const int CANVAS_HEIGHT_MARGIN = 40;

	GraphView();
	virtual ~GraphView();

	virtual void init( GraphPtr newGraph, COLORREF bkColour );

	virtual void update();

	virtual void graph( GraphPtr newGraph );
	virtual GraphPtr graph() const;

	virtual NodeViewPtr nodeHitTest( const CPoint & point ) const;
	virtual EdgeViewPtr edgeHitTest( const CPoint & point ) const;

	virtual NodeViewPtr nodeView( const NodePtr & node ) const;
	virtual EdgeViewPtr edgeView( const EdgePtr & edge ) const;

	void collectNodeViews( BW::vector<NodeView*>& out );

	virtual void resizeToNodes();

	NodeViewPtr selectedNode() const;
	void selectedNode( NodeViewPtr nodeView );

	virtual void pan( const CSize & offset, bool redraw = true );
	virtual const CPoint & panOffset() const;

	virtual int zoom() const;
	virtual void zoom( int zoom );

	virtual void footer( const BW::wstring & text,
										LOGFONT * font, COLORREF colour );

	virtual bool cloning() const { return cloning_; }

	// These methods should be called by nodes and edges being added to the
	// graph so the GraphView knows what to use for drawing the node or edge.
	virtual bool registerNodeView( Node * node, NodeViewPtr nodeView );
	virtual bool registerEdgeView( Edge * edge, EdgeViewPtr edgeView );
	virtual bool unregisterNodeView( Node * node );
	virtual bool unregisterEdgeView( Edge * edge );

protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd( CDC * pDC );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnLButtonDown( UINT flags, CPoint point );	
	afx_msg void OnLButtonUp( UINT flags, CPoint point );
	afx_msg void OnRButtonDown( UINT flags, CPoint point );
	afx_msg void OnRButtonUp( UINT flags, CPoint point );
	afx_msg BOOL OnMouseWheel( UINT flags, short zDelta, CPoint point );
	afx_msg BOOL OnSetCursor( CWnd * pWnd, UINT nHitTest, UINT message );

	DECLARE_MESSAGE_MAP()

private:
	bool startedPan_;
	CPoint lastMousePos_;
	CPoint panOffset_;

	COLORREF bkColour_;

	typedef BW::map< Node *, NodeViewPtr > NodeViewMap;
	typedef BW::map< Edge *, EdgeViewPtr > EdgeViewMap;

	controls::MemDC memDC_;

	GraphPtr graph_;

	NodeViewMap nodeViews_;
	EdgeViewMap edgeViews_;

	NodeViewPtr btnDownNode_;
	CPoint btnDownPt_;
	bool panning_;
	bool dragging_;
	bool cloning_;
	HCURSOR cloneCursor_;
	NodeViewPtr selectedNode_;
	NodeViewPtr hoverNode_;

	int zoom_;
	int curFrame_;

	BW::wstring footerText_;
	CFont footerFont_;
	COLORREF footerColour_;

	void clipCanvasPosition( bool redraw = true );

	void doPanning( const CPoint & point );
	void stopPanning( const CPoint & point );

	void internalBtnDown( const CPoint & point, bool acceptDrag );
	void internalBtnUp( const CPoint & point );
};


} // namespace Graph

BW_END_NAMESPACE

#endif // GRAPH_VIEW_HPP
