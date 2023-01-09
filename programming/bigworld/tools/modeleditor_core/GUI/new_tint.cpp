#include "pch.hpp"

#include <algorithm>

#include "resmgr/auto_config.hpp"
#include "resmgr/string_provider.hpp"
#include "appmgr/options.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"

#include "mru.hpp"
#include "../resource.h"
#include "new_tint.hpp"

BW_BEGIN_NAMESPACE

static AutoConfigString s_new_tint_default_Fx( "system/defaultShaderPath" );
static AutoConfigString s_new_tint_default_Mfm( "system/defaultMfmPath" );


CNewTint::CNewTint( BW::vector< BW::string >& tintNames ):
	CDialog( CNewTint::IDD ),
	tintNames_(tintNames),
	fxFile_(""),
	mfmFile_("")
{}

void CNewTint::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	CRect listRect;
	
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_NEW_TINT_NAME, name_);

	DDX_Control(pDX, IDC_NEW_TINT_FX_CHECK, fxCheck_ );

	DDX_Control(pDX, IDC_NEW_TINT_FX_LIST, fxList_ );

	fxList_.GetWindowRect(listRect);
    ScreenToClient (&listRect);
	listRect.bottom += 256;
	fxList_.MoveWindow(listRect);
	fxList_.SelectString(-1, L"");

	DDX_Control(pDX, IDC_NEW_TINT_FX_SEL, fxSel_ );

	DDX_Control(pDX, IDC_NEW_TINT_MFM_CHECK, mfmCheck_ );
	
	DDX_Control(pDX, IDC_NEW_TINT_MFM_LIST, mfmList_ );

	mfmList_.GetWindowRect(listRect);
    ScreenToClient (&listRect);
	listRect.bottom += 256;
	mfmList_.MoveWindow(listRect);
	mfmList_.SelectString(-1, L"");

	DDX_Control(pDX, IDC_NEW_TINT_MFM_SEL, mfmSel_ );

	DDX_Control(pDX, IDOK, ok_);
}

BOOL CNewTint::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	redrawList( fxList_, "fx" );
	redrawList( mfmList_, "mfm" );

	if (Options::getOptionInt( "settings/lastNewTintFX", 1 ))
	{
		fxCheck_.SetCheck( BST_CHECKED );
		OnBnClickedNewTintFxCheck();
	}
	else
	{
		mfmCheck_.SetCheck( BST_CHECKED );
		OnBnClickedNewTintMfmCheck();
	}

	ok_.ModifyStyle( 0, WS_DISABLED );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CNewTint, CDialog)
	ON_EN_CHANGE(IDC_NEW_TINT_NAME, OnEnChangeNewTintName)
	ON_BN_CLICKED(IDC_NEW_TINT_MFM_CHECK, OnBnClickedNewTintMfmCheck)
	ON_BN_CLICKED(IDC_NEW_TINT_FX_CHECK, OnBnClickedNewTintFxCheck)
	ON_CBN_SELCHANGE(IDC_NEW_TINT_FX_LIST, OnCbnSelchangeNewTintFxList)
	ON_CBN_SELCHANGE(IDC_NEW_TINT_MFM_LIST, OnCbnSelchangeNewTintMfmList)
	ON_BN_CLICKED(IDC_NEW_TINT_FX_SEL, OnBnClickedNewTintFxSel)
	ON_BN_CLICKED(IDC_NEW_TINT_MFM_SEL, OnBnClickedNewTintMfmSel)
END_MESSAGE_MAP()

void CNewTint::checkComplete()
{
	BW_GUARD;

	if (tintName_.length() && (fxFile_.length() || mfmFile_.length()))
	{
		ok_.ModifyStyle( WS_DISABLED, 0 );
	}
	else
	{
		ok_.ModifyStyle( 0, WS_DISABLED );
	}

	ok_.RedrawWindow();
}

