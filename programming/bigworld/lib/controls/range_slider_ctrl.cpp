/**
 * Custom range selection slider
 */


#include "pch.hpp"
#include "user_messages.hpp"
#include "range_slider_ctrl.hpp"
#include <cmath>

BW_BEGIN_NAMESPACE
namespace controls
{

static const int RANGESLIDER_X_OUT_MARGIN = 4;
static const int RANGESLIDER_Y_OUT_MARGIN = 3;
static const int RANGESLIDER_LEFT_IN_MARGIN = 5;
static const int RANGESLIDER_RIGHT_IN_MARGIN = 5;
static const int RANGESLIDER_THUMB_WIDTH = 8;


RangeSliderCtrl::RangeSliderCtrl() :
	multiplier_( 1 ),
	exponent_( 1 ),
	focusThumb_( THUMB_NONE ),
	thumbDragging_( THUMB_NONE ),
	startOffset_( 0 ),
    oldPos_( 0 )
{
}

RangeSliderCtrl::~RangeSliderCtrl()
{
}

void RangeSliderCtrl::getRangeAreaRect( CRect* rect )
{
	BW_GUARD;

	if ( !rect )
		return;

	GetClientRect( rect );
	rect->DeflateRect(
		RANGESLIDER_X_OUT_MARGIN, RANGESLIDER_Y_OUT_MARGIN );
}

int RangeSliderCtrl::applyExponent( int val ) const
{
	BW_GUARD;

	if ( exponent_ == 1.0f )
		return val;

	int start = GetRangeMin();
	int w = GetRangeMax() - GetRangeMin();
	val -= start;

	// add 0.5f to round to the next integer in case it contains decimals
	return int( 0.5f + powf( (float)val * powf( (float)w, exponent_ - 1.0f ) , 1.0f/exponent_ ) + (float)start );
}

int RangeSliderCtrl::removeExponent( int val ) const
{
	BW_GUARD;

	if ( exponent_ == 1.0f )
		return val;

	int start = GetRangeMin();
	int w = GetRangeMax() - GetRangeMin();
	val -= start;

	// add 0.5f to round to the next integer in case it contains decimals
	return int( 0.5f + powf( (float)val, exponent_ ) / powf( (float)w, exponent_ - 1.0f ) + (float)start );
}

void RangeSliderCtrl::setDecimalDigits( int digits )
{
	BW_GUARD;

	if ( digits < 0 )
		return;
	// add 0.5f to round to the next integer in case it contains decimals
	float minval;
	float maxval;
	getRange( minval, maxval );
	multiplier_ = int( pow( 10.0f, float(digits) ) + 0.5f );
	setRange( minval, maxval );
}

void RangeSliderCtrl::setExponent( float exponent )
{
	exponent_ = ( exponent < 0 ? 0 : exponent );
}

void RangeSliderCtrl::setRange( float min, float max, int digits /* = -1 */ )
{
	BW_GUARD;

	setDecimalDigits( digits );
	SetRange( int( min * multiplier_ ), int( max * multiplier_ ) );
}

void RangeSliderCtrl::getRange( float& min, float& max ) const
{
	BW_GUARD;

	int minval;
	int maxval;
	GetRange( minval, maxval );
	min = float( minval ) / multiplier_;
	max = float( maxval ) / multiplier_;
}

void RangeSliderCtrl::setThumbValues( float min, float max )
{
	BW_GUARD;

	SetSelection( applyExponent( int( min * multiplier_ ) ),
		applyExponent( int( max * multiplier_ ) ) );
}

void RangeSliderCtrl::getThumbValues( float& min, float& max ) const
{
	BW_GUARD;

	int minval;
	int maxval;
	GetSelection( minval, maxval );
	min = float( removeExponent( minval ) ) / multiplier_;
	max = float( removeExponent( maxval ) ) / multiplier_;
}

void RangeSliderCtrl::changed()
{
	BW_GUARD;

	sendParentMsg( WM_RANGESLIDER_CHANGED );
}

bool RangeSliderCtrl::sliderHasFocus()
{
	return thumbDragging_ != THUMB_NONE;
}

void RangeSliderCtrl::drawThumb( CDC& dc, int val, bool focused, bool down )
{
	BW_GUARD;

	int coord = coordFromVal( val );

	CRect rect;
	getRangeAreaRect( &rect );

	rect.left += RANGESLIDER_LEFT_IN_MARGIN;
	rect.right -= RANGESLIDER_RIGHT_IN_MARGIN;

	CPoint pt[6];

	pt[0].x = coord - RANGESLIDER_THUMB_WIDTH / 2;
	pt[0].y = rect.bottom;

	pt[1].x = pt[0].x;
	pt[1].y = pt[0].y - rect.Height() / 4;

	pt[2].x = coord;
	pt[2].y = pt[1].y - RANGESLIDER_THUMB_WIDTH / 2;

	pt[3].x = coord + RANGESLIDER_THUMB_WIDTH / 2;
	pt[3].y = pt[1].y;

	pt[4].x = pt[3].x;
	pt[4].y = rect.bottom;

	pt[5] = pt[0];

	COLORREF fillColour = down?GetSysColor( COLOR_BTNSHADOW ):GetSysColor( COLOR_BTNFACE );
	CPen penFill( PS_SOLID, 1, fillColour );
	CPen penHiLight( PS_SOLID, 1, GetSysColor( COLOR_BTNHIGHLIGHT ) );
	CPen penShadow( PS_SOLID, 1, GetSysColor( COLOR_BTNSHADOW ) );
	CPen* oldPen = dc.SelectObject( &penFill );

	CBrush brush( fillColour );
	CBrush* oldBrush = dc.SelectObject( &brush );

	dc.SetPolyFillMode( WINDING );
	dc.Polygon( pt, 6 );

	dc.SelectObject( down?&penShadow:&penHiLight );
	dc.Polyline( pt, 3 );

	dc.SelectObject( down?&penHiLight:&penShadow );
	dc.Polyline( &pt[2], 4 );

	if ( focused )
	{
		int x = ( pt[0].x + pt[3].x ) / 2 - 1;
		int y = ( pt[0].y + pt[3].y ) / 2 - 1;
		dc.FillSolidRect( x, y, 3, 2,
			down?GetSysColor( COLOR_BTNTEXT ):GetSysColor( COLOR_BTNSHADOW ) );
	}

	dc.SelectObject( oldPen );
	dc.SelectObject( oldBrush );
}


int RangeSliderCtrl::coordFromVal( int val )
{
	BW_GUARD;

	CRect rect;
	getRangeAreaRect( &rect );

	rect.left += RANGESLIDER_LEFT_IN_MARGIN;
	rect.right -= RANGESLIDER_RIGHT_IN_MARGIN;

	return ( val - GetRangeMin() ) * rect.Width()
		/ ( GetRangeMax() - GetRangeMin() + 1 ) + rect.left;
}

int RangeSliderCtrl::valFromPoint( CPoint point )
{
	BW_GUARD;

	CRect rect;
	getRangeAreaRect( &rect );

	rect.left += RANGESLIDER_LEFT_IN_MARGIN;
	rect.right -= RANGESLIDER_RIGHT_IN_MARGIN;

	return ( point.x - rect.left ) *
		( GetRangeMax() - GetRangeMin() + 1 ) / rect.Width() + GetRangeMin();
}

RangeSliderCtrl::ThumbType RangeSliderCtrl::thumbHitTest( CPoint point )
{
	BW_GUARD;

	int min;
	int max;
	GetSelection( min, max );

	int xmin = coordFromVal( min );
	if ( point.x >= xmin - RANGESLIDER_THUMB_WIDTH / 2 && point.x <= xmin + RANGESLIDER_THUMB_WIDTH / 2 )
		return THUMB_MIN;

	int xmax = coordFromVal( max );
	if ( point.x >= xmax - RANGESLIDER_THUMB_WIDTH / 2 && point.x <= xmax + RANGESLIDER_THUMB_WIDTH / 2 )
		return THUMB_MAX;

    if ( xmin <= point.x && point.x <= xmax)
        return THUMB_BOTH;

	return THUMB_NONE;
}

void RangeSliderCtrl::sendParentMsg( int msg )
{
	BW_GUARD;

	CWnd* parent = GetParent();
	if ( !parent )
		return;

	parent->SendMessage(
		msg,
		0,
		(LPARAM)this );
}

BEGIN_MESSAGE_MAP(RangeSliderCtrl, CSliderCtrl)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL_REFLECT()
	ON_WM_VSCROLL_REFLECT()
END_MESSAGE_MAP()


BOOL RangeSliderCtrl::OnEraseBkgnd( CDC* dc )
{
	return TRUE; // to fool windows that we have drawn the background
}

void RangeSliderCtrl::OnPaint()
{
	BW_GUARD;

//	CSliderCtrl::OnPaint();

	Invalidate();
	CPaintDC dc( this );

	int min;
	int max;
	GetSelection( min, max );

	CRect rect;
	getRangeAreaRect( &rect );
	rect.InflateRect( 0, 1 );
	dc.Draw3dRect( &rect,
		GetSysColor( COLOR_BTNSHADOW ), GetSysColor( COLOR_BTNHIGHLIGHT ) );
	rect.DeflateRect( 1, 1 );
	if ( !IsWindowEnabled() )
	{
		dc.FillSolidRect( rect, GetSysColor( COLOR_BTNFACE ) );
	}
	else
	{
		int minCoord = coordFromVal( min );
		int maxCoord = coordFromVal( max );
		CRect rectOld = rect;
		rect.right = minCoord;
		dc.FillSolidRect( rect, GetSysColor( COLOR_WINDOW ) );
		rect.left = minCoord;
		rect.right = maxCoord;
		dc.FillSolidRect( rect, GetSysColor( COLOR_HIGHLIGHT ) );
		rect.left = maxCoord;
		rect.right = rectOld.right;
		dc.FillSolidRect( rect, GetSysColor( COLOR_WINDOW ) );
	}

	drawThumb
    ( 
        dc, 
        max, 
        focusThumb_    == THUMB_MAX || focusThumb_ == THUMB_BOTH, 
        thumbDragging_ == THUMB_MAX || focusThumb_ == THUMB_BOTH
    );
	drawThumb
    ( 
        dc, 
        min, 
        focusThumb_    == THUMB_MIN || focusThumb_ == THUMB_BOTH, 
        thumbDragging_ == THUMB_MIN || focusThumb_ == THUMB_BOTH 
    );
	ValidateRect( 0 );
}

void RangeSliderCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	CSliderCtrl::OnLButtonDown( nFlags, point );

