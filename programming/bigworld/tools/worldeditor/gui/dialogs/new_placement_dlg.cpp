#include "pch.hpp"
#include "worldeditor/gui/dialogs/new_placement_dlg.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNAMIC(NewPlacementDlg, CDialog)

NewPlacementDlg::NewPlacementDlg(CWnd* pParent)
	: CDialog(NewPlacementDlg::IDD, pParent)
	, newName_(_T(""))
{
}

NewPlacementDlg::~NewPlacementDlg()
{}

void NewPlacementDlg::existingNames(BW::vector<BW::wstring> const &names)
{
	existingNames_ = names;
}

BOOL NewPlacementDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;
}

void NewPlacementDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NEWPLACEMENTTEXT, newName_);
	DDV_MaxChars(pDX, newName_, 40);
	DDX_Control(pDX, IDC_NEWPLACEMENTTEXT, newNameCtrl_);
}

// Messages

BEGIN_MESSAGE_MAP(NewPlacementDlg, CDialog)
END_MESSAGE_MAP()

void NewPlacementDlg::OnOK()
{
	BW_GUARD;

	CString str;
	newNameCtrl_.GetWindowText( str );
	if (!isValidName(str))
	{
		MessageBox( Localise(L"WORLDEDITOR/GUI/NEW_PLACEMENT_DLG/BAD_PRESET_NAME_TEXT"),
			Localise(L"WORLDEDITOR/GUI/NEW_PLACEMENT_DLG/BAD_PRESET_NAME_TITLE"), MB_ICONERROR );
		newNameCtrl_.SetFocus();
		return;
	}

	CDialog::OnOK();
}


bool NewPlacementDlg::isValidName(CString const &str) const
{
	BW_GUARD;

	CString s = str;
	s.Trim();
	// Empty names are not allowed:
	if (s.IsEmpty())
		return false;
	// A whole bunch of special characters are not allowed:
	if (s.FindOneOf(L"<>/\\:!@#$%^&*()") != -1)
		return false;
	// Names cannot match existing names, unless name is the same as newName_
	// (which occurs when the user does a rename but changes nothing):
	CString newName = newName_;
	for (size_t i = 0; i < existingNames_.size(); ++i)
	{
		if 
		(
			existingNames_[i] == s.GetBuffer() 
			&& 
			existingNames_[i] != newName.GetBuffer()
		)
		{
			return false;
		}
	}
	// Must be ok
	return true;
}

BW_END_NAMESPACE

