
#include "pch.hpp"
#include "phase_node_view.hpp"
#include "phase_node.hpp"
#include "node_resource_holder.hpp"
#include "node_callback.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "view_skin.hpp"
#include "view_draw_utils.hpp"
#include "controls/utils.hpp"
#include "post_processing/phase.hpp"
#include "gizmo/general_properties.hpp"
#include "resource.h"
#include "moo/texture_lock_wrapper.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	NodeResourceHolder * s_phaseNodeViewRes = NULL; // used for brevity.

	const int PREVIEW_INTERLEAVE_CONSTANT = 4;
	int s_nextInterleave = 0;
} // anonymous namespace


/**
 *	Constructor.
 *
 *	@param graphView	Graph View object that is be the canvas for this view.
 *	@param node		Phase node object this view represents.
 *	@param preview	Post processing preview object, used for showing a preview
 *					image of this Phase's result inside the node view.
 */
PhaseNodeView::PhaseNodeView( Graph::GraphView & graphView, const PhaseNodePtr node, PPPreviewPtr preview ) :
	node_( node ),
	rect_( 0, 0, 0, 0 ),
	renderTargetDragRect_( 0, 0, 0, 0 ),
	isRenderTargetDragging_( false ),
	pPreviewImg_( NULL ),
	previewInterleave_( (s_nextInterleave++ % PREVIEW_INTERLEAVE_CONSTANT) ),
	lastOutputW_( 0 ),
	lastOutputH_( 0 ),
	preview_( preview )
{
	BW_GUARD;

	if (!graphView.registerNodeView( node.get(), this ))
	{
		ERROR_MSG( "PhaseNodeView: The node is not in the graph.\n" );
	}

	UINT bmpIDs[] = { IDB_PP_NODE_DELETE };
	NodeResourceHolder::addUser( bmpIDs, ARRAY_SIZE( bmpIDs ) );
	s_phaseNodeViewRes = NodeResourceHolder::instance();

	rect_.right = ViewSkin::phaseNodeSize().cx;
	rect_.bottom = ViewSkin::phaseNodeSize().cy;
}


/**
 *	Destructor.
 */
PhaseNodeView::~PhaseNodeView()
{
	BW_GUARD;

	raw_free( pPreviewImg_ );
	NodeResourceHolder::removeUser();
}


/**
 *	This method returns the Z order, or depth, of the node.
 *
 *	@return	-1 if the node is being dragged, 0 if it's not.
 */
int PhaseNodeView::zOrder() const
{
	BW_GUARD;

	return node_->dragging() ? -1 : 0;
}


/**
 *	This method returns the alpha transparency value of the node.
 *
 *	@return	ViewSkin::dragAlpha if the node is being dragged, 255 if it's not.
 */
int PhaseNodeView::alpha() const
{
	BW_GUARD;

	return node_->dragging() ? ViewSkin::dragAlpha() : 255;
}


/**
 *	This method sets the current position for this node.
 *
 *	@param pos	Position of the node in the canvas.
 */
