#include "about_dialog.hpp"
#include "cstdmf/bwversion.hpp"
#include "resmgr/string_provider.hpp"
#include "cstdmf/bw_stack_container.hpp"

BW_BEGIN_NAMESPACE

LRESULT AboutDialog::OnInitDialog(
	UINT message, WPARAM wParam, LPARAM lParam, BOOL& handled )
{
	CWindow textbox = GetDlgItem( IDC_ABOUT_VERSION );
	Formatter formatter( BWVersion::versionString() );
	DECLARE_WSTACKSTR( versionText, 20 );
	versionText = L"Version ";
	versionText.append( formatter.str() );
	textbox.SetWindowText( versionText.c_str() );
	handled = TRUE;
	return 1;
}

LRESULT AboutDialog::OnClose(
	WORD notifyCode, WORD id, HWND controlHandle, BOOL& handled )
{
	handled = TRUE;
	EndDialog( IDOK );
	return 1;
}

BW_END_NAMESPACE
