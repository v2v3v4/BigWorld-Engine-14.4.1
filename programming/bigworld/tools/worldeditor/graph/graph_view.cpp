
#include "pch.hpp"
#include "graph_view.hpp"
#include "graph.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "controls/cursor_utils.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	This function is used when sorting nodes by their Z order.
	 */
	bool nodeZOrderComparer( const Graph::NodeViewPtr & view1, const Graph::NodeViewPtr & view2 )
	{
		return view1->zOrder() > view2->zOrder();
	}
}


namespace Graph
{


// MFC message map.
BEGIN_MESSAGE_MAP( GraphView, CWnd )
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()


/**
 *	Constructor.
 */
GraphView::GraphView() :
	startedPan_( false ),
	lastMousePos_( 0, 0 ),
	panOffset_( 0, 0 ),
	bkColour_( RGB( 64, 64, 64 ) ),
	panning_( false ),
	dragging_( false ),
	cloning_( false ),
	cloneCursor_( NULL ),
	zoom_( 0 ),
	curFrame_( 0 ),
	footerColour_( RGB( 0, 0, 0 ) )
{
}


/**
 *	Destructor.
 */
GraphView::~GraphView()
{
	BW_GUARD;

	if (cloneCursor_)
	{
		DestroyCursor( cloneCursor_ );
		cloneCursor_ = NULL;
	}
}


/**
 *	This method initialises to a new Graph.
 *
 *	@param newGraph	Graph to visualise.
 *	@param bkColour	Background colour.
 */
void GraphView::init( GraphPtr newGraph, COLORREF bkColour )
{
	BW_GUARD;

	graph( newGraph );

	bkColour_ = bkColour;

	// Create the "Clone" cursor by copying the standard cursor used for dragging
	// and adding a little "[+]" at the bottom-right corner.
	if (!cloneCursor_)
	{
		cloneCursor_ = controls::addPlusSignToCursor(
								LoadCursor( NULL, IDC_SIZEALL ) );
		if (!cloneCursor_)
		{
			ERROR_MSG( "GraphView::init: Could not create custom 'Clone' "
					"cursor. Falling back to the default cursor instead.\n" );
		}
	}
}


/**
 *	This method is called regularly to update the view, including updating
 *	button states and and dragging operations.
 */
void GraphView::update()
{
	BW_GUARD;

	CPoint srcPoint;
	GetCursorPos( &srcPoint );
	CPoint point( srcPoint );
	ScreenToClient( &point );

	CPoint panPoint( point );
	panPoint += CPoint( -panOffset_.x, -panOffset_.y );

	bool isMouseDown =	GetAsyncKeyState( VK_LBUTTON ) < 0 ||
						GetAsyncKeyState( VK_RBUTTON ) < 0;

	if (!isMouseDown && GetCapture() != this)
	{
		if (dragging_ && btnDownNode_)
		{
			dragging_ = false;
			btnDownNode_->endDrag( panPoint, true /*canceled*/ );
		}

		if (panning_)
		{
			panning_ = false;
			stopPanning( point );
		}
	}

	bool isCtrlDown = GetAsyncKeyState( VK_CONTROL ) < 0;

	if (CWnd::WindowFromPoint( srcPoint ) == this || GetCapture() == this)
	{
		SetFocus();

		if (isMouseDown)
		{
			if (btnDownNode_)
			{
				if (!dragging_ &&
					abs(btnDownPt_.x - point.x) > 5 ||
					abs(btnDownPt_.y - point.y) > 5)
				{
					dragging_ = true;
				}
			}
			else if (panning_)
			{
				doPanning( point );
			}
		}

		cloning_ = dragging_ && isCtrlDown;

		bool cursorOverView = false;
		NodeViewPtr prevHoverNode = hoverNode_;
		hoverNode_ = NULL;
		
		if (!panning_ && !dragging_)
		{
			for (NodeViewMap::const_iterator it = nodeViews_.begin();
				it != nodeViews_.end(); ++it)
			{
				if ((*it).second->hitTest( panPoint ))
				{
					::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );
					hoverNode_ = (*it).second;
					cursorOverView = true;
					break;
				}
			}
		}

		if (prevHoverNode != hoverNode_ ||
			dragging_ || panning_)
		{
			RedrawWindow();
		}

		if (dragging_)
		{
			btnDownNode_->doDrag( panPoint );
		}

		bool forcePanning = (GetAsyncKeyState( VK_SPACE ) < 0);
		if (!cursorOverView || forcePanning)
		{
			if (cloning_ && cloneCursor_)
			{
				::SetCursor( cloneCursor_ );
			}
			else
			{
				::SetCursor( ::LoadCursor( NULL, IDC_SIZEALL ) );
			}
		}
	}
	else
	{
		cloning_ = dragging_ && isCtrlDown;
	}
}


