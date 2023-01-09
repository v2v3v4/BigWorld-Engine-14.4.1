#include "pch.hpp"
#include "worldeditor/gui/dialogs/splash_dialog.hpp"
#include "worldeditor/gui/dialogs/wait_dialog.hpp"

BW_BEGIN_NAMESPACE

const int WAITDLG_DESTROY_TIMER_ID = 1971; // Arbitary choice
const int WAITDLG_RESTORE_TIMER_ID = 1972; // Arbitary choice


// WaitDlg dialog

WaitDlg* WaitDlg::s_dlg_ = NULL;
BW::wstring WaitDlg::lastString_ = L"";


IMPLEMENT_DYNAMIC(WaitDlg, CDialog)
WaitDlg::WaitDlg(CWnd* pParent /*=NULL*/)
	: CDialog(WaitDlg::IDD, pParent), bShow_(false)
{
}

WaitDlg::~WaitDlg()
{
}

BEGIN_MESSAGE_MAP(WaitDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void WaitDlg::setSplashVisible( bool visible )
{
	BW_GUARD;

	if ( visible )
		this->ShowWindow( SW_SHOW );
	else
		this->ShowWindow( SW_HIDE );
}

/*static*/ bool WaitDlg::isValid()
{
	if ( s_dlg_ )
		return true;
	else
		return false;
}

ISplashVisibilityControl* WaitDlg::getSVC()
{
	BW_GUARD;

	if ( !s_dlg_ )
	{
		s_dlg_ = new WaitDlg;
		if ( !s_dlg_->Create( WaitDlg::IDD, AfxGetMainWnd() ) )
		{
			bw_safe_delete(s_dlg_);
			return 0;
		}

		CRect rect;
		if ( CSplashDlg::visibleInfo( &rect ) )
		{
			const CWnd* alwaysOnTop = (!IsDebuggerPresent()) ? &CWnd::wndTop : 0;

			s_dlg_->SetWindowPos(
				alwaysOnTop,
				rect.left, rect.bottom,
				rect.Width(), 25,
				0 );
		}
		else
			s_dlg_->CenterWindow();
	}

	return s_dlg_;
}

void WaitDlg::show( const BW::string& info )
{
	BW_GUARD;

	AfxGetMainWnd()->UpdateWindow();
	getSVC();
	s_dlg_->KillTimer( WAITDLG_DESTROY_TIMER_ID );
	s_dlg_->KillTimer( WAITDLG_RESTORE_TIMER_ID );

	if (info == "")
		s_dlg_->GetDlgItem( IDC_WAITDLG_INFO )->SetWindowText( lastString_.c_str() );
	else
		s_dlg_->GetDlgItem( IDC_WAITDLG_INFO )->SetWindowText( bw_utf8tow( info ).c_str() );

	s_dlg_->ShowWindow( SW_SHOW );
	s_dlg_->UpdateWindow();
	s_dlg_->bShow_ = true;
}

void WaitDlg::hide( int delay )
{
	BW_GUARD;

	if( !s_dlg_ )
		return;

	if ( delay )
		s_dlg_->SetTimer( WAITDLG_DESTROY_TIMER_ID, delay, NULL );
	else
	{
		s_dlg_->KillTimer( WAITDLG_DESTROY_TIMER_ID );
		s_dlg_->KillTimer( WAITDLG_RESTORE_TIMER_ID );

		s_dlg_->DestroyWindow();
		AfxGetMainWnd()->UpdateWindow();
		bw_safe_delete(s_dlg_);
	}
}

void WaitDlg::overwriteTemp( const BW::string& info, int delay )
{
	BW_GUARD;

	if (s_dlg_ && s_dlg_->bShow_)
	{
		s_dlg_->KillTimer( WAITDLG_RESTORE_TIMER_ID );
		s_dlg_->SetTimer( WAITDLG_RESTORE_TIMER_ID, delay, NULL );
	}
	else
	{
		if (!s_dlg_)
		{
			getSVC();
			s_dlg_->show( info );
		}

		s_dlg_->KillTimer( WAITDLG_DESTROY_TIMER_ID );
		s_dlg_->SetTimer( WAITDLG_DESTROY_TIMER_ID, delay, NULL );
	}

	wchar_t lastC_String[256];
	s_dlg_->GetDlgItem( IDC_WAITDLG_INFO )->GetWindowText( lastC_String, ARRAY_SIZE( lastC_String ));
	lastString_ = lastC_String;
	s_dlg_->GetDlgItem( IDC_WAITDLG_INFO )->SetWindowText( bw_utf8tow( info ).c_str() );
	s_dlg_->ShowWindow( SW_SHOW );
	s_dlg_->UpdateWindow();
}

void WaitDlg::OnTimer(UINT_PTR nIDEvent)
{
	BW_GUARD;

	if (nIDEvent == WAITDLG_RESTORE_TIMER_ID)
	{
		s_dlg_->KillTimer( WAITDLG_RESTORE_TIMER_ID );
		s_dlg_->GetDlgItem( IDC_WAITDLG_INFO )->SetWindowText( lastString_.c_str() );
		s_dlg_->ShowWindow( SW_SHOW );
		s_dlg_->UpdateWindow();
	}
	else
	{
		hide( 0 );
	}
}
BW_END_NAMESPACE

