
#include "pch.hpp"
#include "checkbox_helper.hpp"
#include "controls/utils.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	const int CHECKBOX_BITMAP_SIZE = 14;
} // anonymous namespace


/**
 *	This method initialises the checkbox bitmaps. It uses the colour of the
 *	top-left pixel as the transparent colour, and black "0" as the foreground.
 *
 *	@param checkedResId		Resource ID of the "checked" checkbox image.
 *	@param uncheckedResId	Resource ID of the "unchecked" checkbox image.
 *	@param fgSel			Foreground colour for a selected item.
 *	@param bgSel			Background colour for a selected item.
 *	@param fgEven			Foreground colour for an even unselected item.
 *	@param bgEven			Background colour for an even unselected item.
 *	@param fgOdd			Foreground colour for an odd unselected item.
 *	@param bgOdd			Background colour for an odd unselected item.
 */
void CheckboxHelper::init( int checkedResId, int uncheckedResId,
		COLORREF fgSel, COLORREF bgSel, COLORREF fgEven, COLORREF bgEven,
		COLORREF fgOdd, COLORREF bgOdd )
{
	BW_GUARD;

	// Since we assume that (0,0,0) is the foreground colour, don't use it as
	// a background colour, and the difference with (1, 1, 1) will be
	// imperceptible.
	if (bgEven == 0) bgEven = RGB( 1, 1, 1 );
	if (bgOdd == 0) bgOdd = RGB( 1, 1, 1 );
	if (bgSel == 0) bgSel = RGB( 1, 1, 1 );

	// check on
	checkOnEven_.LoadBitmap( checkedResId );
	controls::replaceColour( (HBITMAP)checkOnEven_, bgEven );
	controls::replaceColour( (HBITMAP)checkOnEven_, 0, fgEven );

	checkOnOdd_.LoadBitmap( checkedResId );
	controls::replaceColour( (HBITMAP)checkOnOdd_, bgOdd );
	controls::replaceColour( (HBITMAP)checkOnOdd_, 0, fgOdd );

	checkOnSel_.LoadBitmap( checkedResId );
	controls::replaceColour( (HBITMAP)checkOnSel_, bgSel );
	controls::replaceColour( (HBITMAP)checkOnSel_, 0, fgSel );

	// check off
	checkOffEven_.LoadBitmap( uncheckedResId );
	controls::replaceColour( (HBITMAP)checkOffEven_, bgEven );
	controls::replaceColour( (HBITMAP)checkOffEven_, 0, fgEven );

	checkOffOdd_.LoadBitmap( uncheckedResId );
	controls::replaceColour( (HBITMAP)checkOffOdd_, bgOdd );
	controls::replaceColour( (HBITMAP)checkOffOdd_, 0, fgOdd );

	checkOffSel_.LoadBitmap( uncheckedResId );
	controls::replaceColour( (HBITMAP)checkOffSel_, bgSel );
	controls::replaceColour( (HBITMAP)checkOffSel_, 0, fgSel );
}


/**
 *	This method draws the checkbox bmp + the focus rect if necessary.
 *
 *	@param dc		Device context to draw into
 *	@param rect		Rectangle of the list item where the checkbox goes.
 *	@param pt		Mouse position in client coordinates.
 *	@param selected	Whether the item is selected or not.
 *	@param odd		Whether the list position of the item is odd or not.
 *	@param checked	Whether the checkbox is ticked or not.
 */
void CheckboxHelper::draw( CDC & dc, const CRect & rect, const CPoint & pt,
									  bool selected, bool odd, bool checked )
{
	BW_GUARD;

	CRect cbRect = rect;
	this->rect( cbRect );

	CDC bmpDC;
	
	bmpDC.CreateCompatibleDC( &dc );

	CBitmap *checkBmp = NULL;
	if (selected)
	{
		checkBmp = checked ? &checkOnSel_ : &checkOffSel_;
	}
	else if (!odd)
	{
		checkBmp = checked ? &checkOnEven_ : &checkOffEven_;
	}
	else
	{
		checkBmp = checked ? &checkOnOdd_ : &checkOffOdd_;
	}

	CBitmap * oldBmp = bmpDC.SelectObject( checkBmp );

	dc.BitBlt( cbRect.left, cbRect.top,
						CHECKBOX_BITMAP_SIZE, CHECKBOX_BITMAP_SIZE,
						&bmpDC, 0, 0, SRCCOPY );

	bmpDC.SelectObject( oldBmp );

	if (cbRect.PtInRect( pt ))
	{
		dc.DrawFocusRect( &cbRect );
	}
}


/**
 *	This method calculates the rectangle of a checkbox.
 *
 *	@param retRect	The caller passes here the rectangle of the subitem, and
 *					the method returns a the checkbox rect that fits into the
 *					original rect.
 */
void CheckboxHelper::rect( CRect & retRect ) const
{
	BW_GUARD;

	retRect.left = retRect.left + (retRect.Width() - CHECKBOX_BITMAP_SIZE) / 2;
	retRect.top = retRect.top + (retRect.Height() - CHECKBOX_BITMAP_SIZE) / 2;
	retRect.right = retRect.left + CHECKBOX_BITMAP_SIZE;
	retRect.bottom = retRect.top + CHECKBOX_BITMAP_SIZE;
}

BW_END_NAMESPACE

