#ifndef CONTROLS_WND_SIZER_HPP
#define CONTROLS_WND_SIZER_HPP

#include "sizer.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     *  This is a Sizer that holds a CWnd control.
     */
    class WndSizer : public Sizer
    {
    public:
        enum HorizontalFit
        {
            FIT_LEFT,
            FIT_HCENTER,
            FIT_RIGHT,
            FIT_WIDTH
        };

        enum VerticalFit
        {
            FIT_TOP,
            FIT_VCENTER,
            FIT_BOTTOM,
            FIT_HEIGHT
        };

        explicit WndSizer
        (
            CWnd            *child, 
            HorizontalFit   horizFit    = FIT_WIDTH,
            VerticalFit     vertFit     = FIT_HEIGHT,
            int             padLeft     = 0,
            int             padTop      = 0,
            int             padRight    = 0,
            int             padBottom   = 0
        );

        WndSizer
        (
            CWnd            *parent,
            uint32			id,
            HorizontalFit   horizFit    = FIT_WIDTH, 
            VerticalFit     vertFit     = FIT_HEIGHT,
            int             padLeft     = 0,
            int             padTop      = 0,
            int             padRight    = 0,
            int             padBottom   = 0
        );

        HorizontalFit horizontalFit() const;
        void horizontalFit(HorizontalFit fit);

        VerticalFit verticalFit() const;
        void verticalFit(VerticalFit fit);

        /*virtual*/ void onSize(CRect const &extents);
        /*virtual*/ CSize minimumSize() const;
        /*virtual*/ void draw(CDC *dc);

    protected:
        CWnd *window();

    private:
        CWnd                *wnd_;
        uint32        		id_;
        uint32        		minWidth_;
        uint32        		minHeight_;
        HorizontalFit       horizFit_;
        VerticalFit         vertFit_;
        int                 padLeft_;
        int                 padRight_;
        int                 padTop_;
        int                 padBottom_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_WND_SIZER_HPP
