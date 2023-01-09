#ifndef CRITICAL_MESSAGE_BOX_HPP
#define CRITICAL_MESSAGE_BOX_HPP

#include "bw_string.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

// This class is used to displayed the critical message,
// it shouldn't be used anywhere else. MsgBox should be used
// as the common message dialog instead.
class CriticalMsgBox
{
public:
	CriticalMsgBox( const char* info, bool sendInfoBackToBW );
	~CriticalMsgBox();

	// return true if we should enter debugger
	bool doModal();

	static const int INFO_BUFFER_SIZE = 16384;

private:
	static CriticalMsgBox* instance_;
	static BW::map<HWND, WNDPROC> itemsMap_;

	static INT_PTR CALLBACK staticDialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l );
	static INT_PTR CALLBACK itemProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l );

	HWND statusWindow_;
	HWND enterDebugger_;
	HWND exit_;

	wchar_t info_[ INFO_BUFFER_SIZE ];
	bool sendInfoBackToBW_;

	void create( HWND hwnd );
	void kill( HWND hwnd );
	INT_PTR CALLBACK dialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l );
	void copyContent() const;
	void finishSending( HWND hwnd, const wchar_t* status );

	HFONT font_;
};

BW_END_NAMESPACE

#endif //CRITICAL_MESSAGE_BOX_HPP
