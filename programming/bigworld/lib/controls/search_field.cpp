
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "search_field.hpp"
#include "user_messages.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	// TODO: We should get these sizes from the images at some stage, it's
	// surprisingly non-trivial to get size info from HBITMAPs
	const int FILTERS_WIDTH = 18;
	const int FILTERS_HEIGHT = 16;
	const int CLOSE_WIDTH = 15;
	const int CLOSE_HEIGHT = 15;
	const int X_GAP = 2;
	const int Y_GAP = 1;
} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// Section: IdleTextCEdit
///////////////////////////////////////////////////////////////////////////////

SearchField::IdleTextCEdit::IdleTextCEdit() :
	CEdit()
{
}


void SearchField::IdleTextCEdit::idleText( const BW::wstring& idleText )
{
	BW_GUARD;

	idleText_ = idleText;
	Invalidate();
	UpdateWindow();
}


bool SearchField::IdleTextCEdit::idle() const
{
	BW_GUARD;

	CString str;
	GetWindowText( str );
	return str.IsEmpty() && ::GetFocus() != GetSafeHwnd();
}


BEGIN_MESSAGE_MAP( SearchField::IdleTextCEdit, CEdit )
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_PAINT()
END_MESSAGE_MAP()


void SearchField::IdleTextCEdit::OnSetFocus( CWnd * oldWnd )
{
	BW_GUARD;

	Invalidate();
	UpdateWindow();
    CEdit::OnSetFocus(oldWnd);
}


void SearchField::IdleTextCEdit::OnKillFocus( CWnd * newWnd )
{
	BW_GUARD;

	Invalidate();
	UpdateWindow();
    CEdit::OnKillFocus(newWnd);
}


void SearchField::IdleTextCEdit::OnPaint()
{
	BW_GUARD;

	CEdit::OnPaint();
	if (idle())
	{
		Invalidate();
		CPaintDC dc( this );
		CRect rect;
		GetRect( &rect );
		CFont* oldFont = dc.SelectObject( GetFont() );
		COLORREF oldTextColour = dc.SetTextColor( GetSysColor( COLOR_GRAYTEXT ) );
		COLORREF oldBkColour = dc.SetBkColor( GetSysColor( COLOR_WINDOW ) );
		MF_ASSERT( idleText_.length() <= INT_MAX );
		dc.DrawText( idleText_.c_str(), ( int )idleText_.length(), rect, 0 );
		dc.SelectObject( oldFont );
		dc.SetTextColor( oldTextColour );
		dc.SetBkColor( oldBkColour );
		ValidateRect( NULL );
	}
}


///////////////////////////////////////////////////////////////////////////////
// Section: SearchField
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor
 */
SearchField::SearchField()
{
}


/**
 *	Destructor
 */
SearchField::~SearchField()
{
}


/**
 *	This method must be called to properly initialise the control, usually from
 *	within OnInitDialog or similar.
 *
 *	@param filterResID	Bitmap ID of the image used for filters button
 *	@param closeResID	Bitmap ID of the image used for the clear search button
 *	@param idleText		Text that appears when the search field is in focus
 *	@param filterToolTip Tooltip text for the filters button
 *	@param searchToolTip Tooltip text for the search text edit field
 *	@return		true on success, false otherwise.
 */