/**
 *	This method changes the graph.
 *
 *	@param newGraph	New graph to display.
 */
void GraphView::graph( GraphPtr newGraph )
{
	BW_GUARD;

	nodeViews_.clear();
	edgeViews_.clear();
	selectedNode_ = NULL;
	hoverNode_ = NULL;

	graph_ = newGraph;
}


/**
 *	This method returns the graph being displayed.
 *
 *	@return	The graph being displayed.
 */
GraphPtr GraphView::graph() const
{
	return graph_;
}


/**
 *	This method finds the node view under the specified point.
 *
 *	@param point	Point to test.
 *	@return	The node view intersecting the point, or NULL if not found.
 */
NodeViewPtr GraphView::nodeHitTest( const CPoint & point ) const
{
	BW_GUARD;

	NodeViewPtr ret;

	for (NodeViewMap::const_iterator it = nodeViews_.begin();
		it != nodeViews_.end(); ++it)
	{
		if ((*it).second->hitTest( point ))
		{
			ret = (*it).second;
			break;
		}
	}

	return ret;
}


/**
 *	This method finds the edge view under the specified point.
 *
 *	@param point	Point to test.
 *	@return	The edge view intersecting the point, or NULL if not found.
 */
EdgeViewPtr GraphView::edgeHitTest( const CPoint & point ) const
{
	BW_GUARD;

	EdgeViewPtr ret;

	for (EdgeViewMap::const_iterator it = edgeViews_.begin();
		it != edgeViews_.end(); ++it)
	{
		if ((*it).second->hitTest( point ))
		{
			ret = (*it).second;
			break;
		}
	}

	return ret;
}


/**
 *	This method returns the view corresponding to a Graph Node.
 *
 *	@return	The view corresponding to a Graph Node, or NULL if not found.
 */
NodeViewPtr GraphView::nodeView( const NodePtr & node ) const
{
	BW_GUARD;

	NodeViewPtr ret;
	
	NodeViewMap::const_iterator it = nodeViews_.find( node.get() );
	if (it != nodeViews_.end())
	{
		ret = (*it).second;
	}

	return ret;
}


/**
 *	This method returns all the node views.
 *
 *	@param out	Return param, all the node views.
 */
void GraphView::collectNodeViews( BW::vector<NodeView*>& out )
{
	BW_GUARD;

	NodeViewMap::const_iterator it = nodeViews_.begin();
	while (it != nodeViews_.end())
	{
		out.push_back(it->second.get());
		++it;
	}
}


/**
 *	This method returns the view corresponding to a Graph Edge.
 *
 *	@return	The view corresponding to a Graph Edge, or NULL if not found.
 */
EdgeViewPtr GraphView::edgeView( const EdgePtr & edge ) const
{
	BW_GUARD;

	EdgeViewPtr ret;
	
	EdgeViewMap::const_iterator it = edgeViews_.find( edge.get() );
	if (it != edgeViews_.end())
	{
		ret = (*it).second;
	}

	return ret;
}


/**
 *	This method resizes the window's size to fit the current set of node and
 *	edge views.
 */
void GraphView::resizeToNodes()
{
	BW_GUARD;

	clipCanvasPosition( false );
}


/**
 *	This method returns the currently selected node view.
 *
 *	@return	The currently selected node view.
 */
