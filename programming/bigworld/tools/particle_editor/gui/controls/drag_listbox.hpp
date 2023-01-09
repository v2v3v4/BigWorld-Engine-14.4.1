#ifndef DRAGLISTBOX_HPP
#define DRAGLISTBOX_HPP

BW_BEGIN_NAMESPACE

/**
 *  This is a CListBox that drags selected items as text and has a callback
 *  mechanism for tooltips.
 */
class DragListBox : public CListBox
{
public:
    typedef void (*ToolTipCB)
    (
        unsigned int    item, 
        BW::string		&tooltip,
        void            *data
    );

    DragListBox();

    void setTooltipCallback(ToolTipCB cb, void *data = NULL);

protected:
    /*virtual*/ void PreSubclassWindow();

    /*virtual*/ INT_PTR OnToolHitTest(CPoint point, TOOLINFO *ti) const;

    BOOL OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *result);

    afx_msg void OnLButtonDown(UINT flags, CPoint point);

    afx_msg void OnMouseMove(UINT flags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    UINT                        dragIndex_;
    ToolTipCB                   tooltipCB_;
    void                        *data_;
};

BW_END_NAMESPACE

#endif // DRAGLISTBOX_HPP