void PhaseNodeView::position( const CPoint & pos )
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
void PhaseNodeView::draw( CDC & dc, uint32 frame, STATE state )
{
	BW_GUARD;

	// Draw node body
	COLORREF grad1 = node_->active() ? ViewSkin::phaseActiveGradient1() : ViewSkin::phaseInactiveGradient1();
	COLORREF grad2 = node_->active() ? ViewSkin::phaseActiveGradient2() : ViewSkin::phaseInactiveGradient2();
	CBrush * nodeBrush = s_phaseNodeViewRes->gradientBrush(
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

		ViewDrawUtils::drawRect( dc, nodeRect, *nodeBrush, nodePen, ViewSkin::nodeEdgeCurve() );
	}

	dc.SetBkMode( TRANSPARENT );

	// Draw preview
	bool drawPreview = BasePostProcessingNode::previewMode();

	if (drawPreview && preview_->isVisible(this))
	{
		BW_GUARD_PROFILER( PhaseNodeView_drawPreview );

		bool previewDrawn = false;
		
		int rtx = 6;
		int rty = 2;
		int rtw = ViewSkin::phaseNodeSize().cx - rtx * 2;
		int rth = ViewSkin::phaseNodeSize().cy - rty * 2;

		if (node_->active())
		{
			bool needsCalc = ((frame % PREVIEW_INTERLEAVE_CONSTANT) == previewInterleave_);
			if (!pPreviewImg_)
			{
				// Doug Lea's malloc has a known issue with pixel buffers and
				// SetDIBits and similar GDI functions, so must use raw_malloc.
				pPreviewImg_ = (char*)raw_malloc(rtw * rth * 4);
				needsCalc = true;
			}
			
			if (needsCalc)
			{
				CRect srcRect;
				if (preview_->phaseRectForResults(node_->pyPhase().get(), srcRect))
				{
					calcPreviewImg( preview_->pPreview(), srcRect, rtw, rth, pPreviewImg_ );
				}
			}
	
			BITMAPINFO bmpInfo;
			ZeroMemory( &bmpInfo, sizeof( bmpInfo ) );
			bmpInfo.bmiHeader.biSize = sizeof( bmpInfo.bmiHeader );
			bmpInfo.bmiHeader.biWidth = rtw;
			bmpInfo.bmiHeader.biHeight = rth;
			bmpInfo.bmiHeader.biPlanes = 1;
			bmpInfo.bmiHeader.biBitCount = 32;
			bmpInfo.bmiHeader.biCompression = BI_RGB;

			SetDIBitsToDevice( dc.GetSafeHdc(),
				rect_.left + rtx, rect_.top + rty, rtw, rth,
				0, 0, 0, rth, pPreviewImg_,
				&bmpInfo, DIB_RGB_COLORS );

			previewDrawn = true;
		}

		if (!previewDrawn)
		{
			dc.FillSolidRect( rtx + rect_.left, rty + rect_.top, rtw, rth, RGB( 0, 0, 0 ) );

			if (!(state & HOVER))
			{
				COLORREF oldTxtCol = dc.SetTextColor( RGB( 40, 40, 40 ) );
				CFont * oldFont = dc.SelectObject( &(s_phaseNodeViewRes->bigFont()) );

				dc.DrawText( Localise( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PHASE_PREVIEW_NOT_AVAILABLE" ),
							&rect_, DT_SINGLELINE | DT_CENTER | DT_VCENTER );

				dc.SelectObject( oldFont );
				dc.SetTextColor( oldTxtCol );
			}
		}
	}

	if (!drawPreview || (state & HOVER))
	{
		// Draw text
		CRect textRect = rect_;
		textRect.DeflateRect( ViewSkin::nodeRectTextMargin() );

		int fullBottom = textRect.bottom;
		textRect.bottom -= s_phaseNodeViewRes->smallFontHeight();
		if (textRect.Height() > s_phaseNodeViewRes->fontHeight() * 3)
		{
			// Have heaps of room, so save an extra line for the bottom text.
			textRect.bottom -= s_phaseNodeViewRes->smallFontHeight();
		}

		CFont * oldFont = dc.SelectObject( &(s_phaseNodeViewRes->font()) );

		COLORREF oldTxtCol = dc.GetTextColor();

		BW::wstring nodeName;
		bw_utf8tow( node_->getName(), nodeName );

		if (drawPreview)
		{
			dc.SetTextColor( RGB( 0, 0, 0 ) );
			textRect.OffsetRect( 1, 1 );
			dc.DrawText( nodeName.c_str(), &textRect, DT_WORDBREAK | DT_END_ELLIPSIS );
			textRect.OffsetRect( -1, -1 );
		}

		dc.SetTextColor( node_->active() ? ViewSkin::phaseFontActive() : ViewSkin::phaseFontInactive() );

		dc.DrawText( nodeName.c_str(), &textRect, DT_WORDBREAK | DT_END_ELLIPSIS );

		// Draw output render target drag button
		renderTargetDragRect_ = CRect( textRect.right - 5, textRect.bottom + 1, textRect.right + 6, textRect.bottom + 12 );
		if (state & HOVER)
		{
			CPen ellipsePen( PS_SOLID, 1, ViewSkin::nodeNormalEdge() );
			CBrush ellipseBrush( ViewSkin::phaseFontInactive() );

			CPen * oldPen = dc.SelectObject( &ellipsePen );
			CBrush * oldBrush = dc.SelectObject( &ellipseBrush );
			dc.Ellipse( renderTargetDragRect_ );
			dc.SelectObject( &oldBrush );
			dc.SelectObject( &oldPen );

			textRect.right -= 6;
		}

		// Draw output render target text
		dc.SelectObject( &(s_phaseNodeViewRes->smallFont()) );

		textRect.top = textRect.bottom;
		textRect.bottom = fullBottom;

		updateOutputString();

		if (drawPreview)
		{
			dc.SetTextColor( RGB( 0, 0, 0 ) );
			textRect.OffsetRect( 1, 1 );
			dc.DrawText( outputStr_.c_str(), &textRect, DT_WORDBREAK | DT_RIGHT | DT_BOTTOM | DT_WORD_ELLIPSIS );
			textRect.OffsetRect( -1, -1 );
		}
		dc.SetTextColor( ViewSkin::phaseFontInactive() );

		dc.DrawText( outputStr_.c_str(), &textRect, DT_WORDBREAK | DT_RIGHT | DT_BOTTOM | DT_WORD_ELLIPSIS );

		dc.SelectObject( oldFont );
		dc.SetTextColor( oldTxtCol );
	}

	// Draw the delete button
	if (state & HOVER)
	{
		ViewDrawUtils::drawBitmap( dc, s_phaseNodeViewRes->bitmap( IDB_PP_NODE_DELETE ),
					rect_.right - s_phaseNodeViewRes->bitmapWidth( IDB_PP_NODE_DELETE ), rect_.top,
					s_phaseNodeViewRes->bitmapWidth( IDB_PP_NODE_DELETE ), s_phaseNodeViewRes->bitmapHeight( IDB_PP_NODE_DELETE ),
					true /* force draw transparent */ );
	}
}


/**
 *	This method just draws the render target name.
 *
 *	@param dc		Device context where the draw operations should go.
 *	@param pos		Position at which to draw the text.
 */
void PhaseNodeView::drawRenderTargetName( CDC & dc, const CPoint & pos )
{
	int oldBgMode = dc.SetBkMode( OPAQUE );
	COLORREF oldBgCol = dc.SetBkColor( ViewSkin::bkColour() );
	COLORREF oldTxtCol = dc.SetTextColor( ViewSkin::phaseFontActive() );
	CFont * oldFont = dc.SelectObject( &(s_phaseNodeViewRes->smallFont()) );

	int w = 200;
	int h = s_phaseNodeViewRes->smallFontHeight();
	CPoint centrePos( pos.x - w - renderTargetDragRect_.Width(), pos.y - h / 2 );
	CRect textRect( centrePos, CSize( w, h ) );
	dc.DrawText( bw_utf8tow( node_->output() ).c_str(), textRect, DT_RIGHT );

	dc.SelectObject( oldFont );
	dc.SetTextColor( oldTxtCol );
	dc.SetBkColor( oldBgCol );
	dc.SetBkMode( oldBgMode );
}


/**
 *	This method handles when the user presses the left mouse button in order to
 *	track whether or not the user is dragging this Phase's render target.
 *
 *	@param pt	Mouse position.
 */
void PhaseNodeView::leftBtnDown( const CPoint & pt )
{
	BW_GUARD;

	isRenderTargetDragging_ = (renderTargetDragRect_.PtInRect( pt ) == TRUE);
}


/**
 *	This method handles when the user clicks the left mouse button and if so it
 *	notifies the node's callback, and if the user clicked on the delete box, 
 *	the node's callback will also get notified that the node is being deleted.
 *
 *	@param pt	Mouse position.
 */
void PhaseNodeView::leftClick( const CPoint & pt )
{
	BW_GUARD;

	isRenderTargetDragging_ = false;

	if (node_->callback())
	{
		node_->callback()->nodeClicked( node_.get() );

		if (pt.x > rect_.right - s_phaseNodeViewRes->bitmapWidth( IDB_PP_NODE_DELETE ) &&
			pt.x < rect_.right &&
			pt.y > rect_.top &&
			pt.y < rect_.top + s_phaseNodeViewRes->bitmapHeight( IDB_PP_NODE_DELETE ))
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
void PhaseNodeView::doDrag( const CPoint & pt )
{
	BW_GUARD;

	if (!isRenderTargetDragging_)
	{
		node_->dragging( true );
	}
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
void PhaseNodeView::endDrag( const CPoint & pt, bool canceled /*= false*/ )
{
	BW_GUARD;

	isRenderTargetDragging_ = false;

	node_->dragging( false );
	if (node_->callback())
	{
		node_->callback()->endDrag( pt, node_.get(), canceled );
	}
}


/**
 *	This method creates a preview image of this Phase's output.
 *
 *	@param pPreview		Preview object's preview texture.
 *	@param srcRect		Source rectangle in the preview texture that
 *						corresponds to this node.
 *	@param width		Width of the node's preview area.
 *	@param height		Height of the node's preview area.
 *	@param retPixels	Return param, pixels if the Phase's preview image.
 *	@return	True if successful, false otherwise.
 */
bool PhaseNodeView::calcPreviewImg( ComObjectWrap<DX::Texture> pPreview, const CRect& srcRect, int width, int height, char * retPixels ) const
{
	BW_GUARD;

	bool ok = false;

	if (pPreview)
	{
		D3DSURFACE_DESC desc;
		if (!SUCCEEDED( pPreview->GetLevelDesc( 0, &desc ) ))
		{
			return false;
		}

		D3DLOCKED_RECT lockedRect;
		RECT rc(srcRect);

		Moo::TextureLockWrapper textureLock ( pPreview );
		if (!SUCCEEDED( textureLock.lockRect( 0, &lockedRect, &rc,
						D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY ) ))
		{
			return false;
		}

		// No more early returns from here
		int rtw = srcRect.Width();
		int rth = srcRect.Height();
		int rtf = desc.Format;
		int bpp = (rtf == D3DFMT_R8G8B8) ? 3 : 4;

		if (width == rtw && height == rth && bpp == 4)
		{
			uint32* srcbits;
			uint32* dstbits = (uint32*)retPixels;
			uint32 stride = (lockedRect.Pitch/4);

			for (int iy = 0; iy < height; ++iy)
			{
				srcbits = (uint32*)lockedRect.pBits + (height-iy-1) * stride;
				for (int ix = 0; ix < width; ++ix)
				{
					*dstbits++ = *srcbits++;
				}
			}
			ok = true;
		}
		else
		{
			const char * rtbits = (char*)lockedRect.pBits;
			int rtrow = lockedRect.Pitch;
			int bpp = (rtf == D3DFMT_R8G8B8) ? 3 : 4;
			char * pixel = retPixels;
			for (int iy = 0; iy < height; ++iy)
			{
				const char * rtrowbits = rtbits + 
							/* strecthed Y */ (rtrow * ((height - iy - 1) * rth / height));
				for (int ix = 0; ix < width; ++ix)
				{
					const char * rtpix = rtrowbits +
							/* strecthed X */ (bpp * (ix * rtw / width));

					pixel[0] = rtpix[0];
					pixel[1] = rtpix[1];
					pixel[2] = rtpix[2];
					pixel[3] = 0;
					pixel += 4;
				}
			}
			ok = true;
		}

		textureLock.unlockRect( 0 );
	}
	return ok;
}


/**
 *	This method updates the output render target name from the node.
 */
void PhaseNodeView::updateOutputString()
{
	BW_GUARD;

	BW::string output = node_->output();
	int w = 0;
	int h = 0;
	Moo::BaseTexturePtr pTex;
	if (output == PhaseNode::BACK_BUFFER_STR)
	{
		w = Moo::rc().backBufferDesc().Width;
		h = Moo::rc().backBufferDesc().Height;
	}
	else
	{
		pTex = Moo::TextureManager::instance()->get( node_->output() );
		if (pTex)
		{
			w = pTex->width();
			h = pTex->height();
		}
	}
	if (lastOutput_ != output || lastOutputW_ != w || lastOutputH_ != h)
	{
		lastOutput_ = output;
		lastOutputW_ = w;
		lastOutputH_ = h;
		char buf[20];
		bw_snprintf( buf, sizeof( buf ), "\n%dx%d", w, h );
		output.append( buf );
		bw_utf8tow( output, outputStr_ );
	}
}


/**
 *	This method returns the corresponding post-processing Phase Phyton object.
 *
 *	@return	The corresponding post-processing Phase Phyton object.
 */
PostProcessing::Phase* PhaseNodeView::phase() const
{
	BW_GUARD;

	return node_->pyPhase().get();
}

BW_END_NAMESPACE