NodeViewPtr GraphView::selectedNode() const
{
	return selectedNode_;
}


/**
 *	This method sets the selected node view.
 *
 *	@param nodeView	The new selected node view.
 */
void GraphView::selectedNode( NodeViewPtr nodeView )
{
	selectedNode_ = nodeView;
}


/**
 *	This method pans the graph view window by the specified offset.
 *
 *	@param offset	Distance to pan the graph view by.
 *	@param redraw	True to redraw the window, false just pan.
 */
void GraphView::pan( const CSize & offset, bool redraw /*= true */ )
{
	BW_GUARD;

	panOffset_ += offset;
	clipCanvasPosition( redraw );

	if (redraw)
	{
		RedrawWindow();
	}
}


/**
 *	This method returns the current panning position.
 *
 *	@return The current panning position.
 */
const CPoint & GraphView::panOffset() const
{
	return panOffset_;
}


/**
 *	This method returns the current zoom factor.
 *
 *	@return The current zoom factor.
 */
int GraphView::zoom() const
{
	return zoom_;
}


/**
 *	This method sets the zoom factor.
 *
 *	@param zoom New zoom factor.
 */
void GraphView::zoom( int zoom )
{
	zoom_ = zoom;
}


/**
 *	This method sets the graph view footer text.
 *
 *	@param text	Text to display in the footer, or "" to not display any footer.
 *	@param font	Font to use when drawing the footer text.
 *	@param colour Colour to use when drawing the footer text.
 */
void GraphView::footer( const BW::wstring & text, LOGFONT * font, COLORREF colour )
{
	BW_GUARD;

	footerText_ = text;
	footerFont_.CreateFontIndirect( font );
	footerColour_ = colour;
}


/**
 *	This method associates a node view with a node.
 *
 *	@return	True if successful.
 */
bool GraphView::registerNodeView( Node * node, NodeViewPtr nodeView )
{
	BW_GUARD;

	if (!node || !nodeView || nodeViews_.find( node ) != nodeViews_.end())
	{
		return false;
	}

	nodeViews_.insert( std::make_pair( node, nodeView ) );
	return true;
}


/**
 *	This method associates a edge view with a edge.
 *
 *	@return	True if successful.
 */
bool GraphView::registerEdgeView( Edge * edge, EdgeViewPtr edgeView )
{
	BW_GUARD;

	if (!edge || !edgeView || edgeViews_.find( edge ) != edgeViews_.end())
	{
		return false;
	}

	edgeViews_.insert( std::make_pair( edge, edgeView ) );
	return true;
}


/**
 *	This method removes the node view associated with a node.
 *
 *	@return	True if successful.
 */
bool GraphView::unregisterNodeView( Node * node )
{
	BW_GUARD;

	NodeViewMap::iterator it = nodeViews_.find( node );
	if (!node || it == nodeViews_.end())
	{
		return false;
	}

	nodeViews_.erase( it );
	return true;
}


/**
 *	This method removes the edge view associated with a edge.
 *
 *	@return	True if successful.
 */
bool GraphView::unregisterEdgeView( Edge * edge )
{
	BW_GUARD;

	EdgeViewMap::iterator it = edgeViews_.find( edge );
	if (!edge || it == edgeViews_.end())
	{
		return false;
	}

	edgeViews_.erase( it );
	return true;
}


/**
 *	This MFC method is called when the control is repainted, and draws all the
 *	views in the view.
 */
