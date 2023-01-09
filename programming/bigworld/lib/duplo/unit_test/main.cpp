#include "pch.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/init.hpp"
#include "moo/moo_test_window.hpp"
#include "moo/renderer.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#include "input/input.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_census.hpp"
#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"
#include "unit_test_lib/base_resmgr_unit_test_harness.hpp"

#include "unit_test_lib/unit_test.hpp"
#include <memory>

BW_BEGIN_NAMESPACE

extern int ResMgr_token;
extern int PyScript_token;

int s_tokens =
    ResMgr_token |
    PyScript_token;

BW_END_NAMESPACE

BW_USE_NAMESPACE

int main( int argc, char * argv[] )
{
    BW_SYSTEMSTAGE_MAIN();

    BW::Allocator::setCrashOnLeak( true );
    
    const std::auto_ptr<CStdMf> cstdmfSingleton( new CStdMf );

    // saved away for the test harness
    BaseResMgrUnitTestHarness::s_cmdArgC = argc;
    BaseResMgrUnitTestHarness::s_cmdArgV = argv;

    BaseResMgrUnitTestHarness resourceTestHarness( "duplo" );

    // Initialize input devices because blob builds bring in 
    // PyIME when initializing python, and it needs the InputDevices
    // to be initialized.
    InputDevices inputDevices;

    PyImportPaths importPaths;
    importPaths.addNonResPath( "." );
    importPaths.addResPath( "." );

    if (!Script::init( importPaths ))
    {
        ERROR_MSG( "Could not initialise Script module\n" );
        return 1;
    }

    if (!AutoConfig::configureAllFrom( AutoConfig::s_resourcesXML ))
    {
        ERROR_MSG( "Could not find resources.xml, which should "
            "contain the location of system resources!" );
        return 1;
    }

    if (!BgTaskManager::init())
    {
        ERROR_MSG( "Unable to initialise BgTaskManager.\n" );
        BWResource::fini();
        return 1;
    }
    if (!FileIOTaskManager::init())
    {
        ERROR_MSG( "Unable to initialise BgTaskManager.\n" );
        BgTaskManager::fini();
        BWResource::fini();
        return 1;
    }

    std::auto_ptr<Renderer> pRenderer( new Renderer );
    pRenderer->init( true, true );

    if (!Moo::init( true, true ))
    {
        ERROR_MSG( "Unable to initialise Moo.\n" );
        FileIOTaskManager::fini();
        BgTaskManager::fini();
        BWResource::fini();
        pRenderer.reset();
        return 1;
    }

    Chunk::init();

    int result = 0;
    {
        Moo::TestWindow testWindow;
        if (testWindow.init())
        {
            result = BWUnitTest::runTest( "duplo", argc, argv );
        }
        else
        {
            result = 1;
            ERROR_MSG( "Unable to create the Moo test window.\n" );
        }
    }
    
    Script::fini();
    MainLoopTasks::finiAll();
    ChunkVLO::fini();

    Chunk::fini();
    pRenderer.reset();
    Moo::fini();
    FileIOTaskManager::fini();
    BgTaskManager::fini();
    BWResource::fini();

    DebugFilter::fini();

    return result;
}
