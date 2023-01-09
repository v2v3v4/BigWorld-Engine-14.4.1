#ifndef CSEARCH_FILTER_HPP
#define CSEARCH_FILTER_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    //
    // This control is like CEdit, but has a search icon on the left and a
    // clear button image on the right.
    //
    class CSearchFilter : public CWnd
    {
    public:
        CSearchFilter();

        ~CSearchFilter();

        BOOL
        Create
        (
            DWORD           style,
            RECT            const &extents, 
            CWnd            *parent, 
            UINT            id,
            UINT            sbmpID,
            UINT            cbmpID
        );

        BOOL isClearVisible() const;

        BOOL isEmptyTextVisible() const;

        void setEmptyText(wchar_t const *text);

        wchar_t const *getEmptyText() const;

        unsigned int editID() const;

		void clearFilter();

    protected:
        /*virtual*/ BOOL PreTranslateMessage(MSG *msg);
            
        afx_msg void OnSize(UINT type, int cx, int cy);

        afx_msg void OnPaint();

        afx_msg BOOL OnEraseBkgnd(CDC *dc);

        afx_msg BOOL OnSetCursor(CWnd *wnd, UINT hitTest, UINT message);

        afx_msg void OnEditText();

        afx_msg void OnLButtonDown(UINT flags, CPoint point);

        afx_msg LRESULT OnSetWindowText(WPARAM wparam, LPARAM lparam);

        afx_msg LRESULT OnGetWindowText(WPARAM wparam, LPARAM lparam);

        afx_msg LRESULT OnGetTextLength(WPARAM wparam, LPARAM lparam);

        afx_msg void OnSetFocus(CWnd *oldWnd);

        afx_msg void OnKillFocus(CWnd *newWnd);        

        DECLARE_MESSAGE_MAP()

        void 
        getRects
        (
            CRect           const &client,
            CRect           &searchRect,
            CRect           &editRect,
            CRect           &clearRect,
            bool            clearVis
        ) const;

        void updatePositions();

    private:
        friend class CSearchFilterEdit;

        CStatic             searchImg_;
        CSearchFilterEdit   *edit_;
        CStatic             clearImg_;
        HBITMAP             searchBmp_;
        HBITMAP             clearBmp_;
        CToolTipCtrl        toolTip_;
        size_t              filterChanges_;
        BW::wstring        emptyText_;
        bool                showEmptyText_;
    };
}

BW_END_NAMESPACE

#endif // CSEARCH_FILTER_HPP
