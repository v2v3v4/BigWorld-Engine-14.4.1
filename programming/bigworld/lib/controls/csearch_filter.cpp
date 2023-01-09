#include "pch.hpp"
#include "controls/csearch_filter.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/string_provider.hpp"


BW_BEGIN_NAMESPACE
namespace controls
{

class controls::CSearchFilterEdit : public CEdit
{
public:
    CSearchFilterEdit();
    ~CSearchFilterEdit();

    void setSearchFilter(CSearchFilter *filter);

    void setTextColour(COLORREF colour);

    afx_msg void OnSetFocus(CWnd *oldWnd);
    afx_msg void OnKillFocus(CWnd *newWnd);

    afx_msg HBRUSH CtlColor(CDC *dc, UINT ctlColour);

    DECLARE_MESSAGE_MAP()

private:
    CSearchFilter       *searchFilter_;
    COLORREF            textColour_;
    CBrush              backBrush_;
};


BEGIN_MESSAGE_MAP(CSearchFilterEdit, CEdit)
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


CSearchFilterEdit::CSearchFilterEdit():
	CEdit(),
	searchFilter_(NULL),
	textColour_(::GetSysColor(COLOR_WINDOWTEXT))
{
	BW_GUARD;

    backBrush_.CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
}


CSearchFilterEdit::~CSearchFilterEdit()
{
    searchFilter_ = NULL;
}


void CSearchFilterEdit::setSearchFilter(CSearchFilter *filter)
{
    searchFilter_ = filter;
}


void CSearchFilterEdit::setTextColour(COLORREF colour)
{
	BW_GUARD;

    textColour_ = colour;
    Invalidate();
}


void CSearchFilterEdit::OnSetFocus(CWnd *oldWnd)
{
	BW_GUARD;

    if (searchFilter_ != NULL)
        searchFilter_->OnSetFocus(oldWnd);
    CEdit::OnSetFocus(oldWnd);
}


void CSearchFilterEdit::OnKillFocus(CWnd *newWnd)
{
	BW_GUARD;

    if (searchFilter_ != NULL)
        searchFilter_->OnKillFocus(newWnd);
    CEdit::OnKillFocus(newWnd);
}


HBRUSH CSearchFilterEdit::CtlColor(CDC *dc, UINT ctlColour)
{
	BW_GUARD;

    dc->SetTextColor(textColour_);
    return backBrush_;
}


namespace
{
    UINT    EDIT_CONTROL_ID     = 0x2000;
}


BEGIN_MESSAGE_MAP(CSearchFilter, CWnd)
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
    ON_EN_CHANGE(EDIT_CONTROL_ID, OnEditText)
    ON_WM_LBUTTONDOWN()
    ON_MESSAGE(WM_SETTEXT, OnSetWindowText)
    ON_MESSAGE(WM_GETTEXT, OnGetWindowText)
    ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


/**
 *  Constructor.
 */
CSearchFilter::CSearchFilter():
	CWnd(),
	edit_(NULL),
	searchBmp_(NULL),
	clearBmp_(NULL),
	filterChanges_(0),
	showEmptyText_(true)
{
	BW_GUARD;

    edit_ = new CSearchFilterEdit();
    edit_->setSearchFilter(this);
}


/**
 *  Destructor.
 */
CSearchFilter::~CSearchFilter()
{
	BW_GUARD;

    bw_safe_delete(edit_);
}


/**
 *  Create the window.
 * 
 *  @param style        The window's style.
 *  @param extents      The window's extents.
 *  @param parent       The parent window.
 *  @param id           The control's id.
 *  @param sbmpID       The id of the search bitmap.
 *  @param cbmpID       The id of the clear bitmap.
 *  @returns            True if the control was successfully created.
 */
BOOL
CSearchFilter::Create
(
    DWORD           style,
    RECT            const &extents, 
    CWnd            *parent, 
    UINT            id,
    UINT            sbmpID,
    UINT            cbmpID
)
{
	BW_GUARD;

    //// Create a class for this CWnd type:
    static bool   registered = false;
    static wstring className;
    if (!registered)
    {
        className = AfxRegisterWndClass(0);
        registered = true;
    }

    // Create this control:
    BOOL result = 
        CWnd::Create
        (
            className.c_str(),
            NULL,
            style, 
            extents, 
            parent, 
            id
        );

    // Load the bitmaps:
    searchBmp_ = 
        (HBITMAP)::LoadImage
        (
            AfxGetInstanceHandle(),
            MAKEINTRESOURCE(sbmpID),
            IMAGE_BITMAP,
            0,
            0,
            LR_DEFAULTCOLOR | LR_LOADTRANSPARENT
        );
    clearBmp_ = 
        (HBITMAP)::LoadImage
        (
            AfxGetInstanceHandle(),
            MAKEINTRESOURCE(cbmpID),
            IMAGE_BITMAP,
            0,
            0,
            LR_DEFAULTCOLOR | LR_LOADTRANSPARENT
        );

    // Get the size of the sub-controls:
    CRect searchRect, editRect, clearRect;
    getRects(extents, searchRect, editRect, clearRect, true);

    // Create the search image control:
    searchImg_.Create
    (
        NULL, 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_BITMAP, 
        searchRect, 
        this
    );
    searchImg_.SetBitmap(searchBmp_);

    // Create the edit control:
    edit_->Create(WS_CHILD | WS_VISIBLE, editRect, this, EDIT_CONTROL_ID);
    edit_->SetFont(parent->GetFont());

    // Create the clear control:
    clearImg_.Create
    (
        NULL, 
        WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_BITMAP, 
        clearRect, 
        this
    );
    clearImg_.SetBitmap(clearBmp_);

    // Update the positions and redraw (the clear bitmap should be hidden).
    clearImg_.ShowWindow(SW_HIDE);
    updatePositions();

    // Add tooltips:
	if (toolTip_.CreateEx(this, 0, WS_EX_TOPMOST))
	{
        int id = GetDlgCtrlID();
        CString text;
        if (text.LoadString(id) != FALSE)
        {
			text = Localise( text );
		    toolTip_.SetMaxTipWidth(SHRT_MAX);
		    toolTip_.AddTool(edit_      , text);
		    toolTip_.AddTool(&searchImg_, text);
            toolTip_.AddTool(&clearImg_ , text);
		    toolTip_.SetWindowPos
            (
                &CWnd::wndTopMost, 
                0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
            );
		    toolTip_.Activate(TRUE);
        }
	}

    return result;
}


/**
 *  Is the clear filter bitmap visible?
 * 
 *  @returns            TRUE if the clear bitmap is visible, FALSE 
 *                      otherwise.
 */
BOOL CSearchFilter::isClearVisible() const
{
	BW_GUARD;

    return clearImg_.IsWindowVisible();
}


/**
 *  This functions returns whether the text displayed when the edit control is 
 *  empty is currently displayed.
 *
 *  @returns            TRUE if the text is displayed, FALSE otherwise.
 */
BOOL CSearchFilter::isEmptyTextVisible() const
{
	BW_GUARD;

    return showEmptyText_ ? TRUE : FALSE;
}


/**
 *  This function sets the text to display when nothing has been entered and
 *  the control has no focus.
 *
 *  @param text         The text to display.
 */
void CSearchFilter::setEmptyText(wchar_t const *text)
{
	BW_GUARD;

    emptyText_ = text;
    if (showEmptyText_)
    {
        edit_->SetWindowText(text);
        edit_->setTextColour(::GetSysColor(COLOR_GRAYTEXT));
    }
}


/**
 *  This function gets the text to display when nothing has been entered and
 *  the control has no focus.
 *
 *  @returns            The text to displayed.
 */
wchar_t const *CSearchFilter::getEmptyText() const
{
	BW_GUARD;

    return emptyText_.c_str();
}


/**
 *  This function returns the id of the edit text.
 *
 *  @returns            The id of the edit text.
 */ 
unsigned int CSearchFilter::editID() const
{
    return EDIT_CONTROL_ID;
}

/**
 *	This function clears the search filter and displays the default empty
 *	text.
 */
void CSearchFilter::clearFilter() 
{
	BW_GUARD;

    showEmptyText_ = true;
    edit_->SetWindowText(emptyText_.c_str());
    edit_->setTextColour(::GetSysColor(COLOR_GRAYTEXT));
}

/**
 *  Called to pass tooltip messages to the tooltip control.
 *
 *  @param msg          The message.
 *  @returns            CWnd::OnPretranslateMsg(msg).
 */
/*virtual*/ BOOL CSearchFilter::PreTranslateMessage(MSG *msg)
{
	BW_GUARD;

    if (toolTip_.GetSafeHwnd())
        toolTip_.RelayEvent( msg );
    return CWnd::PreTranslateMessage(msg);
}


/**
 *  Called when the window is resized.
 * 
 *  @param type         The type of resize.
 *  @param cx           The new width.
 *  @param cy           The new height.
 */
void CSearchFilter::OnSize(UINT type, int cx, int cy)
{
	BW_GUARD;

    CWnd::OnSize(type, cx, cy);
    updatePositions();
}


/**
 *  Called to paint the control.
 */
void CSearchFilter::OnPaint()
{
	BW_GUARD;

    CPaintDC dc(this);
    CRect client;
    GetClientRect(client);
    dc.Draw3dRect
    (
        &client, 
        ::GetSysColor(COLOR_3DSHADOW),
        ::GetSysColor(COLOR_3DHIGHLIGHT)
    );
}


/**
 *  Called to paint the background of the control.
 * 
 *  @param dc           The drawing context.
 *  @returns            TRUE (the background was drawn).
 */
BOOL CSearchFilter::OnEraseBkgnd(CDC *dc)
{
	BW_GUARD;

    CRect client;
    GetClientRect(client);
    dc->FillSolidRect(&client, ::GetSysColor(COLOR_WINDOW));
    return TRUE;
}


/**
 *  Called to set the cursor.
 * 
 *  @param wnd          The window that contains the cursor.
 *  @param hitTest      The hit-test code.
 *  @param message      The mouse message number.
 *  @returns            TRUE (we processed the mouse cursor message).
 */
BOOL 
CSearchFilter::OnSetCursor
(
    CWnd        * /*wnd*/, 
    UINT        /*hitTest*/, 
    UINT        /*message*/
)
{
	BW_GUARD;

    // Get the cursor point:
    CPoint cursorPt;
    GetCursorPos(&cursorPt);
    ScreenToClient(&cursorPt);
    // Get the window rectantles:
    bool isClearVis = (isClearVisible() != FALSE);
    CRect client, searchRect, editRect, clearRect;
    GetClientRect(&client);
    getRects(client, searchRect, editRect, clearRect, isClearVis);
    if (editRect.PtInRect(cursorPt))
    {
        SetCursor(::LoadCursor(NULL, IDC_IBEAM));
    }
    else
    {
        SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    return TRUE;
}


/**
 *  Called when the text of the edit control is changed.
 */
void CSearchFilter::OnEditText()
{
	BW_GUARD;

    if (!showEmptyText_)
    {
        // Update the visibility of the clear bar:
        bool wasVisible = (isClearVisible() != FALSE);
        CString text;
        edit_->GetWindowText(text);
        bool isVisible = !text.IsEmpty();
        if (wasVisible != isVisible)
        {
            clearImg_.ShowWindow(isVisible ? SW_SHOW : SW_HIDE);
            updatePositions();
        }
        // Send a message to the parent about the updated text:
        CWnd *parent = GetParent();
        if (parent != NULL)
        {
            UINT id = GetDlgCtrlID();
            parent->SendMessage
            (
                WM_COMMAND, 
                MAKEWPARAM(id, EN_CHANGE), 
                (LPARAM)(GetSafeHwnd())
            );
        }
    }
}


/**
 *  Called when the left mouse button is pressed down.  We test to see if the
 *  user has clicked within the clear area, and if so clear the text.
 * 
 *  @param flags        The button flags.
 *  @param point        The cursor position.
 */
void CSearchFilter::OnLButtonDown(UINT flags, CPoint point)
{
	BW_GUARD;

    CWnd::OnLButtonDown(flags, point);
    bool isClearVis = (isClearVisible() != FALSE);
    // If the clear button is visible and the user clicks within it then
    // clear the text.
    if (isClearVis)
    {
        CRect client, searchRect, editRect, clearRect;
        GetClientRect(&client);
        getRects(client, searchRect, editRect, clearRect, isClearVis);
        if (clearRect.PtInRect(point))
        {
            edit_->SetWindowText(CString()); // hiding etc is done in OnEditText
        }
    }
    bool hasFocus = GetFocus() == this || GetFocus() == edit_;
    if (!hasFocus)
    {
        showEmptyText_ = true;
        edit_->SetWindowText(emptyText_.c_str());
        edit_->setTextColour(::GetSysColor(COLOR_GRAYTEXT));
    }
}


/**
 *  Synchronize our text with the edit control's text.
 *
 *  @param wparam       Not used, should be 0.
 *  @param lparam       A pointer to the new text.
 *  @param returns      TRUE (the message was processed).
 */
LRESULT CSearchFilter::OnSetWindowText(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    char const *newText = reinterpret_cast<char const *>(lparam);
    return edit_->SendMessage(WM_SETTEXT, wparam, lparam);
}


/**
 *  Get the text.  We return the text of the edit control instead.
 * 
 *  @param wparam       The number of characters.
 *  @param lparam       Text buffer.
 *  @param returns      TRUE (the message was processed).
 */
LRESULT CSearchFilter::OnGetWindowText(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    return edit_->SendMessage(WM_GETTEXT, wparam, lparam); 
}


/**
 *  Return the length of the text in the edit control.
 * 
 *  @param wparam       Not used, should be 0.
 *  @param lparam       Not used, should be 0.
 *  @returns            The length of the text (not including the null 
 *                      terminator).
 */
LRESULT CSearchFilter::OnGetTextLength(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    return edit_->SendMessage(WM_GETTEXTLENGTH, wparam, lparam);
}


/** 
 *  This is called by the framework when the window or it's edit child gets 
 *  input focus.
 *
 *  @param oldWnd       The window with the old focus.
 */
void CSearchFilter::OnSetFocus(CWnd *oldWnd)
{
	BW_GUARD;

    if (showEmptyText_)
    {
        showEmptyText_ = false;
        edit_->SetWindowText(L"");
        edit_->setTextColour(::GetSysColor(COLOR_WINDOWTEXT));
    }
}


/** 
 *  This is called by the framework when the window or it's edit child loses 
 *  input focus.
 *
 *  @param oldWnd       The window with the old focus.
 */
void CSearchFilter::OnKillFocus(CWnd *newWnd)
{
	BW_GUARD;

    CString text;
    edit_->GetWindowText(text);
    showEmptyText_ = text.IsEmpty();
    if (showEmptyText_)
    {
        edit_->SetWindowText(emptyText_.c_str());
        edit_->setTextColour(::GetSysColor(COLOR_GRAYTEXT));
    }
}


/**
 *  What area should the subcontrols be?
 * 
 *  @param client        The client area.
 *  @param searchRect    The area of the search rectangle.
 *  @param editRect      The area of the edit rectangle.
 *  @param clearRect     The are of the clear rectangle.
 *  @param clearVis      Is the clear area visible?
 */
void 
CSearchFilter::getRects
(
    CRect           const &client,
    CRect           &searchRect,
    CRect           &editRect,
    CRect           &clearRect,
    bool            clearVis
) const
{
	BW_GUARD;

    int width  = client.Width();
    int height = client.Height();

    // Get the size of the bitmaps:
    BITMAP searchBitmap;
    ::GetObject(searchBmp_, sizeof(BITMAP), &searchBitmap);
    BITMAP clearBitmap;
    ::GetObject(clearBmp_, sizeof(BITMAP), &clearBitmap);

    searchRect.left   = 2;
    searchRect.top    = 3;
    searchRect.right  = searchRect.left + searchBitmap.bmWidth;
    searchRect.bottom = searchRect.top + searchBitmap.bmHeight;

    if (clearVis)
    {
        clearRect.left   = width - clearBitmap.bmWidth - 2;
        clearRect.top    = 3;
        clearRect.right  = clearRect.left + clearBitmap.bmWidth;
        clearRect.bottom = clearRect.top + clearBitmap.bmHeight;
    }

    editRect.left     = searchBitmap.bmWidth + 2;
    editRect.top      = 5; // move down a little because of CEdit
    editRect.right    = clearVis ? clearRect.left - 1 : width - 3;
    editRect.bottom   = height - 2;
}


/**
 *  Update the positions of the child controls.
 */
void CSearchFilter::updatePositions()
{
	BW_GUARD;

    if 
    (
        IsWindow(searchImg_.GetSafeHwnd())
        &&
        IsWindow(edit_->GetSafeHwnd())
        &&
        IsWindow(clearImg_.GetSafeHwnd())        
    )
    {
        bool isClearVis = (isClearVisible() != FALSE);
        CRect client;
        GetWindowRect(client);
        ScreenToClient(&client);
        CRect searchRect, editRect, clearRect;
        getRects
        (
            client,
            searchRect, 
            editRect, 
            clearRect, 
            isClearVis
        );
        searchImg_.SetWindowPos
        (
            NULL, 
            searchRect.left, 
            searchRect.top, 
            searchRect.Width(), 
            searchRect.Height(), 
            SWP_NOZORDER
        );
        edit_->SetWindowPos
        (
            NULL,
            editRect.left, 
            editRect.top, 
            editRect.Width(), 
            editRect.Height(), 
            SWP_NOZORDER
        );
        if (isClearVis)
        {
            clearImg_.SetWindowPos
            (
                NULL,
                clearRect.left, 
                clearRect.top, 
                clearRect.Width(), 
                clearRect.Height(), 
                SWP_NOZORDER
            );
        }
    }
}

} // namespace controls
BW_END_NAMESPACE

