
#include "pch.hpp"
#include "guitabs.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/guard.hpp"


BW_BEGIN_NAMESPACE

namespace GUITABS
{

// Drag manager constants
static const int INSERT_AREA_SIZE = 25;
static const int FLOATING_RECT_BORDER = 3;
static const int MIN_DRAG_DISTANCE = 10;
static const DWORD DRAG_RECT_COLOR = RGB( 96,96,96 );


/**
 *	Constructor.
 */
DragManager::DragManager() :
	tabCtrlAtTop_(),
	tab_( 0 ),
	destPanel_( 0 ),
	panel_( 0 ),
	tabInsertIndex_(),
	startX_(),
	startY_(),
	downX_(),
	downY_(),
	isDragging_( false ),
	isFloating_( false ),
	hilightSrc_( false ),
	wnd_( 0 ),
	dc_( 0 )
{
	BW_GUARD;
}


/**
 *	This method is called when the user starts dragging a panel. It initialises
 *	the drag and drop operation and enters into the drag loop.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 *	@param panel	Panel that contains the tab being dragged.
 *	@param tab	Tab being dragged.
 */
void DragManager::startDrag( int x, int y, PanelPtr panel, TabPtr tab )
{
	BW_GUARD;

	isDragging_ = true;
	
	tabCtrlAtTop_ = panel->isTabCtrlAtTop();

	FloaterPtr floater = Manager::instance().dock()->getFloaterByWnd( panel->getCWnd() );

	isFloating_ = ( !!floater );

	panel_ = panel;
	tab_ = tab;
	destPanel_ = 0;
	startX_ = x;
	startY_ = y;
	downX_ = x;
	downY_ = y;

	CRect rect;
	if ( tab_ && tab_->getCWnd() )
		tab_->getCWnd()->GetWindowRect( &rect );
	else
		panel_->GetWindowRect( &rect );
	int w = rect.Width();
	int h = rect.Height();
	
	if ( !isFloating_ )
		if ( tab_ )
			tab_->getPreferredSize( w, h );
		else
			panel_->getPreferredSize( w, h );
	
	if ( startX_ > rect.left + w )
		startX_ = rect.left + w / 2;
	
	if ( startY_ > rect.top + h )
		startY_ = rect.top + h / 2;

	if ( startX_ < rect.left )
		startX_ = rect.left + 10;
	
	if ( startY_ < rect.top )
		startY_ = rect.top + 10;

	prepareDrag();

	updateDrag( x, y );

	dragLoop();
}


/**
 *	This helper method prepares the drag managerfor entering the drag loop.
 */
void DragManager::prepareDrag()
{
	BW_GUARD;

	MSG msg;
	while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
			break;
		DispatchMessage(&msg);
	}

	// save last rect info for DrawDragRect
	lastIns_ = FLOATING;
	lastRect_.SetRectEmpty();
	lastRect_ = CRect( 0, 0, 100, 100 );
	lastSize_.cx = lastSize_.cy = 0;

	// lock window update while dragging
	wnd_ = CWnd::GetDesktopWindow();
//	if (wnd_->LockWindowUpdate())
//		dc_ = wnd->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
//	else
		dc_ = wnd_->GetDCEx(NULL, DCX_WINDOW|DCX_CACHE);

	if ( hilightSrc_ )
	{
		// hilight current panel/tab
		CRect rect;
		CBrush* brush = dc_->GetHalftoneBrush();
		if ( tab_ )
			tab_->getCWnd()->GetWindowRect( &rect );
		else
			panel_->getCWnd()->GetWindowRect( &rect );
		dc_->FillRect( rect, brush );
	}
}


/**
 *	This method implements the drag loop for the dragging of a panel or tab.
 */
