#include "pch.hpp"
#include "debug_app.hpp"

#include "app.hpp"
#include "app_config.hpp"
#include "avatar_drop_filter.hpp"
#include "camera_app.hpp"
#include "client_camera.hpp"
#include "connection_control.hpp"
#include "device_app.hpp"
#include "entity_picker.hpp"
#include "frame_rate_graph.hpp"
#include "python_server.hpp"
#include "version_info.hpp"

#include "chunk/chunk_bsp_holder.hpp"
#include "chunk/chunk_manager.hpp"

#include "connection/smart_server_connection.hpp"

#include "cstdmf/bwversion.hpp"
#include "cstdmf/debug_filter.hpp"

#include "resmgr/datasection.hpp"

#if ENABLE_CONSOLES
#include "romp/console_manager.hpp"
#endif // ENABLE_CONSOLES
#include "romp/engine_statistics.hpp"
#include "romp/frame_logger.hpp"
#include "romp/progress.hpp"
#if ENABLE_CONSOLES
#include "romp/resource_manager_stats.hpp"
#include "romp/resource_statistics.hpp"
#endif // ENABLE_CONSOLES

#include "waypoint/chunk_navigator.hpp"

#include "moo/texture_manager.hpp"
#include "moo/texture_streaming_manager.hpp"

#include "moo/renderer.hpp"
#include "moo/debug_draw.hpp"

BW_BEGIN_NAMESPACE


DebugApp DebugApp::instance;

int DebugApp_token;


DebugApp::DebugApp() :
    pVersionInfo_( NULL ),
    pFrameRateGraph_( NULL ),
#if ENABLE_PYTHON_TELNET_SERVICE
    pPythonServer_( NULL ),
#endif /* ENABLE_PYTHON_TELNET_SERVICE */
    dRenderTime_( 0.f ),
    fps_( 0.f ),
    maxFps_( 0.f ),
    minFps_( 0.f ),
    fpsIndex_( 0 ),
    timeSinceFPSUpdate_( 1.f ),
    slowTime_( 1.f ),
    drawSpecialConsole_( false ),
    enableServers_( false ),
    shouldBreakOnCritical_( true ),
    shouldAddTimePrefix_( false )
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "Debug/App", NULL );
    ZeroMemory( fpsCache_, sizeof( fpsCache_ ) );
    ZeroMemory( dTimeCache_, sizeof( dTimeCache_ ) );
}


DebugApp::~DebugApp()
{
    BW_GUARD;
    // Check that fini was called.
#if ENABLE_PYTHON_TELNET_SERVICE
    MF_ASSERT_DEV( !pPythonServer_ );
#endif /* ENABLE_PYTHON_TELNET_SERVICE */
    MF_ASSERT_DEV( !pVersionInfo_ );
    MF_ASSERT_DEV( !pFrameRateGraph_ );
}

namespace
{
    BW::string getVersionString() { return BWVersion::versionString(); }
}