void GraphView::OnPaint()
{
	BW_GUARD;

	clipCanvasPosition( false );

	// The extents of the control:
	CRect fullExtents;
	GetClientRect( fullExtents );

	// The extents of the control:
	CRect clipRect( fullExtents );
	clipRect.OffsetRect( -panOffset_.x, -panOffset_.y );

	// The drawing contexts:
	CPaintDC paintDC( this );

	paintDC.SetBkColor( bkColour_ );

	controls::MemDCScope memDCScope( memDC_, paintDC, &fullExtents );

	LOGFONT logFont;
	if (!footerText_.empty() && footerFont_.GetLogFont( &logFont ))
	{
		// Calc required rectangle
		CRect textRect = fullExtents;
		textRect.DeflateRect( 4, 2 );
		int oldTextRectBottom = textRect.bottom;
		textRect.top = textRect.bottom - logFont.lfHeight;
		memDC_.DrawText( footerText_.c_str(), &textRect, DT_CALCRECT | DT_WORDBREAK );
		int bottomDelta = oldTextRectBottom - textRect.bottom;
		textRect.OffsetRect( 0, bottomDelta );

		// Draw
		COLORREF oldTxtCol = memDC_.SetTextColor( footerColour_ );
		CFont * oldFont = memDC_.SelectObject( &footerFont_ );
		memDC_.DrawText( footerText_.c_str(), &textRect, DT_WORDBREAK );
		memDC_.SelectObject( oldFont );
		memDC_.SetTextColor( oldTxtCol );
	}

	memDC_.SetViewportOrg( panOffset_.x, panOffset_.y );

	if (graph_)
	{
		const NodesSet & nodes = graph_->allNodes();
		const EdgesSet & edges = graph_->allEdges();

		// First, draw the edges
		for (EdgesSet::const_iterator itEdge = edges.begin();
			itEdge != edges.end(); ++itEdge)
		{
			EdgeViewMap::const_iterator itView =
											edgeViews_.find( (*itEdge).get() );
			if (itView != edgeViews_.end())
			{
				NodeViewMap::const_iterator itStartNodeView =
								nodeViews_.find( (*itEdge)->start().get() );
				NodeViewMap::const_iterator itEndNodeView =
								nodeViews_.find( (*itEdge)->end().get() );
				
				if (itStartNodeView != nodeViews_.end() &&
					itEndNodeView != nodeViews_.end())
				{
					(*itView).second->draw( memDC_, curFrame_,
											(*itStartNodeView).second->rect(),
											(*itEndNodeView).second->rect() );
				}
			}
		}

		// Then, draw the nodes, sorted by zOrder (3 steps):

		// step 1: clip nodes to viewport
		bool needsSort = false;
		bool isTransparent = false;
		int lastZOrder = std::numeric_limits< int >::max();

		BW::vector< NodeViewPtr > visibleNodes;
		for (NodesSet::const_iterator itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
		{
			NodeViewMap::const_iterator itView = nodeViews_.find( (*itNode).get() );
			if (itView != nodeViews_.end())
			{
				NodeViewPtr nodeView = (*itView).second;

				CRect intersectRect;
				intersectRect.IntersectRect( nodeView->rect(), clipRect );

				if (!intersectRect.IsRectEmpty())
				{
					visibleNodes.push_back( nodeView );

					if (lastZOrder != std::numeric_limits< int >::max() &&
						lastZOrder != nodeView->zOrder())
					{
						needsSort = true;
					}
					lastZOrder = nodeView->zOrder();

					if (!isTransparent && nodeView->alpha() != 255)
					{
						isTransparent = true;
					}
				}
			}
		}

		// step 2: sort the visible nodes by zOrder
		if (needsSort)
		{
			std::sort( visibleNodes.begin(), visibleNodes.end(), nodeZOrderComparer );
		}

		// step 3: draw
		CDC alphaDC;
		CBitmap alphaBmp;
		CBitmap * oldBmp = NULL;
		BLENDFUNCTION blendFunc;

		if (isTransparent)
		{
			alphaDC.CreateCompatibleDC( &paintDC );
			alphaBmp.CreateCompatibleBitmap( &paintDC, fullExtents.Width(), fullExtents.Height() );
			alphaDC.SelectObject( &alphaBmp );

			alphaDC.FillSolidRect( fullExtents, bkColour_ );

			alphaDC.SetViewportOrg( panOffset_.x, panOffset_.y );

			blendFunc.BlendOp = AC_SRC_OVER;
			blendFunc.BlendFlags = 0;
			blendFunc.SourceConstantAlpha = 128;
			blendFunc.AlphaFormat = 0;
		}

		for (BW::vector< NodeViewPtr >::iterator itView = visibleNodes.begin();
			itView != visibleNodes.end(); ++itView)
		{
			NodeView::STATE state = ((*itView) == selectedNode_) ? NodeView::SELECTED : NodeView::NORMAL;
			if ((*itView) == hoverNode_ && !dragging_ && !panning_)
			{
				state |= NodeView::HOVER;
			}

			if ((*itView)->alpha() != 255)
			{
				// draw the node transparent (useful for drag & dropping for example)
				(*itView)->draw( alphaDC, curFrame_, state );
				
				CRect rect = (*itView)->rect();
				rect.IntersectRect( rect, clipRect );

				blendFunc.SourceConstantAlpha = (*itView)->alpha();

				memDC_.AlphaBlend(
					rect.left, rect.top, rect.Width(), rect.Height(),
					&alphaDC,
					rect.left, rect.top, rect.Width(), rect.Height(),
					blendFunc );
			}
			else
			{
				(*itView)->draw( memDC_, curFrame_, state );
			}
		}

		if (isTransparent)
		{
			alphaDC.SelectObject( oldBmp );
		}

		curFrame_++;
	}

	memDC_.SetViewportOrg( 0, 0 );
}


/**
 *	This MFC method is called when the window should erase its background.
 * 
 *  @param dc		The device context to erase with.
 *  @return			TRUE - the background is erased - which is not needed for
 *					this control.	
 */	
BOOL GraphView::OnEraseBkgnd( CDC * pDC )
{
	return TRUE;
}


/**
 *	This MFC method is overriden to adjust the panning and redraw on resize.
 *
 *	@param nType	MFC resize type.
 *	@param cx	New width.
 *	@param cy	New height.
 */
void GraphView::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CStatic::OnSize( nType, cx, cy );
	
	clipCanvasPosition();

	RedrawWindow();
}