void DragManager::dragLoop()
{
	BW_GUARD;

	// don't handle if capture already set
	if (::GetCapture() != NULL)
		return;

	// set capture to the window which received this message
	panel_->SetCapture();

	// This flag is used to properly validate when a panel is drag&dropped too
	// fast, or when a drag operation is interrupted by a modal dialog.
	bool lastMessage = false;

	// get messages until capture lost or cancelled/accepted
	while ( CWnd::GetCapture() == panel_->getCWnd() )
	{
		MSG msg;
		if ( !::GetMessage( &msg, NULL, 0, 0 ) )
		{
			AfxPostQuitMessage( (int)msg.wParam );
			break;
		}

		switch (msg.message)
		{
		case WM_LBUTTONUP:
			if ( isDragging_ )
				endDrag( msg.pt.x, msg.pt.y );
			return;
		case WM_MOUSEMOVE:
			if ( isDragging_ )
			{
				if ( GetAsyncKeyState( VK_LBUTTON ) < 0 )
					updateDrag( msg.pt.x, msg.pt.y );
				else
				{
					// Mouse button is not down for some reason. It could be
					// because it was drag&dropped too fast, or because the
					// drag was interrupted, maybe by a modal dialog.
					// Put a flag to process the next message and then end,
					// in case the last message is actually the LBUTTONUP of
					// a fast drag.
					lastMessage = true;
					continue;
				}
			}
			break;
		case WM_KEYDOWN:
			if ( msg.wParam == VK_ESCAPE )
			{
				cancelDrag();
				return;
			}
			else if ( msg.wParam == VK_CONTROL )
			{
				CPoint pt;
				GetCursorPos( &pt );
				updateDrag( pt.x, pt.y );
			}
			break;
		case WM_KEYUP:
			if ( msg.wParam == VK_CONTROL )
			{
				CPoint pt;
				GetCursorPos( &pt );
				updateDrag( pt.x, pt.y );
			}
			break;
		case WM_RBUTTONDOWN:
			cancelDrag();
			return;

		default:
			DispatchMessage( &msg );
			break;
		}
		if ( lastMessage )
			break;
	}

	cancelDrag();
}


