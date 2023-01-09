#include "pch.hpp"
#include "critical_message_box.hpp"
#include "message_box.hpp"
#include "guard.hpp"
#include "string_utils.hpp"
#include "ftp.hpp"
#include "bwversion.hpp"

#include <mmsystem.h>
#include <strsafe.h>

#pragma comment( lib, "winmm" )

BW_BEGIN_NAMESPACE

namespace
{

#define __LONG_TEXT(t) L##t
#define LONG_TEXT(t) __LONG_TEXT(t)

const wchar_t*	CMB_DIALOG_TITLE			=	L"Critical Error";
const wchar_t*	CMB_STATIC_MESSAGE			=	L"An unrecoverable error has occurred.\r\n\r\n"
												L"Do you want to enter the debugger or exit the program?";
const wchar_t*	CMB_STATIC_PROG_INFO		=	L"Debugging information for programmers:";

const wchar_t*	CMB_BUTTON_ENTER_DEBUGGER	=	L"Enter &Debugger";
const wchar_t*	CMB_BUTTON_EXIT				=	L"E&xit";
const wchar_t*	CMB_STATUS_SENDING			=	L"Sending crash dump to BigWorld ...";
const wchar_t*	CMB_STATUS_SENT				=	L"Sending crash dump to BigWorld: done";
const wchar_t*	CMB_STATUS_FAILURE			=	L"Sending crash dump to BigWorld: failed";
const wchar_t*	CMB_STATUS_TIMEOUT			=	L"Sending crash dump to BigWorld: time out";

// you can use something like IndiFTPD to test this functionality
const wchar_t*	CMB_FTP_SERVER				=	L"localhost";
const wchar_t*	CMB_USER_NAME				=	L"anonymous";
const wchar_t*	CMB_PASSWORD				=	L"anonymous";
const wchar_t*	CMB_FOLDER					=	L"/dumps-" LONG_TEXT(BW_VERSION_MSVC_RC_STRING);

const INT_PTR CMB_STATUS_WND_ID					=	100;
const INT_PTR CMB_ENTER_DEBUGGER_WND_ID			=	1;
const INT_PTR CMB_EXIT_WND_ID					=	2;


// This class will collect the information of the current software
// and hard system at startup and store them as a string
class SysInfo
{
public:
	wchar_t sysInfoFileName_[ 128 ];
	char sysInfo_[ 65536 ];

	SysInfo()
	{
		char computername[128];
		unsigned int tickCount = timeGetTime();
		DWORD size = sizeof( computername ) / sizeof( *computername );
		SYSTEM_INFO system_info;

		GetComputerNameA( computername, &size );
		StringCchPrintf( sysInfoFileName_, sizeof( sysInfoFileName_ ), L"BW%S%d.txt", computername, tickCount );
		GetSystemInfo( &system_info );

		StringCchPrintfA( sysInfo_, sizeof( sysInfo_ ), "COMPUTERNAME = %s\nAPPLICATION = %S\r\n"
			"%d PROCESSOR(S) = %x - %x %x\r\n", computername, GetCommandLine(),
			system_info.dwNumberOfProcessors, system_info.wProcessorArchitecture,
			system_info.wProcessorLevel, system_info.wProcessorRevision );

		for( DWORD dev = 0; ; ++dev )
		{
			static DISPLAY_DEVICE device = { sizeof( device ) };

			if( EnumDisplayDevices( NULL, dev, &device, 0 ) )
			{
				StringCchPrintfA( sysInfo_ + strlen( sysInfo_ ), sizeof( sysInfo_ ) - strlen( sysInfo_ ), "DISPLAYDEVICE %d = %S, %S, %S\r\n",
					dev, device.DeviceName, device.DeviceString, device.DeviceID );
			}
			else
			{
				break;
			}
		}
	}
}
s_sysInfo;


}


CriticalMsgBox* CriticalMsgBox::instance_;
BW::map<HWND, WNDPROC> CriticalMsgBox::itemsMap_;


CriticalMsgBox::CriticalMsgBox( const char* info, bool sendInfoBackToBW )
	:font_( NULL ), statusWindow_( NULL ),
	sendInfoBackToBW_( sendInfoBackToBW )
{
	BW_GUARD;

	bw_utf8tow( info, strlen( info ), info_, INFO_BUFFER_SIZE );
	instance_ = this;
	StringCbCatA( s_sysInfo.sysInfo_, sizeof( s_sysInfo.sysInfo_ ), info );
}