bool DebugApp::init()
{
    BW_GUARD;

    if (App::instance().isQuiting())
    {
        return false;
    }
    DataSectionPtr configSection = AppConfig::instance().pRoot();

    // 1. Version Info

    // query the version
    pVersionInfo_ = new VersionInfo();
    pVersionInfo_->queryAll();

    //retrieve host name
    char buf[256];
    if ( !gethostname( buf, 256 ) )
        hostName_ = buf;
    else
        hostName_ = "LocalHost";

    //retrieve device and driver info
    /*D3DEnum_DeviceInfo* device = NULL;
    D3DEnum_DriverInfo* driver = NULL;

    Moo::rc().getDeviceParams( &device, &driver );
    if ( driver != NULL )
    {
        driverDesc_ = driver->strDesc;
        driverName_ = driver->strName;
    }
    else
    {*/
        driverDesc_ = "unknown";
        driverName_ = "unknown";
    //}

    /*if ( device != NULL )
    {
        deviceName_ = device->strName;
    }
    else
    {*/
        deviceName_ = "unknown";
    //}

    MF_WATCH( "System/Network/Host", hostName_, Watcher::WT_READ_WRITE, "Server hostname" );

    MF_WATCH( "System/Operating System/Version", *pVersionInfo_, &VersionInfo::OSName, "Name of Operating System" );
    MF_WATCH( "System/Operating System/Major", *pVersionInfo_, &VersionInfo::OSMajor, "Major version number of Operating System" );
    MF_WATCH( "System/Operating System/Minor", *pVersionInfo_, &VersionInfo::OSMinor, "Minor version number of Operating System" );
    MF_WATCH( "System/Operating System/Pack", *pVersionInfo_, &VersionInfo::OSServicePack, "Installed service pack" );

    MF_WATCH( "System/DirectX/Version", *pVersionInfo_, &VersionInfo::DXName, "Version of installed DirectX" );
    MF_WATCH( "System/DirectX/Major", *pVersionInfo_, &VersionInfo::DXMajor, "Major version number of installed DirectX" );
    MF_WATCH( "System/DirectX/Minor", *pVersionInfo_, &VersionInfo::DXMinor, "Minor version number of installed DirectX" );
    MF_WATCH( "System/DirectX/Device", deviceName_, Watcher::WT_READ_WRITE, "DirectX device name" );
    MF_WATCH( "System/DirectX/Driver Name", driverName_, Watcher::WT_READ_WRITE, "DirectX driver name" );
    MF_WATCH( "System/DirectX/Driver Desc", driverDesc_, Watcher::WT_READ_WRITE, "DirectX driver description" );
    MF_WATCH( "System/Video Adapter/Driver", *pVersionInfo_, &VersionInfo::adapterDriver, "Name of video adapter driver" );
    MF_WATCH( "System/Video Adapter/Desc", *pVersionInfo_, &VersionInfo::adapterDesc, "Description of video adapter driver" );
    MF_WATCH( "System/Video Adapter/Major Version", *pVersionInfo_, &VersionInfo::adapterDriverMajorVer, "Major version number of video adapter" );
    MF_WATCH( "System/Video Adapter/Minor Version", *pVersionInfo_, &VersionInfo::adapterDriverMinorVer, "Minor version number of video adapter" );

    MF_WATCH( "System/Global Memory/Total Physical(K)", *pVersionInfo_, &VersionInfo::totalPhysicalMemory, "Total physical memory" );
    MF_WATCH( "System/Global Memory/Avail Physical(K)", *pVersionInfo_, &VersionInfo::availablePhysicalMemory, "Available physical memory" );
    MF_WATCH( "System/Global Memory/Total Virtual(K)", *pVersionInfo_, &VersionInfo::totalVirtualMemory, "Total virtual memory" );
    MF_WATCH( "System/Global Memory/Avail Virtual(K)", *pVersionInfo_, &VersionInfo::availableVirtualMemory, "Available virtual memory" );
    MF_WATCH( "System/Global Memory/Total Page File(K)", *pVersionInfo_, &VersionInfo::totalPagingFile, "Total page file memory" );
    MF_WATCH( "System/Global Memory/Avail Page File(K)", *pVersionInfo_, &VersionInfo::availablePagingFile, "Available page file memory" );

    MF_WATCH( "System/Process Memory/Page Faults", *pVersionInfo_, &VersionInfo::pageFaults, "Memory page faults" );
    MF_WATCH( "System/Process Memory/Peak Working Set", *pVersionInfo_, &VersionInfo::peakWorkingSet, "Peak working set size" );
    MF_WATCH( "System/Process Memory/Working Set", *pVersionInfo_, &VersionInfo::workingSet, "Current working set size" );
    MF_WATCH( "System/Process Memory/Quota Peaked Page Pool Usage", *pVersionInfo_, &VersionInfo::quotaPeakedPagePoolUsage, "Quota peak amount of paged pool usage" );
    MF_WATCH( "System/Process Memory/Quota Page Pool Usage", *pVersionInfo_, &VersionInfo::quotaPagePoolUsage, "Current quota amount of paged pool usage" );
    MF_WATCH( "System/Process Memory/Quota Peaked Nonpage Pool Usage", *pVersionInfo_, &VersionInfo::quotaPeakedNonPagePoolUsage, "Quota peak amount of non-paged pool usage" );
    MF_WATCH( "System/Process Memory/Quota Nonpage Pool Usage", *pVersionInfo_, &VersionInfo::quotaNonPagePoolUsage, "Current quota amount of non-paged pool usage" );
    MF_WATCH( "System/Process Memory/Peak Page File Usage", *pVersionInfo_, &VersionInfo::peakPageFileUsage, "Peak amount of page file space used" );
    MF_WATCH( "System/Process Memory/Page File Usage", *pVersionInfo_, &VersionInfo::pageFileUsage, "Current amount of page file space used" );

    static float cpuSpeed_ = float( stampsPerSecondD() / 1000000.0 );
    //
    MF_WATCH( "System/CPU/Speed", cpuSpeed_, Watcher::WT_READ_ONLY, "CPU Speed" );

#if ENABLE_WATCHERS
    //
    MF_WATCH<int32>( "Memory/UnclaimedSize", &memoryUnclaimed, NULL, "Amount of unclaimed memory" );

//  MF_WATCH( "Memory/Tale KB", &memoryAccountedFor );
    MF_WATCH<uint32>( "Memory/Used KB", &memUsed, NULL, "Amount of memory in use" );
#endif

    MF_WATCH( "Memory/TextureManagerReckoning", *Moo::TextureManager::instance(),
        &Moo::TextureManager::textureMemoryUsed, "Amount of texture being used by textures (total)" );
    MF_WATCH( "Memory/TextureManagerReckoningFrame", Moo::ManagedTexture::totalFrameTexmem_,
        Watcher::WT_READ_ONLY, "Amount of texture being used by textures (frame)" );

    const uint32 MB = 1024*1024;

    // 2. Misc watchers

    // Top Level Info.
    MF_WATCH( "Render/FPS",
        fps_,
        Watcher::WT_READ_ONLY,
        "Frames-per-second of the previous frame." );
    MF_WATCH( "Render/FPSAverage",
        fpsAverage_,
        Watcher::WT_READ_ONLY,
        "Frames-per-second running average for the last 50 frames." );
    MF_WATCH( "Render/FPS(max)",
        maxFps_,
        Watcher::WT_READ_ONLY,
        "Recorded maxmimum FPS" );
    MF_WATCH( "Render/FPS(min)",
        minFps_,
        Watcher::WT_READ_ONLY,
        "Recorded minimum FPS" );
#if 0 // TODO: Doesn't always exist. It should register itself...
    MF_WATCH( "System/Network/SendReportThreshold", 
        *ConnectionControl::serverConnection(), 
        MF_ACCESSORS( double, SmartServerConnection,
            sendTimeReportThreshold ) );
#endif

    MF_WATCH( "debug/filterThreshold",
        DebugFilter::instance(),
        MF_ACCESSORS( DebugMessagePriority, DebugFilter, filterThreshold ) );

    MF_WATCH( "Build/Version", &getVersionString );

    static const char * compileTime = App::instance().compileTime();

    MF_WATCH( "Build/Date", compileTime,
        Watcher::WT_READ_ONLY,
        "Build date of this executable." );

    MF_WATCH( "Build/Config", const_cast<char*&>(configString),
        Watcher::WT_READ_ONLY,
        "Build configuration of this executable." );

    MF_WATCH( "Client Settings/Slow time",
        slowTime_,
        Watcher::WT_READ_WRITE,
        "Time divisor.  Increase this value to slow down time." );

    MF_WATCH( "Client Settings/Consoles/Special",
        drawSpecialConsole_,
        Watcher::WT_READ_WRITE,
        "Toggle drawing of the special (debug) console." );

    // 4. Python server
#if ENABLE_PYTHON_TELNET_SERVICE
    MF_WATCH( "Debug/Python Server",
        *this,
        MF_ACCESSORS( bool, DebugApp, enableServers ),
        "If enabled the Python and keyboard servers will run on ports 50001 and 50002 respectively" );

    pPythonServer_ = new PythonServer();
    enableServers( configSection->readBool( "debug/enableServers", enableServers_ ) );
#endif

    // 5. Critical message callback (move much earlier?)
    shouldBreakOnCritical_ = configSection->readBool(
        "breakOnCritical", shouldBreakOnCritical_ );
    shouldAddTimePrefix_ = configSection->readBool(
        "addTimePrefix", shouldAddTimePrefix_ );
    DebugFilter::instance().hasDevelopmentAssertions(
        configSection->readBool( "hasDevelopmentAssertions",
        DebugFilter::instance().hasDevelopmentAssertions() ) );

    MF_WATCH( "Debug/Break on critical",
        shouldBreakOnCritical_,
        Watcher::WT_READ_WRITE,
        "If enabled, will display an error dialog on critical errors. "
        "Otherwise the application will attempt to continue running." );

    MF_WATCH( "Debug/Add time prefix",
        shouldAddTimePrefix_,
        Watcher::WT_READ_WRITE,
        "" );

    MF_WATCH( "Debug/Log slow frames",
        EngineStatistics::logSlowFrames_,
        Watcher::WT_READ_WRITE,
        "If enabled, slow frames are logged to a file so you can cross-check "
        "them with other debug output." );

#if ENABLE_CONSOLES
    // 6. Consoles
    ConsoleManager & mgr = ConsoleManager::instance();
    XConsole * pStatisticsConsole =
        new StatisticsConsole( &EngineStatistics::instance() );
    mgr.add( pStatisticsConsole,    "Statistics" );
    mgr.add( new ResourceUsageConsole( &ResourceStatistics::instance() ), "Special" );
#if ENABLE_WATCHERS
    mgr.add( new DebugConsole(),    "Watcher" );
#endif
    mgr.add( new HistogramConsole(),    "Histogram" );

    pFrameRateGraph_ = new FrameRateGraph();
#endif // ENABLE_CONSOLES

    #if ENABLE_DOG_WATCHERS
        FrameLogger::init();
    #endif // ENABLE_DOG_WATCHERS

    // 7. Watcher value specific settings
    DataSectionPtr pWatcherSection = configSection->openSection( "watcherValues" );
    if (pWatcherSection)
    {
        pWatcherSection->setWatcherValues();
    }

    DebugDraw::enabled( true );

    bool ret = DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);

    return ret;
}


