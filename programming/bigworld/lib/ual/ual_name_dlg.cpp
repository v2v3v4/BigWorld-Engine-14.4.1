
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "ual_resource.h"
#include "ual_name_dlg.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param pParent	Parent window, if any.
 */
UalNameDlg::UalNameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(UalNameDlg::IDD, pParent)
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
UalNameDlg::~UalNameDlg()
{
	BW_GUARD;
}


/**
 *	This MFC method initialises the controls from the dialog's resource, and
 *	also validates and assignsthe short and long texts to the member variables.
 *
 *	@param pDX	MFC data exchange struct.
 */
void UalNameDlg::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	DDX_Text(pDX, IDC_UALNAMELONG, longName_);
	DDV_MaxChars(pDX, longName_, 80);
	DDX_Text(pDX, IDC_UALNAMESHORT, shortName_);
	DDV_MaxChars(pDX, shortName_, 20);
	CDialog::DoDataExchange(pDX);
}


/**
 *	This method returns the current short and long names.
 *
 *	@param shortName	Return param, returns the short name.
 *	@param longName		Return param, returns the long name.
 */
void UalNameDlg::getNames( BW::wstring& shortName, BW::wstring& longName )
{
	BW_GUARD;

	shortName = shortName_;
	longName = longName_;
}


/**
 *	This method sets the short (tab) and long (panel) names.
 *
 *	@param shortName	The short name.
 *	@param longName		The long name.
 */
void UalNameDlg::setNames( const BW::wstring& shortName, const BW::wstring& longName )
{
	BW_GUARD;

	shortName_ = shortName.c_str();
	longName_ = longName.c_str();
}


// MFC message map
BEGIN_MESSAGE_MAP(UalNameDlg, CDialog)
END_MESSAGE_MAP()


/**
 *	This MFC method validates the short and long names and in case they are not
 *	OK, it warns the user and keeps the dialog open.
 *
 *	@param shortName	The short name.
 *	@param longName		The long name.
 */
void UalNameDlg::OnOK()
{
	BW_GUARD;

	UpdateData( TRUE );
	if ( longName_.Trim().IsEmpty() || shortName_.Trim().IsEmpty() )
	{
		MessageBox
		( 
			Localise(L"UAL/UAL_NAME_DLG/TYPE_BOTH_TEXT"),
			Localise(L"UAL/UAL_NAME_DLG/TYPE_BOTH_TITLE"),
			MB_ICONERROR 
		);
		return;
	}
	CDialog::OnOK();
}

BW_END_NAMESPACE

