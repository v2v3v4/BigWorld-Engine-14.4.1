// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifdef _WIN32
#pragma once

#include <stdio.h>
#include <tchar.h>
#include <windows.h>

#else
#include <stdlib.h>
#endif // _WIN32


// TODO: reference additional headers your program requires here
#include "CppUnitLite2/src/Test.h"

#include "unit_test_lib/unit_test.hpp"

// TODO: Move this to cstdmf
inline void mySleep( int milliseconds )
{
#ifdef _WIN32
	Sleep( milliseconds );
#else
	usleep( milliseconds * 1000 );
#endif
}

// stdafx.h
