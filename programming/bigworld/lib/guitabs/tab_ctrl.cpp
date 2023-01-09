/**
 *	GUI Tearoff panel framework - Tab Control widget class implementation
 */

#include "pch.hpp"
#include "guitabs.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

namespace GUITABS
{

int TabCtrl::s_tabbarHeight = 0;

/**
 *	Constructor.
 *
 *	@param parent	Parent window for the control (usually a Panel).
 *	@param align	Tab control display and behaviour style, TOP or BOTTOM.
 */
TabCtrl::TabCtrl( CWnd* parent, Alignment align ) :
	eventHandler_( 0 ),
	btnHilight_( PS_SOLID, 1, GetSysColor(COLOR_BTNHILIGHT) ),
	btnShadow_( PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW) ),
	btnSeparator_( PS_SOLID, 1,	// tab separator color is 1/4 the shadow color
		RGB(
			(GetRValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetRValue( GetSysColor(COLOR_BTNFACE) ))/4,
			(GetGValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetGValue( GetSysColor(COLOR_BTNFACE) ))/4,
			(GetBValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetBValue( GetSysColor(COLOR_BTNFACE) ))/4 )
		),
	curItem_( 0 ),
	multiline_( true ),
	numLines_( 1 ),
	align_( align )
{
	BW_GUARD;

	CreateEx(
		0,
		AfxRegisterWndClass( CS_DBLCLKS|CS_OWNDC, ::LoadCursor(NULL, IDC_ARROW), 0 ),
		L"TabCtrl",
		WS_CHILD,
		CRect( 0, 0, 100, s_tabbarHeight ),
		parent,
		0,
		0);

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	font_.CreateFontIndirect( &metrics.lfSmCaptionFont );

	if (s_tabbarHeight == 0)
	{
		CDC* winDC = this->GetWindowDC();
		CFont* saveFont = winDC->SelectObject( &font_ );

		s_tabbarHeight = winDC->GetTextExtent( _T( "H" ), 1 ).cy + TABBAR_VMARGIN * 2;

		winDC->SelectObject( saveFont );
		this->ReleaseDC( winDC );
	}
}


/**
 *	Destructor.
 */
TabCtrl::~TabCtrl()
{
	BW_GUARD;
}


/**
 *	This method returns the alignment style for the control, TOP or BOTTOM.
 *
 *	@return	The alignment style for the control, TOP or BOTTOM.
 */
TabCtrl::Alignment TabCtrl::getAlignment()
{
	BW_GUARD;

	return align_;
}


/**
 *	This method sets the event handler object to receive notifications from
 *	this control.
 *
 *	@param handler	Event handler that wants to receive notifications.
 */
void TabCtrl::setEventHandler( TabCtrlEventHandler* handler )
{
	BW_GUARD;

	eventHandler_ = handler;
}


/**
 *	This method inserts a tab to the control.
 *
 *	@param caption	Caption text for the new tab.
 *	@param icon		Icon to show next to the caption text, or NULL for no icon.
 *	@param itemData	Custom item data associated with the tab, used as an ID and
 *					passed into events to be used by the event handler.
 */
void TabCtrl::insertItem( const BW::wstring & caption, HICON icon, void* itemData )
{
	BW_GUARD;

	if ( getItem( itemData ) )
		return;
	itemList_.push_back( TabCtrlItem( caption, icon, itemData ) );
	paint();
}


/**
 *	This method removes a tab from the control.
 *
 *	@param itemData	Custom item data that identifies the tab.
 */
void TabCtrl::removeItem( void* itemData )
{
	BW_GUARD;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			itemList_.erase( i );
			break;
		}
	}

	if ( curItem_ >= (int)itemList_.size() )
		curItem_ = 0;
	paint();
}


/**
 *	This method returns whether or not this control contains a tab.
 *
 *	@param itemData	Custom item data that identifies the tab.
 *	@return	True if the tab is in the control, false if not.
 */
