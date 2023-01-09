#include "../resource.h"

#include "controls/auto_tooltip.hpp"

BW_BEGIN_NAMESPACE

class CPrefsDlg : public CDialog
{
	DECLARE_AUTO_TOOLTIP( CPrefsDlg, CDialog )

public:
	CPrefsDlg( IMainFrame * mainFrame );

	enum { IDD = IDD_PREFS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
public:
	virtual BOOL OnInitDialog();

	void OnOK();

private:
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	CButton showSplashScreen_;
	CButton loadLastModel_;
	CButton loadLastLights_;
	
	CButton zoomOnLoad_;
	CButton regenBBOnLoad_;

	CButton animateZoom_;
	CButton lockLOD_;
	CButton invertMouse_;

	IMainFrame * mainFrame_;

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedPrefsInvertMouse();
};

BW_END_NAMESPACE

