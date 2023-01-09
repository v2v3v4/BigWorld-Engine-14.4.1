#include "pch.hpp"
#include "wnd_sizer.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

/**
 *  This is the WndSizer constructor.
 *
 *  @param child        The child window of the Sizer.
 *  @param horizFit     How does the child fit horizontally?
 *  @param vertFit      How does the child fit vertically?
 *  @param padLeft      
 */
/*explicit*/ WndSizer::WndSizer
(
    CWnd                *child, 
    HorizontalFit       horizFit    /*= FIT_WIDTH*/,
    VerticalFit         vertFit     /*= FIT_HEIGHT*/,
    int                 padLeft     /*= 0*/,
    int                 padTop      /*= 0*/,
    int                 padRight    /*= 0*/,
    int                 padBottom   /*= 0*/
):
    wnd_(child),
    id_(0),
    horizFit_(horizFit),
    vertFit_(vertFit),
    padLeft_(padLeft),
    padTop_(padTop),
    padRight_(padRight),
    padBottom_(padBottom)
{
	BW_GUARD;

    CRect extents;
    child->GetWindowRect(&extents);
    minWidth_   = extents.Width();
    minHeight_  = extents.Height();
    Sizer::extents(extents);
}


/**
 *  This is the WndSizer constructor.
 *
 *  @param parent       The parent of the window to size.
 *  @param id           The id of the child window to size.
 *  @param horizFit     How does the child fit horizontally?
 *  @param vertFit      How does the child fit vertically?
 */
WndSizer::WndSizer
(
    CWnd                *parent,
    uint32				id,
    HorizontalFit       horizFit    /*= FIT_WIDTH*/,
    VerticalFit         vertFit     /*= FIT_HEIGHT*/,
    int                 padLeft     /*= 0*/,
    int                 padTop      /*= 0*/,
    int                 padRight    /*= 0*/,
    int                 padBottom   /*= 0*/
):
    wnd_(parent),
    id_(id),
    horizFit_(horizFit),
    vertFit_(vertFit),
    padLeft_(padLeft),
    padTop_(padTop),
    padRight_(padRight),
    padBottom_(padBottom)
{
	BW_GUARD;

    CWnd *wnd = window();
    CRect extents;
    wnd->GetWindowRect(&extents);
    minWidth_   = extents.Width();
    minHeight_  = extents.Height();
    Sizer::extents(extents);
}


/**
 *  This gets how the window is fitted horizontally when sized.
 *
 *  @returns            The horizontal fitting.
 */
WndSizer::HorizontalFit WndSizer::horizontalFit() const
{
    return horizFit_;
}


/**
 *  This sets how the window is fitted horizontally when resized.
 *
 *  @param fit          The new horizontal fitting.
 */
void WndSizer::horizontalFit(HorizontalFit fit)
{
    horizFit_ = fit;
}


/**
 *  This gets how the window is fitted vertically when sized.
 *
 *  @returns            The vertical fitting.
 */
WndSizer::VerticalFit WndSizer::verticalFit() const
{
    return vertFit_;
}


/**
 *  This sets how the window is fitted vertically when resized.
 *
 *  @param fit          The new vertical fitting.
 */
void WndSizer::verticalFit(VerticalFit fit)
{
    vertFit_ = fit;
}


/**
 *  This is called to fit the WndSizer (and hence it's window) into
 *  a rectangle of the given area.
 *
 *  @param extents      The area that the window should be fitted into.
 */
/*virtual*/ void WndSizer::onSize(CRect const &extents)
{
	BW_GUARD;

    Sizer::onSize(extents);

    CRect curExt;
    window()->GetWindowRect(&curExt);

    // Work out the x-coordinates:
    int left, width;
    switch (horizFit_)
    {
    case FIT_LEFT:
        left  = extents.left + padLeft_;
        width = curExt.Width();
        break;

    case FIT_HCENTER:
        left  = (extents.left + extents.right - curExt.Width())/2;
        width = curExt.Width();
        break;

    case FIT_RIGHT:
        left  = extents.right - curExt.Width() - padRight_;
        width = curExt.Width();
        break;

    case FIT_WIDTH:
        left  = extents.left + padLeft_;
        width = extents.Width() - padLeft_ - padRight_;
        break;
    }

    // Work out the y-coordinates:
    int top, height;
    switch (vertFit_)
    {
    case FIT_TOP:
        top    = extents.top + padTop_;
        height = curExt.Height();
        break;

    case FIT_VCENTER:
        top    = (extents.top + extents.bottom - curExt.Height())/2;
        height = curExt.Height();
        break;

    case FIT_BOTTOM:
        top    = extents.bottom - curExt.Height() - padBottom_;
        height = curExt.Height();
        break;

    case FIT_HEIGHT:
        top    = extents.top + padTop_;
        height = extents.Height() - padTop_ - padBottom_;
        break;
    }

    window()->SetWindowPos
    (
        NULL,
        left , top   , 
        width, height,
        SWP_NOZORDER
    );
}


/**
 *  This gets the minimum size of the WndSizer.
 *
 *  @returns            The size of the window when the WndSizer was 
 *                      constructed.
 */
/*virtual*/ CSize WndSizer::minimumSize() const
{
    return CSize(minWidth_, minHeight_);
}


/**
 *  This gets called to draw the WndSizer for debugging.
 *
 *  @param dc       The device context to draw into.
 */
/*virtual*/ void WndSizer::draw(CDC *dc)
{
	BW_GUARD;

    drawRect(dc, extents(), RGB(0, 0, 255));
}


/**
 *  This gets the window that we are sizing for.
 *
 *  @returns            The window that we are sizing for.  The return value
 *                      can only be used temporarily.
 */
CWnd *WndSizer::window()
{
	BW_GUARD;

    if (id_ != 0)
        return wnd_->GetDlgItem(id_);
    else
        return wnd_;
}

} // namespace controls
BW_END_NAMESPACE