bool TabCtrl::contains( void* itemData )
{
	BW_GUARD;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		if ( (*i).itemData == itemData )
			return true;

	return false;
}


/**
 *	This method returns the number of tabs/items in the control.
 *
 *	@return The number of tabs/items in the control.
 */
int TabCtrl::itemCount()
{
	BW_GUARD;

	return (int)itemList_.size();
}


/**
 *	This method returns the custom item data associated with the tab item at
 *	position "index".
 *
 *	@param index	Index of the desired tab item in the internal list.
 *	@return	Custom item data associated with the tab item at position "index".
 */
void* TabCtrl::getItemData( int index )
{
	BW_GUARD;

	if ( index < 0 || index >= (int)itemList_.size() )
		return 0;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, --index )
		if ( !index )
			return (*i).itemData;

	return 0;
}


/**
 *	This method returns the height of the control.
 *
 *	@return	The height of the control.
 */
int TabCtrl::getHeight()
{
	BW_GUARD;

	return TabCtrl::s_tabbarHeight * numLines_;
}


/**
 *	This method adjusts the height of the control and the width of elements in
 *	the lines of the control.
 *
 *	@return	Height of the control.
 */
int TabCtrl::recalcHeight()
{
	BW_GUARD;

	int ret = recalcHeightOnly();
	recalcLineSizes();
	return ret;
}


/**
 *	This method adjusts the height of the control only.
 *
 *	@return	Height of the control.
 */
int TabCtrl::recalcHeightOnly()
{
	BW_GUARD;

	// startup
	numLines_ = 1;

	if ( itemList_.size() == 0 )
		return 0;

	CClientDC dc(this);

	CRect rect;
	GetClientRect( &rect );

	CFont* oldFont = dc.SelectObject( &font_ );

	// Calc the total size to see if it overflows the actual window size
	int totalSize = TABBAR_HMARGIN;
	int lineSize = TABBAR_HMARGIN;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		CString caption( (*i).caption.c_str() );
		int itemSize = 0;
		if ( (*i).icon )
			itemSize += ICON_SIZE + TABBAR_HMARGIN;
		itemSize += dc.GetTextExtent( caption ).cx + TABBAR_HMARGIN*2;
		totalSize += itemSize;
		if ( multiline_ )
		{
			lineSize += itemSize;
			if ( lineSize + TABBAR_HMARGIN > rect.Width() )
			{
				numLines_++;
				lineSize = TABBAR_HMARGIN + itemSize;
			}
		}
	}

	totalSize += TABBAR_HMARGIN;
	if ( totalSize < rect.Width() )
		totalSize = rect.Width();

	if ( numLines_ > (int)itemList_.size() )
		numLines_ = (int)itemList_.size();

	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	numLines_ = (int)ceil( (float)itemList_.size() / (float)itemsPerLine );

	dc.SelectObject( oldFont );

	return totalSize;
}


/**
 *	This method adjusts the width of elements in the lines of the control.
 */
void TabCtrl::recalcLineSizes()
{
	BW_GUARD;

	CClientDC dc(this);

	CFont* oldFont = dc.SelectObject( &font_ );

	// Calc the total size to see if it overflows the actual window size
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	int lineSize = TABBAR_HMARGIN;
	// calc line sizes, in order to proportionally calc item sizes in multiline
	if ( multiline_ )
	{
		int count = 0;
		int lineSize = TABBAR_HMARGIN;
		lineSizes_.clear();
		for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		{
			CString caption( (*i).caption.c_str() );
			int itemSize = 0;
			if ( (*i).icon )
				itemSize += ICON_SIZE + TABBAR_HMARGIN;
			itemSize += dc.GetTextExtent( caption ).cx + TABBAR_HMARGIN*2;
			lineSize += itemSize;
			count++;
			if ( count == itemsPerLine )
			{
				lineSizes_.push_back( lineSize + TABBAR_HMARGIN );
				lineSize = TABBAR_HMARGIN;
				count = 0;
			}
		}
		lineSizes_.push_back( lineSize + TABBAR_HMARGIN );
	}

	dc.SelectObject( oldFont );
}


