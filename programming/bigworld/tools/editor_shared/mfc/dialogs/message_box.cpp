#include <afxwin.h>
#include "../../dialogs/message_box.hpp"

BW_BEGIN_NAMESPACE

int MessageBox( void * parent, const wchar_t * text, const wchar_t * caption,
			   MessageBoxFlags flags )
{
	UINT mfcFlags = 
		( flags & BW_MB_OK ? MB_OK : 0 ) |
		( flags & BW_MB_ICONWARNING ? MB_ICONWARNING : 0 );
	return ::MessageBox(
		parent == NULL ? AfxGetApp()->m_pMainWnd->GetSafeHwnd() : (HWND) parent,
		text,
		caption,
		mfcFlags );
}

BW_END_NAMESPACE