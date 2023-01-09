#pragma once
#include "core\Globals.h"
#include "core\Plugin_Header.h"

// You must implement the three functions here, as they will be called by the framework when your DLL is loaded to retrieve information
// about your plugins.

// Make sure you disable name mangling with extern C
extern "C" { 
__declspec(dllexport) void SetupGlobalAccess(WMGlobalStruc *global);
__declspec(dllexport) int  GetHeaderVersion();
__declspec(dllexport) bool GetDevPlugin(const int i, DevPluginStruc *result);
};