/**
 *	This method paints the control.
 *
 *	@param multilineSwap	True to allow swapping vertical lines so that the
 *							current item's line gets placed near the bottom.
 */
void TabCtrl::paint( bool multilineSwap )
{
	BW_GUARD;

	int totalSize = 0;
	if ( multilineSwap || numLines_ == 1 )
		totalSize = recalcHeight();
	else
		recalcLineSizes();

	// startup
	CClientDC dc(this);

	CRect rect;
	GetClientRect( &rect );

	// background
	COLORREF backColor = GetSysColor(COLOR_BTNFACE);
	int r = GetRValue( backColor ) + BACK_COLOR_ADDR;
	int g = GetGValue( backColor ) + BACK_COLOR_ADDG;
	int b = GetBValue( backColor ) + BACK_COLOR_ADDB;
	int maxComp = max( r, max( g, b ) );
	if ( maxComp > 255 )
	{
		// normalize to 255
		r = r * 255 / maxComp;
		g = g * 255 / maxComp;
		b = b * 255 / maxComp;
	}
	dc.FillSolidRect( rect, RGB( min( 255, r ), min( 255, g ), min( 255, b ) ) );

	if ( itemList_.size() == 0 )
		return;

	// Tab buttons + text
	COLORREF textActive = GetSysColor(COLOR_BTNTEXT);
	COLORREF textInactive = GetSysColor(COLOR_GRAYTEXT);

	int oldBkMode = dc.SetBkMode(TRANSPARENT);
	CFont* oldFont = dc.SelectObject( &font_ );
	COLORREF oldColor = dc.SetTextColor( textActive );
	CPen* oldPen = 0;

	if ( align_ == TOP )
	{
		oldPen = dc.SelectObject( &btnHilight_ );
		dc.MoveTo( 0, rect.bottom - 1 );
		dc.LineTo( rect.right, rect.bottom - 1 );
	}
	else
	{
		oldPen = dc.SelectObject( &btnShadow_ );
		dc.MoveTo( 0, rect.top );
		dc.LineTo( rect.right, rect.top );
	}

	// items per line is used in multiline calculations
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );

	if ( multilineSwap && multiline_ )
	{
		// prepare line ypos array puting the current item's line at the bottom in a multiline tab control
		lineYPos_.clear();
		for( int i = 0; i < numLines_; ++i )
			lineYPos_.push_back( s_tabbarHeight * i );

		int count = 0;
		for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		{
			if ( curItem_ == count )
			{
				int icur = min( count / itemsPerLine, (int)lineYPos_.size() - 1 );
				int isel = (align_ == TOP) ? (int)lineYPos_.size()-1 : 0;
				
				int tmp = lineYPos_.at( icur );
				lineYPos_.at( icur ) = lineYPos_.at( isel );
				lineYPos_.at( isel ) = tmp;
				break;
			}

			count++;
		}
	}

	// do the draw loop
	int xpos = TABBAR_HMARGIN;
	int ypos;
	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		// calc desired item size
		CString caption( (*i).caption.c_str() );
		int actualSize = dc.GetTextExtent( caption ).cx + TABBAR_HMARGIN * 2;
		int iconSize = 0;
		if ( (*i).icon )
		{
			iconSize = ICON_SIZE + TABBAR_HMARGIN;
			actualSize += iconSize;
		}

		// adjust item pos and size depending if it's single or multi line 
		int itemSize;
		if ( numLines_ == 1 )
			itemSize = actualSize * rect.Width() / totalSize;
		else
			itemSize = actualSize * rect.Width() / lineSizes_.at(
				min( count / itemsPerLine, (int)lineSizes_.size() - 1 ) );

		if ( (*i).icon && itemSize < iconSize + TABBAR_HMARGIN )
			iconSize = 0;

		if ( multiline_ && count && (count % itemsPerLine) == 0 )
			xpos = TABBAR_HMARGIN;	// new line, reset pos

		ypos = lineYPos_.at( min( count / itemsPerLine, (int)lineYPos_.size() - 1 ) );

		if ( curItem_ == count )
		{
			// draw tab arround item
			CRect tabRect( xpos, ypos, xpos + itemSize, ypos + s_tabbarHeight - 2 );
			if ( align_ == TOP )
				tabRect.OffsetRect( 0, 2 );

			dc.FillSolidRect( tabRect, GetSysColor(COLOR_BTNFACE) );

			dc.SelectObject( &btnHilight_ );
			if ( align_ == TOP )
			{
				dc.MoveTo( tabRect.left, tabRect.bottom );
				dc.LineTo( tabRect.left, tabRect.top );
				dc.LineTo( tabRect.right, tabRect.top );

				dc.SelectObject( &btnShadow_ );
				dc.LineTo( tabRect.right, tabRect.bottom );
				if ( !multilineSwap && numLines_ > 1 )
					dc.LineTo( tabRect.left, tabRect.bottom );
			}
			else
			{
				dc.MoveTo( tabRect.left, tabRect.top );
				dc.LineTo( tabRect.left, tabRect.bottom );

				dc.SelectObject( &btnShadow_ );
				dc.LineTo( tabRect.right, tabRect.bottom );
				dc.LineTo( tabRect.right, tabRect.top );
				if ( !multilineSwap && numLines_ > 1 )
				{
					dc.SelectObject( &btnHilight_ );
					dc.LineTo( tabRect.left, tabRect.top );
				}
			}

			dc.SetTextColor( textActive );
		}
		else
		{
			// if it's not the last to the right and not the last, draw the separator
			if ( ((count + 1) % itemsPerLine ) != 0 && count < (int)itemList_.size()-1 )
			{
				dc.SelectObject( &btnSeparator_ );
				dc.MoveTo( xpos + itemSize, ypos + 4 );
				dc.LineTo( xpos + itemSize, ypos + s_tabbarHeight - 4 );
			}
			dc.SetTextColor( textInactive );
		}

		// draw icon
		if ( iconSize && itemSize > iconSize )
			DrawIconEx( dc.GetSafeHdc(),
				xpos + TABBAR_HMARGIN, ypos + TABBAR_HMARGIN,
				(*i).icon, ICON_SIZE, ICON_SIZE, 0, NULL, DI_NORMAL );

		// draw text
		int centerOff = 0;
		if ( numLines_ != 1 )
			centerOff = max( 0, (int)( itemSize - dc.GetTextExtent( caption ).cx - iconSize - TABBAR_HMARGIN*2 ) / 2 );

		CRect txtRect(
			xpos + centerOff + TABBAR_HMARGIN + iconSize, ypos + TABBAR_HMARGIN + 1,
			xpos + centerOff + itemSize - TABBAR_HMARGIN + 1, ypos + s_tabbarHeight - TABBAR_HMARGIN );
		dc.DrawText( CString( (*i).caption.c_str() ) , &txtRect, DT_SINGLELINE | DT_LEFT /*| DT_END_ELLIPSIS*/ );

		// move current item position
		(*i).curLeft = xpos;
		xpos += itemSize;
		(*i).curRight = xpos;
		(*i).curTop = ypos;
		++count;
	}

	// Restore old DC objects
	dc.SelectObject( oldPen );
	dc.SelectObject( oldFont );
	dc.SetBkMode( oldBkMode );
	dc.SetTextColor( oldColor );
}


