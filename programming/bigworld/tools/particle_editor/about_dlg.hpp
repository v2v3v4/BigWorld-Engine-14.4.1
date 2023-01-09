#ifndef ABOUT_DLG_HPP
#define ABOUT_DLG_HPP

#include "resource.h"

BW_BEGIN_NAMESPACE

class CAboutDlg : public CDialog
{
public:
    enum { IDD = IDD_ABOUTBOX };

    CAboutDlg();

protected:
    /*virtual*/ BOOL OnInitDialog();

    afx_msg void OnPaint();

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

    DECLARE_MESSAGE_MAP()

private:
    CBitmap         m_background;
    CFont           m_buildFont;
	CFont           m_copyrightFont;
};

BW_END_NAMESPACE
#endif // ABOUT_DLG_HPP
