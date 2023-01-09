
#include "pch.hpp"
#include "nice_splitter_wnd.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
NiceSplitterWnd::NiceSplitterWnd() :
	eventHandler_( 0 ),
	lastWidth_( 0 ),
	lastHeight_( 0 ),
	allowResize_( true ),
	minRowSize_( 0 ),
	minColSize_( 0 )
{
	BW_GUARD;
}


/**
 *	This method sets the event handler object that will receive notifications
 *	from this splitter window.
 *
 *	@param handler	Object derived from SplitterEventHandler that will receive
 *					notifications from this splitter window.
 */
void NiceSplitterWnd::setEventHandler( SplitterEventHandler* handler )
{
	BW_GUARD;

	eventHandler_ = handler;
}


/**
 *	This method sets whether or not this splitter allows its divisory line to
 *	be moved.
 *
 *	@param allow	True to allow the splitter division to be moved.
 */
void NiceSplitterWnd::allowResize( bool allow )
{
	BW_GUARD;

	allowResize_ = allow;
}


/**
 *	This method sets the minimum row size for a vertical splitter.
 *
 *	@param minSize	Minimum row size for a vertical splitter.
 */
void NiceSplitterWnd::setMinRowSize( int minSize )
{
	BW_GUARD;

	minRowSize_ = minSize;
}


/**
 *	This method sets the minimum column size for a vertical splitter.
 *
 *	@param minSize	Minimum column size for a vertical splitter.
 */
void NiceSplitterWnd::setMinColSize( int minSize )
{
	BW_GUARD;

	minColSize_ = minSize;
}


// Overrides


/**
 *	This MFC method performs custom drawing for a prettier splitter look.
 *
 *	@param pDC	MFC device context.
 *	@param nType	MFC splitter type.
 *	@param rectArg	MFC window rectangle.
 */
void NiceSplitterWnd::OnDrawSplitter( CDC* pDC, ESplitType nType, const CRect& rectArg )
{
	BW_GUARD;

	// this code was copied and modified from winsplit.cpp from the mfc source code.
	// this is the way microsoft sugests it (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vcmfc98/html/_mfcnotes_tn029.asp).

	// if pDC == NULL, then just invalidate
	if (pDC == NULL)
	{
		RedrawWindow(rectArg, NULL, RDW_INVALIDATE|RDW_NOCHILDREN);
		return;
	}
	ASSERT_VALID(pDC);

	// otherwise, actually draw
	CRect rect = rectArg;
	DWORD bodyCol = ::GetSysColor(COLOR_BTNFACE);
	DWORD shadowCol = ::GetSysColor(COLOR_BTNSHADOW);
	switch (nType)
	{
	case splitBorder:
		pDC->Draw3dRect(rect, bodyCol, bodyCol );
		rect.InflateRect( -::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER) );
		pDC->Draw3dRect(rect, shadowCol, shadowCol );
		return;

	case splitIntersection:
		break;

	case splitBox:
		pDC->Draw3dRect(rect, bodyCol, bodyCol );
		rect.InflateRect( -::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER) );
		pDC->Draw3dRect(rect, bodyCol, bodyCol );
		rect.InflateRect( -::GetSystemMetrics(SM_CXBORDER), -::GetSystemMetrics(SM_CYBORDER) );
		break;

	case splitBar:
		break;

	default:
		ASSERT(FALSE);  // unknown splitter type
	}

	// fill the middle
	pDC->FillSolidRect(rect, bodyCol );
}


/**
 *	This MFC method is overriden in order to prevent the cursor from changing
 *	if the splitter doesn't allow resizing its panes/moving its divisory line.
 *
 *	@param ht	MFC new cursor.
 */
void NiceSplitterWnd::SetSplitCursor(int ht)
{
	BW_GUARD;

	if ( !allowResize_ )
		return;

	CSplitterWnd::SetSplitCursor( ht );
}


/**
 *	This MFC method is overriden in order to prevent the divisory line from
 *	moving if the splitter doesn't allow resizing its panes.
 *
 *	@param ht	MFC new cursor.
 */
void NiceSplitterWnd::StartTracking(int ht)
{
	BW_GUARD;

	if ( !allowResize_ )
		return;

	CSplitterWnd::StartTracking( ht );
}


/**
 *	This MFC method is overriden in order to deal properly with child windows
 *	that have a caption bar (more than one panel in a floater in GUITABS). It
 *	also checks the minimum row size.
 *
 *	@param y	New Y size of the row.
 *	@param row	Row being resized.
 */
void NiceSplitterWnd::TrackRowSize(int y, int row)
{
	BW_GUARD;

	// Trick "y" by substracting the non client area (caption) of the pane,
	// so the CSplitterWnd method works properly with panes with caption bar.
	CRect paneRect;
	GetPane(row, 0)->GetWindowRect( &paneRect );
	CPoint pt(0, paneRect.top);
	GetPane(row, 0)->ScreenToClient(&pt);
	y -= pt.y; // diff between the window's top and the top of the client area

	CSplitterWnd::TrackRowSize( y, row );
	if ( m_pRowInfo[row].nIdealSize < minRowSize_ )
		m_pRowInfo[row].nIdealSize = minRowSize_;
}


/**
 *	This MFC method is overriden in order to checks the minimum column size.
 *
 *	@param x	New X size of the column.
 *	@param row	Column being resized.
 */
void NiceSplitterWnd::TrackColumnSize(int x, int col)
{
	BW_GUARD;

	CSplitterWnd::TrackColumnSize( x, col );
	if ( m_pColInfo[col].nIdealSize < minColSize_ )
		m_pColInfo[col].nIdealSize = minColSize_;
}


// MFC messages
BEGIN_MESSAGE_MAP(NiceSplitterWnd,CSplitterWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()


/**
 *	This MFC method is overriden in order to notify the event handler and to
 *	record the previous size.
 *
 *	@param x	New X size of the column.
 *	@param row	Column being resized.
 */
void NiceSplitterWnd::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if ( cx == 0 || cy == 0 )
		return;

	if ( eventHandler_ )
		eventHandler_->resizeSplitter( lastWidth_, lastHeight_, cx, cy );

	CSplitterWnd::OnSize( nType, cx, cy );

	lastWidth_ = cx;
	lastHeight_ = cy;
}

BW_END_NAMESPACE

