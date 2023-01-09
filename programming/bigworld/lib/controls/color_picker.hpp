#ifndef CONTROLS_COLOR_PICKER_HPP
#define CONTROLS_COLOR_PICKER_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "math/vector4.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This control allows selection of a colour using a HLS type
    // drawing.
    //
    // The controls sends the messages listed below.  The WPARAM is the
    // control's id and the LPARAM is the controls HWND.
    //
    //  WM_CP_LBUTTONDOWN       The user clicked down on the control.  The
    //                          selected color has been changed.
    //  WM_CP_LBUTTONMOVE       The user clicked down and dragged around on the
    //                          control.  The selected color has been updated.
    //  WM_CP_LBUTTONUP         The user clicked up after clicking down on the
    //                          control.  The selected color has been updated.
    //
    class CONTROLS_DLL ColorPicker : public CWnd
    {
    public:
        ColorPicker();

        /*virtual*/ ~ColorPicker();

        BOOL 
        Create
        (
            DWORD           style, 
            CRect           pos, 
            CWnd            *parent, 
            bool            alphaSel  = true
        );
        
        void setRGB(COLORREF ref);

        COLORREF getRGB();

        void setHLS(float hue, float lum, float sat);

        void getHLS(float & hue, float & luminance, float & saturation);

        void setRGBA(Vector4 const &color);

        Vector4 getRGBA() const;

    private:
        afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
        afx_msg void OnMouseMove(UINT nFlags, CPoint point);
        afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
        afx_msg void OnPaint();
        afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
        afx_msg void OnSize(UINT nType, int cx, int cy);

        DECLARE_MESSAGE_MAP()

    private:
        void generatePicker();
        void generateHueBar();
        void generateAlphaBar();

        void setPicker(bool redraw = true);

        void drawCrossAt(CPoint &point, CDC &dc, CSize &sz);
        void calculateSizes();

        void UpdateParent(UINT message);

    private:
        enum MouseMoveMode
        {
            MMM_NONE,
            MMM_HUE,
            MMM_SL_PICKER,
            MMM_ALPHA
        };

        CDC             pickerDC_;
        CDC             hueBarDC_;
        CDC             alphaBarDC_;
        bool            createPickerDC_;
        bool            createHueBarDC_;
        bool            createAlphaBarDC_;
        bool            slGraphDirty_;      // saturation and luminance graph dirty flag
        bool            hueBarDirty_;       // hue bar dirty flag
        bool            slPickerDirty_;     // picker position dirty
        bool            alphaBarDirty_;
        MouseMoveMode   mouseMoveMode_;     // moving on picker or on hue
        CPoint          slPickerPosition_;  // position of the picker
        float           currentLuminance_;
        float           currentSaturation_;
        float           currentHue_;
        COLORREF        currentColor_;
        float           currentAlpha_;
        CRect           slGraphRect_;
        CRect           hueBarRect_;
        CRect           alphaBarRect_;
        CRect           totalRect_;
        bool            alphaSelection_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_COLOR_PICKER_HPP
