#include "pch.hpp"
#include "controls/utils.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

/**
 *  This function replaces srcColour in bitmap with dstColour.  It only works
 *  with 8bpp, 24bpp and 32bpp images.  It has only been tested with 8bpp
 *  images.
 *
 *  @param hbitmap      The bitmap to change.
 *  @param srcColour    The colour to replace.
 *  @param dstColour    The colour to replace with.
 */
void
replaceColour
(
    HBITMAP             hbitmap,
    COLORREF            srcColour,
    COLORREF            dstColour
)
{
	BW_GUARD;

    if (hbitmap == NULL)
        return;

    // This can almost be certainly sped up, but at the moment I only use it
    // on small images.
    CBitmap bitmap;
    bitmap.Attach(hbitmap);
    CDC dc;
    dc.CreateCompatibleDC(NULL);
    CBitmap *oldBMP = dc.SelectObject(&bitmap);
    BITMAP bitmapInfo;
    bitmap.GetBitmap(&bitmapInfo);
    for (int y = 0; y < bitmapInfo.bmHeight; ++y)
    {
        for (int x = 0; x < bitmapInfo.bmWidth; ++x)
        {
            COLORREF thisPixel = dc.GetPixel(x, y);
            if (thisPixel == srcColour)
                dc.SetPixel(x, y, dstColour);
        }
    }
    bitmap.Detach();
    dc.SelectObject(oldBMP);
}


/**
 *  This function replaces the colour in the top-left pixel of the bitmap with 
 *  dstColour.  It only works with 8bpp, 24bpp and 32bpp images.  It has only 
 *  been tested with 8bpp images.
 *
 *  @param hbitmap      The bitmap to change.
 *  @param dstColour    The colour to replace with.
 */
void 
replaceColour
(
    HBITMAP             hbitmap,
    COLORREF            dstColour        
)
{
	BW_GUARD;

    if (hbitmap == NULL)
        return;

    CBitmap bitmap;
    bitmap.Attach(hbitmap);
    CDC dc;
    dc.CreateCompatibleDC(NULL);
    CBitmap *oldBMP = dc.SelectObject(&bitmap);
    COLORREF srcColour = dc.GetPixel(0, 0);
    BITMAP bitmapInfo;
    bitmap.GetBitmap(&bitmapInfo);
    for (int y = 0; y < bitmapInfo.bmHeight; ++y)
    {
        for (int x = 0; x < bitmapInfo.bmWidth; ++x)
        {
            COLORREF thisPixel = dc.GetPixel(x, y);
            if (thisPixel == srcColour)
                dc.SetPixel(x, y, dstColour);
        }
    }
	dc.SelectObject(oldBMP);
    bitmap.Detach();
    replaceColour(hbitmap, srcColour, dstColour);
}


/**
 *  Return the colour of the (x, y)'th pixel in hbitmap.
 *
 *  @param hbitmap      The image to get the pixel from.
 *  @param x            The x coordinate.
 *  @param y            The y coordinate.
 *  @returns            The pixel at (x, y).
 */
COLORREF
getPixel
(
    HBITMAP             hbitmap,
    unsigned int        x,
    unsigned int        y
)
{
	BW_GUARD;

    CDC dc;
    dc.CreateCompatibleDC(NULL);
    HGDIOBJ oldBMP = dc.SelectObject(hbitmap);
    COLORREF srcColour = dc.GetPixel(x, y);
    dc.SelectObject(oldBMP);
    return srcColour;
}


/**
 *  This function resizes a child control to the given extents.
 *
 *  @param wnd      The window to resize.
 *  @param left     The left coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param top      The top coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param right    The right coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param bottom   The bottom coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 */
void childResize
(
    CWnd            &wnd, 
    int             left, 
    int             top, 
    int             right, 
    int             bottom
)
{
	BW_GUARD;

    if (wnd.GetSafeHwnd() != 0)
    {
        CWnd *parent = wnd.GetParent();
        if (parent != NULL)
        {
            RECT extents;
            wnd.GetWindowRect(&extents);
            parent->ScreenToClient(&extents);
            if (left == NO_RESIZE)
                left = extents.left;
            if (top == NO_RESIZE)
                top = extents.top;
            if (right == NO_RESIZE)
                right = extents.right;
            if (bottom == NO_RESIZE)
                bottom = extents.bottom;
            wnd.SetWindowPos
            (
                NULL, 
                left, 
                top, 
                right - left, 
                bottom - top, 
                SWP_NOZORDER
            );
        }
    }
}


