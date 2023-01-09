
#include "pch.hpp"
#include "effect_node_view.hpp"
#include "effect_node.hpp"
#include "node_callback.hpp"
#include "node_resource_holder.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "view_skin.hpp"
#include "view_draw_utils.hpp"
#include "controls/utils.hpp"
#include "post_processing/effect.hpp"
#include "gizmo/general_properties.hpp"
#include "resource.h"

BW_BEGIN_NAMESPACE

namespace
{
	NodeResourceHolder * s_res = NULL; // used for brevity.
	
	int HIDEBUTTON_RIGHT_MARGIN = 6;
	int HIDEBUTTON_BOTTOM_MARGIN = 6;
} // anonymous namespace


/**
 *	Constructor.
 *
 *	@param graphView	Graph View object that is be the canvas for this view.
 *	@param node		Effect node object this view represents.
 */
EffectNodeView::EffectNodeView( Graph::GraphView & graphView, const EffectNodePtr node ) :
	node_( node ),
	rect_( 0, 0, 0, 0 )
{
	BW_GUARD;

	if (!graphView.registerNodeView( node.get(), this ))
	{
		ERROR_MSG( "EffectNodeView: The node is not in the graph.\n" );
	}

	UINT bmpIDs[] = { IDB_EFFECT_NODE_BTN_ON, IDB_EFFECT_NODE_BTN_OFF, IDB_PP_NODE_DELETE };
	NodeResourceHolder::addUser( bmpIDs, ARRAY_SIZE( bmpIDs ) );
	s_res = NodeResourceHolder::instance();

	rect_.right = ViewSkin::effectNodeSize().cx;
	rect_.bottom = ViewSkin::effectNodeSize().cy;
}


/**
 *	Destructor.
 */
EffectNodeView::~EffectNodeView()
{
	BW_GUARD;

	NodeResourceHolder::removeUser();
}


/**
 *	This method returns the Z order, or depth, of the node.
 *
 *	@return	-1 if the node is being dragged, 0 if it's not.
 */
int EffectNodeView::zOrder() const
{
	BW_GUARD;

	return node_->dragging() ? -1 : 0;
}


/**
 *	This method returns the alpha transparency value of the node.
 *
 *	@return	ViewSkin::dragAlpha if the node is being dragged, 255 if it's not.
 */
int EffectNodeView::alpha() const
{
	BW_GUARD;

	return node_->dragging() ? ViewSkin::dragAlpha() : 255;
}


/**
 *	This method sets the current position for this node.
 *
 *	@param pos	Position of the node in the canvas.
 */
void EffectNodeView::position( const CPoint & pos )
{
	BW_GUARD;

	rect_.OffsetRect( pos.x - rect_.left, pos.y - rect_.top );
}


/**
 *	This method draws this node into the canvas.
 *
 *	@param dc		Device context where the draw operations should go.
 *	@param frame	Frame number, ignored.
 *	@param state	State of the node, hovering, selected, etc (see STATE).
 */
