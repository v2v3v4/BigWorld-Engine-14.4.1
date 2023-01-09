#ifdef _MSC_VER 
#pragma once
#endif

#ifndef BW_WINMAIN_HPP
#define BW_WINMAIN_HPP

#include "cstdmf/cstdmf_windows.hpp"

BW_BEGIN_NAMESPACE

extern const char * compileTimeString;

extern bool g_bAppStarted;

BW_END_NAMESPACE

int BWWinMain(	HINSTANCE hInstance,
				LPCTSTR lpCmdLine,
				int nCmdShow,
				LPCTSTR lpClassName,
				LPCTSTR lpWindowName );

LRESULT BWWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

bool BWProcessOutstandingMessages();


#endif // BW_WINMAIN_HPP


// bw_winmain.hpp