/**
 *	This MFC method is called when the mouse button is pressed.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
void GraphView::OnLButtonDown( UINT flags, CPoint point )
{
	BW_GUARD;

	internalBtnDown( point, true /*accept node drag*/ );
}


/**
 *	This MFC method is called when the mouse button is released.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
void GraphView::OnLButtonUp( UINT flags, CPoint point )
{
	BW_GUARD;

	internalBtnUp( point );
}


/**
 *	This MFC method is called when the right mouse button is pressed.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
void GraphView::OnRButtonDown( UINT flags, CPoint point )
{
	BW_GUARD;

	internalBtnDown( point, false /*just pan*/ );
}


/**
 *	This MFC method is called when the right mouse button is released.
 *
 *  @param flags	The flags which shift-key status etc.
 *  @param point	The coordinates of the mouse.
 */
void GraphView::OnRButtonUp( UINT flags, CPoint point )
{
	BW_GUARD;

	internalBtnUp( point );
}


/**
 *	This MFC method is called when the mouse wheel is rotated.
 *
 *  @param flags	The flags which shift-key status etc.
 *	@param zDelta	Amount of wheel movement.
 *  @param point	The coordinates of the mouse.
 */
BOOL GraphView::OnMouseWheel( UINT flags, short zDelta, CPoint point )
{
	BW_GUARD;

	if (CWnd::WindowFromPoint( point ) == this)
	{
		int step = zDelta / WHEEL_DELTA;

		zoom_ += step;
	}

	return TRUE;
}


/**
 *	This MFC method prevents the parent from setting our cursor.
 *
 *  @param pWnd		Window the cursor is over.
 *  @param nHitTest	Hit test location.
 *	@param message	Mouse message
 *	@return		TRUE to prevent further processing, FALSE to relay to the parent.
 */
BOOL GraphView::OnSetCursor( CWnd * pWnd, UINT nHitTest, UINT message )
{
	if (pWnd == this)
	{
		return TRUE;
	}

	return FALSE;
}


/**
 *	This method adjusts the panning to the new window size if necessary.
 *
 *  @param redraw	True to also redraw the window.
 */
