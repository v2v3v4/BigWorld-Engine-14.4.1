
#include "pch.hpp"
#include "popup_drag_target.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// Constants
	const int HORZ_MARGIN = 5;
	const int VERT_MARGIN = 7;
	const int ITEM_FONT_SIZE = 16;
	const int ITEM_HEIGHT = 20;
	const int ITEM_WIDTH = 110;
	const int ARROW_SIZE = 8;

	const CPoint BK_ROUNDNESS( 14, 14 );
	const CPoint HILITE_ROUNDNESS( 8, 8 );
	const COLORREF BK_COLOUR = RGB( 0, 0, 0 );
	const COLORREF KEY_COLOUR = RGB( 255, 0, 255 );
	const COLORREF TXT_COLOUR = RGB( 255, 255, 255 );
	const COLORREF HILITE_BK_COLOUR = RGB( 64, 64, 64 );
	const COLORREF HILITE_EDGE_COLOUR = RGB( 192, 192, 192 );
} // anonymous namespace


/**
 *	Constructor.
 */
PopupDragTarget::PopupDragTarget() :
	arrowUp_( true )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
PopupDragTarget::~PopupDragTarget()
{
	BW_GUARD;
}


/**
 *	This method calculates the size of the popup from the item list.
 *
 *	@param itemList	List of items in the popup.
 *	@return	Width and height of the popup.
 */
CSize PopupDragTarget::calcSize( const ItemList & itemList ) const
{
	BW_GUARD;

	size_t y = VERT_MARGIN * 2 + itemList.size() * ITEM_HEIGHT + ARROW_SIZE;
	MF_ASSERT( y <= INT_MAX );
	return CSize( HORZ_MARGIN * 2 + ITEM_WIDTH, ( int ) y );
}


/**
 *	This method opens the popup.
 *
 *	@param pt	Point in the screen where to create the popup.
 *	@param itemList	List of items to display on the popup.
 *	@param arrowUp	True to display the arrow above the popup's rect, false to
 *					display it below.
 *	@return	True if successful, false if not.
 */
bool PopupDragTarget::open( const CPoint & pt, const ItemList & itemList, bool arrowUp )
{
	BW_GUARD;

	items_ = itemList;
	arrowUp_ = arrowUp;

	CSize wndSize = calcSize( itemList );

	this->CreateEx( WS_EX_LAYERED | // Layered Windows
				WS_EX_TRANSPARENT | // Don't hittest this window
				WS_EX_TOPMOST | WS_EX_TOOLWINDOW, 
				AfxRegisterWndClass( NULL ), L"BWPopupDragTarget",
				WS_POPUP | WS_VISIBLE,
				pt.x - wndSize.cx / 2, pt.y, wndSize.cx, wndSize.cy,
				NULL, (HMENU)0 );

	if (!font_.GetSafeHandle())
	{
		NONCLIENTMETRICS metrics;
		metrics.cbSize = sizeof( metrics );
		SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
		metrics.lfCaptionFont.lfWeight = FW_NORMAL;
		metrics.lfCaptionFont.lfHeight = ITEM_FONT_SIZE;

		if (!font_.CreateFontIndirect( &metrics.lfCaptionFont ))
		{
			ERROR_MSG( "Could not create big font.\n" );
		}
	}

	return (this->GetSafeHwnd() != NULL);
}


/**
 *	This method updates the popup.
 *
 *	@param alpha	Alpha transparency for the popup.
 *	@return	The item the mouse is over, or the empty string.
 */
