#ifndef PLUGIN_HPP
#define PLUGIN_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class PluginLoader;

#ifdef _DEBUG
	#define PLUGIN_INIT PluginInit_d
	#define PLUGIN_FINI PluginFini_d
#else
	#define PLUGIN_INIT PluginInit
	#define PLUGIN_FINI PluginFini
#endif

#ifdef _DEBUG
	#define PLUGIN_INIT_FUNC extern "C" __declspec(dllexport) bool PLUGIN_INIT( PluginLoader & pluginLoader )
	#define PLUGIN_FINI_FUNC extern "C" __declspec(dllexport) bool PLUGIN_FINI( PluginLoader & pluginLoader )
#else
	#define PLUGIN_INIT_FUNC extern "C" __declspec(dllexport) bool PLUGIN_INIT( PluginLoader & pluginLoader )
	#define PLUGIN_FINI_FUNC extern "C" __declspec(dllexport) bool PLUGIN_FINI( PluginLoader & pluginLoader )
#endif

BW_END_NAMESPACE

#endif //PLUGIN_HPP
