#pragma warning ( disable: 4503 )
#pragma warning ( disable: 4786 )


#include "expsets.hpp"
#include <string>

// max includes
#include "max.h"

// exporter resources
#include "resource.h"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE

//the instance
extern HINSTANCE hInstance;

ExportSettings::ExportSettings()
: 
  staticFrame_( 0 ),
  exportMode_( NORMAL ),
  nodeFilter_( SELECTED ),
  allowScale_( false ),
  bumpMapped_( false ),
  fixCylindrical_( false ),
  useLegacyOrientation_( false ),
  keepExistingMaterials_( true ),
  boneCount_( 17 ),
  visualCheckerEnable_( true ),
  snapVertices_(false)
{
}

ExportSettings::~ExportSettings()
{
}

bool ExportSettings::displayDialog( HWND hWnd )
{
	if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_VISUALEXPORT_DLG), hWnd, dialogProc, (LPARAM)this)) 
	{
		return false;
	}
	return true;
}

INT_PTR CALLBACK ExportSettings::dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ExportSettings *settings = (ExportSettings*)GetWindowLongPtr(hWnd,GWLP_USERDATA); 

	switch (msg)
	{
	case WM_INITDIALOG:
		{
			settings = (ExportSettings*)lParam;
			SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
			CenterWindow(hWnd, GetParent(hWnd)); 

			switch( settings->exportMode_ )
			{
			default:
			case NORMAL:
				CheckRadioButton( hWnd, IDC_EXPORT_NORMAL, IDC_EXPORT_MESH_PARTICLES, IDC_EXPORT_NORMAL );
				break;
			case STATIC:
				CheckRadioButton( hWnd, IDC_EXPORT_NORMAL, IDC_EXPORT_MESH_PARTICLES, IDC_EXPORT_STATIC );
				break;
			case STATIC_WITH_NODES:
				CheckRadioButton( hWnd, IDC_EXPORT_NORMAL, IDC_EXPORT_MESH_PARTICLES, IDC_EXPORT_STATIC_WITH_NODES );
				break;
			case MESH_PARTICLES:
				CheckRadioButton( hWnd, IDC_EXPORT_NORMAL, IDC_EXPORT_MESH_PARTICLES, IDC_EXPORT_MESH_PARTICLES );
				break;
			}

			CheckDlgButton( hWnd, IDC_ALLOWSCALE, settings->allowScale_ );
			CheckDlgButton( hWnd, IDC_BUMPMAP, settings->bumpMapped_ );
			CheckDlgButton( hWnd, IDC_KEEP_EXISTING_MATERIALS, settings->keepExistingMaterials_ );
			CheckDlgButton( hWnd, IDC_SNAP_VERTICES, settings->snapVertices_ );

#ifdef UNICODE
			BW::wstring typeDesc = L"Checking against: " +  bw_acptow( settings->visualTypeIdentifier_ );
#else
			BW::string typeDesc = "Checking against: " +  settings->visualTypeIdentifier_;
#endif // UNICODE
		
			SendDlgItemMessage( hWnd, IDC_VISUAL_TYPE_ID, WM_SETTEXT, 0, (LPARAM) typeDesc.c_str() );
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			settings->allowScale_ = IsDlgButtonChecked( hWnd, IDC_ALLOWSCALE ) == TRUE;
			settings->bumpMapped_ = IsDlgButtonChecked( hWnd, IDC_BUMPMAP ) == TRUE;
			settings->keepExistingMaterials_ = IsDlgButtonChecked( hWnd, IDC_KEEP_EXISTING_MATERIALS ) == TRUE;
			settings->snapVertices_ = IsDlgButtonChecked( hWnd, IDC_SNAP_VERTICES ) == TRUE;

			if (IsDlgButtonChecked(hWnd, IDC_EXPORT_NORMAL))
			{
				settings->exportMode_ = NORMAL;
			}
			else if (IsDlgButtonChecked(hWnd, IDC_EXPORT_STATIC))
			{
				settings->exportMode_ = STATIC;
			}
			else if (IsDlgButtonChecked(hWnd, IDC_EXPORT_STATIC_WITH_NODES))
			{
				settings->exportMode_ = STATIC_WITH_NODES;
			}
			else if (IsDlgButtonChecked(hWnd, IDC_EXPORT_MESH_PARTICLES))
			{
				settings->exportMode_ = MESH_PARTICLES;				
			}			

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

		ISpinnerControl  *spin;
		spin = GetISpinner(GetDlgItem(hWnd, IDC_BONE_STEP_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_BONE_STEP), EDITTYPE_INT ); 
		spin->SetLimits(3, 85, TRUE); 
		spin->SetScale(1.0f);
		spin->SetValue(settings->boneCount() ,FALSE);
		ReleaseISpinner(spin);

		CheckDlgButton( hWnd, IDC_VISUALCHECKER, !settings->visualCheckerEnable() );

		CheckDlgButton( hWnd, IDC_CYLINDRICAL, settings->fixCylindrical() );

		CheckDlgButton( hWnd, IDC_LEGACY_ORIENTATION, settings->useLegacyOrientation() );
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) 
		{
		case IDOK:
			{
			ISpinnerControl  *spin;
			spin = GetISpinner(GetDlgItem(hWnd, IDC_BONE_STEP_SPIN)); 
			settings->boneCount( spin->GetIVal() );
			ReleaseISpinner(spin);

			settings->visualCheckerEnable( IsDlgButtonChecked( hWnd, IDC_VISUALCHECKER ) != TRUE );

			settings->fixCylindrical( IsDlgButtonChecked( hWnd, IDC_CYLINDRICAL ) == TRUE );

			settings->useLegacyOrientation( IsDlgButtonChecked( hWnd, IDC_LEGACY_ORIENTATION ) == TRUE );

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
	WCHAR * filters = L"MFX Files\0*.MFX\0\0";
	WCHAR	filename[512] = L"";
	WCHAR * title = L"Select reference hierarchy file";
	WCHAR * defExt = L"MFX";
#else
	char * filters = "MFX Files\0*.MFX\0\0";
	char   filename[512] = "";
	char * title = "Select reference hierarchy file";
	char * defExt = "MFX";
#endif
	
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filters;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = ARRAY_SIZE(filename);
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR | OFN_READONLY | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = defExt;


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

