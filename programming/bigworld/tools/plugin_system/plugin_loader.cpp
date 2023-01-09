#include "plugin_loader.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"

#include <fstream>
#include <iterator>

#define STR( X ) #X
#define PLUGIN_GET_PROC_ADDRESS( hPlugin, func ) ::GetProcAddress( hPlugin, STR( func ) )

BW_BEGIN_NAMESPACE

void PluginLoader::initPlugins()
{
	BW_GUARD;

	BW::string configFile = BWUtil::executableDirectory() + 
		BWUtil::executableBasename() + "_plugins.txt";
	
	std::ifstream configStream( configFile.c_str() );
	if ( !configStream.good() )
	{
		ERROR_MSG("Could not open plugin list file %S\n", configFile);
		return;
	}

	typedef std::istream_iterator< BW::string > string_istream_iterator;
	for (auto it = string_istream_iterator(configStream), end = string_istream_iterator(); it != end; ++it)
	{
		INFO_MSG("Loading Plugin %S as specified in file\n", it->c_str());
		loadPlugin( it->c_str() );
	}
}

void PluginLoader::finiPlugins()
{
	BW_GUARD;

	size_t numPlugins = plugins_.size();
	for ( size_t i = numPlugins; i > 0; --i )
	{
		unloadPlugin( plugins_[i - 1] );
	}
}

HMODULE PluginLoader::loadPlugin( const BW::string& pluginName )
{
	BW_GUARD;

	BW::wstring pluginFileName = bw_utf8tow( 
		BWUtil::executableDirectory() + pluginName );

	pluginFileName.append(L".dll");

	INFO_MSG( "Loading plugin file %S\n", pluginFileName.c_str() );

	HMODULE hPlugin = ::LoadLibrary( pluginFileName.c_str() );
	if (hPlugin != NULL)
	{
		PluginInitFunc pluginInit =
			(PluginInitFunc) PLUGIN_GET_PROC_ADDRESS( hPlugin, PLUGIN_INIT );

		if (pluginInit != NULL && ( *pluginInit )( *this ))
		{
			plugins_.push_back( hPlugin );
			pluginNames_.push_back( bw_wtoutf8( pluginFileName ) );
			return hPlugin;
		}

		PluginFiniFunc pluginFini =
			(PluginFiniFunc) PLUGIN_GET_PROC_ADDRESS( hPlugin, PLUGIN_FINI );

		if (pluginFini != NULL)
		{
			( *pluginFini )( *this );
		}
		::FreeLibrary( hPlugin );
	}
	return NULL;
}

bool PluginLoader::unloadPlugin( HMODULE hPlugin )
{
	BW_GUARD;

	WCHAR filename[MAX_PATH];
	GetModuleFileName( hPlugin, &filename[0], MAX_PATH );
	BW::string pluginName;
	bw_wtoacp( filename, pluginName );
	INFO_MSG( "Unloading plugin %s\n", pluginName.c_str() );

	for ( auto it = plugins_.cbegin(); it != plugins_.cend(); ++it )
	{
		if (*it != hPlugin)
		{
			continue;
		}

		PluginFiniFunc pluginFini =
			(PluginFiniFunc) PLUGIN_GET_PROC_ADDRESS( *it, PLUGIN_FINI );

		if (pluginFini != NULL && !( *pluginFini )( *this ))
		{
			return false;
		}

		::FreeLibrary( *it );
		plugins_.erase( it );

		auto nameIt = std::find( std::begin( pluginNames_ ), std::end( pluginNames_ ), pluginName );
		if (nameIt != std::end( pluginNames_ ))
		{
			pluginNames_.erase( nameIt );
		}

		break;
	}
	return true;
}

const PluginLoader::PluginNameList & PluginLoader::pluginNames() const
{
	return pluginNames_;
}

BW_END_NAMESPACE

#undef PLUGIN_GET_PROC_ADDRESS
#undef STR