/**
 *	This method returns the tab item at the specified position.
 *
 *	@param idx	Position of the item on the internal list.
 *	@return	The tab item at position "idx".
 */
TabCtrlItem* TabCtrl::getItem( int idx )
{
	BW_GUARD;

	TabCtrlItem* ret = 0;
	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( idx == count++ )
		{
			ret = &(*i);
			break;
		}
	}

	return ret;
}


/**
 *	This method returns the tab item that matches the specified item data.
 *
 *	@param itemData	Custom item data identifying the tab.
 *	@return	The tab item that matches the specified item data.
 */
TabCtrlItem* TabCtrl::getItem( void* itemData )
{
	BW_GUARD;

	TabCtrlItem* ret = 0;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			ret = &(*i);
			break;
		}
	}

	return ret;
}


/**
 *	This method returns the tab item under the specified screen position.
 *
 *	@param x	Screen X position.
 *	@param y	Screen Y position.
 *	@return	The tab item under the specified screen position.
 */
TabCtrlItem* TabCtrl::getItem( int x, int y )
{
	BW_GUARD;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).curLeft <= x && (*i).curRight >= x &&
			(*i).curTop <= y && (*i).curTop + s_tabbarHeight >= y )
			return &(*i);
	}
	return 0;
}


/**
 *	This method sets the current item to the one under the specified screen
 *	position.
 *
 *	@param x	Screen X position.
 *	@param y	Screen Y position.
 */
