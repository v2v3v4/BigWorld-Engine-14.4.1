#ifndef ABOUT_BOX_HPP
#define ABOUT_BOX_HPP

#include "../resource.h"       // main symbols

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class CAboutDlg : public CDialog
{
	CBitmap mBackground;
	CFont mBuildFont;
	CFont mCopyRightFont;
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
};

BW_END_NAMESPACE

#endif // ABOUT_BOX_HPP
