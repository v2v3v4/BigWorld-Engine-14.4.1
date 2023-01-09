#include "pch.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/allocator.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "moo/init.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_vlo.hpp"
#include "moo/moo_test_window.hpp"
#include "moo/renderer.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_cache.hpp"
#include "resmgr/data_section_census.hpp"
#include "unit_test_lib/unit_test.hpp"
#include "moo/texture_manager.hpp"
#include <memory>


BW_USE_NAMESPACE

const char BIGWORLD_RES_RESOURCE_PATH[] = "../../../res/bigworld/";

int main( int argc, char* argv[] )
{
    BW_SYSTEMSTAGE_MAIN();

    BW::Allocator::setCrashOnLeak( true );
    
    const std::auto_ptr<CStdMf> cstdmfSingleton( new CStdMf );

    //For the unit tests, we force a particular resource path.
    const char * myargv[] =
        {
            argv[0],
            //Test-specific resources.
            "--res", UNIT_TEST_RESOURCE_PATH,
            //Also need the general resource path, so default materials
            //and shaders can be used. Useful when loading test models.
            "--res", BIGWORLD_RES_RESOURCE_PATH
        };
    int myargc = ARRAY_SIZE( myargv );

    if ( !BWResource::init( myargc, myargv ) )
    {
        ERROR_MSG( "Unable to initialise BWResource.\n" );
        return 1;
    }

    DataSectionCache::instance();

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
        ERROR_MSG( "Unable to initialise FileIOTaskManager.\n" );
        BgTaskManager::fini();
        BWResource::fini();
        return 1;
    }

    std::auto_ptr<Renderer> pRenderer( new Renderer );

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
            result = BWUnitTest::runTest( "model", argc, argv );

            FileIOTaskManager::fini();
            BgTaskManager::fini();

        }
        else
        {
            FileIOTaskManager::fini();
            BgTaskManager::fini();
            result = 1;
            ERROR_MSG( "Unable to create the Moo test window.\n" );
        }
    }

    Chunk::fini();
    ChunkVLO::fini();

    pRenderer.reset();
    Moo::fini();

    MainLoopTasks::finiAll();

    BWResource::fini();

    DebugFilter::fini();

    return result;
}
