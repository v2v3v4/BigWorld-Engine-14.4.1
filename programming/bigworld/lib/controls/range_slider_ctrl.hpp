/**
 * Custom range selection slider
 */

/**
 *  NOTE: THIS IS NOT A CSliderCtrl COMPATIBLE CONTROL
 *  This control inherits from CSliderCtrl to take advantage from some of its
 *  functionality.
 */

#ifndef RANGE_SLIDER_CTRL_HPP
#define RANGE_SLIDER_CTRL_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{

    // This control sends custom messages to its parent when time it's values
    // change (WM_RANGESLIDER_CHANGED) as well as when the user is actually
    // dragging one of the thumbs (WM_RANGESLIDER_TRACK). Lo and Hi words of 
    // WPARAM contain the ranges and LPARAM contains a pointer to the control.
    // Since WPARAM can only contain two short integers, an application
    // requiring greater precision will need to retrieve the values manuall by 
    // calling GetSelection.
    //
    // You should set "Enable Selection Range" to true in the resource editor. 
    // You should also set the control to have the TBS_NOTHUMB style.
    class CONTROLS_DLL RangeSliderCtrl : public CSliderCtrl
    {
    public:
	    RangeSliderCtrl();
	    virtual ~RangeSliderCtrl();

	    virtual void setExponent( float exponent );

	    virtual void setRange( float min, float max, int digits = -1 );
	    virtual void getRange( float& min, float& max ) const;

	    virtual void setThumbValues( float min, float max );
	    virtual void getThumbValues( float& min, float& max ) const;

	    virtual void changed();

		virtual bool sliderHasFocus();

    protected:
        enum ThumbType
        {
            THUMB_MIN,
            THUMB_MAX,
            THUMB_BOTH,
            THUMB_NONE
        };

	    int multiplier_;
	    float exponent_;
	    ThumbType focusThumb_;
	    ThumbType thumbDragging_;
	    int startOffset_;
        int oldPos_;

	    virtual void setDecimalDigits( int digits );
		virtual void getRangeAreaRect( CRect* rect );
	    virtual int applyExponent( int val ) const;
	    virtual int removeExponent( int val ) const;
	    virtual int coordFromVal( int val );
	    virtual int valFromPoint( CPoint point );
	    virtual ThumbType thumbHitTest( CPoint point );
	    virtual void drawThumb( CDC& dc, int val, bool focused = false, bool down = false );
	    virtual void sendParentMsg( int msg );

	    void handleScroll(UINT nTBCode, UINT nPos, bool vertical );

	    afx_msg BOOL OnEraseBkgnd( CDC* dc );
	    afx_msg void OnPaint();
	    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	    afx_msg void HScroll(UINT nTBCode, UINT nPos);
	    afx_msg void VScroll(UINT nTBCode, UINT nPos);
	    DECLARE_MESSAGE_MAP()

	    // hidding CSliderCtrl methods that should not be available to public
	    virtual int GetPos() const { return CSliderCtrl::GetPos(); };
	    virtual void SetPos( int nPos ) { CSliderCtrl::SetPos( nPos ); };
    	
	    virtual void SetSelection( int nMin, int nMax );
	    virtual void GetSelection( int& nMin, int& nMax ) const;

	    virtual void SetRange( int nMin, int nMax, BOOL bRedraw = FALSE  )
		    { SetRangeMin( nMin, bRedraw ); SetRangeMax( nMax, bRedraw ); };
	    virtual void SetRangeMin( int nMin, BOOL bRedraw = FALSE );
	    virtual void SetRangeMax( int nMax, BOOL bRedraw = FALSE );

	    virtual void GetRange( int& nMin, int& nMax ) const
		    { nMin = GetRangeMin(); nMax = GetRangeMax(); };
	    virtual int GetRangeMin() const;
	    virtual int GetRangeMax() const;
    };


    /**
     *	This class extends RangeSliderCtrl to add limits to the sliders.
     */
    class CONTROLS_DLL RangeLimitSliderCtrl : public RangeSliderCtrl
    {
    public:
	    RangeLimitSliderCtrl();

		void setRangeLimit( float min, float max );
		float getMinRangeLimit() const;
		float getMaxRangeLimit() const;

	protected:
		void SetRangeMin( int nMin, BOOL bRedraw );
		void SetRangeMax( int nMax, BOOL bRedraw );

	private:
		int minLimit_;
		int maxLimit_;
	};
}

BW_END_NAMESPACE

#endif // RANGE_SLIDER_CTRL_HPP