void CNewTint::OnEnChangeNewTintName()
{
	BW_GUARD;

	CString tintName_cstr;
	name_.GetWindowText( tintName_cstr );
 	tintName_ = bw_wtoutf8( tintName_cstr.GetString() );

	BW::string::size_type first = tintName_.find_first_not_of(" ");
	BW::string::size_type last = tintName_.find_last_not_of(" ") + 1;
	if (first != BW::string::npos)
	{
		tintName_ = tintName_.substr( first, last-first );
	}
	else
	{
		tintName_ = "";
	}
	
	checkComplete();
}

void CNewTint::OnBnClickedNewTintFxCheck()
{
	BW_GUARD;

	fxList_.ModifyStyle( WS_DISABLED, 0 );
	fxSel_.ModifyStyle( WS_DISABLED, 0 );
	fxSel_.RedrawWindow();

	mfmList_.ModifyStyle( 0, WS_DISABLED );
	mfmSel_.ModifyStyle( 0, WS_DISABLED );
	mfmSel_.RedrawWindow();

	Options::setOptionInt( "settings/lastNewTintFX", 1 );

	BW::vector<BW::string> fx;
	MRU::instance().read( "fx", fx );
	if (fx.size() != 0)
		fxFile_ = fx[0];
	else
		fxFile_ = "";
	mfmFile_ = "";

	checkComplete();
}

void CNewTint::OnBnClickedNewTintMfmCheck()
{
	BW_GUARD;

	fxList_.ModifyStyle( 0, WS_DISABLED );
	fxSel_.ModifyStyle( 0, WS_DISABLED );
	fxSel_.RedrawWindow();

	mfmList_.ModifyStyle( WS_DISABLED, 0 );
	mfmSel_.ModifyStyle( WS_DISABLED, 0 );
	mfmSel_.RedrawWindow();

	Options::setOptionInt( "settings/lastNewTintFX", 0 );

	BW::vector<BW::string> mfm;
	MRU::instance().read( "mfm", mfm );
	if (mfm.size() != 0)
		mfmFile_ = mfm[0];
	else
		mfmFile_ = "";
	fxFile_ = "";

	checkComplete();
}

void CNewTint::redrawList( CComboBox& list, const BW::string& name )
{
	BW_GUARD;

	BW::vector<BW::string> data;
	MRU::instance().read( name, data );
	list.ResetContent();
	for (unsigned i=0; i<data.size(); i++)
	{
		BW::string::size_type first = data[i].rfind("/") + 1;
		BW::string::size_type last = data[i].rfind(".");
		BW::string dataName = data[i].substr( first, last-first );
		list.InsertString( i, bw_utf8tow( dataName ).c_str() );
	}
	size_t index = data.size();
	MF_ASSERT( index <= INT_MAX );
	list.InsertString( ( int ) index, Localise(L"MODELEDITOR/OTHER") );
	list.SetCurSel( data.size() ? 0 : -1 );

	checkComplete();
}

void CNewTint::OnCbnSelchangeNewTintFxList()
{
	BW_GUARD;

	if (fxList_.GetCurSel() == fxList_.GetCount()-1)
	{
		redrawList( fxList_, "fx" );
		OnBnClickedNewTintFxSel();
		return;
	}
	
	BW::vector<BW::string> fx;
	MRU::instance().read( "fx", fx );
	fxFile_ = fx[ fxList_.GetCurSel() ];
	MRU::instance().update( "fx", fxFile_, true );

	redrawList( fxList_, "fx" );
}

void CNewTint::OnCbnSelchangeNewTintMfmList()
{
	BW_GUARD;

	if (mfmList_.GetCurSel() == mfmList_.GetCount()-1)
	{
		redrawList( mfmList_, "mfm" );
		OnBnClickedNewTintMfmSel();
		return;
	}
	
	BW::vector<BW::string> mfm;
	MRU::instance().read( "mfm", mfm );
	mfmFile_ = mfm[ mfmList_.GetCurSel() ];
	MRU::instance().update( "mfm", mfmFile_, true );

	redrawList( mfmList_, "mfm" );
}