void TabCtrl::setCurItem( int x, int y )
{
	BW_GUARD;

	void* itemData = 0;
	TabCtrlItem* item = getItem( x, y );
	if ( item )
		itemData = item->itemData;
	setCurItem( itemData );
}


/**
 *	This method sets the current item to the one matching the specified custom
 *	item data.
 *
 *	@param itemData	Custom item data matching the desired item to make current.
 */
void TabCtrl::setCurItem( void* itemData )
{
	BW_GUARD;

	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			curItem_ = count;
			paint();
			break;
		}
		++count;
	}
}


/**
 *	This method returns the index of the tab item matching the specified custom
 *	item data.
 *
 *	@param itemData Custom item data identifying the desired tab item.
 *	@return	Index of the tab item that matches itemData, or -1 if not found.
 */
int TabCtrl::getItemIndex( void* itemData )
{
	BW_GUARD;

	int cnt = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, ++cnt )
		if ( (*i).itemData == itemData )
			return cnt;

	return -1;
}


/**
 *	This method sets the tab item to the one under the specified screen
 *	position.
 *
 *	@param itemData Custom item data identifying the desired tab item.
 *	@param x	Screen X position.
 *	@param y	Screen Y position.
 *	@return	Index of the tab item under the specified screen position.
 */
int TabCtrl::getItemIndex( void* itemData, int x, int y )
{
	BW_GUARD;

	// try to find the item that contains the x.y coords
	int count = 0;
	bool passedCurItem = false;
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, ++count )
	{
		if ( x >= (*i).curLeft && x <= (*i).curRight &&
			y >= (*i).curTop && y <= (*i).curTop + s_tabbarHeight )
		{
			// found
			if ( ( x >= ( (*i).curRight + (*i).curLeft ) /2 &&				// if its at right...
				!( !passedCurItem && ( count + 1 ) % itemsPerLine == 0 ) ) ||		// and NOT (it's before the cur item and it's end of line)...
				( passedCurItem && count % itemsPerLine == 0 ) )			// or it's past the cur item and its at the begining of the line
			{
				// if it's in the right half of the item, it means "insert after" (add 1)
				++count;
			}
			break;
		}
		if ( (*i).itemData == itemData )
			passedCurItem = true;
	}

	return count;
}


/**
 *	This method updates the position of an item so it's under the specified
 *	screen position.
 *
 *	@param itemData Custom item data identifying the desired tab item.
 *	@param x	Mouse X position.
 *	@param y	Mouse Y position.
 */
void TabCtrl::updateItemPosition( void* itemData, int x, int y )
{
	BW_GUARD;

	if ( !itemData || !getItem( itemData ) )
		return;

	// save item's data
	TabCtrlItem item = *getItem( itemData );

	updateItemPosition( itemData, getItemIndex( itemData, x, y ) );
}


/**
 *	This method updates the position of an item so it's moved to the specified
 *	index in the internal list.
 *
 *	@param itemData Custom item data identifying the desired tab item.
 *	@param index	Desired index for the tab item.
 */