void EffectNodeView::draw( CDC & dc, uint32 frame, STATE state )
{
	BW_GUARD;

	// Draw node body
	COLORREF grad1 = node_->active() ? ViewSkin::effectActiveGradient1() : ViewSkin::effectInactiveGradient1();
	COLORREF grad2 = node_->active() ? ViewSkin::effectActiveGradient2() : ViewSkin::effectInactiveGradient2();
	CBrush * nodeBrush = s_res->gradientBrush(
								grad1, grad2, rect_.Height(), true /* vertical */ );

	if (nodeBrush)
	{
		CRect nodeRect = rect_;
		nodeRect.DeflateRect( 1, 1 );
		
		CPoint dcOrg = dc.GetViewportOrg();
		dc.SetBrushOrg( CPoint( rect_.TopLeft() + dcOrg ) );

		int penWidth = (state & SELECTED) ? ViewSkin::nodeSelectedEdgeSize() : ViewSkin::nodeNormalEdgeSize();
		int penColour = (state & SELECTED) ? ViewSkin::nodeSelectedEdge() : ViewSkin::nodeNormalEdge();
		CPen nodePen( PS_SOLID, penWidth, penColour );

		ViewDrawUtils::drawRect( dc, nodeRect, *nodeBrush, nodePen, ViewSkin::nodeEdgeCurve());
	}

	// Draw text
	CRect textRect = rect_;
	textRect.DeflateRect( ViewSkin::nodeRectTextMargin() );

	COLORREF oldTxtCol =
		dc.SetTextColor( node_->active() ? ViewSkin::effectFontActive() : ViewSkin::effectFontInactive() );
	CFont * oldFont = dc.SelectObject( &(s_res->font()) );

	dc.SetBkMode( TRANSPARENT );

	dc.DrawText( bw_utf8tow( node_->name() ).c_str(), &textRect, DT_WORDBREAK | DT_END_ELLIPSIS );

	dc.SelectObject( oldFont );
	dc.SetTextColor( oldTxtCol );

	// Draw active button
	int bmpID = node_->active() ? IDB_EFFECT_NODE_BTN_ON : IDB_EFFECT_NODE_BTN_OFF;
	ViewDrawUtils::drawBitmap( dc, s_res->bitmap( bmpID ),
				rect_.right - s_res->bitmapWidth( bmpID ) - HIDEBUTTON_RIGHT_MARGIN,
				rect_.bottom - s_res->bitmapHeight( bmpID ) - HIDEBUTTON_BOTTOM_MARGIN,
				s_res->bitmapWidth( bmpID ), s_res->bitmapHeight( bmpID ),
				true /* force draw transparent */ );

	// Draw the delete button
	if (state & HOVER)
	{
		ViewDrawUtils::drawBitmap( dc, s_res->bitmap( IDB_PP_NODE_DELETE ),
					rect_.right - s_res->bitmapWidth( IDB_PP_NODE_DELETE ), rect_.top,
					s_res->bitmapWidth( IDB_PP_NODE_DELETE ), s_res->bitmapHeight( IDB_PP_NODE_DELETE ),
					true /* force draw transparent */ );
	}
}


/**
 *	This method handles when the user clicks the left mouse button and if so it
 *	notifies the node's callback, and if the user clicked on the delete box, 
 *	the node's callback will also get notified that the node is being deleted.
 *
 *	@param pt	Mouse position.
 */
void EffectNodeView::leftClick( const CPoint & pt )
{
	BW_GUARD;

	if (pt.x > rect_.right - s_res->bitmapWidth( IDB_EFFECT_NODE_BTN_ON ) - HIDEBUTTON_RIGHT_MARGIN &&
		pt.x < rect_.right &&
		pt.y > rect_.bottom - s_res->bitmapHeight( IDB_EFFECT_NODE_BTN_ON ) - HIDEBUTTON_BOTTOM_MARGIN &&
		pt.y < rect_.bottom)
	{
		node_->active( !node_->active() );
	}

	if (node_->callback())
	{
		node_->callback()->nodeClicked( node_.get() );

		if (pt.x > rect_.right - s_res->bitmapWidth( IDB_PP_NODE_DELETE ) &&
			pt.x < rect_.right &&
			pt.y > rect_.top &&
			pt.y < rect_.top + s_res->bitmapHeight( IDB_PP_NODE_DELETE ))
		{
			node_->callback()->nodeDeleted( node_.get() );
		}
	}
}


/**
 *	This method handles when the user starts dragging the node arround, simply
 *	flagging the node as dragging=true and forwarding the notification to the
 *	callback.
 *
 *	@param pt	Mouse position.
 */
void EffectNodeView::doDrag( const CPoint & pt )
{
	BW_GUARD;

	node_->dragging( true );
	if (node_->callback())
	{
		node_->callback()->doDrag( pt, node_.get() );
	}
}


/**
 *	This method handles when the user ends dragging the node arround, simply
 *	flagging the node as dragging=false and forwarding the notification to the
 *	callback.
 *
 *	@param pt	Mouse position.
 */
void EffectNodeView::endDrag( const CPoint & pt, bool canceled /*= false*/ )
{
	BW_GUARD;

	node_->dragging( false );
	if (node_->callback())
	{
		node_->callback()->endDrag( pt, node_.get(), canceled );
	}
}

BW_END_NAMESPACE

