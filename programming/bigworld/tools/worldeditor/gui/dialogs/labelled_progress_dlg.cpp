#include "pch.hpp"
#include "common/utilities.hpp"
#include "worldeditor/gui/dialogs/labelled_progress_dlg.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/framework/world_editor_app.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

// LabelledProgress dialog

IMPLEMENT_DYNAMIC(LabelledProgressDlg, CDialog)
LabelledProgressDlg::LabelledProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LabelledProgressDlg::IDD, pParent)
	, cancelled_( false )
	, fractionComplete_( 0.f )
{
	BW_GUARD;

	MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
	if ( mainFrame )
	{
		mainFrame->EnableWindow(FALSE);
	}
	WorldManager::instance().beginModalOperation();
	Create(LabelledProgressDlg::IDD, pParent);
}

LabelledProgressDlg::~LabelledProgressDlg()
{
	BW_GUARD;

	this->fullProgressBar( progress_ );
	DestroyWindow();

	MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
	if ( mainFrame )
	{
		mainFrame->EnableWindow(TRUE);
		mainFrame->BringWindowToTop();
	}
	WorldManager::instance().endModalOperation();
}

void LabelledProgressDlg::fullProgressBar( CProgressCtrl& progressbar )
{
	//Progress bar in Windows 7 is crappy, have to do this to show a full progress bar on screen
	progressbar.SetRange( 0, 1 );
	progressbar.SetPos( 1 );
}

void LabelledProgressDlg::checkCancel()
{
	BW_GUARD;
	bool isEscPressed = false;
	Utilities::processMessagesForTask( &isEscPressed );
	if ( isEscPressed )
	{
		cancelled_ = true;
	}
}

void LabelledProgressDlg::setStage( const BW::string& stage, const float fraction, const int stepCount, bool cancellable )
{
	BW_GUARD;
	if (cancelled_)
		return;

	float totalSteps = stepCount / fraction;
	progress_.SetRange32( 0, (int)totalSteps );
	progress_.SetPos( (int)(fractionComplete_ * totalSteps) );
	progress_.SetStep( 1 );
	fractionComplete_ += fraction;

	stage_.SetWindowTextW( bw_utf8tow( stage ).c_str() );
	buttonCancel_.EnableWindow( cancellable ? TRUE : FALSE );
}

void LabelledProgressDlg::length( int len )
{
	progress_.SetRange32( 0, len );
}

void LabelledProgressDlg::SetPos( int pos )
{
	progress_.SetPos( pos );
	this->checkCancel();
}

bool LabelledProgressDlg::step( int progress )
{
	BW_GUARD;
	if (cancelled_)
	{
		return false;
	}

	progress_.SetStep( progress );
	progress_.StepIt();
	progress_.SetStep( 1 );

	this->checkCancel();
	
	return true;
}

void LabelledProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STAGE, stage_ );
	DDX_Control(pDX, IDC_PROGRESS1, progress_ );
	DDX_Control(pDX, IDCANCEL, buttonCancel_);
}

BEGIN_MESSAGE_MAP(LabelledProgressDlg, CDialog)
END_MESSAGE_MAP()


// LabelledProgress message handlers

BOOL LabelledProgressDlg::OnInitDialog()
{
	BW_GUARD;

	if (!CDialog::OnInitDialog())
		return FALSE;

	INIT_AUTO_TOOLTIP();
	
	UpdateData( FALSE );

	EnableMenuItem( ::GetSystemMenu(m_hWnd, FALSE), SC_CLOSE,
		 MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );

	return TRUE;  // return TRUE unless you set the focus to a control
}

void LabelledProgressDlg::PostNcDestroy()
{
	BW_GUARD;

	cancelled_ = true;
}

void LabelledProgressDlg::OnCancel()
{
	BW_GUARD;

	
	cancelled_ = true;
	stage_.SetWindowTextW( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG_PROGRESS_BAR/CANCELLING" ));
	buttonCancel_.EnableWindow( FALSE );
	
	return;
}

void LabelledProgressDlg::OnOK()
{
	return;
}

BW_END_NAMESPACE

