#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxcmn.h>
#include <afxole.h>
#include <atlimage.h>

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "math/math_lib.hpp"
#include "moo/moo_lib.hpp"
#endif // PCH_SUPPORT

