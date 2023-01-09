#include "asset_pipeline/compiler/compiler.hpp"
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include "asset_pipeline/conversion/converter_info.hpp"
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/renderer.hpp"
#include "plugin_system/plugin.hpp"
#include "plugin_system/plugin_loader.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"

#include "effect_converter.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "EffectConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

ConverterInfo effectConverterInfo;
ResourceCallbacks resourceCallbacks;

static std::auto_ptr<Renderer> s_pRenderer;

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

    Moo::init( true, true );

    WNDCLASS wc = { 0, DefWindowProc, 0, 0, NULL, NULL, NULL, NULL, NULL, L"EffectConverter" };
    if ( !RegisterClass( &wc ) )
    {
        printf( "Could not register window class\n" );
        return false;
    }

    HWND hWnd = CreateWindow(
        L"EffectConverter", L"EffectConverter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        100, 100,
        NULL, NULL, NULL, 0 );

    s_pRenderer.reset( new Renderer );

    s_pRenderer->init( true, true );

    if (!Moo::rc().createDevice( hWnd,0,0,true,false,Vector2(0,0),true,false ))
    {
        ERROR_MSG( "Could not create render device\n" );
    }

    INIT_CONVERTER_INFO( effectConverterInfo, 
                         EffectConverter, 
                         ConverterInfo::CACHE_CONVERSION );

    compiler->registerConverter( effectConverterInfo );
    compiler->registerResourceCallbacks( resourceCallbacks );

    return true;
}

PLUGIN_FINI_FUNC
{
    Moo::fini();

    BWResource::fini();

    // DataSectionCensus is created on first use, so delete at end of App
    DataSectionCensus::fini();

    Watcher::fini();

    s_pRenderer.reset();

    return true;
}

BW_END_NAMESPACE