/**
 *	This method is called to update the drop target and give visual feedback to
 *	the user about the potential resulting position for the panel.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void DragManager::updateDrag( int x, int y )
{
	BW_GUARD;

	if ( abs( downX_ - x ) > MIN_DRAG_DISTANCE || abs( downY_ - y ) > MIN_DRAG_DISTANCE )
	{
		// cursor far enough from the click point, so must be dragging
		CRect rect;
		DockNodePtr node;

		// get the drop node and insert position
		node = Manager::instance().dock()->getNodeByPoint( x, y );
		InsertAt ins = getInsertLocation( node, x, y, rect );

		if ( node && node->getCWnd() != panel_->getCWnd() && ins == TAB )
		{
			// dropping onto a different panel as a tab, setup the destination
			// panel and tab position
			PanelPtr curDestPanel = Manager::instance().dock()->getPanelByWnd( node->getCWnd() );
			if ( destPanel_ != curDestPanel )
			{
				// only change the destPanel if it has changed to avoid flicker
				drawDragRect( lastIns_, lastRect_, lastSize_ );
				if ( !!destPanel_ )
					destPanel_->removeTempTab();
				destPanel_ = curDestPanel;
				if ( !!destPanel_ )
				{
					if ( tab_ || panel_->visibleTabCount() == 1 )
					{
						destPanel_->insertTempTab( panel_->getActiveTab() );
					}
					else
					{
						if (tempTab_ == NULL)
						{
							// Create a temporal tab that, and keep it until
							// the drag manager is destroyed.
							tempTab_ = new TempTab();
						}
						tempTab_->setLabel( Localise(L"GUITABS/DRAG_MANAGER/MANY_TABS") );
						destPanel_->insertTempTab( tempTab_ );
					}
					destPanel_->updateTempTab( 0, 0 );
				}
				drawDragRect( lastIns_, lastRect_, lastSize_ );
			}
		}
		else
		{
			// not dropping onto a different panel as a tab, remove tab position
			if ( !!destPanel_ )
			{
				drawDragRect( lastIns_, lastRect_, lastSize_ );
				destPanel_->removeTempTab();
				drawDragRect( lastIns_, lastRect_, lastSize_ );
			}
			destPanel_ = 0;
		}

		if ( ins == TAB && node )
		{
			// inserting as a tab
			if ( node->getCWnd() == panel_->getCWnd() && tab_ )
			{
				// inserting in the same panel, so only reorder
				drawDragRect( lastIns_, lastRect_, lastSize_ );
				panel_->updateTabPosition( x, y );
				lastIns_ = ins;
				lastSize_ = CSize( 0, 0 );
				return;
			}
			else
			{
				// inserting as a tab in another panel, so update tab position
				if ( !!destPanel_ && ( tab_ || panel_->visibleTabCount() == 1 ) )
					destPanel_->updateTempTab( x, y );
			}
		}

		if ( node && node->getCWnd() == panel_->getCWnd() &&
			( !tab_ || ins == TAB ) )
			ins = FLOATING; // it's being dragged onto itself

		if ( ins == FLOATING )
		{
			// floating the panel, update the feedback rect properly
			if ( tab_ && tab_->getCWnd() )
				tab_->getCWnd()->GetWindowRect( &rect );
			else
				panel_->GetWindowRect( &rect );
			int w = rect.Width();
			int h = rect.Height();
			if ( !isFloating_ )
				if ( tab_ )
					tab_->getPreferredSize( w, h );
				else
					panel_->getPreferredSize( w, h );
			rect.OffsetRect( x - startX_, y - startY_ );
			rect.right = min( rect.right, w + rect.left );
			rect.bottom = min( rect.bottom, h + rect.top );
		}
		CSize size( FLOATING_RECT_BORDER, FLOATING_RECT_BORDER );

		if ( lastIns_ != ins || lastRect_ != rect || lastSize_ != size )
		{
			// erase the last rect
			drawDragRect( lastIns_, lastRect_, lastSize_ );
			// draw the new one, and remember last size
			drawDragRect( ins, rect, size );

			lastIns_ = ins;
			lastRect_ = rect;
			lastSize_ = size;
		}
	}
	else
	{
		// cursor too close to the click point, so don't drag
		drawDragRect( lastIns_, lastRect_, lastSize_ );

		lastIns_ = FLOATING;
		lastRect_ = CRect( 0, 0, 1, 1 );
		lastSize_ = CSize( 0, 0 );
	}
}


/**
 *	This method is called when the user successfully finishes dragging a panel.
 *
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void DragManager::endDrag( int x, int y )
{
	BW_GUARD;

	PanelPtr panel = panel_;
	TabPtr tab = tab_;
	PanelPtr destPanel = destPanel_;

	InsertAt ins = lastIns_;

	if ( ins == TAB && ( tab || panel->visibleTabCount() == 1 ) )
		cancelDrag( true );
	else
		cancelDrag( false );

	if ( abs( downX_ - x ) > MIN_DRAG_DISTANCE || abs( downY_ - y ) > MIN_DRAG_DISTANCE )
	{
		DockNodePtr node;

		node = Manager::instance().dock()->getNodeByPoint( x, y );
		
		if ( node && node->getCWnd() == panel->getCWnd() && tab && ins == TAB )
			return;

		CWnd* ptr = 0;
		if ( node )
			ptr = node->getCWnd();

		if ( ptr == 0 || ptr == panel->getCWnd() ) 
		{
			ptr = panel->getCWnd();

			if ( !tab || ins == TAB )
				ins = FLOATING;
		}

		if ( tab )
			Manager::instance().dock()->dockTab( panel, tab, ptr, ins, startX_, startY_, x, y );
		else
			Manager::instance().dock()->dockPanel( panel, ptr, ins, startX_, startY_, x, y );
	}
}


/**
 *	This method is called when the user stops dragging a panel either by
 *	canceling it, or by successfully ending the dragging (from endDrag).
 *
 *	@param leaveTempTab	True to leave the temporary tab in.
 */
void DragManager::cancelDrag( bool leaveTempTab )
{
	BW_GUARD;

	drawDragRect( lastIns_, lastRect_, lastSize_ );

	ReleaseCapture();

	if ( destPanel_ && !leaveTempTab )
		destPanel_->removeTempTab();

//	wnd->UnlockWindowUpdate();
	if ( dc_ != 0 )
	{
		wnd_->ReleaseDC( dc_ );
		dc_ = 0;
		wnd_ = 0;
	}

	panel_->recalcSize();
	panel_->RedrawWindow( 0, 0, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN );

	// remove references so these smart pointers don't keep owning the objects after endDrag
	panel_ = 0;
	tab_ = 0;
	destPanel_ = 0;
}


