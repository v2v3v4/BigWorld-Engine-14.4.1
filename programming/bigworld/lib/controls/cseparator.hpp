#ifndef CONTROLS_CSEPARATOR_HPP
#define CONTROLS_CSEPARATOR_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

namespace controls
{
    //
    // This class draws a horizontal (if width >= height) or vertical 
    // (if height > width) line.  To use this control you typically add a 
    // dummy static control in your dialog using the dialog editor, declaring
    // a CSeparator in your dialog class and using the usual DDX subclassing
    // mechanism.  Usually you should set the text of the separator to be 
    // empty, which makes this control hard to see in the dialog editor, so
    // set the sunken flag to make it visible.  This flag is removed in
    // PreSubclassWindow.
    //
    class CONTROLS_DLL CSeparator : public CStatic
    {
    public:
        CSeparator();

    protected:
        /*virtual*/ void PreSubclassWindow();

        afx_msg void OnPaint();

        DECLARE_MESSAGE_MAP()
    };
}

#endif // CONTROLS_CSEPARATOR_HPP