bool SearchField::init( LPTSTR filterResID, LPTSTR closeResID,
	const BW::wstring & idleText,
	const BW::wstring & filterToolTip, const BW::wstring & searchToolTip )
{
	BW_GUARD;

	// Create child controls for the SearchField control's components, and
	// setup tooltips, idle text, etc.
	this->SetWindowText( L"" );

	CRect rect( 0, 0, 10, 10 ); // Temp rect used for creation
	search_.Create( ES_AUTOHSCROLL | WS_VISIBLE, rect, this, 1 );
	search_.SetFont( this->GetFont() );
	search_.idleText( idleText );

	if (filterResID)
	{
		filters_.Create( L"", SS_BITMAP | SS_NOTIFY | WS_VISIBLE, rect, this );
		HBITMAP imgFilters = (HBITMAP)LoadImage( AfxGetInstanceHandle(),
			filterResID, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_SHARED );

		filters_.SetBitmap( imgFilters );
	}
	

	if (closeResID)
	{
		close_.Create( L"", SS_BITMAP | SS_NOTIFY, rect, this );
		HBITMAP imgClose = (HBITMAP)LoadImage( AfxGetInstanceHandle(),
			closeResID, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_SHARED );
		close_.SetBitmap( imgClose );
	}
	

	this->GetClientRect( rect );
	resizeInternal( rect.right, rect.bottom );

	if (toolTip_.CreateEx( this, 0, WS_EX_TOPMOST ))
	{
		toolTip_.SetMaxTipWidth( SHRT_MAX );
		if (!filterToolTip.empty())
		{
			toolTip_.AddTool( &filters_, filterToolTip.c_str() );
		}
		if (!searchToolTip.empty())
		{
			toolTip_.AddTool( &search_, searchToolTip.c_str() );
		}
		const BW::wstring closeToolTip = Localise( L"SEARCH_FIELD/TOOL_TIP_SEARCH_CANCEL" );
		if (!closeToolTip.empty())
		{
			toolTip_.AddTool( &close_, closeToolTip.c_str() );
		}
		toolTip_.SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0,
									SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
		toolTip_.Activate( TRUE );
	}

	return true;
}


/**
 *	This method returns the current idle text string.
 *
 *	@return		Current idle text.
 */
const BW::wstring & SearchField::idleText() const
{
	BW_GUARD;

	return search_.idleText();
}


/**
 *	This method allows changing the idle text string.
 *
 *	@param idleText	 New string to use as idle text.
 */
void SearchField::idleText( const BW::wstring & idleText )
{
	BW_GUARD;

	search_.idleText( idleText );
}


/**
 *	This method allows changing the filters button image.
 *
 *	@param filterResID	New Bitmap ID of the image used for filters button.
 */
void SearchField::filtersImg( LPTSTR filterResID )
{
	BW_GUARD;

	HBITMAP img = (HBITMAP)LoadImage( AfxGetInstanceHandle(),
		filterResID, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT | LR_SHARED );
	filters_.SetBitmap( img );
}


/**
 *	This method allows manually changing the search text edit field.
 *
 *	@param text		New string to put in the search text edit field.
 */
void SearchField::searchText( const BW::wstring & text )
{
	BW_GUARD;

	search_.SetWindowText( text.substr( 0, MAX_SEARCH_TEXT ).c_str() );
}


/**
 *	This method returns the search text currently in the edit field.
 *
 *	@return		Search text.
 */
const BW::wstring SearchField::searchText() const
{
	BW_GUARD;

	wchar_t searchTxt[ MAX_SEARCH_TEXT + 1 ];
	ZeroMemory( searchTxt, MAX_SEARCH_TEXT + 1 );
	search_.GetWindowText( searchTxt, MAX_SEARCH_TEXT );
	return searchTxt;
}


/**
 *	This method is required for the correct operation of MFC's tooltip class.
 *
 *	@param msg	Windows' message about to be processed by Windows.
 *	@return		0 to tell Windows that it should keep processing the message.
 */
BOOL SearchField::PreTranslateMessage( MSG* msg )
{
	BW_GUARD;

    if (toolTip_.GetSafeHwnd())
	{
        toolTip_.RelayEvent( msg );
	}

	return 0;
}


/**
 *	This method intercepts windows messages generated by the search related
 *	controls (search text editing, filters click, etc) and process them
 *	accordingly.
 *
 *	@param message		Windows message being processed by Windows
 *	@param wParam		The message's wParam.
 *	@param lParam		The message's lParam.
 *	@return		We should return the result of the base class' WindowProc.
 */