void CNewTint::OnBnClickedNewTintFxSel()
{
	BW_GUARD;

	static wchar_t BASED_CODE szFilter[] =	L"Effect (*.fx)|*.fx||";
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		( BWFileDialog::FD_FILEMUSTEXIST | BWFileDialog::FD_HIDEREADONLY );
	BWFileDialog fileDlg ( true, L"", L"", flags, szFilter );

	BW::string fxDir;
    MRU::instance().getDir("fx", fxDir, s_new_tint_default_Fx );
	BW::wstring wfxDir = bw_utf8tow( fxDir );
	fileDlg.initialDir( wfxDir.c_str() );

	if ( fileDlg.showDialog() )
	{
		fxFile_ = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ) );

		if (BWResource::validPath( fxFile_ ))
		{
			MRU::instance().update( "fx", fxFile_, true );

			redrawList( fxList_, "fx" );
		}
		else
		{
			MessageBox( Localise(L"MODELEDITOR/GUI/NEW_TINT/BAD_DIR_EFFECT"),
				Localise(L"MODELEDITOR/GUI/NEW_TINT/UNABLE_RESOLVE_EFFECT"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

void CNewTint::OnBnClickedNewTintMfmSel()
{
	BW_GUARD;

	static wchar_t BASED_CODE szFilter[] =	L"MFM (*.mfm)|*.mfm||";
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		( BWFileDialog::FD_FILEMUSTEXIST | BWFileDialog::FD_HIDEREADONLY );
	BWFileDialog fileDlg ( true, L"", L"", flags, szFilter);

	BW::string mfmDir;
    MRU::instance().getDir("mfm", mfmDir, s_new_tint_default_Mfm );
	BW::wstring wmfmDir = bw_utf8tow( mfmDir );
	fileDlg.initialDir( wmfmDir.c_str() );

	if ( fileDlg.showDialog() )
	{
		mfmFile_ = BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ));

		if (BWResource::validPath( mfmFile_ ))
		{
			MRU::instance().update( "mfm", mfmFile_, true );

			redrawList( mfmList_, "mfm" );
		}
		else
		{
			MessageBox( Localise(L"MODELEDITOR/GUI/NEW_TINT/BAD_DIR_MFM"),
				Localise(L"MODELEDITOR/GUI/NEW_TINT/UNABLE_RESOLVE_MFM"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

void CNewTint::OnOK()
{
	BW_GUARD;

	BW::string::size_type first = tintName_.find_first_not_of(" ");
	BW::string::size_type last = tintName_.find_last_not_of(" ") + 1;
	if (first != BW::string::npos)
	{
		tintName_ = tintName_.substr( first, last-first );
	}
	else
	{
		MessageBox( Localise(L"MODELEDITOR/GUI/NEW_TINT/BAD_TINT_NAME"),
			Localise(L"MODELEDITOR/GUI/NEW_TINT/INVALID_TINT_NAME"), 
			MB_OK | MB_ICONERROR );

		tintName_ = "";
		name_.SetWindowText( L"" );
		checkComplete();
		name_.SetFocus();

		return;
	}
	
	name_.SetWindowText( bw_utf8tow( tintName_ ).c_str() );
	name_.SetSel( 0, -1 );
		
	if ( std::find( tintNames_.begin(), tintNames_.end(), tintName_ ) != tintNames_.end() )
	{
		MessageBox( Localise(L"MODELEDITOR/GUI/NEW_TINT/TINT_ALREADY_EXISTS"),
			Localise(L"MODELEDITOR/GUI/NEW_TINT/TINT_EXISTS"), 
			MB_OK | MB_ICONERROR );
			
		name_.SetFocus();

		return;
	}

	if (tintName_ == "Default")
	{
		MessageBox( Localise(L"MODELEDITOR/GUI/NEW_TINT/DEFAULT_TINT_RESERVED"), 
			Localise(L"MODELEDITOR/GUI/NEW_TINT/DEFAULT_TINT"), 
			MB_OK | MB_ICONERROR );
			
		name_.SetFocus();

		return;
	}

	CDialog::OnOK();
}


BW_END_NAMESPACE

