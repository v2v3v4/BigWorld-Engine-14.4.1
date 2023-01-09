#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/converter_info.hpp"
#include "plugin_system/plugin.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/win_file_system.hpp"

#include "hierarchical_config_converter.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "HierarchicalConfigConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

ConverterInfo hierarchicalConfigConverterInfo;
ResourceCallbacks resourceCallbacks;

PLUGIN_INIT_FUNC
{
	Compiler * compiler = dynamic_cast< Compiler * >( &pluginLoader );
	if (compiler == NULL)
	{
		return false;
	}

	// Initialise the file systems
	const auto & paths = compiler->getResourcePaths();
	bool bInitRes = BWResource::init( paths );

	if ( !AutoConfig::configureAllFrom( "resources.xml" ) )
	{
		ERROR_MSG("Couldn't load auto-config strings from resource.xml\n" );
	}

	WinFileSystem::useFileTypeCache( true );
	
	INIT_CONVERTER_INFO( hierarchicalConfigConverterInfo, 
		HierarchicalConfigConverter, 
		ConverterInfo::DEFAULT_FLAGS );

	compiler->registerConverter( hierarchicalConfigConverterInfo );
	compiler->registerResourceCallbacks( resourceCallbacks );

	return true;
}

PLUGIN_FINI_FUNC
{		
	BWResource::fini();

	// DataSectionCensus is created on first use, so delete at end of App
	DataSectionCensus::fini();

	Watcher::fini();

	return true;
}

BW_END_NAMESPACE
