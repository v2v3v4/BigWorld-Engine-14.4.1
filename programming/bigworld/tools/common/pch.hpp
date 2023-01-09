#ifndef TOOLS_COMMON_PCH_HPP
#define TOOLS_COMMON_PCH_HPP

#include "script/first_include.hpp"

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "chunk/chunk_lib.hpp"
#include "input/input_lib.hpp"
#include "math/math_lib.hpp"
#include "moo/moo_lib.hpp"
#include "physics2/worldtri.hpp"
#include "physics2/collision_obstacle.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "romp/romp_lib.hpp"
#include "terrain/terrain_lib.hpp"
#endif // PCH_SUPPORT

#endif // TOOLS_COMMON
