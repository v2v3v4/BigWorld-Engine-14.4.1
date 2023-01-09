#ifndef CONTROLS_SEPARATOR_HPP
#define CONTROLS_SEPARATOR_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This class draws a horizontal (if width >= height) or vertical 
    // (if height > width) line.  To use this control you typically add a 
    // dummy static control in your dialog using the dialog editor, declaring
    // a Separator in your dialog class and using the usual DDX subclassing
    // mechanism.  Usually you should set the text of the separator to be 
    // empty, which makes this control hard to see in the dialog editor, so
    // set the sunken flag to make it visible.  This flag is removed in
    // PreSubclassWindow.
    //
    class CONTROLS_DLL Separator : public CStatic
    {
    public:
        Separator();

    protected:
        /*virtual*/ void PreSubclassWindow();

        afx_msg void OnPaint();

        DECLARE_MESSAGE_MAP()
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_SEPARATOR_HPP
