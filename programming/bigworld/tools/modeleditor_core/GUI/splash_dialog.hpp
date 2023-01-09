#pragma once

#include "../Resource.h"
#include "tools/common/resource_loader.hpp"

BW_BEGIN_NAMESPACE

// CSplashDlg dialog

class CSplashDlg : public CDialog, public ISplashVisibilityControl
{
	DECLARE_DYNAMIC(CSplashDlg)

public:
	CSplashDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplashDlg();

// Dialog Data
	enum { IDD = IDD_SPLASH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	static ISplashVisibilityControl* getSVC();
	static void ShowSplashScreen(CWnd* pParentWnd);
	static BOOL PreTranslateAppMessage(MSG* pMsg);

	void HideSplashScreen(void);
	void setSplashVisible( bool visible );
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnStnClickedSplash();
};

extern CSplashDlg* s_SplashDlg;
BW_END_NAMESPACE

