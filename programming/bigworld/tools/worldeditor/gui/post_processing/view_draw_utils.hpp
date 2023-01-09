#ifndef VIEW_DRAW_UTILS_HPP
#define VIEW_DRAW_UTILS_HPP

BW_BEGIN_NAMESPACE

/**
 *	This class contains a set of utility functions useful for drawing in the
 *	post-processing panel.
 */
class ViewDrawUtils
{
public:
	static void drawBoxConection( CDC & dc, const CRect & rectStart,
					const CRect & rectEnd, CRect & retRect, COLORREF colour );

	static void drawBitmap( CDC & dc, CBitmap & bmp, int x, int y, int w, int h, bool forceTransparent = false );

	static void drawRect( CDC & dc, const CRect & rect, CBrush & brush, CPen & pen, int roundSize );
};

BW_END_NAMESPACE

#endif // VIEW_DRAW_UTILS_HPP