/**
 *	This method draws an XOR-ed rect to show feedback of the potential drop
 *	location of a dragged panel.
 *
 *	@param ins	Insertion type.
 *	@param rect	Rectangle position and size.
 *	@param size	Rectangle border width.
 */
void DragManager::drawDragRect( InsertAt ins, CRect rect, SIZE size )
{
	BW_GUARD;

	if ( size.cx == 0 || size.cy == 0 )
		return;

	CPen pen( PS_DOT, max( size.cx, size.cy ), DRAG_RECT_COLOR );

	CPen* oldPen = dc_->SelectObject( &pen );
	int oldROP = dc_->SetROP2( R2_NOTXORPEN );

	dc_->Rectangle( rect );

	dc_->SetROP2( oldROP );
	dc_->SelectObject( oldPen );
}


/**
 *	This method is called when the user successfully finishes dragging a panel.
 *
 *	@param node	At which the mouse is at.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 *	@param locationRect	Return param, the feedback rectangle.
 */
InsertAt DragManager::getInsertLocation( DockNodePtr node, int x, int y, CRect& locationRect )
{
	BW_GUARD;

	if ( !node )
		return FLOATING;

	CRect floatRect;

	node->getCWnd()->GetWindowRect( &locationRect );

	if ( GetAsyncKeyState( VK_LCONTROL ) < 0 || GetAsyncKeyState( VK_RCONTROL ) < 0 )
		return FLOATING;

	PanelPtr panel = Manager::instance().dock()->getPanelByWnd( node->getCWnd() );

	int captionSize = 0;
	int tabCtrlSize = 0;
	if ( panel )
	{
		captionSize = panel->getCaptionSize();
		tabCtrlSize = max( INSERT_AREA_SIZE, panel->getTabCtrlSize() );
	}

	CPoint pt( x, y );
	
	CRect captionRect = locationRect;
	captionRect.bottom = captionRect.top + captionSize;

	CRect tabRect = locationRect;
	if ( tabCtrlAtTop_ ) 
	{
		tabRect.top = captionRect.bottom;
		tabRect.bottom = tabRect.top + tabCtrlSize;
	}
	else
		tabRect.top = locationRect.bottom - tabCtrlSize;

	CRect insideRect = locationRect;
	insideRect.top += captionSize;

	if ( tabCtrlAtTop_ ) 
		insideRect.top += tabCtrlSize;
	else
		insideRect.bottom -= tabCtrlSize;
	

	CRect leftRect = insideRect;
	leftRect.right = leftRect.left + INSERT_AREA_SIZE;

	CRect rightRect = insideRect;
	rightRect.left = rightRect.right - INSERT_AREA_SIZE;
	
	// the +/- 10 tweak improves feedback when changing from inserting as a tab
	// to inserting into top/bottom and the dest panel has 1 tab (which toggles
	// the tabbar on/off deppending on the insert location
	CRect topRect = insideRect;
	topRect.bottom = topRect.top + INSERT_AREA_SIZE + ( tabCtrlAtTop_?10:0 );
	
	CRect bottomRect = insideRect;
	bottomRect.top = bottomRect.bottom - INSERT_AREA_SIZE - ( !tabCtrlAtTop_?10:0 );

	if ( leftRect.PtInRect( pt ) )
	{
		locationRect.right = locationRect.left + locationRect.Width()/2;
		return LEFT;
	}
	else if ( rightRect.PtInRect( pt ) )
	{
		locationRect.left = locationRect.right - locationRect.Width()/2;
		return RIGHT;
	}
	else if ( tabRect.PtInRect( pt ) )
	{
		locationRect = insideRect;
		return TAB;
	}
	else if ( topRect.PtInRect( pt ) || captionRect.PtInRect( pt ) )
	{
		locationRect.bottom = locationRect.top + locationRect.Height()/2;
		return TOP;
	}
	else if ( bottomRect.PtInRect( pt ) )
	{
		locationRect.top = locationRect.bottom - locationRect.Height()/2;
		return BOTTOM;
	}

	return FLOATING;
}

}	// namespace
BW_END_NAMESPACE

