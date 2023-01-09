#ifndef SPLASH_DIALOG_HPP
#define SPLASH_DIALOG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "common/resource_loader.hpp"

BW_BEGIN_NAMESPACE

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
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void setSplashVisible( bool visible );

	static ISplashVisibilityControl* getSVC();
	static void ShowSplashScreen(CWnd* pParentWnd);
	static void HideSplashScreen(void);
	static bool visibleInfo( CRect* rect );
	static BOOL PreTranslateAppMessage(MSG* pMsg);
};

extern CSplashDlg* s_SplashDlg;

BW_END_NAMESPACE

#endif // SPLASH_DIALOG_HPP
