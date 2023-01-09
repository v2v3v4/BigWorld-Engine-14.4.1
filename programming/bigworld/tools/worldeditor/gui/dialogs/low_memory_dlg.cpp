#include "pch.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "low_memory_dlg.hpp"

BW_BEGIN_NAMESPACE

// LowMemoryDlg dialog

IMPLEMENT_DYNAMIC(LowMemoryDlg, CDialog)

LowMemoryDlg::LowMemoryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LowMemoryDlg::IDD, pParent)
	, scopedShowCursor_( true )
{
}

LowMemoryDlg::~LowMemoryDlg()
{
}

void LowMemoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, mOK);
}


BEGIN_MESSAGE_MAP(LowMemoryDlg, CDialog)
	ON_BN_CLICKED(IDC_CONTINUE, &LowMemoryDlg::OnBnClickedContinue)
	ON_BN_CLICKED(IDC_SAVE, &LowMemoryDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDOK, &LowMemoryDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// LowMemoryDlg message handlers

void LowMemoryDlg::OnBnClickedContinue()
{
	BW_GUARD;

	mOK.EnableWindow( TRUE );
}

void LowMemoryDlg::OnBnClickedSave()
{
	BW_GUARD;

	mOK.EnableWindow( TRUE );
}

void LowMemoryDlg::OnBnClickedOk()
{
	BW_GUARD;

	CButton* button = (CButton*)GetDlgItem( IDC_SAVE );
	EndDialog( button->GetCheck() ? IDC_SAVE : IDC_CONTINUE );
}

void LowMemoryDlg::OnCancel()
{}

BW_END_NAMESPACE

