#ifndef INPUT_PCH_HPP
#define INPUT_PCH_HPP

#if PCH_SUPPORT

#include "cstdmf/cstdmf_lib.hpp"

#include "math/vector2.hpp"
#include "pyscript/pyscript_lib.hpp"

#endif // PCH_SUPPORT

#ifdef _WIN32
#define INITGUID
#include "cstdmf/cstdmf_windows.hpp"
#include <windowsx.h>
#include <winnls.h>

#include <objbase.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif // _WIN32

#endif // INPUT_PCH_HPP
