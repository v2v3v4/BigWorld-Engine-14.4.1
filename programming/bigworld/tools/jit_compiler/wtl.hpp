#ifndef WTL_HPP
#define WTL_HPP

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <Shellapi.h>

#define _WTL_NO_AUTOMATIC_NAMESPACE
#define _SECURE_ATL 1

#pragma warning(push)
// suppress warning C4302: 'type cast' : truncation from 'LPCTSTR' to 'WORD'
#pragma warning(disable: 4302)

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlmisc.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlframe.h>

#pragma warning(pop)

#undef min
#undef max
#define NOMINMAX

#include "resource.h"

#endif // WTL_HPP
