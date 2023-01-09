#include "pch.hpp"
#include "controls/image_button.hpp"


BW_BEGIN_NAMESPACE
namespace controls
{

BEGIN_MESSAGE_MAP(ImageButton, CButton)
	ON_WM_LBUTTONDOWN()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


/**
 *  Constructor.
 */
ImageButton::ImageButton():
	CButton(),
	defID_(0),
	disabledID_(0),
	clrMask_(RGB(255, 255, 255)),
	isToggleButton_(false),
	toggled_(false)
{
}


/**
 *  Destructor.
 */
ImageButton::~ImageButton()
{
}


/**
 *  Set the id of the bitmap to draw with.
 * 
 *  @param defID            The id of the default bitmap to draw.
 *  @param disabledID       The id of the disabled bitmap to draw.
 *  @param clrMask          The transparent colour.
 */
void 
ImageButton::setBitmapID
(
    UINT        defID, 
    UINT        disabledID, 
    COLORREF    clrMask     /* = RGB(255, 255, 255)*/
)
{
    defID_        = defID;
    disabledID_   = disabledID;
    clrMask_      = clrMask;
}


/**
 *	This sets whether the button is a toggle button.
 *
 *	@param enable			If true then the button is a toggle button, if 
 *							false then it is a regular button.
 */
void ImageButton::toggleButton(bool enable)
{
	isToggleButton_ = enable;
}


/**
 *	This returns whether the button is a toggle button.
 *
 *	@return					True if the button is a toggle button, false if it
 *							is a regular button.
 */
bool ImageButton::isToggleButton() const
{
	return isToggleButton_;
}


/**
 *	This sets whether the button is in a toggled state.
 *
 *	@param toggled			If true then the button is in the down state and if
 *							false then it is in the up state.
 */
void ImageButton::toggle(bool toggled)
{
	BW_GUARD;

	if (isToggleButton_ && toggled != toggled_)
	{
		toggled_ = toggled;
		InvalidateRect(NULL);
	}
}


/**
 *	This gets whether the button is in the toggled state.
 *
 *	@returns				True if the button is in the down state, false if 
 *							is in the up state.
 */
bool ImageButton::isToggled() const
{
	return toggled_;
}


/**
 *  Draw the button.
 * 
 *  @drawItemStruct         Drawing information.
 */
/*virtual*/ void ImageButton::DrawItem(LPDRAWITEMSTRUCT drawItemStruct)
{
	BW_GUARD;

    CDC   *dc  = CDC::FromHandle(drawItemStruct->hDC);
    CRect rect = drawItemStruct->rcItem;

    bool sel     = ((drawItemStruct->itemState & ODS_SELECTED) == ODS_SELECTED);
    bool disable = ((drawItemStruct->itemState & ODS_DISABLED) == ODS_DISABLED);

    // Draw the frame:
    UINT state = DFCS_BUTTONPUSH;
    if (sel || toggled_)
        state |= DFCS_PUSHED;
    if (disable)
        state |= DFCS_INACTIVE;
    dc->DrawFrameControl(&drawItemStruct->rcItem, DFC_BUTTON, state);
    
    // Draw the bitmap:
    UINT id = disable ? disabledID_ : defID_;
    HBITMAP hbitmap = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(id));
    if (hbitmap != NULL)
    {
        // Get the dimensions:
        BITMAP bitmap;
        ::GetObject(hbitmap, sizeof(BITMAP), &bitmap);

        int delta = /*sel ? 2 :*/ 0;

        transparentBlit
        (
            dc, 
            (rect.Width () - bitmap.bmWidth )/2 + delta,
            (rect.Height() - bitmap.bmHeight)/2 + delta,
            bitmap.bmWidth,
            bitmap.bmHeight,
            hbitmap,
            clrMask_
        );

        // Cleanup:
        ::DeleteObject(hbitmap);
    }
}


/**
 *  Add the owner draw style to the button.
 */
/*afx_msg*/ void ImageButton::PreSubclassWindow()
{
	BW_GUARD;

    ModifyStyle(0, BS_OWNERDRAW);
    CButton::PreSubclassWindow();
}


/**
 *  Draw the background of the button.
 * 
 *  @param dc           The drawing context.
 *  @returns            TRUE.
 */
/*afx_msg*/ BOOL ImageButton::OnEraseBkgnd(CDC *dc)
{
	BW_GUARD;

    CRect rect;
    GetClientRect(&rect);
    dc->FillSolidRect(rect, ::GetSysColor(COLOR_BTNFACE));
    return TRUE;
}


/**
 *	This is called when the user presses the left-mouse button down.
 *
 *	@param flags		Modifier key flags.
 *	@param point		The point that mouse went down at.
 */
void ImageButton::OnLButtonDown(UINT flags, CPoint point)
{
	BW_GUARD;

	if (isToggleButton_)
	{
		toggled_= !toggled_;
		InvalidateRect(NULL);
	}
	CButton::OnLButtonDown(flags, point);
}


/**
 *  Transparent blit.
 * 
 *  @param dc           The drawing context.
 *  @param left         The left coordinate.
 *  @param top          The top coordinate.
 *  @param width        The width of the bitmap.
 *  @param height       The height of the bitmap.
 *  @param bitmap       The bitmap to draw.
 *  @param clrMask      The transparent colour.
 */
void
ImageButton::transparentBlit
(
    CDC                 *dc,
    int                 left,
    int                 top,
    int                 width,
    int                 height,
    HBITMAP             bitmap,
    COLORREF            clrMask
) const
{
	BW_GUARD;

    CDC srcDC;
    srcDC.CreateCompatibleDC(dc);
    HGDIOBJ oldBmp = srcDC.SelectObject(bitmap);
    dc->TransparentBlt
    (
        left, 
        top, 
        width, 
        height, 
        &srcDC, 
        0, 
        0, 
        width, 
        height, 
        clrMask
    );
	srcDC.SelectObject(oldBmp);
    srcDC.DeleteDC();
}

} // namespace controls
BW_END_NAMESPACE

