#ifndef MESSAGE_BOX_HPP
#define MESSAGE_BOX_HPP

#include "bw_string.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class MsgBox
{
public:
	typedef bool (*TickFunction)( MsgBox* msgBox, bool initDialog );

	CSTDMF_DLL MsgBox( const BW::wstring& caption, const BW::wstring& text,
		const BW::wstring& buttonText1 = L"",
		const BW::wstring& buttonText2 = L"",
		const BW::wstring& buttonText3 = L"",
		const BW::wstring& buttonText4 = L"" );
	CSTDMF_DLL MsgBox( const BW::wstring& caption, const BW::wstring& text,
		const BW::vector<BW::wstring>& buttonTexts );
	CSTDMF_DLL ~MsgBox();

	CSTDMF_DLL void status( const BW::wstring& status );
	CSTDMF_DLL void disableButtons();
	CSTDMF_DLL void enableButtons();

	CSTDMF_DLL unsigned int doModal( HWND parent = NULL, bool topmost = false,
		unsigned int timeOutTicks = INFINITE,
		TickFunction tickFunction = NULL );
	CSTDMF_DLL void doModalless( HWND parent = NULL, unsigned int timeOutTicks = INFINITE,
		TickFunction tickFunction = NULL );
	CSTDMF_DLL unsigned int getResult() const;
	CSTDMF_DLL bool stillActive() const;

	CSTDMF_DLL static HWND getDefaultParent()
	{
		return defaultParent_;
	}

	CSTDMF_DLL static void setDefaultParent( HWND hwnd )
	{
		defaultParent_ = hwnd;
	}

	static const unsigned int TIME_OUT = INFINITE;

private:
	static BW::map<HWND, MsgBox*> wndMap_;
	static BW::map<const MsgBox*, HWND> msgMap_;
	static BW::map<HWND, WNDPROC> buttonMap_;
	static HWND defaultParent_;

	static INT_PTR CALLBACK staticDialogProc( HWND hwnd, UINT msg,
		WPARAM w, LPARAM l );
	static INT_PTR CALLBACK buttonProc( HWND hwnd, UINT msg,
		WPARAM w, LPARAM l );

	BW::wstring caption_;
	BW::wstring text_;
	BW::vector<BW::wstring> buttons_;
	unsigned int result_;
	bool model_;
	bool topmost_;
	unsigned int timeOut_;

	BW::wstring status_;
	HWND statusWindow_;
	HWND savedFocus_;

	void create( HWND hwnd );
	void kill( HWND hwnd );
	INT_PTR CALLBACK dialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l );
	void copyContent() const;

	HFONT font_;
	TickFunction tickFunction_;
};

BW_END_NAMESPACE

#endif //MESSAGE_BOX_HPP
