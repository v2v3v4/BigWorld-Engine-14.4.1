#pragma warning ( disable: 4503 )
#pragma warning ( disable: 4786 )


#include "expsets.hpp"
#include <string>

// max includes
#include "max.h"

// exporter resources
#include "resource.h"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

//the instance
extern HINSTANCE hInstance;


ExportSettings::ExportSettings()
: staticFrame_( 0 ),
  frameFirst_( 0 ),
  frameLast_( 100 ),
  nodeFilter_( VISIBLE ),
  allowScale_( false ),
  exportNodeAnimation_( true ),
  exportCueTrack_( false ),
  useLegacyOrientation_( false )
{
}

ExportSettings::~ExportSettings()
{
}

bool ExportSettings::displayDialog( HWND hWnd )
{
	if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MFEXPORT_DLG), hWnd, dialogProc, (LPARAM)this)) 
	{
		return false;
	}
	return true;
}

void ExportSettings::displayReferenceHierarchyFile( HWND hWnd ) const
{
	BW::string displayName("");
	if (!this->referenceNodesFile_.empty())
	{
		displayName = "( " + this->referenceNodesFile_ + " )";
	}
#ifdef _UNICODE
	SetDlgItemText( hWnd, IDC_REFERENCEHIERARCHYFILE,
		bw_utf8tow( this->referenceNodesFile_ ).c_str() );
	SetDlgItemText( hWnd, IDC_REFERENCEHIERARCHY_FILENAME, bw_utf8tow( displayName ).c_str() );
#else
	SetDlgItemText( hWnd, IDC_REFERENCEHIERARCHYFILE,
		this->referenceNodesFile_.c_str() );
	SetDlgItemText( hWnd, IDC_REFERENCEHIERARCHY_FILENAME, displayName.c_str() );
#endif
	
}

INT_PTR CALLBACK ExportSettings::dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
//	ISpinnerControl  *spin;

	ExportSettings *settings = (ExportSettings*)GetWindowLongPtr(hWnd,GWLP_USERDATA); 

	switch (msg)
	{
	case WM_INITDIALOG:
		settings = (ExportSettings*)lParam;
		SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam); 
		CenterWindow(hWnd, GetParent(hWnd)); 

		CheckDlgButton( hWnd, IDC_ALLOWSCALE, settings->allowScale_ );
		CheckDlgButton( hWnd, IDC_EXPORTNODEANIMATION, settings->exportNodeAnimation_ );

		CheckDlgButton( hWnd, IDC_REFERENCEHIERARCHY,
            settings->referenceNodesFile_ != "");
		settings->displayReferenceHierarchyFile( hWnd );

		CheckDlgButton( hWnd, IDC_CUETRACK, settings->exportCueTrack_ );
		CheckDlgButton( hWnd, IDC_LEGACY_ORIENTATION, settings->useLegacyOrientation_ );
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDC_REFERENCEHIERARCHY:
			if (IsDlgButtonChecked(hWnd, IDC_REFERENCEHIERARCHY) != TRUE)
			{
				settings->referenceNodesFile_ = "";
			}
			else
			{
				settings->referenceNodesFile_ = settings->getReferenceFile( hWnd );
			}

			CheckDlgButton( hWnd, IDC_REFERENCEHIERARCHY,
				settings->referenceNodesFile_ != "");

			settings->displayReferenceHierarchyFile( hWnd );

			break;

		case IDOK:
			settings->allowScale_ = IsDlgButtonChecked( hWnd, IDC_ALLOWSCALE ) == TRUE;
			settings->exportNodeAnimation_ = IsDlgButtonChecked( hWnd, IDC_EXPORTNODEANIMATION ) == TRUE;

			// store whether we should export cue tracks or not
			settings->exportCueTrack_ = IsDlgButtonChecked( hWnd, IDC_CUETRACK ) == BST_CHECKED;

			settings->useLegacyOrientation_ = IsDlgButtonChecked( hWnd, IDC_LEGACY_ORIENTATION ) == TRUE;

			EndDialog(hWnd, 1);
			break;

		case IDSETTINGS:
			DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SETTINGSDIALOG), hWnd, settingsDialogProc, (LPARAM)settings);
			break;

		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK ExportSettings::settingsDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ExportSettings *settings = (ExportSettings*)GetWindowLongPtr(hWnd,GWLP_USERDATA); 

	BW::string typeDesc;

	switch (msg)
	{
	case WM_INITDIALOG:
		{
		settings = (ExportSettings*)lParam;
		SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam); 
		CenterWindow(hWnd, GetParent(hWnd)); 		
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			{
			EndDialog(hWnd, 1);
			}
			break;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

BW::string ExportSettings::getReferenceFile( HWND hWnd )
{
	OPENFILENAME	ofn;
	memset( &ofn, 0, sizeof(ofn) );

#ifdef _UNICODE
	WCHAR * filters = L"Visual Files\0*.visual\0\0";
	WCHAR	filename[512] = L"";
	WCHAR*	sTitle = L"Select reference hierarchy file";
	WCHAR*	ext = L"MFX";
#else
	char * filters = "Visual Files\0*.visual\0\0";
	char	filename[512] = "";
	char*	sTitle = "Select reference hierarchy file";
	char*	ext = "MFX";
#endif
	
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filters;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = sTitle;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR | OFN_READONLY | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = ext;


	if (!GetOpenFileName( &ofn )) return "";

#ifdef _UNICODE
	return bw_wtoutf8( filename );
#else
	return filename;
#endif
}

ExportSettings& ExportSettings::instance()
{
	static ExportSettings expsets;
	return expsets;
}


std::ostream& operator<<(std::ostream& o, const ExportSettings& t)
{
	o << "ExportSettings\n";
	return o;
}

float ExportSettings::unitScale( ) const
{
	return (float)GetMasterScale( UNITS_CENTIMETERS ) / 100;
}

BW_END_NAMESPACE