void DebugApp::fini()
{
    BW_GUARD;
    bw_safe_delete(pFrameRateGraph_);
    bw_safe_delete(pVersionInfo_);

#if ENABLE_PYTHON_TELNET_SERVICE
        if (pPythonServer_)
        {
            enableServers( false );
            bw_safe_delete( pPythonServer_ );
        }
#endif /* ENABLE_PYTHON_TELNET_SERVICE */

#if ENABLE_CONSOLES
    // Cleanup the ResourceManagerStats
    ResourceManagerStats::fini();
#endif // ENABLE_CONSOLES
}


void DebugApp::tick( float /* dGameTime */, float dRenderTime )
{
    BW_GUARD_PROFILER( AppTick_Debug );
    dRenderTime_ = dRenderTime;

    if (fpsIndex_ >= 50)
        fpsIndex_ = 0;

    fpsCache_[ fpsIndex_ ] = 1.f / dRenderTime;
    dTimeCache_[ fpsIndex_ ] = dRenderTime;

    minFps_ = fpsCache_[0];
    maxFps_ = fpsCache_[0];

    float totalTimeInCache = 0.f;
    for (uint i = 1; i < 50; i++)
    {
        minFps_ = min( minFps_, fpsCache_[i] );
        maxFps_ = max( maxFps_, fpsCache_[i] );
        totalTimeInCache += dTimeCache_[i];
    }

    fpsIndex_++;

    timeSinceFPSUpdate_ += dRenderTime;
    if (timeSinceFPSUpdate_ >= 0.5f)
    {
        fps_ = 1.f / dRenderTime;
        fpsAverage_ = 50.f / totalTimeInCache;
        timeSinceFPSUpdate_ = 0;
    }

#if ENABLE_PYTHON_TELNET_SERVICE
    pPythonServer_->pollInput();
#endif /* ENABLE_PYTHON_TELNET_SERVICE */
    // A combined metric of total memory load and page file usage relative
    // to total physical memory. Used to trigger the memoryCritical callback.
    static bool first = true;
    if ( first && 
         ( (pVersionInfo_->memoryLoad() >= 99 &&
            pVersionInfo_->pageFileUsage() > (pVersionInfo_->totalPhysicalMemory()*0.7)) ||
            Moo::rc().memoryCritical() ) )
    {
        // TODO: want this to appear again later?? Only triggering once for now.
        first = false;
        WARNING_MSG("Memory load critical, adjust the detail settings.\n");
        App::instance().memoryCriticalCallback();
    }
}

