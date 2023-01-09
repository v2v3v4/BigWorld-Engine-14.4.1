#ifndef ABOUT_DIALOG_HPP
#define ABOUT_DIALOG_HPP

#include "wtl.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class AboutDialog : 
	public ATL::CDialogImpl<AboutDialog>
{
public:
	enum { IDD = IDR_ABOUT_DIALOG };

	BEGIN_MSG_MAP( AboutDialog )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_HANDLER( IDOK, BN_CLICKED, OnClose )
		COMMAND_HANDLER( IDCANCEL, BN_CLICKED, OnClose )
	END_MSG_MAP()

	LRESULT OnInitDialog(
		UINT message, WPARAM wParam, LPARAM lParam, BOOL& handled );
	LRESULT OnClose(
		WORD notifyCode, WORD id, HWND controlHandle, BOOL& handled );
};

BW_END_NAMESPACE

#endif // ABOUT_DIALOG_HPP
