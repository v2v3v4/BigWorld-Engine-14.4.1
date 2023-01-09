#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/converter_info.hpp"
#include "moo/convert_texture_tool.hpp"
#include "plugin_system/plugin.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"

#include "texture_converter.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "TextureConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

ConverterInfo textureConverterInfo;
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
		// this sets up the config variable that points the texture conversion rules to the
		//	texturedetails defined in the resources.xml that appears in the root of the filenames.
		//	If the texture details cannot be initialised, it is bad for texture-convert, but not necesarily 
		//	bad for other commands. So it doesnt reprot a failure here.
		//		Possibly should differentiate between commands?
		ERROR_MSG("Could not load auto-config strings from resource.xml\n" );
		return false;
	}
	
	// create a texture conversion tool that converts in other threads
	//
	//	this needs to happen in the plugin init as we are initialising moo,
	//		which contains an init singleton that should only be initialised
	//		once.
	//
	//	Also it is needless to init & deinit all these resources per texture
	//	   converted

	TextureConverter::s_pTool = new Moo::ConvertTextureTool();
	if (TextureConverter::s_pTool == NULL)
	{
		ERROR_MSG("Could not create convert texture tool\n" );
		return false;
	}

	bool bSuccessfulInit = TextureConverter::s_pTool->initResources();
	if (bSuccessfulInit == false)
	{
		TextureConverter::s_pTool->resourcesFini();
		// free up the class that did the actual conversion.
		bw_safe_delete( TextureConverter::s_pTool );

		ERROR_MSG( "Could not initialise a new ConvertTextureTool.\n" );
		return false;
	}

	INIT_CONVERTER_INFO( textureConverterInfo, 
						 TextureConverter, 
						 ConverterInfo::DEFAULT_FLAGS );

	compiler->registerConverter( textureConverterInfo );
	compiler->registerResourceCallbacks( resourceCallbacks );

	return true;
}

PLUGIN_FINI_FUNC
{
	TextureConverter::s_pTool->resourcesFini();
	bw_safe_delete( TextureConverter::s_pTool );
		
	BWResource::fini();

	// DataSectionCensus is created on first use, so delete at end of App
	DataSectionCensus::fini();

	Watcher::fini();

	return true;
}

BW_END_NAMESPACE
