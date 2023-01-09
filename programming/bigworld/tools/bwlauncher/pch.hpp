#ifndef PCH__HPP
#define PCH__HPP

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#pragma warning(disable:4995)

#include <windows.h>
#include <cassert>
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>

#include "cstdmf/bw_vector.hpp"
#include <string>
#include "cstdmf/bw_map.hpp"
#include <algorithm>

#ifdef _UNICODE
typedef BW::wstring tstring;
typedef std::wifstream tifstream;
#else // _UNICODE
typedef std::string tstring;
typedef std::ifstream tifstream;
#endif // _UNICODE






#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

#endif
