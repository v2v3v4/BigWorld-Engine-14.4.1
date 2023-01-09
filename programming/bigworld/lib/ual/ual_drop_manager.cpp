
#include "pch.hpp"

#include "ual_drop_manager.hpp"

#include <afxwin.h>
#include "memhook/memhook.hpp"

namespace
{
	CRect toCRect( const BW::RectInt & rect )
	{
		return CRect( rect.xMin(), rect.yMax(), rect.xMax(), rect.yMin() );
	}

	BW::RectInt toRect( const CRect & rect )
	{
		return BW::RectInt( rect.left, rect.bottom, rect.right, rect.top );
	}
}

bool operator==( const CRect & left, const BW::RectInt & right )
{
	return ( left == toCRect( right ) ) == TRUE;
}

BW_BEGIN_NAMESPACE

bool operator==( const RectInt & left, const RectInt & right )
{
	return	left.xMin() == right.xMin() &&
		left.yMin() == right.yMin() &&
		left.xMax() == right.xMax() &&
		left.yMax() == right.yMax();
}


// Constants implementation.
/*static*/ RectInt UalDropManager::HIT_TEST_NONE( -1, -1, -1, -1 );
/*static*/ RectInt UalDropManager::HIT_TEST_MISS(  0,  0,  0,  0 );
/*static*/ RectInt UalDropManager::HIT_TEST_OK_NO_RECT( -5,  -5,  -5,  -5 );

/**
 *	Constructor.
 */
UalDropManager::UalDropManager():
	ext_(""),
	pen_( NULL ),
	lastHighlightRect_( 0, 0, 0, 0 ),
	lastHighlightWnd_( NULL )
{
	BW_GUARD;
#ifdef USE_MEMHOOK
	// Need to disable custom allocation so MFC doesn't get confused on exit
	// trying to delete the doc template created classes below.
	BW::Memhook::AllocFuncs sysAllocFuncs = { ::malloc, ::free, ::_aligned_malloc, ::_aligned_free, ::realloc, ::_msize };
	BW::Memhook::AllocFuncs bwAllocFuncs= BW::Memhook::allocFuncs();
	BW::Memhook::allocFuncs( sysAllocFuncs );
#endif
	pen_ = new CPen( PS_SOLID, DRAG_BORDER, DRAG_COLOUR );
#ifdef USE_MEMHOOK
	// Re-enabling custom allocation
	BW::Memhook::allocFuncs( bwAllocFuncs );
#endif
}

UalDropManager::~UalDropManager()
{
	delete pen_;
}

void UalDropManager::clear()
{
	BW_GUARD;

	droppings_.clear();
	dontHighlightHwnds_.clear();
}

/**
 *	This method adds a drop target callback to the manager.
 *
 *	@param dropping		Drop traget callback object.
 *	@param useHighlighting	True to automatically draw a halftoned rectangle
 *							arround the drop target when the mouse is over it,
 *							false to not have anything drawn by the manager.
 */
void UalDropManager::add( SmartPointer< UalDropCallback > dropping, bool useHighlighting /*= true*/ )
{
	BW_GUARD;

	if (dropping == NULL || dropping->cWnd() == NULL)
	{
		WARNING_MSG( "UalDropManager::add: Tried to add a NULL drop target.\n" );
		return;
	}
	droppings_.insert( DropType( dropping->hWnd(), dropping ) );

	if (useHighlighting)
	{
		dontHighlightHwnds_.erase( dropping->hWnd() );
	}
	else
	{
		dontHighlightHwnds_.insert( dropping->hWnd() );
	}
}


/**
 *	This method must be called when drag and drop operation starts.
 *
 *	@param ext	Filename extension of the dragged asset.
 */
void UalDropManager::start( const BW::string& ext )
{
	BW_GUARD;

	ext_ = ext;
	std::transform( ext_.begin(), ext_.end(), ext_.begin(), tolower );
	lastHighlightWnd_ = NULL;
}


/**
 *	This method draws a halftoned highlight rectangle.
 *
 *	@param hwnd		Window handle of the window on which the highlight
 *					rectangle will be drawn.
 *	@param rect		Rectangle coordinates and size.
 */
void UalDropManager::drawHighlightRect( HWND hwnd, const CRect & rect )
{
	BW_GUARD;

	CWnd * pWnd = CWnd::FromHandle( hwnd );
	CRect wndRect;
	pWnd->GetWindowRect( wndRect );
	CRect xformedRect( rect );
	xformedRect.OffsetRect( -wndRect.left, -wndRect.top );
	CDC * dc = pWnd->GetDCEx( NULL, DCX_WINDOW | DCX_CACHE );
	CPen * oldPen = dc->SelectObject( pen_ );
	int oldROP = dc->SetROP2( R2_NOTXORPEN );
	dc->Rectangle( xformedRect );
	dc->SetROP2( oldROP );
	dc->SelectObject( oldPen );
	pWnd->ReleaseDC( dc );
}


