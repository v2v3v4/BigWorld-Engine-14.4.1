#ifndef PLUGIN_LOADER_HPP
#define PLUGIN_LOADER_HPP

#include <WTypes.h>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "plugin.hpp"

#define PLUGIN_GET_PROC_ADDRESS( hPlugin, func ) ::GetProcAddress( hPlugin, #func )

BW_BEGIN_NAMESPACE

typedef bool ( *PluginInitFunc )( PluginLoader & pluginLoader );
typedef bool ( *PluginFiniFunc )( PluginLoader & pluginLoader );

class PluginLoader
{
public:
	typedef BW::vector< BW::string > PluginNameList;

public:
	PluginLoader() {}
	virtual ~PluginLoader() {}

	void initPlugins();
	void finiPlugins();

	HMODULE loadPlugin( const BW::string& pluginName );
	bool unloadPlugin( HMODULE plugin );

	const PluginNameList & pluginNames() const;

private:

	typedef BW::vector< HMODULE > PluginList;
	PluginList plugins_;

	PluginNameList pluginNames_;
};

BW_END_NAMESPACE

#undef PLUGIN_GET_PROC_ADDRESS

#endif //PLUGIN_LOADER_HPP
