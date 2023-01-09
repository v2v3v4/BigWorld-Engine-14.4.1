#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/compiler/test_compiler/test_compiler.hpp"
#include "asset_pipeline/conversion/converter_info.hpp"
#include "plugin_system/plugin.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"

#include "test_converter.hpp"
#include "test_conversion_rule.hpp"
#include "test_root_conversion_rule.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "TestConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

TestConversionRule testConversionRule;
TestRootConversionRule testRootConversionRule;
ConverterInfo testConverterInfo;
ResourceCallbacks resourceCallbacks;

PLUGIN_INIT_FUNC
{
	TestCompiler * compiler = dynamic_cast< TestCompiler * >( &pluginLoader );
	if (compiler == NULL)
	{
		return false;
	}

	// Initialise the file systems
	//For the unit tests, we force a particular resource path.
	const char * UNIT_TEST_RESOURCE_PATH = "../..";
	const char * myargv[] =
	{
		//Test-specific resources.
		"--res", UNIT_TEST_RESOURCE_PATH
	};
	int myargc = ARRAY_SIZE( myargv );

	bool bInitRes = BWResource::init( myargc, myargv );

	INIT_CONVERTER_INFO( testConverterInfo, 
						 TestConverter, 
						 ConverterInfo::DEFAULT_FLAGS );

	compiler->registerConversionRule( testConversionRule );
	compiler->registerConversionRule( testRootConversionRule );
	compiler->registerConverter( testConverterInfo );
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