LRESULT SearchField::WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	if (message == WM_COMMAND)
	{
		// Handle component events.
		WORD notif = HIWORD( wParam );
		HWND hwnd = HWND( lParam );
		if (hwnd == search_.GetSafeHwnd() && notif == EN_CHANGE)
		{
			// Search text has changed, notify the parent.
			BW::wstring searchTxt = this->searchText();
			bool show = !searchTxt.empty();
			close_.ShowWindow( show ? SW_SHOW : SW_HIDE );
			close_.RedrawWindow();
			this->GetParent()->SendMessage( WM_SEARCHFIELD_CHANGE, (WPARAM)this, 0 );
		}
		else if (hwnd == close_.GetSafeHwnd() && notif == STN_CLICKED)
		{
			search_.SetWindowText( L"" );
			this->GetParent()->SendMessage( WM_SEARCHFIELD_CLEAR, (WPARAM)this, 0 );
		}
		else if (hwnd == filters_.GetSafeHwnd() && notif == STN_CLICKED)
		{
			// Tell the parent that the magnifying glass has been clicked.
			this->GetParent()->SendMessage( WM_SEARCHFIELD_FILTERS, (WPARAM)this, 0 );
		}
	}

	return CStatic::WindowProc( message, wParam, lParam );
}


// MFC message map
BEGIN_MESSAGE_MAP( SearchField, CStatic )
    ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP() 


/**
 *	This method overrides the base class' paint method to ensure this control
 *	uses the same colour as the search text edit control.
 */
void SearchField::OnPaint()
{
	BW_GUARD;

	CPaintDC dc( this );
	CRect rect;
	this->GetClientRect( rect );

	// Use a background colour that blends with the search text CEdit
	CBrush back( GetSysColor( COLOR_WINDOW ) );
	dc.FillRect( rect, &back );

	if (filters_.GetSafeHwnd())
	{
		filters_.RedrawWindow();
	}

	if (close_.GetSafeHwnd())
	{
		close_.RedrawWindow();
	}
}


/**
 *	This method resizes and repositions the internal controls when this class
 *	is resized.
 *
 *	@param nType	Type of resize.
 *	@param cx		Width of the window rect.
 *	@param cy		Height of the window rect.
 */
void SearchField::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CStatic::OnSize( nType, cx, cy );

	// We need the client rect, so get it and ignore cx and cy.
	CRect clientRect;
	this->GetClientRect( clientRect );
	resizeInternal( clientRect.right, clientRect.bottom );
}


/**
 *	This method is used internally to reposition and resize all the internal
 *	controls to the desired size (usually the size of the client rect of this
 *	class)
 *
 *	@param cx		Desired width.
 *	@param cy		Desired height.
 */
void SearchField::resizeInternal( int cx, int cy )
{
	BW_GUARD;

	if (search_.GetSafeHwnd())
	{
		int filterWidth = 0;
		int closeWidth = 0;
		int filterHeight = 0;
		int closeHeight = 0;

		
		if (filters_.GetSafeHwnd())
		{
			filterWidth = FILTERS_WIDTH;
			filterHeight = FILTERS_HEIGHT;
			filters_.SetWindowPos( 0, 0, Y_GAP,
				filterWidth, filterHeight,
				SWP_NOZORDER );

			filters_.RedrawWindow();
		}
		
		
		if (close_.GetSafeHwnd())
		{
			closeWidth = CLOSE_WIDTH;
			closeHeight = CLOSE_HEIGHT;

			close_.SetWindowPos( 0, cx - closeWidth - X_GAP, Y_GAP,
				closeWidth, closeHeight,
				SWP_NOZORDER );

			close_.RedrawWindow();
		}
		

		search_.SetWindowPos( 0,
				filterWidth + X_GAP, Y_GAP,
				cx - filterWidth - closeWidth - X_GAP * 2, cy - Y_GAP * 2,
				SWP_NOZORDER );
		
	}
}

BW_END_NAMESPACE