CriticalMsgBox::~CriticalMsgBox()
{
	BW_GUARD;

	if( font_ )
	{
		DeleteObject( font_ );
	}

	instance_ = NULL;
}


bool CriticalMsgBox::doModal()
{
	BW_GUARD;

	DLGTEMPLATE dlg[ 2 ] = { { 0 } };
	dlg->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_VISIBLE;
	dlg->dwExtendedStyle = 0;
	dlg->cdit = 0;
	dlg->x = 0;
	dlg->y = 0;
	dlg->cx = 10;
	dlg->cy = 10;

	unsigned int result = 
		(unsigned int)
		DialogBoxIndirectParam
		( 
			GetModuleHandle( NULL ), 
			dlg, 
			NULL, 
			staticDialogProc, 
			(LPARAM)this 
		);

	return result == CMB_ENTER_DEBUGGER_WND_ID;
}


static void CriticalMessageBox_centerWindow( HWND hwnd )
{
	BW_GUARD;

	RECT parentRect;
	RECT selfRect;
	HWND parent = GetParent( hwnd );

	GetWindowRect( parent ? parent : GetDesktopWindow(), &parentRect );
	GetWindowRect( hwnd, &selfRect );

	LONG x = ( parentRect.right + parentRect.left ) / 2 - ( selfRect.right - selfRect.left ) / 2;
	LONG y = ( parentRect.bottom + parentRect.top ) / 2 - ( selfRect.bottom - selfRect.top ) / 2;

	MoveWindow( hwnd, x, y, selfRect.right - selfRect.left, selfRect.bottom - selfRect.top, TRUE );
}


INT_PTR CALLBACK CriticalMsgBox::staticDialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	BW_GUARD;

	return instance_->dialogProc( hwnd, msg, w, l );
}


INT_PTR CALLBACK CriticalMsgBox::dialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	static DWORD start = GetTickCount();
	static const DWORD TIME_OUT = 10000;

	switch( msg )
	{
	case WM_INITDIALOG:
		create( hwnd );
		CriticalMessageBox_centerWindow( hwnd );

		if (sendInfoBackToBW_)
		{
			SetTimer( hwnd, 1, 10, NULL );
		}

		break;

	case WM_TIMER:
		{
			static Ftp ftp( CMB_FTP_SERVER, CMB_USER_NAME, CMB_PASSWORD, CMB_FOLDER,
				s_sysInfo.sysInfoFileName_, s_sysInfo.sysInfo_,
				strlen( s_sysInfo.sysInfo_ ) );

			if (ftp.completed())
			{
				finishSending( hwnd, CMB_STATUS_SENT );
			}
			else if (ftp.failure())
			{
				finishSending( hwnd, CMB_STATUS_FAILURE );
			}
			else if (GetTickCount() - start > TIME_OUT)
			{
				finishSending( hwnd, CMB_STATUS_TIMEOUT );
			}
		}

		break;

	case WM_COMMAND:
		if (LOWORD( w ) == CMB_ENTER_DEBUGGER_WND_ID ||
			LOWORD( w ) == CMB_EXIT_WND_ID)
		{
			EndDialog( hwnd, LOWORD( w ) );
		}
		break;
	}

	return FALSE;
}


INT_PTR CALLBACK CriticalMsgBox::itemProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	BW_GUARD;

	INT_PTR result = CallWindowProc( itemsMap_[ hwnd ], hwnd, msg, w, l );

	if (msg == WM_KEYDOWN && w == 'C' && GetKeyState( VK_CONTROL ))
	{
		instance_->copyContent();
	}
	else if (msg == WM_DESTROY)
	{
		SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)itemsMap_[ hwnd ] );
		itemsMap_.erase( hwnd );
	}

	return result;
}


