#include "pch.hpp"

#include "loading_dialog.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

CLoadingDialog::CLoadingDialog( const BW::wstring& fileName ) :
CDialog(CLoadingDialog::IDD),
fileName_( fileName )
{
	BW_GUARD;

	Create( IDD_LOADING );
}

CLoadingDialog::~CLoadingDialog()
{
	BW_GUARD;

	DestroyWindow();
}

void CLoadingDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_BAR, bar_);

}

BOOL CLoadingDialog::OnInitDialog() 
{
	BW_GUARD;

   CDialog::OnInitDialog();
   
   SetWindowText( fileName_.c_str() );

   return TRUE;
}

void CLoadingDialog::setRange( int num )
{
	BW_GUARD;

	bar_.SetRange( 0, num );
	bar_.SetStep( 1 );
}

void CLoadingDialog::step()
{
	BW_GUARD;

	bar_.StepIt();
}

BEGIN_MESSAGE_MAP(CLoadingDialog, CDialog)
END_MESSAGE_MAP()

BW_END_NAMESPACE

