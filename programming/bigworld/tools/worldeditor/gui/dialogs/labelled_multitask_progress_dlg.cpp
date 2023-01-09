#include "pch.hpp"
#include "common/utilities.hpp"
#include "worldeditor/gui/dialogs/labelled_multitask_progress_dlg.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "cstdmf/debug.hpp"

#define ENTIRE_PROGRESS_HIGH_RANGE	10000

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 2 )

BW_BEGIN_NAMESPACE

// LabelledMultiTaskProgressDlg dialog

IMPLEMENT_DYNAMIC(LabelledMultiTaskProgressDlg, CDialog)
	LabelledMultiTaskProgressDlg::LabelledMultiTaskProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LabelledMultiTaskProgressDlg::IDD, pParent),
	taskCnt_(0),
	curTask_(0),
	curProgress_(0),
	curLength_(0),
	cancelled_(false),
	freezeEntireProgressMovement_(false)
{
	BW_GUARD;

	MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
	if ( mainFrame )
	{
		mainFrame->EnableWindow(FALSE);
	}
	WorldManager::instance().beginModalOperation();
	Create(LabelledMultiTaskProgressDlg::IDD, pParent);
}

LabelledMultiTaskProgressDlg::~LabelledMultiTaskProgressDlg()
{
	BW_GUARD;

	this->playFinishingAnimation();

	DestroyWindow();
	MainFrame* mainFrame = (MainFrame *)WorldEditorApp::instance().mainWnd();
	if ( mainFrame )
	{
		mainFrame->EnableWindow(TRUE);
		mainFrame->BringWindowToTop();
	}
	WorldManager::instance().endModalOperation();
}


void LabelledMultiTaskProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ENTIRE_PROGRESS, entireProgress_);
	DDX_Control(pDX, IDC_CURRENT_PROGRESS, currentProgress_);
	DDX_Control(pDX, IDC_CURRENT_MESSAGE, labelledMessage_);
	DDX_Control(pDX, IDC_CANCEL, btnCancel_);
}

BEGIN_MESSAGE_MAP(LabelledMultiTaskProgressDlg, CDialog)
	ON_BN_CLICKED(IDC_CANCEL, &LabelledMultiTaskProgressDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// LabelledMultiTaskProgressDlg message handlers

BOOL LabelledMultiTaskProgressDlg::OnInitDialog()
{
	BW_GUARD;

	if (!CDialog::OnInitDialog())
		return FALSE;

	INIT_AUTO_TOOLTIP();

	UpdateData( FALSE );

	entireProgress_.SetRange( 0, ENTIRE_PROGRESS_HIGH_RANGE );
	EnableMenuItem( ::GetSystemMenu(m_hWnd, FALSE), SC_CLOSE,
		MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );

	return TRUE;  // return TRUE unless you set the focus to a control
}

void LabelledMultiTaskProgressDlg::playFinishingAnimation()
{
	if( !cancelled_ && curTask_ < taskCnt_ )
	{
		for ( int i = curTask_ + 1; i <= taskCnt_; i++ )
		{
			this->setCurrentTask( i );
			BW::string str = "";
			bw_wtoutf8( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG_PROGRESS_BAR/FINISHING"), str );
			this->setCurrentTaskInfo( str, 2, false );
			for ( int j = 0; j < 2; j++)
			{
				this->step();
				Sleep( 20 );
			}
		}
	}
	this->fillProgressBar( entireProgress_ );
}

void LabelledMultiTaskProgressDlg::recalcProgress()
{
	BW_GUARD;
	if ( !freezeEntireProgressMovement_ )
	{
		int entireLength = (int)(taskCnt_ * curLength_);
		int entireProgress = (int)(curTask_ * curLength_ + curProgress_);
		entireProgress_.SetPos( ENTIRE_PROGRESS_HIGH_RANGE * entireProgress / entireLength );
	}
	currentProgress_.SetRange( 0, (int)curLength_ );
	currentProgress_.SetPos( cancelled_ ? 0 : (int)curProgress_ );
	//currentProgress_.SetStep( 1 );
	
	bool isEscPressed = false;
	Utilities::processMessagesForTask( &isEscPressed );
	if ( isEscPressed )
	{
		cancelled_ = true;
	}

	int low, high;
	int test1 = (int) curLength_;
	int test2 = (int) curProgress_;
	currentProgress_.GetRange( low, high );

	//make sure a FULL progressbar can be seen
	if ( !cancelled_ && curLength_ == curProgress_ )
	{
		this->fillProgressBar( currentProgress_ );
	}
	if ( taskCnt_ == curTask_ )
	{
		this->fillProgressBar( entireProgress_ );
	}
}

void LabelledMultiTaskProgressDlg::fillProgressBar( CProgressCtrl& progressBar )
{
	//ProgressBar in Windows 7 is crappy, have to do this to show a full progressbar on screen
	progressBar.SetRange( 0, 1 );
	progressBar.SetPos( 1 );
}

void LabelledMultiTaskProgressDlg::setTaskCnt( int taskCnt )
{
	BW_GUARD;
	taskCnt_ = taskCnt;
}

void LabelledMultiTaskProgressDlg::setCurrentLength( float length )
{
	BW_GUARD;
	curLength_ = length;
}

void LabelledMultiTaskProgressDlg::setCurrentTaskInfo( const BW::string& sMsg, float length, bool escapable )
{
	BW_GUARD;
	//todo msg and escapable;
	if ( length == 0)
	{
		length = 1;
	}
	curLength_ = length;
	if (curProgress_ != 0)
	{
		//entire progress should never go back
		freezeEntireProgressMovement_ = true;
	}
	curProgress_ = 0;

	if ( cancelled_ == true )
	{
		labelledMessage_.SetWindowTextW( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG_PROGRESS_BAR/CANCELLING" ));
	}
	else
	{
		labelledMessage_.SetWindowTextW( bw_utf8tow( sMsg ).c_str() );
	}

	btnCancel_.EnableWindow( (!cancelled_ && escapable) ? TRUE : FALSE );
	this->recalcProgress();
}

void LabelledMultiTaskProgressDlg::setCurrentTask( int task )
{
	BW_GUARD;
	if ( curTask_ != task && task <= taskCnt_ )
	{
		curTask_ = task;
		curProgress_ = 0;
		freezeEntireProgressMovement_ = false;
		entireProgress_.SetPos( ENTIRE_PROGRESS_HIGH_RANGE * curTask_ / taskCnt_ );
	}
}

bool LabelledMultiTaskProgressDlg::step( float progress )
{
	BW_GUARD;
	if ( cancelled_ )
	{
		return false;
	}
	curProgress_ += progress;
	
	this->recalcProgress();

	return true;
}

bool LabelledMultiTaskProgressDlg::isCancelled()
{
	BW_GUARD;
	return cancelled_;
}

bool LabelledMultiTaskProgressDlg::set( float done/* = 0.f*/ )
{
	BW_GUARD;
	if ( cancelled_ )
	{
		return false;
	}
	curProgress_ = done;
	this->recalcProgress();
	return true;
}

void LabelledMultiTaskProgressDlg::OnBnClickedCancel()
{
	BW_GUARD;
	cancelled_ = true;
	labelledMessage_.SetWindowTextW( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG_PROGRESS_BAR/CANCELLING" ));
	btnCancel_.EnableWindow( FALSE );
}

BW_END_NAMESPACE

