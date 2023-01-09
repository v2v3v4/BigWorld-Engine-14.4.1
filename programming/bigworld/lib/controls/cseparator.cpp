#include "pch.hpp"
#include "controls/cseparator.hpp"


using namespace controls;


BEGIN_MESSAGE_MAP(CSeparator, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()


/**
 *  Constructor.
 */
CSeparator::CSeparator()
:
CStatic()
{
}


/**
 *  Remove the SS_SUNKEN style which makes the empty static control visible
 *  within the dialog editor.
 */
void CSeparator::PreSubclassWindow()
{
    ModifyStyleEx(SS_SUNKEN | WS_EX_STATICEDGE, 0);
    CStatic::PreSubclassWindow();
}


/** 
 *  Paint handler
 */
void CSeparator::OnPaint()
{
    // Get the size of the control:
    CRect rectWnd, rectClient;
    GetWindowRect(rectWnd);
    GetClientRect(rectClient);

    // If the control is taller than wide then draw a vertical line:
    if (rectWnd.Height() > rectWnd.Width())
    {
        CPaintDC dc(this);
        dc.Draw3dRect
        (
            rectWnd.Width()/2,  // left
            0,                  // top
            2,                  // width
            rectWnd.Height(),   // height
            ::GetSysColor(COLOR_3DSHADOW),
            ::GetSysColor(COLOR_3DHIGHLIGHT)
        );
    }
    else
    {
        CPaintDC dc(this);
        DWORD    style  = GetStyle();
        UINT     format = DT_TOP;
        if ((style & SS_NOPREFIX) != 0)
        {
            format |= DT_NOPREFIX;
        }
        dc.Draw3dRect
        (
            0,                      // left
            rectWnd.Height()/2,     // top
            rectWnd.Width(),        // width
            2,                      // height
            ::GetSysColor(COLOR_3DSHADOW),
            ::GetSysColor(COLOR_3DHIGHLIGHT)
        );
        CString text;
        GetWindowText(text);
        if ((style & SS_CENTER) != 0)
        {
            text = _T("  ") + text + _T("  ");
            format |= DT_CENTER;
        }
        else if ((style & SS_RIGHT) != 0)
        {
            text = _T("  ") + text;
            format |= DT_RIGHT;
        }
        else
        {
            text = text + _T("  ");
            format |= DT_LEFT;
        }
        HGDIOBJ  oldFont  = dc.SelectObject(GetFont());
        COLORREF oldBkClr = dc.SetBkColor(::GetSysColor(COLOR_BTNFACE));
        dc.DrawText(text, rectClient, format);
        dc.SetBkColor(oldBkClr);
        dc.SelectObject(oldFont);
    }

    CPaintDC        dc(this);

}
