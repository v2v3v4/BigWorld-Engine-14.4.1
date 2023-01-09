
#include "pch.hpp"
#include "view_skin.hpp"
#include "view_draw_utils.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This static method draws an arrow between two boxes, joining the correct
 *	edges of the start and end boxes. For example, if the start box is on top
 *	of the end box and both boxes are fairly aligned vertically, the arrow will
 *	go from the bottom edge of the start box to the top edge of the end box.
 *
 *	@param dc	Device context for performing the drawing.
 *	@param rectStart	Start box rectangle.
 *	@param rectEnd		End box rectangle.
 *	@param colour		Colour to draw the arrow in.
 */
/*static*/
void ViewDrawUtils::drawBoxConection( CDC & dc, const CRect & rectStart,
					const CRect & rectEnd, CRect & retRect, COLORREF colour )
{
	BW_GUARD;

	// Draw line
	CPoint pt1 = rectStart.CenterPoint();
	CPoint pt2 = rectEnd.CenterPoint();

	int xDelta = pt1.x - pt2.x;
	int yDelta = pt1.y - pt2.y;
	bool isHorizontal = abs( yDelta ) < abs( xDelta );

	if (isHorizontal)
	{
		// snap to vertical centre, and clip horizontal coords to rects
		if (xDelta < 0)
		{
			pt1.x = rectStart.right + 1;
			pt2.x = rectEnd.left - 2;
		}
		else
		{
			pt2.x = rectEnd.right + 1;
			pt1.x = rectStart.left - 2;
		}
	}
	else
	{
		// snap to horizontal centre, and clip vertical coords to rects
		if (yDelta < 0)
		{
			pt1.y = rectStart.bottom + 1;
			pt2.y = rectEnd.top - 2;
		}
		else
		{
			pt2.y = rectEnd.bottom + 1;
			pt1.y = rectStart.top - 2;
		}
	}

	int rectX1 = pt1.x;
	int rectY1 = pt1.y;
	int rectX2 = pt2.x;
	int rectY2 = pt2.y;

	if (xDelta > 0) std::swap( rectX1, rectX2 );
	if (yDelta > 0) std::swap( rectY1, rectY2 );

	int arrowSize = ViewSkin::edgeArrowSize();

	CPoint arrowPoints[ 3 ];
	arrowPoints[0] = pt2;
	if (isHorizontal)
	{
		int xSign = pt1.x < pt2.x ? -1 : 1;
		arrowPoints[1] = CPoint( pt2.x + arrowSize * xSign, pt2.y - arrowSize );
		arrowPoints[2] = CPoint( pt2.x + arrowSize * xSign, pt2.y + arrowSize );
		pt2.x += arrowSize * xSign;

		retRect.SetRect( rectX1, rectY1 - arrowSize, rectX2, rectY2 + arrowSize );
	}
	else
	{
		int ySign = pt1.y < pt2.y ? -1 : 1;
		arrowPoints[1] = CPoint( pt2.x - arrowSize, pt2.y + arrowSize * ySign );
		arrowPoints[2] = CPoint( pt2.x + arrowSize, pt2.y + arrowSize * ySign );
		pt2.y += arrowSize * ySign;

		retRect.SetRect( rectX1 - arrowSize, rectY1, rectX2 + arrowSize, rectY2 );
	}

	CPen linePen( PS_SOLID, ViewSkin::edgeLineSize(), colour );
	CPen * oldPen = dc.SelectObject( &linePen );

	CBrush arrowBrush( colour );
	CBrush * oldBrush = dc.SelectObject( &arrowBrush );

	dc.MoveTo( pt1 );
	dc.LineTo( pt2 );

	dc.Polygon( arrowPoints, 3 );

	dc.SelectObject( oldBrush );
	dc.SelectObject( oldPen );
}


/**
 *	This static method draws an image.
 *
 *	@param dc	Device context for performing the drawing.
 *	@param bmp	The bitmap to draw.
 *	@param x	Desired X position.
 *	@param y	Desired Y position.
 *	@param w	Desired width.
 *	@param h	Desired height.
 *	@param forceTransparent	True to use the pixel at (0,0) as key colour.
 */
/*static*/
void ViewDrawUtils::drawBitmap( CDC & dc, CBitmap & bmp, int x, int y, int w, int h, bool forceTransparent )
{
	BW_GUARD;

	CDC bmpDC;
	
	bmpDC.CreateCompatibleDC( &dc );

	CBitmap * oldBmp = bmpDC.SelectObject( &bmp );

	if (forceTransparent || ViewSkin::keyColourTransparency())
	{
		COLORREF keyColour = forceTransparent ? bmpDC.GetPixel( 0, 0 ): ViewSkin::keyColour();
		for (int iy = 0; iy < h; ++iy)
		{
			for (int ix = 0; ix < w; ++ix)
			{
				COLORREF bmpPixel = bmpDC.GetPixel( ix, iy );
				if (bmpPixel != keyColour)
				{
					dc.SetPixel( ix + x, iy + y, bmpPixel );
				}
			}
		}
	}
	else
	{
		dc.BitBlt( x, y, w, h, &bmpDC, 0, 0, SRCCOPY );
	}

	bmpDC.SelectObject( oldBmp );
}


/**
 *	This static method draws rounded rectangle.
 *
 *	@param dc	Device context for performing the drawing.
 *	@param rect	The rectangle's position and size.
 *	@param brush	Brush used to paint the area of the rectangle.
 *	@param pen		Pen used to draw the border of the rectangle.
 *	@param roundSize	Radius in pixels of the round corners, 0 for straight
 *						corners.
 */
/*static*/
void ViewDrawUtils::drawRect( CDC & dc, const CRect & rect, CBrush & brush, CPen & pen, int roundSize )
{
	BW_GUARD;

	CBrush * pOldBrush = dc.SelectObject( & brush );
	CPen* pOldPen = dc.SelectObject( &pen );

	if (roundSize)
	{
		dc.RoundRect( rect, CPoint( roundSize, roundSize ) );
	}
	else
	{
		dc.Rectangle( rect );
	}

	dc.SelectObject( pOldBrush );
	dc.SelectObject( pOldPen );
}

BW_END_NAMESPACE

