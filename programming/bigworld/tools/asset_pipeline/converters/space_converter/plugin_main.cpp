#include <asset_pipeline/compiler/compiler.hpp>
#include "asset_pipeline/compiler/resource_callbacks.hpp"
#include <asset_pipeline/conversion/converter_info.hpp>
#include <plugin_system/plugin.hpp>
#include <plugin_system/plugin_loader.hpp>
#include <resmgr/auto_config.hpp>
#include <resmgr/bwresource.hpp>
#include <resmgr/data_section_census.hpp>

#include "appmgr/options.hpp"

#include "moo/init.hpp"
#include "moo/texture_manager.hpp"
#include "moo/renderer.hpp"

#include "physics2/material_kinds.hpp"

#include "romp/water.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/texture_feeds.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "terrain/manager.hpp"
#include "terrain/terrain2/resource.hpp"

#include "space_converter.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_item_amortise_delete.hpp"

#include "chunk/user_data_object_link_data_type.hpp"

BW_BEGIN_NAMESPACE

DECLARE_WATCHER_DATA( "SpaceConverter" )
DECLARE_COPY_STACK_INFO( true )
DEFINE_CREATE_EDITOR_PROPERTY_STUB

ConverterInfo spaceConverterInfo;
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

    WNDCLASS wc = { 0, DefWindowProc, 0, 0, NULL, NULL, NULL, NULL, NULL, L"SpaceConverter" };
    if ( !RegisterClass( &wc ) )
    {
        printf( "Could not register window class\n" );
        return false;
    }

    HWND hWnd = CreateWindow(
        L"SpaceConverter", L"SpaceConverter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        100, 100,
        NULL, NULL, NULL, 0 );

    s_pRenderer.reset( new Renderer );

    s_pRenderer->init( true, true );

    if (!Moo::rc().createDevice( hWnd ))
    {
        ERROR_MSG( "Could not create render device\n" );
    }

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::init();
#endif

    /*
    Options::init( 0, 0, L"spaceconverter.options"  );

    Moo::init();

    new AmortiseChunkItemDelete();

    WNDCLASS wc = { 0, DefWindowProc, 0, 0, NULL, NULL, NULL, NULL, NULL, L"SpaceConverter" };
    if ( !RegisterClass( &wc ) )
    {
        printf( "Could not register window class\n" );
        return false;
    }

    HWND hWnd = CreateWindow(
        L"SpaceConverter", L"SpaceConverter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        100, 100,
        NULL, NULL, NULL, 0 );

    if (!Moo::rc().createDevice( hWnd ))
    {
        ERROR_MSG( "Could not create render device\n" );
    }
    
    if (!MaterialKinds::init())
    {
        return false;
    }

    if (!Moo::TextureManager::instance())
        Moo::TextureManager::init();

    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
    
    // Needed to properly initialise elements such as Water
    Waters::instance().init();

    speedtree::SpeedTreeRenderer::enviroMinderLighting( false );

    Moo::Camera cam( 0.25f, 200.f, DEG_TO_RAD(60.f), 2.f );
    cam.aspectRatio(
        float(Moo::rc().screenWidth()) / float(Moo::rc().screenHeight()) );
    Moo::rc().camera( cam );

    Moo::VisualCompound::disable( true );

    Terrain::Manager::init();
    TextureFeeds::init();
    LensEffectManager::init();
    ChunkManager::instance().init();
    EnviroMinder::init();

    Terrain::ResourceBase::defaultStreamType( Terrain::RST_Syncronous );

    ShowCursor( TRUE );
    */
    
    INIT_CONVERTER_INFO( spaceConverterInfo, DefaultSpaceConverter, 
        ConverterInfo::DEFAULT_FLAGS & ~(ConverterInfo::THREAD_SAFE) );

    compiler->registerConverter( spaceConverterInfo );
    compiler->registerResourceCallbacks( resourceCallbacks );

    return true;
}

PLUGIN_FINI_FUNC
{
#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::fini();
#endif

    Moo::TextureManager::fini();
    Moo::fini();

    /*
    ChunkManager::instance().fini();
    Terrain::Manager::fini();
    Moo::fini();
    */

    BWResource::fini();

    // DataSectionCensus is created on first use, so delete at end of App
    DataSectionCensus::fini();

    Watcher::fini();

    s_pRenderer.reset();

    return true;
}

BW_END_NAMESPACE
