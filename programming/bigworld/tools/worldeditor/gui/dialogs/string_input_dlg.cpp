// StringInputDlg.cpp : implementation file
//

#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "resource.h"
#include "string_input_dlg.hpp"

BW_BEGIN_NAMESPACE

// StringInputDlg dialog

/**
 *	Constructor
 */
StringInputDlg::StringInputDlg( CWnd * pParent /*=NULL*/ ) :
	CDialog( StringInputDlg::IDD, pParent ),
	maxLen_( 80 )
{
}


/**
 *	Destructor
 */
StringInputDlg::~StringInputDlg()
{
}


/**
 *	This method must be called before "DoModal" to initialise all the dialog's
 *	variables.
 */
void StringInputDlg::init( const BW::wstring & caption,
			const BW::wstring & label, int maxLen, const BW::string & str )
{
	BW_GUARD;

	caption_ = caption.c_str();
	label_ = label.c_str();
	str_ = bw_utf8tow( str ).c_str();
	maxLen_ = maxLen;
}


/**
 *	This method is called from the MFC framework, and sets the custom caption
 *	and label set in the "init" method.
 */
BOOL StringInputDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	this->SetWindowText( caption_ );
	this->GetDlgItem( IDC_STRING_INPUT_LABEL )->SetWindowText( label_ );
	this->GetDlgItem( IDC_STRING_INPUT_EDIT )->SetFocus();

	return FALSE;
}


/**
 *	This method is called from the MFC framework to validate the data.
 */
void StringInputDlg::DoDataExchange( CDataExchange* pDX )
{
	DDX_Text( pDX, IDC_STRING_INPUT_EDIT, str_ );
	DDV_MaxChars( pDX, str_, maxLen_ );
	CDialog::DoDataExchange( pDX );
}


/**
 *	MFC message map
 */
BEGIN_MESSAGE_MAP( StringInputDlg, CDialog )
END_MESSAGE_MAP()

BW_END_NAMESPACE

// StringInputDlg message handlers