static DogWatch g_watchDebugApp("DebugApp");

#ifdef CHUNKINTEST
/**
 * This class implements a collision callback that displays the triangles 
 * that the collision code intersects with.
 */
class DisplayCC : public CollisionCallback
{
private:
    virtual int operator()( const ChunkObstacle & obstacle,
        const WorldTriangle & triangle, float dist )
    {
        Moo::rc().push();
        Moo::rc().world( obstacle.transform_ );

        Moo::Colour yellow( 0x00ffff00 );
        Geometrics::drawLine( triangle.v0(), triangle.v1(), yellow );
        Geometrics::drawLine( triangle.v1(), triangle.v2(), yellow );
        Geometrics::drawLine( triangle.v2(), triangle.v0(), yellow );

        Moo::rc().pop();
        return COLLIDE_ALL;
    }
};
#endif


void DebugApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_Debug );
    GPU_PROFILER_SCOPE(DebugApp_draw);
    g_watchDebugApp.start();

#if ENABLE_BSP_MODEL_RENDERING
    ChunkBspHolder::drawBsps( Moo::rc() );
#endif // ENABLE_BSP_MODEL_RENDERING

    ChunkNavigator::drawDebug();

    AvatarDropFilter::drawDebugStuff();

    CameraApp::instance().clientCamera().drawDebugStuff();

    CameraApp::instance().entityPicker().drawDebugStuff();

    pFrameRateGraph_->graph( dRenderTime_ );

    PyModel::debugStuff( dRenderTime_ );

    CameraApp::instance().clientSpeedProvider().drawDebugStuff();

    //--
    Renderer::instance().pipeline()->drawDebugStuff();

    DebugDraw::draw();

    Moo::TextureManager::instance()->streamingManager()->drawDebug();