/**
 *  This function resizes a child control to the given extents.
 *
 *  @param wnd      The window to resize.
 *  @param extents  The extents of the child control.  If any of the 
 *                  coordinates are set to NO_RESIZE then the existing
 *                  coordinates are used.
 */
void childResize
(
    CWnd            &wnd, 
    CRect           const &extents
)
{
	BW_GUARD;

    childResize(wnd, extents.left, extents.top, extents.right, extents.bottom);
}


/**
 *  This function resizes a child control to the given extents.
 *
 *  @param parent   The parent of the window to resize.
 *  @param id		The id of the child to resize.
 *  @param left     The left coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param top      The top coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param right    The right coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 *  @param bottom   The bottom coordinate of the child control.  If this is set
 *                  to NO_RESIZE then the coordinate is left as is.
 */
void childResize
(
	CWnd			&parent, 
	UINT			id, 
	int				left, 
	int				top, 
	int				right, 
	int				bottom
)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
		if (child != NULL)
			childResize(*child, left, top, right, bottom);
	}
}


/**
 *  This function resizes a child control to the given extents.
 *
 *  @param parent   The parent of the window to resize.
 *  @param id		The id of the child to resize.
 *  @param extents  The extents of the child control.  If any of the 
 *                  coordinates are set to NO_RESIZE then the existing
 *                  coordinates are used.
 */
void childResize(CWnd &parent, UINT id, CRect const &extents)
{
	BW_GUARD;

    childResize(parent, id, extents.left, extents.top, extents.right, extents.bottom);
}


/**
 *  This function gets the extens of a child window in the parent window's 
 *  coordinate system
 *
 *  @param child    The child window.
 *  @returns        The extents of the child window.
 */
CRect childExtents(CWnd const &child)
{
	BW_GUARD;

    if (child.GetSafeHwnd() != 0)
    {
        CRect result;
        child.GetWindowRect(&result);
        CWnd *parent = child.GetParent();
        if (parent)
            parent->ScreenToClient(&result);
        return result;
    }
    return CRect();
}


/**
 *  This function gets the extens of a child window in the parent window's 
 *  coordinate system
 *
 *  @param parent   The parent window.
 *  @param id       The id of the child window.
 *  @returns        The extents of the child window.
 */
CRect childExtents(CWnd const &parent, UINT id)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
            return childExtents(*child);
    }
    return CRect();
}


/**
 *  This function enables/disables a child control.
 *
 *  @param parent   The parent window.
 *  @param id       The id of the child window to enable/disable.
 *  @param enable   If true the window is enabled, otherwise it is disabled.
 */
void enableWindow(CWnd &parent, UINT id, bool enable)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
            child->EnableWindow(enable ? TRUE : FALSE);
    }
}


/**
 *	This function shows/hides a child control.
 *
 *  @param parent   The parent window.
 *  @param id       The id of the child window to show or hide.
 *  @param show     The show/hide command.
 */
void showWindow(CWnd &parent, UINT id, UINT show)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
            child->ShowWindow(show);
    }
}


/**
 *  This gets the checked status of a button.
 *
 *  @param parent   The parent window.
 *  @param id       The id of the child button.
 *  @returns        True if the button is checked.
 */
bool isButtonChecked(CWnd const &parent, UINT id)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CButton const *child = 
			reinterpret_cast<CButton *>(parent.GetDlgItem(id));
        if (child != NULL)
        {
            return child->GetCheck() == BST_CHECKED;
        };
    }
    return false;
}


/**
 *  This gets the checked status of a button.
 *
 *  @param parent   The parent window.
 *  @param id       The id of the child button.
 *  @param check    If true then the button will be checked.
 */
void checkButton(CWnd &parent, UINT id, bool check)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CButton *child = reinterpret_cast<CButton *>(parent.GetDlgItem(id));
        if (child != NULL)
        {
            child->SetCheck(check ? BST_CHECKED : BST_UNCHECKED);
        }
    }
}


/**
 *  This sets the text of a child control.
 *
 *  @param parent       The parent control.
 *  @param id           The id of the child control.
 *  @param text         The new text of the control.
 */