void TabCtrl::updateItemPosition( void* itemData, int index )
{
	BW_GUARD;

	if ( !itemData || !getItem( itemData ) )
		return;

	// get an iterator to it in order to erase from the list later
	TabCtrlItemItr j = itemList_.begin();
	for( ; j != itemList_.end(); ++j )
		if ( (*j).itemData == itemData )
			break;

	// get the iterator for the insert index corresponding to the coordinates
	int count = 0;
	TabCtrlItemItr i = itemList_.begin();
	for( ; i != itemList_.end(); ++i, ++count )
		if ( count == index )
			break;

	// if over the same item that's being moved, the leave it there
	if ( i == j )
		return;

	// do the move:
	// save old's item data to later find position in array
	TabCtrlItem item = *getItem( itemData );
	if ( i != itemList_.end() )
	{
		// save old item's itemdata as an id, and erase the item to be moved
		void* indexItemData = (*i).itemData;
		itemList_.erase( j );

		// found the 'index' item, so insert the itemData item before it
		// find the position of the item to be inserted into
		count = 0;
		// iterators might have been invalidated by erase, so iterate again
		for( j = itemList_.begin(); j != itemList_.end(); ++j, ++count )
			if ( (*j).itemData == indexItemData )
				break;
		// insert it into the new position
		curItem_ = count;
		itemList_.insert( j, item );
	}
	else
	{
		// erase item from the old position
		itemList_.erase( j );
		// 'index' out of range, so assume positioning as the last tab
		curItem_ = (int)itemList_.size();
		itemList_.push_back( item );
	}
	// redraw with the paint multilineSwap flag to false, so it doesn't
	// try to put the current tab down while the user is reordering tabs
	paint( false );
}


/**
 *	This method changes the caption and icon of an existing item.
 *
 *	@param itemData Custom item data identifying the desired tab item.
 *	@param caption	New caption text for the tab item.
 *	@param icon		New icon for the tab item, or NULL for no item.
 */
void TabCtrl::updateItemData( void* itemData, const BW::wstring& caption, HICON icon )
{
	BW_GUARD;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		if ( (*i).itemData == itemData )
		{
			(*i).caption = caption;
			(*i).icon = icon;
			break;
		}
}


// MFC message map
BEGIN_MESSAGE_MAP(TabCtrl,CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


/**
 *	This MFC method performs the controls custom painting.
 */
void TabCtrl::OnPaint()
{
	BW_GUARD;

	CWnd::OnPaint();

	paint();
}


/**
 *	This MFC method handles clicking on the control by setting the current item
 *	and letting the event handler know.
 *
 *	@param nFlags	MFC event flags, ignored.
 *	@param point	MFC mouse position.
 */
void TabCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	setCurItem( point.x, point.y );
	CWnd::OnLButtonDown( nFlags, point );
	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( curItem_ );
		if ( item )
			eventHandler_->clickedTab( item->itemData, point.x, point.y );
	}
}


/**
 *	This MFC method simply makes sure the control is repainted when its size
 *	changes.
 *
 *	@param nType	MFC resize type, ignored.
 *	@param cx		MFC new width, ignored.
 *	@param cy		MFC new height, ignored.
 */
void TabCtrl::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	CWnd::OnSize( nType, cx, cy );
	paint();
}


/**
 *	This MFC method simply forwards double-click events on the control to the
 *	event handler.
 *
 *	@param nFlags	MFC event flags, ignored.
 *	@param point	MFC mouse position.
 */
void TabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	BW_GUARD;

	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( curItem_ );
		eventHandler_->doubleClickedTab( item->itemData, point.x, point.y );
	}
}


/**
 *	This MFC method simply forwards right-click events on the control to the
 *	event handler.
 *
 *	@param nFlags	MFC event flags, ignored.
 *	@param point	MFC mouse position.
 */
void TabCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	CWnd::OnRButtonDown( nFlags, point );
	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( point.x, point.y );
		if ( item )
			eventHandler_->rightClickedTab( item->itemData, point.x, point.y );
	}
}



}	// namespace
BW_END_NAMESPACE

