#include "pch.hpp"
#include "dialog_toolbar.hpp"
#include "utils.hpp"
#include <afxpriv.h>

BW_BEGIN_NAMESPACE
namespace controls
{

BEGIN_MESSAGE_MAP(DialogToolbar, CToolBar)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()

DialogToolbar::DialogToolbar():
	CToolBar()
{
}

/**
 *  This function 'subclasses' an existing control with the toolbar.  This is
 *  not proper subclassing, as we hide the subclassed control, but borrow its
 *  position.
 *
 *  @param id       The id of the control to subclass.
 */
void DialogToolbar::Subclass(UINT id)
{
	BW_GUARD;

    CWnd  *parent        = GetParent();
    CWnd  *toolbarHolder = parent->GetDlgItem(id);
    CSize sizeBar        = CalcFixedLayout(FALSE, TRUE);
    WINDOWPLACEMENT wpl;
    toolbarHolder->GetWindowPlacement(&wpl);
    wpl.rcNormalPosition.bottom = wpl.rcNormalPosition.top  + sizeBar.cy + 4;
    wpl.rcNormalPosition.right  = wpl.rcNormalPosition.left + sizeBar.cx + 4;
    toolbarHolder->SetWindowPlacement(&wpl);
    SetWindowPlacement(&wpl);
    RepositionBars
    (
        AFX_IDW_CONTROLBAR_FIRST, 
        AFX_IDW_CONTROLBAR_LAST, 
        0
    );
    toolbarHolder->ShowWindow(SW_HIDE);
    SetWindowPos
    (
        &CWnd::wndTop,
        0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE
    );
    INIT_AUTO_TOOLTIP();
}


/**
 *  This function loads a non-8bpp bitmap for the toolbar and replaces the
 *  top-left pixel with the appropriate colour for the background.
 *
 *  @param id           The id of the bitmap to use.
 *  @param disabledId   The optional bitmap to use for disabled items.
 *  @returns            TRUE upon success.
 */
BOOL DialogToolbar::LoadToolBarEx(UINT id, UINT disabledId /*= 0*/)
{
	BW_GUARD;

    BOOL ok = TRUE;
    ok &= LoadToolBar(id);
    CBitmap bitmap;
    ok &= bitmap.LoadBitmap(id);
    controls::replaceColour(bitmap, ::GetSysColor(COLOR_BTNFACE));
    SetBitmap((HBITMAP)bitmap.Detach());

    if (disabledId != 0)
    {   
        ok &= disabledBmp_.LoadBitmap(disabledId);
        BITMAP bitmapInfo;
        ::GetObject(disabledBmp_, sizeof(BITMAP), &bitmapInfo);
        controls::replaceColour(bitmap, ::GetSysColor(COLOR_BTNFACE));
        disabledImgList_.Create
        (
            bitmapInfo.bmHeight, 
            bitmapInfo.bmHeight, 
            ILC_COLOR24 | ILC_MASK, 
            bitmapInfo.bmWidth/bitmapInfo.bmHeight, 
            1
        );
        disabledImgList_.Add(&disabledBmp_, getPixel(disabledBmp_, 0, 0));
        GetToolBarCtrl().SetDisabledImageList(&disabledImgList_);
    }

    return ok;
}


/**
 *  This is called during idle to update the UI.
 *
 *  @param wparam        The wparam.
 *  @param lparam        The lparam.
 *  @returns             0.
 */
LRESULT DialogToolbar::OnIdleUpdateCmdUI(WPARAM wparam, LPARAM lparam)
{
	BW_GUARD;

    if (IsWindowVisible() != FALSE)
    {
        // Apparently it doesn't matter if the parent is a CFrameWnd.
        CFrameWnd *parent = (CFrameWnd *)GetParent();
        if (parent != NULL)
            OnUpdateCmdUI(parent, (BOOL)wparam);
    }
    return 0;
}

} // namespace controls
BW_END_NAMESPACE