	SetFocus();

	CRect rect;
	getRangeAreaRect( &rect );

	if ( !rect.PtInRect( point ) )
		return;

	thumbDragging_ = thumbHitTest( point );
	if ( thumbDragging_ != THUMB_NONE )
		focusThumb_ = thumbDragging_;
	SetCapture();
	RedrawWindow();
    oldPos_ = valFromPoint( point );

	sendParentMsg( WM_RANGESLIDER_CHANGED );
}

void RangeSliderCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	BW_GUARD;

	if ( GetCapture() != this )
		return;

	thumbDragging_ = THUMB_NONE;
    if (focusThumb_ == THUMB_BOTH)
        focusThumb_ = THUMB_NONE;
	ReleaseCapture();
	RedrawWindow();
	sendParentMsg( WM_RANGESLIDER_CHANGED );
}

void RangeSliderCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	BW_GUARD;

	if ( thumbDragging_ == THUMB_NONE || GetCapture() != this )
		return;

	int newPos = valFromPoint( point );

	int min;
	int max;
	GetSelection( min, max );

	if ( thumbDragging_ == THUMB_MIN )
	{
		min = newPos;
		if ( max < min )
		{
			min = max;
			max = newPos;
			thumbDragging_ = THUMB_MAX;
			focusThumb_ = thumbDragging_;
		}
	}
	else if ( thumbDragging_ == THUMB_MAX )
	{
		max = newPos;
		if ( max < min )
		{
			max = min;
			min = newPos;
			thumbDragging_ = THUMB_MIN;
			focusThumb_ = thumbDragging_;
		}
	}
    else if ( thumbDragging_ == THUMB_BOTH )
    {
        int delta = newPos - oldPos_;
        oldPos_ = newPos;
        max += delta;
        min += delta;
    }

	SetSelection( min, max );

	sendParentMsg( WM_RANGESLIDER_TRACK );
}

