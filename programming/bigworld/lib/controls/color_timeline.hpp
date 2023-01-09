#ifndef CONTROLS_COLOR_TIMELINE_HPP
#define CONTROLS_COLOR_TIMELINE_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "math/vector4.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This represents a position (in normalized time) and a colour.
    //
    struct CONTROLS_DLL ColorScheduleItem
    {  
        float                   normalisedTime_;    // time (0.0 to 1.0)
        Vector4                 color_;             // colour at this time

        ColorScheduleItem();

        bool operator<(ColorScheduleItem const &other) const;

    protected:
        friend class ColorTimeline;

        int                     heightPos_;
        ColorStatic             *timeStatic_;
    };

    typedef BW::vector<ColorScheduleItem> ColorScheduleItems;

    //
    // This class allows editing of a collection of ColorScheduleItems.
    //
    // It sends the messages listed below.  Unless otherwise state the
    // WPARAM will be the control's id and the LPARAM will be the HWND of the
    // ColorTimeline.
    //
    //  WM_CT_UPDATE_BEGIN      The user has started editing the position of
    //                          a color.
    //  WM_CT_UPDATE_MIDDLE     The user has moved a color around but has not
    //                          yet finished.
    //  WM_CT_UPDATE_DONE       The user has moved a color around and has
    //                          finished.
    //  WM_CT_UPDATE_CANCEL     The user clicked to move a color but did not
    //                          actually do so.
    //  WM_CT_ADDING_COLOR      A new color is about to be added.
    //  WM_CT_ADDED_COLOR       A new color has been added.
    //  WM_CT_NEW_SELECTION     A color was selected.
    //  WM_CT_SEL_TIME          The user selected a time by clicking on the
    //                          color or alpha area (but not on a color mark).
    //                          LPARAM is the selected time*10000.
    //
    class CONTROLS_DLL ColorTimeline : public CWnd
    {
        DECLARE_DYNAMIC(ColorTimeline)

    public:
        enum TimeScale
        {
            TS_SECONDS,         // time is in seconds
            TS_HOURS_MINS       // time is in hours.minutes format
        };

        ColorTimeline();

        /*virtual*/ ~ColorTimeline();        

        BOOL 
        Create
        (
            DWORD               dwStyle, 
            CRect               rcPos, 
            CWnd                *parent, 
            ColorScheduleItems  const &colorSchedule,
            bool                showAlpha               = true,
            TimeScale           timeScale               = TS_SECONDS,
            bool                wrap                    = false
        );        

        ColorScheduleItems const &colourScheduleItems() const;

        void clearSchedule();

        void addScheduleItem(ColorScheduleItem const &item);

        void totalScheduleTime(float time);

        float totalScheduleTime() const;

        bool itemSelected() const;

        ColorScheduleItem *getColorScheduleItemSelected() const;

        void setColorScheduleItemSelected(float time);

        Vector4 getColorScheduleItemSelectedColor();

        void 
        setColorScheduleItemSelectedColor
        (
            Vector4             const &color, 
            bool                redraw      = true
        );

        bool removeSelectedColor();

        void addColorAtLButton(bool add = true);

        bool waitingToAddColor() const;

        void showLineAtTime(float time);

        void hideLineAtTime();

    protected:
        /*virtual*/ void PostNcDestroy();

        afx_msg void OnPaint();
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnMouseMove(UINT nFlags, CPoint point);        
        afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
        afx_msg BOOL OnEraseBkgnd(CDC* pDC);
        afx_msg BOOL OnSetCursor(CWnd *wnd, UINT hittest, UINT message);

        DECLARE_MESSAGE_MAP()

        void 
        drawColorMark
        (
            CDC                 &pDC, 
            ColorScheduleItem   const &item,
            CRect               const &clientRect
        );

        void UpdateParent(UINT message);

        BW::wstring timeToString(float time) const;

        void CalcColorRect(CRect &clrRect) const;

        void CalcAlphaRect(CRect &alphaRect) const;

        void 
        CalcMarkRect
        (
            ColorScheduleItem   const &item,
            CRect               const &clientRect,
            CRect               &markRect 
        ) const;

    private:
        ColorScheduleItems              colorSchedule_;
        int                             colorGradientHeight_;
        ColorScheduleItems::iterator    colorScheduleIterSelected_;
        CPoint                          mousePosition_;
        float                           unitHeightTimeIncrement_;
        float                           totalScheduleTime_;
        bool                            addColorAtLButton_;
        bool                            showAlpha_;
        TimeScale                       timeScale_;
        bool                            wrap_;
        float                           lineAtTime_;
        bool                            dragging_;
        int                             startPos_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_COLOR_TIMELINE_HPP
