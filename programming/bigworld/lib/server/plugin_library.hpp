#ifndef PLUGIN_LIBRARY_HPP
#define PLUGIN_LIBRARY_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This namespace contains functions related to plugin libraries.
 */
namespace PluginLibrary
{
	bool loadAllFromDirRelativeToApp(
		bool prefixWithAppName, const char * partialDir );
};

BW_END_NAMESPACE

#endif // PLUGIN_LIBRARY_HPP
