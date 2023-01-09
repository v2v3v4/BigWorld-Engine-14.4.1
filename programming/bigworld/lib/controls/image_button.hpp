#ifndef CONTROLS_IMAGE_BUTTON_HPP
#define CONTROLS_IMAGE_BUTTON_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This class is allows placing bitmaps onto a button.
    //
    class CONTROLS_DLL ImageButton : public CButton
    {
    public:
        ImageButton();
        ~ImageButton();

        void 
        setBitmapID
        (
            UINT                defID, 
            UINT                disabledID, 
            COLORREF            clrMask     = RGB(255, 255, 255)
        );

		void toggleButton(bool enable);
		bool isToggleButton() const;

		void toggle(bool toggled);
		bool isToggled() const;		

        /*virtual*/ void DrawItem(LPDRAWITEMSTRUCT drawItemStruct);

    protected:
        afx_msg void PreSubclassWindow();

        afx_msg BOOL OnEraseBkgnd(CDC *dc);

		afx_msg void OnLButtonDown(UINT flags, CPoint point);

        void
        transparentBlit
        (
            CDC                 *dc,
            int                 left,
            int                 top,
            int                 width,
            int                 height,
            HBITMAP             bitmap,
            COLORREF            clrMask
        ) const;

        DECLARE_MESSAGE_MAP()

    private:
        UINT                    defID_;
        UINT                    disabledID_;
        COLORREF                clrMask_;
		bool					isToggleButton_;
		bool					toggled_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_IMAGE_BUTTON_HPP
