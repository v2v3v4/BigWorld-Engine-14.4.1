// pch.hpp : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

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

#include <afxdlgs.h>
#include <afxole.h>
#include <afxpriv.h>

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "appmgr/appmgr_lib.hpp"
#include "chunk/chunk_lib.hpp"
#include "fmodsound/fmodsound_lib.hpp"
#include "gizmo/gizmo_lib.hpp"
#include "guimanager/guimanager_lib.hpp"
#include "guitabs/guitabs_content.hpp"
#include "guitabs/manager.hpp"
#include "math/math_lib.hpp"
#include "moo/moo_lib.hpp"

#include "particle/meta_particle_system.hpp"
#include "particle/particle.hpp"
#include "particle/particles.hpp"
#include "particle/particle_system.hpp"
#include "particle/particle_system_manager.hpp"
#include "particle/py_meta_particle_system.hpp"
#include "particle/py_particle_system.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "particle/renderers/particle_system_renderer.hpp"

#include "pyscript/pyscript_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "romp/romp_lib.hpp"
#include "space/space_lib.hpp"

#include "tools/common/resource_loader.hpp"

#include "ual/ual_lib.hpp"

#include <Python.h>

#endif // PCH_SUPPORT