void CriticalMsgBox::create( HWND hwnd )
{
	BW_GUARD;

	HWND dlgItem;

	// 1. dialog
	SetWindowText( hwnd, CMB_DIALOG_TITLE );
	font_ = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
	MoveWindow( hwnd, 0, 0, 420, 380, TRUE );

	// 2. statics and edits
	dlgItem = CreateWindow( L"STATIC", CMB_STATIC_MESSAGE, WS_CHILD | WS_VISIBLE,
		10, 10, 400, 60, hwnd, NULL, GetModuleHandle( NULL ), NULL );
	SendMessage( dlgItem, WM_SETFONT, (WPARAM)font_, FALSE );

	dlgItem = CreateWindow( L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_GRAYRECT,
		0, 115, 420, 1, hwnd, NULL, GetModuleHandle( NULL ), NULL );
	SendMessage( dlgItem, WM_SETFONT, (WPARAM)font_, FALSE );

	dlgItem = CreateWindow( L"STATIC", CMB_STATIC_PROG_INFO, WS_CHILD | WS_VISIBLE,
		10, 125, 400, 20, hwnd, NULL, GetModuleHandle( NULL ), NULL );
	SendMessage( dlgItem, WM_SETFONT, (WPARAM)font_, FALSE );

	dlgItem = CreateWindow( L"EDIT", info_,
		WS_CHILD | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_BORDER | WS_VSCROLL,
		10, 155, 395, 155, hwnd, (HMENU)100, GetModuleHandle( NULL ), NULL );
	SendMessage( dlgItem, WM_SETFONT, (WPARAM)font_, FALSE );

	// 3. buttons
	enterDebugger_ = CreateWindow( L"BUTTON", CMB_BUTTON_ENTER_DEBUGGER,
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_DISABLED,
		60, 80, 120, 25, hwnd, reinterpret_cast<HMENU>(CMB_ENTER_DEBUGGER_WND_ID), GetModuleHandle( NULL ), NULL );
	SendMessage( enterDebugger_, WM_SETFONT, (WPARAM)font_, FALSE );
	itemsMap_[ enterDebugger_ ] = (WNDPROC)SetWindowLongPtr( enterDebugger_, GWLP_WNDPROC, (LONG_PTR)itemProc );

	exit_ = CreateWindow( L"BUTTON", CMB_BUTTON_EXIT, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED,
		250, 80, 120, 25, hwnd, reinterpret_cast<HMENU>(CMB_EXIT_WND_ID), GetModuleHandle( NULL ), NULL );
	SendMessage( exit_, WM_SETFONT, (WPARAM)font_, FALSE );
	itemsMap_[ exit_ ] = (WNDPROC)SetWindowLongPtr( exit_, GWLP_WNDPROC, (LONG_PTR)itemProc );

	// 4. BW dependent stuff
	if (sendInfoBackToBW_)
	{
		statusWindow_ = CreateWindow( L"STATIC", CMB_STATUS_SENDING, WS_CHILD | WS_VISIBLE | SS_SIMPLE,
			10, 320, 400, 20, hwnd, reinterpret_cast<HMENU>(CMB_STATUS_WND_ID), GetModuleHandle( NULL ), NULL );
		SendMessage( statusWindow_, WM_SETFONT, (WPARAM)font_, FALSE );
	}
	else
	{
		MoveWindow( dlgItem, 10, 155, 395, 185, FALSE );
		finishSending( hwnd, NULL );
	}

	return;
}


void CriticalMsgBox::copyContent() const
{
	BW_GUARD;

	if (OpenClipboard( NULL ))
	{
		size_t infoLen = wcslen( info_ );
		HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, ( infoLen + 1 ) * sizeof( wchar_t ) );

		if (mem)
		{
			LPVOID pv = GlobalLock( mem );

			if (pv)
			{
				memcpy( pv, info_, ( infoLen + 1 ) * sizeof( wchar_t ) );

				GlobalUnlock( mem );

				SetClipboardData( CF_UNICODETEXT, mem );
			}
		}

		CloseClipboard();

		if (mem)
		{
			GlobalFree( mem );
		}
	}
}


void CriticalMsgBox::finishSending( HWND hwnd, const wchar_t* status )
{
	if (statusWindow_ && status)
	{
		SetWindowText( statusWindow_, status );
	}

	KillTimer( hwnd, 1 );

	EnableWindow( enterDebugger_, TRUE );
	EnableWindow( exit_, TRUE );

	SetFocus( enterDebugger_ );
}

BW_END_NAMESPACE
