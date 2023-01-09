#ifndef SCALEFORM_CONFIG_HPP
#define SCALEFORM_CONFIG_HPP


// Set this define to 1 if you wish to use the Scaleform IME library in your game.
// This comes as an additional package that must be installed over the top of
// the basic GFx installation.
// Please see http://www.scaleform.com for licensing details.

// Note that enabling Scaleform IME will disable the built-in IME support in BigWorld.
#define SCALEFORM_IME 0

// We don't want Scaleform functions to be exported from our EXE
#define SF_BUILD_STATICLIB 1

// Include config of FMOD
#include "fmodsound/fmod_config.hpp"

#if SCALEFORM_SUPPORT

#include <GFx.h>
#include <GFx_Kernel.h>
#include <GFx_Render.h>
#include <GFx_Renderer_D3D9.h>
#include <GFx_Sound_FMOD.h>
#include <GFx_FontProvider_Win32.h>

#include "Kernel/SF_Memory.h"
#include "Kernel/SF_Debug.h"
#include <Kernel/SF_MemoryHeap.h>
#include <Video/Video_VideoPC.h>
#include <Sound/Sound_SoundRendererFMOD.h>

#include <Xml/XML_Expat.h>

#ifdef GFX_AMP_SERVER
#include <Amp/GFx_AmpServer.h>
#include <Amp/Amp_ProfileFrame.h>
#endif

using namespace Scaleform;
namespace SF = Scaleform;

typedef unsigned                        UInt;
typedef int                             SInt;
typedef float                           Float;
using namespace                         SF::BaseTypes;

typedef SF::Render::Color               GColor;
typedef SF::Render::RectF               GRectF;
typedef SF::Render::PointF              GPointF;
typedef SF::Render::SizeF               GSizeF;
typedef SF::String                      GString;
typedef SF::File                        GFile;

#endif


#endif // SCALEFORM_CONFIG_HPP
