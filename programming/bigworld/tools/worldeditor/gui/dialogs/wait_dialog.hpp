#ifndef WAIT_DIALOG_HPP
#define WAIT_DIALOG_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "common/resource_loader.hpp"

BW_BEGIN_NAMESPACE

class WaitDlg : public CDialog, public ISplashVisibilityControl
{
	DECLARE_DYNAMIC(WaitDlg)

public:
// Dialog Data
	enum { IDD = IDD_WAITDLG };

	WaitDlg(CWnd* pParent = NULL);
	virtual ~WaitDlg();

	/*virtual*/ void setSplashVisible( bool visible );

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	static WaitDlg* s_dlg_;
	static BW::wstring lastString_;
	bool bShow_;

public:
	static bool isValid();
	static ISplashVisibilityControl* getSVC();
	static void show( const BW::string& info );
	static void hide( int delay = 0 );
	static void overwriteTemp( const BW::string& info, int delay );
};

BW_END_NAMESPACE

#endif // WAIT_DIALOG_HPP
