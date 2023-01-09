/**
 *	Class that draws an overlay image over the button, allowing image buttons
 *	with XP/Vista themes.
 */

// TODO: add features, such as icon alignment, multiple images, etc.

#include "pch.hpp"
#include "themed_image_button.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{

IMPLEMENT_DYNAMIC(ThemedImageButton, CButton)


/**
 *	Constructor
 */
ThemedImageButton::ThemedImageButton() :
	hIcon_( NULL ),
	hCheckedIcon_( NULL )
{
}


/**
 *	Destructor
 */
ThemedImageButton::~ThemedImageButton()
{
}


/**
 *	Sets the button's image icon. If the button is a push-like checkbox, this
 *	icon will be used when the button is unchecked.
 *	NOTES:
 *	  - This is NOT a virtual method in the base class, careful with pointers
 *	  - Whoever calls this method owns the icon, and should clean it up
 *	
 *	@param hIcon          icon to draw on top of the button
 *	@return               the previous icon
 */
HICON ThemedImageButton::SetIcon( HICON hIcon )
{
	BW_GUARD;

	HICON oldHIcon = hIcon_;
	hIcon_ = hIcon;
	Invalidate();
	return oldHIcon;
}


/**
 *	Sets the button's image icon to display when it's checked (BST_CHECKED) 
 *	
 *	@param hIcon          icon to draw on top of the button
 *	@return               the previous icon
 */
HICON ThemedImageButton::SetCheckedIcon( HICON hIcon )
{
	BW_GUARD;

	HICON oldHIcon = hIcon_;
	hCheckedIcon_ = hIcon;
	Invalidate();
	return oldHIcon;
}


BEGIN_MESSAGE_MAP(ThemedImageButton, CButton)
	ON_NOTIFY_REFLECT (NM_CUSTOMDRAW, OnNotifyCustomDraw)
END_MESSAGE_MAP()


/**
 *	Custom draw notification handler
 */
void ThemedImageButton::OnNotifyCustomDraw (
	NMHDR * pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	if ( !hIcon_ )
		return;

	HICON icon = hIcon_;

	if ( GetCheck() == BST_CHECKED && hCheckedIcon_ )
	{
		icon = hCheckedIcon_;
	}

	LPNMCUSTOMDRAW pCustomDraw = (LPNMCUSTOMDRAW) pNotifyStruct;
	HDC hDC = pCustomDraw->hdc;

	CRect rect;
	GetWindowRect( rect );
	ScreenToClient( rect );
	
	DWORD style = GetStyle();

	// determine size of icon image
	ICONINFO ii;
	memset (&ii, 0, sizeof (ii));
	GetIconInfo (icon, &ii);
	BITMAPINFO bi;
	memset (&bi, 0, sizeof (bi));
	bi.bmiHeader.biSize = sizeof (bi);
	int cx = 0;
	int cy = 0;
	if (ii.hbmColor != NULL)
	{
		// icon has separate image and mask bitmaps - use size directly
		GetDIBits(hDC, ii.hbmColor, 0, 0, NULL, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
		cx = bi.bmiHeader.biWidth;
		cy = bi.bmiHeader.biHeight;
	}
	else
	{
		// icon has single mask bitmap which is twice as high as icon
		GetDIBits(hDC, ii.hbmMask, 0, 0, NULL, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
		cx = bi.bmiHeader.biWidth;
		cy = bi.bmiHeader.biHeight / 2;
	}

	// determine position of top-left corner of icon
	int x = ( rect.Width() - cx ) / 2;
	int y = ( rect.Height() - cy ) / 2;

	// Draw the icon
	DrawState(hDC, NULL, NULL, (LPARAM) icon, 0, x, y, cx, cy,
		(style & WS_DISABLED) != 0 ? (DST_ICON | DSS_DISABLED) : (DST_ICON | DSS_NORMAL));

	// Cleanup
	::DeleteObject(ii.hbmMask);
	::DeleteObject(ii.hbmColor);
}

} // namespace controls
BW_END_NAMESPACE

