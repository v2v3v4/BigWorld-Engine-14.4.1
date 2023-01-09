#include "pch.hpp"
#include "particle_editor.hpp"
#include "splash_dialog.hpp"

BW_BEGIN_NAMESPACE

namespace
{
    const int SPASH_TIMER_ID = 1977; // Aribatry choice...
}

CSplashDlg* s_SplashDlg = 0;

IMPLEMENT_DYNAMIC(CSplashDlg, CDialog)
CSplashDlg::CSplashDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSplashDlg::IDD, pParent)
{
}

CSplashDlg::~CSplashDlg()
{
}

void CSplashDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSplashDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CSplashDlg message handlers

ISplashVisibilityControl* CSplashDlg::getSVC()
{
	return s_SplashDlg;
}


void CSplashDlg::ShowSplashScreen(CWnd* pParentWnd /*= NULL*/)
{
	BW_GUARD;

	// Allocate a new splash screen, and create the window.
		s_SplashDlg = new CSplashDlg;
	if (!s_SplashDlg->Create(CSplashDlg::IDD, pParentWnd))
			delete s_SplashDlg;
	else
	s_SplashDlg->ShowWindow(SW_SHOW);
	s_SplashDlg->UpdateWindow();

 s_SplashDlg->SetTimer( SPASH_TIMER_ID, 2000, NULL);
}

void CSplashDlg::setSplashVisible( bool visible )
{
	BW_GUARD;

	if ( visible )
		this->ShowWindow( SW_SHOW );
	else
		this->ShowWindow( SW_HIDE );
}

void CSplashDlg::HideSplashScreen()
{
	BW_GUARD;

	// Destroy the window, and update the mainframe.
	s_SplashDlg->KillTimer( SPASH_TIMER_ID );
	DestroyWindow();
	//AfxGetMainWnd()->UpdateWindow();
	delete s_SplashDlg;
	s_SplashDlg = NULL;
}

BOOL CSplashDlg::PreTranslateAppMessage(MSG* pMsg)
{
	BW_GUARD;

	if (s_SplashDlg == NULL)
		return FALSE;

	// If you receive a keyboard or mouse message, hide the splash screen.
	if (s_SplashDlg->m_hWnd != NULL && pMsg->message == WM_KEYDOWN ||
	    pMsg->message == WM_SYSKEYDOWN ||
	    pMsg->message == WM_LBUTTONDOWN ||
	    pMsg->message == WM_RBUTTONDOWN ||
	    pMsg->message == WM_MBUTTONDOWN ||
	    pMsg->message == WM_NCLBUTTONDOWN ||
	    pMsg->message == WM_NCRBUTTONDOWN ||
	    pMsg->message == WM_NCMBUTTONDOWN)
		{
			s_SplashDlg->HideSplashScreen();
			return TRUE;	// message handled here
		}

	return FALSE;	// message not handled

}

void CSplashDlg::OnTimer(UINT_PTR nIDEvent)
{
	BW_GUARD;

	// Destroy the splash screen window.
	HideSplashScreen();
}

BOOL CSplashDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	CWnd* wnd = ChildWindowFromPoint( CPoint( 20, 20 ) );
	if( wnd )
	{
		HBITMAP bmp = (HBITMAP)wnd->SendMessage( STM_GETIMAGE, IMAGE_BITMAP, 0L);
		if( bmp )
		{
			BITMAP bitmap;
			GetObject( bmp, sizeof( bitmap ), &bitmap );
			MoveWindow( 0, 0, bitmap.bmWidth, bitmap.bmHeight, FALSE );
		}
	}

	CenterWindow();

	 SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
      SWP_NOMOVE|SWP_NOSIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BW_END_NAMESPACE