void setWindowText(CWnd &parent, UINT id, BW::string const &text)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
		{
			BW::wstring wtext;
			bw_utf8tow( BW::string( text.c_str() ), wtext );
            child->SetWindowText(wtext.c_str());
		}
    }
}


/**
 *  This sets the text of a child control.
 *
 *  @param parent       The parent control.
 *  @param id           The id of the child control.
 *  @param text         The new text of the control.
 */
void setWindowText(CWnd &parent, UINT id, BW::wstring const &text)
{
	BW_GUARD;

    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
		{
            child->SetWindowText(text.c_str());
		}
    }
}


/**
 *  This gets the text of a child control.
 *
 *  @param parent       The parent control.
 *  @param id           The id of the child control.
 *  @returns            The text of the child control.
 */
BW::string getWindowText(CWnd const &parent, UINT id)
{
	BW_GUARD;

    BW::string result;
    if (parent.GetSafeHwnd() != 0)
    {
        CWnd *child = parent.GetDlgItem(id);
        if (child != NULL)
        {
            CString txt;
            child->GetWindowText(txt);
            bw_wtoutf8( txt.GetBuffer(), result );
        }
    }
    return result;
}


/**
 *  Makes sure a dialog gets positioned completely inside the screen, near to
 *  the original desired position
 *
 *  @param size           Size of the dialog/window to position.
 *  @param pt             Desired point where the dialog should be positioned.
 *  @param hAlignment     Desired horizontal alignment.
 *  @param vAlignment     Desired vertical alignment.
 *
 *  @returns              Validated position for the dialog.
 */
CPoint validateDlgPos( const CSize& size, const CPoint& pt,
	HALIGNMENT hAlignment, VALIGNMENT vAlignment )
{
	BW_GUARD;

	MONITORINFO monitorInfo = { sizeof( monitorInfo ) };
	GetMonitorInfo( MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST ), &monitorInfo );
	CRect monitor = monitorInfo.rcWork;

	CPoint leftTop;
	if ( hAlignment == CENTRE_HORZ )
	{
		leftTop.x = -size.cx / 2;
	}
	else if ( hAlignment == LEFT )
	{
		leftTop.x = 0;
	}
	else if ( hAlignment == RIGHT )
	{
		leftTop.x = -size.cx;
	}

	if ( vAlignment == CENTRE_VERT )
	{
		leftTop.y = -size.cy / 2;
	}
	else if ( vAlignment == TOP )
	{
		leftTop.y = 0;
	}
	else if ( vAlignment == BOTTOM )
	{
		leftTop.y = -size.cy;
	}

	CPoint bottomRight = leftTop + size;
	if ( leftTop.x + pt.x < monitor.left )
	{
		leftTop.x = monitor.left - pt.x;
		bottomRight.x = leftTop.x + size.cx;
	}
	if ( leftTop.y + pt.y < monitor.top )
	{
		leftTop.y = monitor.top - pt.y;
		bottomRight.y = leftTop.y + size.cy;
	}
	if ( bottomRight.x + pt.x > monitor.right )
	{
		bottomRight.x = monitor.right - pt.x;
		leftTop.x = bottomRight.x - size.cx;
	}
	if ( bottomRight.y + pt.y > monitor.bottom )
	{
		bottomRight.y = monitor.bottom - pt.y;
		leftTop.y = bottomRight.y - size.cy;
	}

	return leftTop + pt;
}


/**
 *	This function loads a resource string.
 *
 *  @param id		The id of the string to load.
 *  @returns		The string with the given id.  This will be empty if the
 *					string could not be loaded.
 */
BW::string loadString(UINT id)
{
	BW_GUARD;

	CString cstring;
	if (cstring.LoadString(id) != FALSE)
	{
		BW::string nbuffer;
		bw_wtoutf8( cstring.GetBuffer(), nbuffer );
		return nbuffer;
	}
	else
	{
		return BW::string();
	}
}

BW::wstring controls::loadStringW(UINT id)
{
	BW_GUARD;

	CString cstring;
	if (cstring.LoadString(id) != FALSE)
	{
		return cstring.GetString();
	}
	else
	{
		return BW::wstring();
	}
}

} // namespace controls
BW_END_NAMESPACE