BW::string PopupDragTarget::update( int alpha )
{
	BW_GUARD;

	CRect rect;
	this->GetWindowRect( rect );

	// Create the DC for the translucent layered window
	HDC hdcScreen = ::GetDC( NULL );
	HDC hdc = ::CreateCompatibleDC( hdcScreen );
	HBITMAP memBmp = ::CreateCompatibleBitmap( hdcScreen, rect.Width(), rect.Height() );
	HBITMAP oldBmp = (HBITMAP)::SelectObject( hdc, memBmp );

	// Draw the background rounded rect
	CDC * pDC = CDC::FromHandle( hdc );

	pDC->FillSolidRect( 0, 0, rect.Width(), rect.Height(), KEY_COLOUR );

	CBrush bgRectBrush( BK_COLOUR );
	CPen bgRectPen( PS_SOLID, 1, BK_COLOUR );

	CBrush * pOldBrush = pDC->SelectObject( &bgRectBrush );
	CPen* pOldPen = pDC->SelectObject( &bgRectPen );

	pDC->RoundRect(
			CRect( 0, arrowUp_ ? ARROW_SIZE : 0,
				rect.Width(), arrowUp_ ? rect.Height() : rect.Height() - ARROW_SIZE ),
			BK_ROUNDNESS );

	// Draw the tip, pointing either up or down.
	CPoint arrowPts[3];
	arrowPts[0] = CPoint( rect.Width() / 2 - ARROW_SIZE, arrowUp_ ? ARROW_SIZE : rect.Height() - ARROW_SIZE );
	arrowPts[1] = CPoint( rect.Width() / 2, arrowUp_ ? 0 : rect.Height() );
	arrowPts[2] = CPoint( rect.Width() / 2 + ARROW_SIZE, arrowUp_ ? ARROW_SIZE : rect.Height() - ARROW_SIZE );
	pDC->Polygon( arrowPts, 3 );

	pDC->SelectObject( pOldBrush );
	pDC->SelectObject( pOldPen );

	BW::string retItem;

	// Calculate the item list starting position
	CPoint pt;
	::GetCursorPos( &pt );
	pt.Offset( -rect.left, -rect.top );
	CRect curItem( HORZ_MARGIN, VERT_MARGIN,
					HORZ_MARGIN + ITEM_WIDTH, VERT_MARGIN + ITEM_HEIGHT );
	if (arrowUp_)
	{
		curItem.OffsetRect( 0, ARROW_SIZE );
	}

	// And draw the item list
	COLORREF pOldTxtCol = pDC->SetTextColor( TXT_COLOUR );
	CFont * pOldFont = pDC->SelectObject( &font_ );

	for (ItemList::const_iterator it = items_.begin(); it != items_.end(); ++it)
	{
		COLORREF bkColour = BK_COLOUR;
		if (curItem.PtInRect( pt ))
		{
			// The cursor is over an item, return it as the selected item and
			// draw a highlight rectangle around it.
			retItem = *it;

			CRect hiliteRect( curItem );
			hiliteRect.InflateRect( 1, 0 );

			bkColour = HILITE_BK_COLOUR;
			CBrush hiliteRectBrush( HILITE_BK_COLOUR );
			CPen hiliteRectPen( PS_SOLID, 1, HILITE_EDGE_COLOUR );

			CBrush * pOldBrush = pDC->SelectObject( &hiliteRectBrush );
			CPen* pOldPen = pDC->SelectObject( &hiliteRectPen );

			pDC->RoundRect( hiliteRect, HILITE_ROUNDNESS );

			pDC->SelectObject( pOldBrush );
			pDC->SelectObject( pOldPen );
		}

		COLORREF pOldBkCol = pDC->SetBkColor( bkColour );
		
		size_t len = it->length();
		MF_ASSERT( len <= INT_MAX );
		pDC->DrawText( bw_utf8tow( *it ).c_str(), ( int ) len, curItem,
						DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS );

		pDC->SetBkColor( pOldBkCol );

		curItem.OffsetRect( 0, ITEM_HEIGHT );
	}

	pDC->SelectObject( pOldFont );
	pDC->SetTextColor( pOldTxtCol );

	// Draw the translucent layered window and release resources.
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.AlphaFormat = 0;
	blend.SourceConstantAlpha = alpha;
 
	POINT srcPt = { 0, 0 };

	CSize sz( rect.Size() );
	this->UpdateLayeredWindow( NULL, NULL, &sz, pDC, &srcPt,
							KEY_COLOUR, &blend, ULW_COLORKEY | ULW_ALPHA );

	::SelectObject( hdc, oldBmp );
	::DeleteObject( memBmp );
	::DeleteDC( hdc );
	::ReleaseDC( NULL, hdcScreen );

	return retItem;
}


/**
 *	This method closes the popup.
 */
void PopupDragTarget::close()
{
	BW_GUARD;

	if (this->GetSafeHwnd() != NULL)
	{
		this->DestroyWindow();
		items_.clear();
	}
}

BW_END_NAMESPACE