#ifdef CHUNKINTEST
    static DogWatch dwHullCollide( "HullCollide" );
    // test the collision scene (testing only!)
    Vector3 camPos = Moo::rc().invView().applyToOrigin();
    Vector3 camDir = Moo::rc().invView().applyToUnitAxisVector( 2 );
    camPos += camDir * Moo::rc().camera().nearPlane();
    DisplayCC dcc;

    dwHullCollide.start();
    Vector3 camDirProp(camDir.z,0,-camDir.x);
    WorldTriangle camTri(
        camPos - Moo::rc().invView().applyToUnitAxisVector( 0 ),
        camPos + Moo::rc().invView().applyToUnitAxisVector( 0 ),
        camPos + Moo::rc().invView().applyToUnitAxisVector( 1 ) );

    float res = ChunkManager::instance().space( 0 )->collide(
        camTri, camPos + camDir * 15, dcc );
    dwHullCollide.stop();

    char resBuf[256];
    bw_snprintf( resBuf, sizeof(resBuf), "Collision result: %f\n\n", res );
    g_specialConsoleString += resBuf;
#endif

#if ENABLE_CONSOLES
    // update our frame statistics
    EngineStatistics::instance().tick( dRenderTime_ );
#endif // ENABLE_CONSOLES

    g_watchDebugApp.stop();

#if ENABLE_CONSOLES
    // draw consoles
    static DogWatch consoleWatch( "Console" );
    consoleWatch.start();

    ConsoleManager::instance().draw( dRenderTime_ );

    // draw our special console too
    if (drawSpecialConsole_)
    {
        XConsole * specialConsole =
            ConsoleManager::instance().find( "Special" );
        specialConsole->clear();
        if (ChunkManager::s_specialConsoleString.empty())
        {
            specialConsole->setCursor( 0, 0 );
            specialConsole->print( "Special Console (empty)" );
        }
        else
        {
            specialConsole->setCursor( 0, 0 );
            specialConsole->print( ChunkManager::s_specialConsoleString );
        }
        specialConsole->draw( dRenderTime_ );
    }
    ChunkManager::s_specialConsoleString = "";

    consoleWatch.stop();
#endif // ENABLE_CONSOLES
}

#if ENABLE_PYTHON_TELNET_SERVICE
bool DebugApp::enableServers() const
{
    return enableServers_;
}

void DebugApp::enableServers( bool enable )
{
    if ( enable == enableServers_ )
    {
        // Don't do anything if no change is requested
        return;
    }

    enableServers_ = enable;

    if ( enableServers_ )
    {
        pPythonServer_->startup( ConnectionControl::dispatcher(), 50001 );
    }
    else
    {
        pPythonServer_->shutdown();
    }
}
#endif /* ENABLE_PYTHON_TELNET_SERVICE */

#if ENABLE_STACK_TRACKER
/**
 *  Returns StackTracker report to Python scripts
 */
PyObject* py_getCallStack( PyObject * args )
{
    char stack[ 2048 ];
    StackTracker::buildReport( stack, ARRAY_SIZE( stack ) );
    return PyString_FromString( stack );
}
PY_MODULE_FUNCTION( getCallStack, BigWorld )
#endif

BW_END_NAMESPACE

// debug_app.cpp