void GraphView::clipCanvasPosition( bool redraw /* = true */ )
{
	BW_GUARD;

	int x = panOffset_.x;
	int y = panOffset_.y;

	CRect thisRect;
	GetClientRect( thisRect );

	// Calc width & height needed
	int w = 100;
	int h = 100;

	for (NodeViewMap::const_iterator it = nodeViews_.begin();
		it != nodeViews_.end(); ++it )
	{
		if ((*it).second->rect().right > w)
		{
			w = (*it).second->rect().right;
		}

		if ((*it).second->rect().bottom > h)
		{
			h = (*it).second->rect().bottom;
		}
	}

	w += CANVAS_WIDTH_MARGIN;
	h += CANVAS_HEIGHT_MARGIN;

	// Adjust paning
	if (x > 0)
	{
		x = 0;
	}
	else if (w + x < thisRect.Width())
	{
		x = std::min( 0, thisRect.Width() - w );
	}

	if (y > 0)
	{
		y = 0;
	}
	else if (h + y < thisRect.Height())
	{
		y = std::min( 0, thisRect.Height() - h );
	}

	panOffset_.x = x;
	panOffset_.y = y;

	if (redraw)
	{
		RedrawWindow();
	}
}


/**
 *	This method handles panning by the mouse.
 *
 *  @param point	Current mouse position.
 */
void GraphView::doPanning( const CPoint & point )
{
	BW_GUARD;

	CPoint scrPoint( point );
	ClientToScreen( &scrPoint );

	if (!startedPan_)
	{
		startedPan_ = true;
		lastMousePos_ = scrPoint;
	}
	else
	{
		panOffset_ += (scrPoint - lastMousePos_);
		SetCursorPos( lastMousePos_.x, lastMousePos_.y );

		clipCanvasPosition();
	}
}


/**
 *	This method handles stopping to pan by the mouse.
 *
 *  @param point	Current mouse position.
 */
void GraphView::stopPanning( const CPoint & point )
{
	BW_GUARD;

	startedPan_ = false;

	clipCanvasPosition();
}


/**
 *	This method handles pressing down a mouse button.
 *
 *  @param point	Current mouse position.
 *  @param acceptDrag	Whether or not this button press could result in a drag
 *						operation starting.
 */
void GraphView::internalBtnDown( const CPoint & point, bool acceptDrag )
{
	BW_GUARD;

	CPoint panPoint( point );
	panPoint += CPoint( -panOffset_.x, -panOffset_.y );

	SetFocus();

	SetCapture();
	
	bool spaceBarPressed = GetAsyncKeyState( VK_SPACE ) < 0;
	if (acceptDrag && !spaceBarPressed)
	{
		for (NodeViewMap::const_iterator it = nodeViews_.begin();
			it != nodeViews_.end(); ++it)
		{
			if ((*it).second->hitTest( panPoint ))
			{
				btnDownNode_ = (*it).second;
				btnDownPt_ = point;
				(*it).second->leftBtnDown( panPoint );
				break;
			}
		}
	}

	if (!btnDownNode_)
	{
		panning_ = true;
	}
}


/**
 *	This method handles releasing up a mouse button.
 *
 *  @param point	Current mouse position.
 */
void GraphView::internalBtnUp( const CPoint & point )
{
	BW_GUARD;

	CPoint panPoint( point );
	panPoint += CPoint( -panOffset_.x, -panOffset_.y );

	if (btnDownNode_)
	{
		for (NodeViewMap::const_iterator it = nodeViews_.begin();
			it != nodeViews_.end(); ++it)
		{
			if ((*it).second->hitTest( panPoint ) && (*it).second == btnDownNode_)
			{
				selectedNode_ = (*it).second;
				(*it).second->leftClick( panPoint );
				break;
			}
		}
		
		if (dragging_)
		{
			btnDownNode_->endDrag( panPoint );
		}
		
		btnDownNode_ = NULL;
	}
	else
	{
		stopPanning( point );
	}

	if (GetCapture() == this)
	{
		ReleaseCapture();
	}

	dragging_ = false;
	cloning_ = false;
	panning_ = false;
}


} // namespace Graph
BW_END_NAMESPACE