/**
 *	This method highlights a drop target window.
 *
 *	@param hwnd		Window handle of the window on which the highlight
 *					rectangle will be drawn.
 *	@param rect		Rectangle coordinates and size.
 */
void UalDropManager::highlight( HWND hwnd, const CRect & rect )
{
	BW_GUARD;

	if (rect == HIT_TEST_OK_NO_RECT)
	{
		// nothing to do, the user doesn't want our highlighting.
		lastHighlightWnd_ = NULL;
		return;
	}

	if (hwnd == lastHighlightWnd_ && rect == lastHighlightRect_)
	{
		// nothing to do, return to avoid flickering.
		return;
	}

	if (lastHighlightWnd_ && dontHighlightHwnds_.find( lastHighlightWnd_ ) == dontHighlightHwnds_.end())
	{
		drawHighlightRect( lastHighlightWnd_, toCRect( lastHighlightRect_ ) );
	}

	if (hwnd && dontHighlightHwnds_.find( hwnd ) == dontHighlightHwnds_.end())
	{
		drawHighlightRect( hwnd, rect );
	}

	lastHighlightRect_ = toRect( rect );
	lastHighlightWnd_ = hwnd;
}


/**
 *	This method tests an item being dragged agains the drop targets associated
 *	to a window, if any.
 *
 *	@param hwnd		Window handle of the window under the mouse.
 *	@param ii		Information for the item being dragged.
 *	@return		The drop target callback object under the mouse, or NULL.
 */
SmartPointer< UalDropCallback > UalDropManager::test( HWND hwnd, UalItemInfo* ii )
{
	BW_GUARD;

	std::pair<DropMapIter,DropMapIter> drops = droppings_.equal_range( hwnd );

	CRect tempCRect;

	for (DropMapIter i=drops.first; i!=drops.second; ++i)
	{
		if (i->second->ext().empty() || i->second->ext() == ext_)
		{
			RectInt & temp =i->second->test( ii );
			if (temp == HIT_TEST_NONE) // No test
			{
				i->second->cWnd()->GetClientRect( &tempCRect );
				highlightRect_  = toRect( tempCRect );
			}
			else if (temp == HIT_TEST_MISS) // Test failed
			{
				return NULL;
			}
			else // Success
			{
				highlightRect_ = temp;
			}
			
			i->second->cWnd()->ClientToScreen( &tempCRect );
			highlightRect_ = toRect( tempCRect );
			return i->second;
		}
	}
	return NULL;
}


/**
 *	This method tests an item being dragged agains the drop targets.
 *
 *	@param ii		Information for the item being dragged.
 *	@return		The drop target callback object under the mouse, or NULL.
 */
SmartPointer< UalDropCallback > UalDropManager::test( UalItemInfo* ii )
{
	BW_GUARD;

	HWND hwnd = ::WindowFromPoint( CPoint( ii->x(), ii->y() ) );
	
	SmartPointer< UalDropCallback > drop = test( hwnd, ii );
	if (drop == NULL)
	{
		drop = test( ::GetParent( hwnd ), ii );
	}

	highlight( drop ? hwnd : NULL, toCRect( highlightRect_ ) );

	return drop;
}


/**
 *	This method is called when the drag & drop operation has ended.  If
 *	successful (the mouse button was release over a drop target), the drop
 *	target's "execute" method is called.
 *
 *	@param ii	Information for the item being dragged.
 *	@return		True if the drag and drop operation was successful.
 */
bool UalDropManager::end( UalItemInfo* ii )
{
	BW_GUARD;

	SmartPointer< UalDropCallback > res = test( ii );

	highlight( NULL, toCRect( highlightRect_ ) );
	
	ext_ = "";
	
	if (res)
		return res->execute( ii );

	return false;
}


/*virtual */RectInt UalDropCallback::test( UalItemInfo* ii )
{
	return UalDropManager::HIT_TEST_NONE;
}

CWnd* UalDropCallback::cWnd()
{
	BW_GUARD;

	if (id_ == 0)
		return wnd_;
	else
		return wnd_->GetDlgItem(id_);
}

HWND UalDropCallback::hWnd()
{
	BW_GUARD;

	if (cWnd() != NULL)
		return cWnd()->GetSafeHwnd();
	else
		return NULL;
}

BW_END_NAMESPACE