void RangeSliderCtrl::handleScroll(UINT nTBCode, UINT nPos, bool vertical )
{
	BW_GUARD;

	bool posChanged = false;

	if ( focusThumb_ == THUMB_MIN || focusThumb_ == THUMB_MAX )
	{
		int minv;
		int maxv;
		GetSelection( minv, maxv );
		int* thumbVal = focusThumb_ == THUMB_MIN ? &minv : &maxv;

		switch (nTBCode)
		{
		case TB_PAGEUP:
			*thumbVal -= GetPageSize();
			break;
		case TB_PAGEDOWN:
			*thumbVal += GetPageSize();
			break;
		case TB_LINEUP:
			*thumbVal -= GetLineSize();
			break;
		case TB_LINEDOWN:
			*thumbVal += GetLineSize();
			break;
		case TB_TOP:
			*thumbVal = GetRangeMin();
			break;
		case TB_BOTTOM:
			*thumbVal = GetRangeMax();
			break;
		}

		if ( focusThumb_ == THUMB_MIN )
		{
			minv = *thumbVal;
			if ( maxv < minv )
			{
				minv = maxv;
				maxv = *thumbVal;
				focusThumb_ = THUMB_MAX;
			}
		}
		else if ( focusThumb_ == THUMB_MAX )
		{
			maxv = *thumbVal;
			if ( maxv < minv )
			{
				maxv = minv;
				minv = *thumbVal;
				focusThumb_ = THUMB_MIN;
			}
		}

		minv = min( GetRangeMax(), max( GetRangeMin(), minv ) );
		maxv = min( GetRangeMax(), max( GetRangeMin(), maxv ) );

		SetSelection( minv, maxv );
		posChanged = true;
	}

	if ( posChanged )
		sendParentMsg( WM_RANGESLIDER_CHANGED );
}

