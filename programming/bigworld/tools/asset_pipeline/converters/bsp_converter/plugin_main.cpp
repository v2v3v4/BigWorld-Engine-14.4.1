#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/converter_info.hpp"
#include "moo/convert_texture_tool.hpp"
#include "plugin_system/plugin.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"

#include "bsp_conversion_rule.hpp"
#include "bsp_converter.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "BSPConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

BSPConversionRule bspConversionRule;
ConverterInfo bspConverterInfo;
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
	
	INIT_CONVERTER_INFO( bspConverterInfo, 
						 BSPConverter, 
						 ConverterInfo::DEFAULT_FLAGS | ConverterInfo::UPGRADE_CONVERSION );

	// Register the bsp conversion rule and converter
	compiler->registerConversionRule( bspConversionRule );
	compiler->registerConverter( bspConverterInfo );
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
