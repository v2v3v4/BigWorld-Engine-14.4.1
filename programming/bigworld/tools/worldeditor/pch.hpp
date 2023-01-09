#ifndef WE_PCH_HPP
#define WE_PCH_HPP

// pch.hpp : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

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

#if PCH_SUPPORT
#include "cstdmf/cstdmf_lib.hpp"
#include "appmgr/appmgr_lib.hpp"
#include "chunk/chunk_lib.hpp"
#include "fmodsound/fmodsound_lib.hpp"
#include "gizmo/gizmo_lib.hpp"
#include "guimanager/guimanager_lib.hpp"
#include "input/input_lib.hpp"
#include "math/math_lib.hpp"
#include "model/model_lib.hpp"
#include "moo/moo_lib.hpp"
#include "physics2/physics2_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "romp/romp_lib.hpp"
#include "terrain/terrain_lib.hpp"
#include "resmgr/resmgr_lib.hpp"
#include "scene/scene_object.hpp"
#include "space/space_lib.hpp"

#include "common/base_mainframe.hpp"
#include "common/romp_harness.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/range_slider_ctrl.hpp"
#include "controls/slider.hpp"
#include "controls/themed_image_button.hpp"
#include "editor_shared/gui/i_main_frame.hpp"
#include "guitabs/guitabs_content.hpp"
#include "guitabs/manager.hpp"

#include "ual/ual_lib.hpp"

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "worldeditor/world/world_manager.hpp"

#include <Python.h>
#endif // PCH_SUPPORT

#endif // WE_PCH_HPP