void RangeSliderCtrl::HScroll(UINT nTBCode, UINT nPos)
{
	BW_GUARD;

	handleScroll(nTBCode, nPos, false);
}

void RangeSliderCtrl::VScroll(UINT nTBCode, UINT nPos)
{
	BW_GUARD;

	handleScroll(nTBCode, nPos, true);
}

void RangeSliderCtrl::SetSelection( int nMin, int nMax )
{
	BW_GUARD;

	CSliderCtrl::SetSelection( nMin - startOffset_, nMax - startOffset_ );
	RedrawWindow();
}

void RangeSliderCtrl::GetSelection( int& nMin, int& nMax ) const
{
	BW_GUARD;

	CSliderCtrl::GetSelection( nMin, nMax );
	nMin += startOffset_;
	nMax += startOffset_;
}

void RangeSliderCtrl::SetRangeMin( int nMin, BOOL bRedraw )
{
	BW_GUARD;

	startOffset_ = nMin;
	CSliderCtrl::SetRangeMin( 0, bRedraw );
}

void RangeSliderCtrl::SetRangeMax( int nMax, BOOL bRedraw )
{
	BW_GUARD;

	CSliderCtrl::SetRangeMax( nMax - startOffset_, bRedraw );
}

int RangeSliderCtrl::GetRangeMin() const
{
	BW_GUARD;

	return CSliderCtrl::GetRangeMin() + startOffset_;
}

int RangeSliderCtrl::GetRangeMax() const
{
	BW_GUARD;

	return CSliderCtrl::GetRangeMax() + startOffset_;
}

//-----------------------------------------------------------------------------
// RangeLimitSliderCtrl Section
//-----------------------------------------------------------------------------


/**
 *	Default constructor
 */
RangeLimitSliderCtrl::RangeLimitSliderCtrl() :
	minLimit_( 0 ), maxLimit_ ( 1000 )
{
}


/**
 *	Sets the minimum and maximum range of the slider.
 *
 *	@param min	The minimum range the slider will be set to.
 *	@param max	The maximum range the slider will be set to.
 */
void RangeLimitSliderCtrl::setRangeLimit( float min, float max )
{
	BW_GUARD;

	minLimit_ = int( min * multiplier_ );
	maxLimit_ = int( max * multiplier_ );

	int minval;
	int maxval;
	GetRange( minval, maxval );
	if( minval < minLimit_ || maxLimit_ < maxval )
	{
		SetRange(
			minval < minLimit_	? minLimit_	: minval,
			maxLimit_ < maxval	? maxLimit_	: maxval );
	}
}


/**
 *	Returns the slider's minimum range.
 *
 *	@return The minimum range of the slider.
 */
float RangeLimitSliderCtrl::getMinRangeLimit() const
{
	return float( minLimit_ ) / multiplier_;
}


/**
 *	Returns the slider's maximum range.
 *
 *	@return The maximum range of the slider.
 */
float RangeLimitSliderCtrl::getMaxRangeLimit() const
{
	return float( maxLimit_ ) / multiplier_;
}


/**
 *	Sets the minimum range of the slider.
 *
 *	@param nMin		The new minimum range of the slider.
 *	@param bRedraw	Flag used to force a redraw of the slider.
 */
void RangeLimitSliderCtrl::SetRangeMin( int nMin, BOOL bRedraw )
{
	BW_GUARD;

	if (nMin < minLimit_)
	{
		nMin = minLimit_;
	}

	startOffset_ = nMin;
	CSliderCtrl::SetRangeMin( 0, bRedraw );
}


/**
 *	Sets the maximum range of the slider.
 *
 *	@param nMax		The new maximum range of the slider.
 *	@param bRedraw	Flag used to force a redraw of the slider.
 */
void RangeLimitSliderCtrl::SetRangeMax( int nMax, BOOL bRedraw )
{
	BW_GUARD;

	if (nMax > maxLimit_)
	{
		nMax = maxLimit_;
	}

	CSliderCtrl::SetRangeMax( nMax - startOffset_, bRedraw );
}

} // namespace controls
BW_END_NAMESPACE

