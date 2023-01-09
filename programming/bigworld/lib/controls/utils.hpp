#ifndef CONTROLS_UTILS_HPP
#define CONTROLS_UTILS_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
	enum HALIGNMENT
	{
		LEFT,
		CENTRE_HORZ,
		RIGHT
	};

	enum VALIGNMENT
	{
		TOP,
		CENTRE_VERT,
		BOTTOM
	};

    void 
    replaceColour
    (
        HBITMAP             hbitmap,
        COLORREF            srcColour,
        COLORREF            dstColour
    );

    void
    replaceColour
    (
        HBITMAP             hbitmap,
        COLORREF            dstColour        
    );

    COLORREF
    getPixel
    (
        HBITMAP             hbitmap,
        unsigned int        x,
        unsigned int        y
    );

    static int NO_RESIZE = std::numeric_limits<int>::max();

    void childResize(CWnd &wnd, int left, int top, int right, int bottom);
    void childResize(CWnd &wnd, CRect const &extents);

    void childResize(CWnd &parent, UINT id, int left, int top, int right, int bottom);
    void childResize(CWnd &parent, UINT id, CRect const &extents);

    CRect childExtents(CWnd const &child);
    CRect childExtents(CWnd const &parent, UINT id);

    void enableWindow(CWnd &parent, UINT id, bool enable);
	void showWindow(CWnd &parent, UINT id, UINT show);

    bool isButtonChecked(CWnd const &parent, UINT id);
    void checkButton(CWnd &parent, UINT id, bool check);

	void setWindowText(CWnd &parent, UINT id, BW::string const &text);
	void setWindowText(CWnd &parent, UINT id, BW::wstring const &text);
    BW::string getWindowText(CWnd const &parent, UINT id);

	CPoint validateDlgPos( const CSize& size, const CPoint& pt,
		HALIGNMENT hAlignment, VALIGNMENT vAlignment );

	BW::string loadString(UINT id);
	BW::wstring loadStringW(UINT id);
}

BW_END_NAMESPACE

#endif // CONTROLS_UTILS_HPP
