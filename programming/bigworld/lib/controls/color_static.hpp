#ifndef CONTROLS_COLORSTATIC_HPP
#define CONTROLS_COLORSTATIC_HPP


#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This is a CStatic class where the colour of the text and the
    // background can be set.
    //
    class CONTROLS_DLL ColorStatic : public CStatic
    {
    public:
        ColorStatic();
        ~ColorStatic();

        void SetTextColour(COLORREF colour);
        COLORREF GetTextColour() const;

        void SetBkColour(COLORREF colour);
        COLORREF GetBkColour() const;

        static COLORREF TransparentBackground();

    protected:
        //
        // Windows callback.  Does the actual setting of colours.
        //
        // @param dc            The dc the control will be drawn onto.
        // @param cltColour     The control's colour (not used).
        // @returns             The background brush.
        //
        afx_msg HBRUSH CtlColor(CDC *dc, UINT ctlColour);

        DECLARE_MESSAGE_MAP()

    protected:
        CBrush              m_backBrush;
        COLORREF            m_textColour;
        COLORREF            m_backColour;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_COLORSTATIC_HPP
