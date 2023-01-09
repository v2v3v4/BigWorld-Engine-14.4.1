
#include "aboutbox.hpp"

#ifndef CODE_INLINE
#include "aboutbox.ipp"
#endif

BW_BEGIN_NAMESPACE

AboutBox::AboutBox()
{
}

AboutBox::~AboutBox()
{
}

void AboutBox::display( HWND hWnd )
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, dialogProc, 0);
}

INT_PTR CALLBACK AboutBox::dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterWindow(hWnd, GetParent(hWnd)); 
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hWnd, 1);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}

std::ostream& operator<<(std::ostream& o, const AboutBox& t)
{
	o << "AboutBox\n";
	return o;
}

BW_END_NAMESPACE

/*aboutbox.cpp*/
