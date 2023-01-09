#include "pch.hpp"
#include "asyn_msg.hpp"
#include "resource.h"
#include "cstdmf/debug_exception_filter.hpp"
#include <process.h>


BW_BEGIN_NAMESPACE

static volatile HWND hwnd = NULL;
static wchar_t logFileName[1024];
FILE * logFile = NULL;

static const UINT WM_ADDERROR = WM_USER + 0x345;

INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	BOOL result = TRUE;
	switch( uMsg )
	{
	case WM_INITDIALOG:
		hwnd = hwndDlg;
		ShowWindow( hwnd, SW_HIDE );
		if( logFile == NULL )
			AsyncMessage().reportMessage( 
				( BW::wstring( L"cannot create log file " ) + logFileName ).c_str(), true );
		break;
	case WM_CLOSE:
		ShowWindow( hwndDlg, SW_HIDE );
		break;
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;
	case WM_ADDERROR:
		{
			AsyncMessage().printMsg( (wchar_t*)wParam );
			raw_free( (wchar_t*)wParam );
		}
		break;
	default:
		result = FALSE;
	}
	return result;
}

void AsyncMessageThread( LPVOID )
{
#if ENABLE_STACK_TRACKER && !_DEBUG
	__try
#endif
	{
		// get the log file name
		GetModuleFileName( NULL, logFileName, ARRAY_SIZE( logFileName ) );
		wchar_t* end = logFileName + wcslen( logFileName ) - 1;
		while( *end != L'.' )
			--end;
		wcscpy( end + 1, L"log" );

		// create the log file
		logFile = _wfopen( logFileName, L"a+" );

		// create the error log dialog
		DialogBox( GetModuleHandle( NULL ),
			MAKEINTRESOURCE( IDD_MESSAGEDIALOG ), NULL /*GetDesktopWindow()*/, DialogProc );

		// close if the file is opened successfully
		if( logFile != NULL )
			fclose( logFile );
	}
#if ENABLE_STACK_TRACKER && !_DEBUG
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}
#endif
}

void AsyncMessage::reportMessage( const wchar_t * msg, bool severity )
{
	BW_GUARD;

	if (severity)
	{
		PostMessage( hwnd, WM_ADDERROR, (WPARAM)raw_wcsdup( msg ), 0 );

		if( !isShow() )
			show();
	}
	else
	{
		writeToLog( dateMsg( msg ).c_str() );
	}
}

void AsyncMessage::show()
{
	BW_GUARD;

	ShowWindow( hwnd, SW_SHOW );
	SetForegroundWindow( hwnd );
}

void AsyncMessage::hide()
{
	BW_GUARD;

	ShowWindow( hwnd, SW_HIDE );
}

bool AsyncMessage::isShow() const
{
	BW_GUARD;

	return ( GetWindowLong( hwnd, GWL_STYLE ) & WS_VISIBLE ) != 0;
}

HWND AsyncMessage::handle()
{
	return hwnd;
}

const wchar_t* AsyncMessage::getLogFileName() const
{
	return logFileName;
}


void AsyncMessage::printMsg( const wchar_t * msg ) const
{
	BW_GUARD;

	BW::wstring datedMsg( dateMsg( msg ) );
	BW::wstring datedMsgWithoutCR( datedMsg );

	// remove newline and carriage return characters
	std::replace( datedMsgWithoutCR.begin(), datedMsgWithoutCR.end(), '\r', ' ' );
	std::replace( datedMsgWithoutCR.begin(), datedMsgWithoutCR.end(), '\n', ' ' );
	HWND message = GetDlgItem( hwnd, IDC_MESSAGELIST );
	SendMessage( message, LB_SETTOPINDEX,
		SendMessage( message, LB_ADDSTRING, 0, (LPARAM)datedMsgWithoutCR.c_str() ), 0 );

	writeToLog( datedMsg.c_str() );
}


BW::wstring AsyncMessage::dateMsg( const wchar_t * msg ) const
{
	BW_GUARD;

	SYSTEMTIME time;
	GetLocalTime( &time );
	wchar_t dateTime[1023];
	bw_snwprintf( dateTime, ARRAY_SIZE(dateTime), L"%02d/%02d/%04d - %02d:%02d:%02d : ",
		time.wMonth, time.wDay, time.wYear,
		time.wHour, time.wMinute, time.wSecond );

	BW::wstring datedMsg = dateTime;
	datedMsg.append( msg );
	return datedMsg;
}


void AsyncMessage::writeToLog( const wchar_t * msg ) const
{
	BW_GUARD;

	if( logFile != NULL )
	{
		fputws( msg, logFile );
		fflush( logFile );
	}
}


namespace
{

struct ThreadCreator
{
	ThreadCreator()
	{
		BW_GUARD;

		_beginthread( AsyncMessageThread, 0, NULL );
		while( !hwnd )
			Sleep( 1 );
	}
	~ThreadCreator()
	{
		BW_GUARD;

		EndDialog( hwnd, 0 );
	}
}
ThreadCreator;

}

BW_END_NAMESPACE
