/*
 * The start-up code that interfaces to the Win32 API.
 *
 * Two functions defined in this file: WinMain and WndProc.
 * WinMain is the entry point in Windows. WndProc is the callback function to
 * handle window messages..
 *
 */


#include "pch.hpp"

#include "cstdmf/cstdmf_windows.hpp"
#include "cstdmf/debug_exception_filter.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "client/bw_winmain.hpp"
#include "pyscript/script.hpp"
#include "resource.h"


DECLARE_DEBUG_COMPONENT2( "App", 0 )

BW_USE_NAMESPACE
//
//	Application Specific Definitions:
//	Some pre-processing and debug defines for the application.
//
static const LPCTSTR APP_NAME = L"BWClass";



#if defined( _DEBUG )
	const wchar_t * APP_TITLE = L"BigWorld Client Debug Version";
	wchar_t * BUILD_CONFIGURATION = L"Debug Version";
#elif defined( _HYBRID )
    const wchar_t * APP_TITLE = L"BigWorld Client Hybrid Version";
	wchar_t * BUILD_CONFIGURATION = L"Hybrid Version";
#elif defined( _EVALUATION )
    const wchar_t * APP_TITLE = L"BigWorld Client Evaluation Version";
	wchar_t * BUILD_CONFIGURATION = L"Evaluation Version";
#else
	const wchar_t * APP_TITLE = L"BigWorld Client";
	wchar_t * BUILD_CONFIGURATION = L"";
#endif


/*static*/ bool isEval()
{
#if defined( _EVALUATION )
	return true;
#else
	return false;
#endif
}
PY_AUTO_MODULE_FUNCTION( RETDATA, isEval, END, BigWorld )




LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Defined in compile_time.cpp
extern const char * compileTimeString;

BW::wstring s_className( L"App" );

#ifdef _WINDLL
HINSTANCE g_hinstance;
volatile bool s_mainThreadTerminated = false;

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
	g_hinstance = hinstDLL;
	
	// Perform actions based 
	// on the reason for calling.
	switch( fdwReason ) 
	{ 
	case DLL_PROCESS_ATTACH:
		{
			BW::wstringstream wss;

			wss << L"BWClass" << GetTickCount();

			s_className = wss.str();

			WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hinstDLL,
				LoadIcon( hinstDLL, MAKEINTRESOURCE(IDI_ICON2)),
				LoadCursor( NULL, MAKEINTRESOURCE( IDC_NULL ) ),
				NULL, NULL, s_className.c_str() };

			if (!RegisterClass( &wc ))
				return FALSE;
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		UnregisterClass( s_className.c_str(), hinstDLL );

		if (!s_mainThreadTerminated)
		{
			ExitProcess( 0 );
		}

		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}


#include "cstdmf/unique_id.hpp"


extern "C" __declspec(dllexport) BOOL run( const wchar_t* path, const wchar_t* commandLine )
{
	BW_GUARD;

	SetCurrentDirectory( path );

	BWResource::overrideAppDirectory( bw_wtoutf8( path ) );

	BW::wstring wcommandLine = commandLine;
	BWWinMain( g_hinstance, wcommandLine.c_str(), SW_SHOWNORMAL, s_className.c_str(), APP_TITLE );

	s_mainThreadTerminated = true;

	return TRUE;
};


static PyObject * py_bwclient_run( PyObject * self, PyObject * args )
{
	BW_GUARD;
	char * commandline = NULL;
	int ok = PyArg_ParseTuple(args, "s", &commandline);

	BW::wstring wcommandline;
	bw_utf8tow( commandline, wcommandline );
	BWWinMain( g_hinstance, wcommandline.c_str(), SW_SHOWNORMAL, s_className.c_str(), APP_TITLE );

	Py_RETURN_NONE;
};


static PyMethodDef bwclient_methods[] = {
	{"run", py_bwclient_run, METH_VARARGS, "run( commandline )"},
	{NULL, NULL}
};


#define PY_MODULE_INIT_IMP(MODULE_NAME)                \
	PyMODINIT_FUNC init##MODULE_NAME()                 \
	{                                                  \
		Py_InitModule(#MODULE_NAME, bwclient_methods); \
	}
	
#define PY_MODULE_INIT(MODULE_NAME)                    \
	PY_MODULE_INIT_IMP(MODULE_NAME)

// BWCLIENT_NAME can be defined in the project 
// settings (C++/Preprocessor) using the $(TargetName) 
// macro: /D "BWCLIENT_NAME=$(TargetName).
PY_MODULE_INIT( BWCLIENT_NAME )


#else // BWCLIENT_AS_PYTHON_MODULE

//-----------------------------------------------------------------------------
// WinMain()
// Desc: Application entry point.
//-----------------------------------------------------------------------------
int PASCAL wWinMain(HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPWSTR lpCmdLine,
					int nCmdShow )
{
	BW_GUARD_BEGIN;
	
	BW::CStdMf::checkUnattended();

    // Set up the window class
	HCURSOR cursor = LoadCursor( hInstance, MAKEINTRESOURCE(IDC_NULL) );
    WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, LoadIcon( hInstance, MAKEINTRESOURCE(IDI_CLIENT_ICON)),
					cursor, NULL, NULL, s_className.c_str() };

    if( !RegisterClass( &wc ) )
        return FALSE;

	int ret = CallWithExceptionFilter( BWWinMain, hInstance, (LPCTSTR)lpCmdLine, nCmdShow, s_className.c_str(), APP_TITLE );

	BOOL success = UnregisterClass( s_className.c_str(), hInstance );

	return ret;
}

#endif // BWCLIENT_AS_PYTHON_MODULE


//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Window procedure for the game window
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return BWWndProc( hWnd, msg, wParam, lParam );
}


// winmain.cpp
