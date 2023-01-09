#include "pch.hpp"
#include "sizer.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

Sizer::Sizer():
    extents_(0, 0, 0, 0)
{
}


Sizer::~Sizer()
{
}


void Sizer::onStart(CWnd *wnd)
{
	BW_GUARD;

	CRect rect;
	wnd->GetClientRect(&rect);
	onSize(rect.Width(), rect.Height());
}


CRect Sizer::extents() const
{
    return extents_;
}


void Sizer::onSize(int cx, int cy)
{
	BW_GUARD;

    CRect rect(0, 0, cx, cy);
    onSize(rect);
}


/*virtual*/ void Sizer::onSize(CRect const &e)
{
    extents_ = e;
}


void Sizer::onGetMinMaxInfo(MINMAXINFO *mmi)
{
	BW_GUARD;

    if (mmi != NULL)
    {
        CSize minSize = minimumSize();
        mmi->ptMinTrackSize.x = minSize.cx;
        mmi->ptMinTrackSize.y = minSize.cy;
    }
}


void Sizer::extents(CRect const &ext)
{
    extents_ = ext;
}


void Sizer::drawRect(CDC *dc, CRect const &rect, COLORREF clr)
{
	BW_GUARD;

    if (dc == NULL)
        return;

    CPen pen(PS_SOLID, 1, clr);
    CPen *oldPen = dc->SelectObject(&pen);
    dc->MoveTo(rect.left , rect.top   );
    dc->LineTo(rect.right, rect.top   );
    dc->LineTo(rect.right, rect.bottom);
    dc->LineTo(rect.left , rect.bottom);
    dc->LineTo(rect.left , rect.top   );
    dc->SelectObject(oldPen);
}

} // namespace controls
BW_END_NAMESPACE

