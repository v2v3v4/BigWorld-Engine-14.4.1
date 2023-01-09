#include "pch.hpp"
#include "dlg_modeless_info.hpp"


BW_BEGIN_NAMESPACE

static BW::wstring s_msg;
static BW::wstring s_title;
static HWND s_dlgWnd;
static RECT s_ownerRect;

INT_PTR CALLBACK DlgProg(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
	BW_GUARD;

	switch ( msg )
	{
	case WM_INITDIALOG:
		{
			s_dlgWnd = hwnd;
			SetDlgItemText(hwnd,IDC_INFO_MSG, s_msg.c_str());
			SetWindowText(hwnd,s_title.c_str());

			SetWindowPos(hwnd, 
				HWND_TOP, 
				s_ownerRect.left + (s_ownerRect.right / 2), 
				s_ownerRect.top + (s_ownerRect.bottom / 2), 
				0, 0,
				SWP_NOSIZE); 
			ShowWindow(hwnd, SW_SHOW);
			InvalidateRect(hwnd, NULL, TRUE );
			UpdateWindow(hwnd);
			return TRUE;
		}
	}

	return FALSE;
}

ModelessInfoDialog::ModelessInfoDialog( HWND hwnd, const BW::wstring& title, const BW::wstring& msg, bool okButton )
{
	BW_GUARD;

	s_msg = msg;
	s_title = title;
	GetClientRect( hwnd, &s_ownerRect );
	HWND dlg = CreateDialog(GetModuleHandle(NULL),
		MAKEINTRESOURCE(IDD_MODELESS_INFO),
		hwnd, DlgProg);
	if ( !okButton )
	{
		HWND okbut = GetDlgItem( dlg, IDOK );
		ShowWindow( okbut, SW_HIDE );
	}
}

ModelessInfoDialog::~ModelessInfoDialog()
{
	BW_GUARD;

	EndDialog(s_dlgWnd, 0);
}

BW_END_NAMESPACE
