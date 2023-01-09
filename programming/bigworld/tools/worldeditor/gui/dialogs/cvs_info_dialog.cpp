#include "pch.hpp"
#include "worldeditor/gui/dialogs/cvs_info_dialog.hpp"

BW_BEGIN_NAMESPACE

CVSInfoDialog::CVSInfoDialog( const BW::wstring& title ) :
CDialog(CVSInfoDialog::IDD),
title_( title )
{
	BW_GUARD;

	Create( IDD_MODELESS_INFO );
	//CenterWindow();
	//RedrawWindow();
}

CVSInfoDialog::~CVSInfoDialog()
{
	BW_GUARD;

	if( IsWindow( m_hWnd ) )
	{
		CWnd* okButton = GetDlgItem( IDC_OK );
		if( okButton )
			okButton->EnableWindow( TRUE );

		MSG msg;
		while( GetMessage( &msg, NULL, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			if( !IsWindow( m_hWnd ) )
				break;
		}
	}
}

void CVSInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CVSInfoDialog::OnInitDialog() 
{
	BW_GUARD;

	CDialog::OnInitDialog();
   
	SetWindowText(title_.c_str());

	CWnd* okButton = GetDlgItem( IDC_OK );
	if( okButton )
		okButton->EnableWindow( FALSE );

	CMenu* mnu = this->GetSystemMenu(FALSE);
	mnu->ModifyMenu(SC_CLOSE,MF_BYCOMMAND | MF_GRAYED );

	return TRUE;
}

void CVSInfoDialog::add( const BW::wstring& msg )
{
	BW_GUARD;

	CRichEditCtrl* infoText = (CRichEditCtrl*)GetDlgItem( IDC_INFO );

	if (infoText)
	{
		infoText->SetSel( -1, -1 );
		infoText->ReplaceSel( msg.c_str() );
		infoText->Invalidate();
		infoText->UpdateWindow();
	}
}

BEGIN_MESSAGE_MAP(CVSInfoDialog, CDialog)
	ON_BN_CLICKED(IDC_OK, &CVSInfoDialog::OnBnClickedOk)
END_MESSAGE_MAP()

void CVSInfoDialog::OnBnClickedOk()
{
	BW_GUARD;

	DestroyWindow();
}
BW_END_NAMESPACE

