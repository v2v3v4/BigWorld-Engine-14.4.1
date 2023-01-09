#ifndef CONTROLS_DIALOG_TOOLBAR_HPP
#define CONTROLS_DIALOG_TOOLBAR_HPP


#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "controls/auto_tooltip.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     *  This is a CToolbar suitable for embedding into CDialog derived classes.
     */
    class CONTROLS_DLL DialogToolbar : public CToolBar
    {
    public:
        DialogToolbar();

        void Subclass(UINT id);

        BOOL LoadToolBarEx(UINT id, UINT disabledId = 0);

    protected:
        afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wparam, LPARAM lparam);

        DECLARE_MESSAGE_MAP()

        DECLARE_AUTO_TOOLTIP(DialogToolbar, CToolBar)

    private:
        CBitmap             disabledBmp_;
        CImageList          disabledImgList_;
    };
}

BW_END_NAMESPACE

#endif // CONTROLS_DIALOG_TOOLBAR_HPP
