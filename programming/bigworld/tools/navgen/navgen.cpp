#include "pch.hpp"

#include "asyn_msg.hpp"
#include "chunk_flooder.hpp"
#include "chunk_waypoint_generator.hpp"
#include "dlg_modeless_info.hpp"
#include "girth.hpp"
#include "resource.h"
#include "waypoint_annotator.hpp"
#include "navgen_udo.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_terrain.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk_scene_adapter/client_chunk_space_adapter.hpp"

#include "space/client_space.hpp"
#include "space/space_manager.hpp"
#include "space/deprecated_space_helpers.hpp"

#include "common/format.hpp"
#include "common/space_mgr.hpp"
#include "common/utilities.hpp"
#include "common/bwlockd_connection.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/cstdmf_init.hpp"
#include "cstdmf/debug_exception_filter.hpp"
#include "cstdmf/log_meta_data.hpp"
#include "cstdmf/log_msg.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "cstdmf/message_box.hpp"
#include "cstdmf/stack_tracker.hpp"
#include "cstdmf/timestamp.hpp"

#include "duplo/foot_print_renderer.hpp"

#include "entitydef/constants.hpp"

#include "input/input.hpp"

#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/mrt_support.hpp"
#include "moo/texture_manager.hpp"
#include "moo/renderer.hpp"
#include "moo/draw_context.hpp"

#include "particle/actions/particle_system_action.hpp"

#include "physics2/material_kinds.hpp"

#include "pyscript/py_import_paths.hpp"
#include "pyscript/py_callback.hpp"
#include "pyscript/script.hpp"
#include "pyscript/automation.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/data_section_cache.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/xml_section.hpp"

#include "romp/font.hpp"
#include "romp/font_manager.hpp"
#include "romp/lens_effect_manager.hpp"
#include "romp/texture_feeds.hpp"
#include "romp/time_of_day.hpp"
#include "romp/water.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "terrain/manager.hpp"
#include "terrain/terrain2/terrain_block2.hpp"
#include "terrain/terrain2/terrain_height_map2.hpp"

#include "waypoint_generator/chunk_view.hpp"
#include "waypoint_generator/waypoint_generator.hpp"

#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif

#include <GL/gl.h>
#include <commctrl.h>
#include <commdlg.h>
#include <fstream>
#include <sstream>
#include "cstdmf/bw_string.hpp"
#include <strstream>
#include <windows.h>


DECLARE_DEBUG_COMPONENT2( "NAVGen", 0 )

//These are used for Script::tick and the BigWorld.callback fn.
static double g_totalTime = 0.0;

void incrementTotalTime( float dTime )
{
    g_totalTime += (double)dTime;
}

double getTotalTime()
{       
    return g_totalTime;
}

namespace BW 
{ 
    extern int PyLogging_token;
}


BW_BEGIN_NAMESPACE

namespace Terrain
{
    class Manager;
}

int forcePyLogging_token = PyLogging_token;

static AutoConfigString s_LanguageFile( "system/language" );
static AutoConfigString s_engineConfigXML("system/engineConfigXML");

typedef std::auto_ptr< FontManager > FontManagerPtr;
static FontManagerPtr s_pFontManager;

typedef std::auto_ptr< TextureFeeds > TextureFeedsPtr;
static TextureFeedsPtr s_pTextureFeeds;

typedef std::auto_ptr< Terrain::Manager > TerrainManagerPtr;
static TerrainManagerPtr s_pTerrainManager;

typedef std::auto_ptr< LensEffectManager > LensEffectManagerPtr;
static LensEffectManagerPtr s_pLensEffectManager;

#define ZOOM_FACTOR         1.5f

namespace 
{
    LRESULT CALLBACK reannotatingWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
        BW_GUARD;

        return uMsg == WM_CLOSE ? 0 : DefWindowProc( hwnd, uMsg, wParam, lParam );
    }

    char * s_cmdLineBuffer = NULL;
    char ** s_argv = NULL;
}

class NavgenStatusWindow
{
    HWND hwnd_;
    BW::wstring action_;
    BW::wstring chunk_;
    int skip_;
    int processed_;
    int allProcessed_;
    DWORD start_;
public:
    bool create( HWND parent )
    {
        BW_GUARD;

        hwnd_ = CreateStatusWindow( WS_CHILD | WS_VISIBLE, L"", parent, 0);

        int partWidth[] = {
            150, // action: clearing/...
            320, // processed
            420, // skip
            540, // elapsed time
            640, // average time
            -1 };

        SendMessage( hwnd_, SB_SETPARTS, ARRAY_SIZE( partWidth ), (LPARAM)partWidth );
        return !!hwnd_;
    }
    void begin( const BW::wstring& action )
    {
        BW_GUARD;

        action_ = action;
        skip_ = 0;
        processed_ = 0;
        allProcessed_ = 0;
        start_ = GetTickCount();
        chunk_.clear();
        update();
    }
    void end()
    {
        BW_GUARD;

        BW::string st;
        bw_wtoacp( status(), st );

        if( !st.empty() )
        {
            INFO_MSG( st.c_str() );
        }

        action_.clear();
        update();
    }
    void skip( bool skipBecauseOfProcessed = false )
    {
        BW_GUARD;

        ++skip_;
        if( skipBecauseOfProcessed )
            ++allProcessed_;
        update();
    }
    void process( const BW::string& chunkName )
    {
        BW_GUARD;

        BW::wstring wchunkName;
        bw_utf8tow( chunkName, wchunkName );
        process( wchunkName );
    }
    void process( const BW::wstring& chunkName )
    {
        BW_GUARD;

        ++processed_;
        ++allProcessed_;
        chunk_ = chunkName;
        update();
    }
    BW::wstring status() const
    {
        BW_GUARD;

        if( action_.empty() )
            return L"";

        wchar_t buf[ 1024 ];
        DWORD average = ( GetTickCount() - start_ ) / ( processed_ ? processed_  : 1 );
        if( allProcessed_ == processed_ )
        {
            bw_snwprintf( buf, ARRAY_SIZE( buf ),
                L"%s finished with %d chunks processed, skipped %d chunks, takes %s, average %s.\n",
                action_.c_str(), allProcessed_, skip_, tickToTime( GetTickCount() - start_ ).c_str(),
                tickToTime( average ).c_str() );
        }
        else
        {
            bw_snwprintf( buf, ARRAY_SIZE( buf ),
                L"%s finished with %d chunks processed, %d new navmeshes were created, %d navmeshes were already up-to-date,\n"
                L"skipped %d chunks, takes %s, average %s.\n",
                action_.c_str(), allProcessed_, processed_, allProcessed_ - processed_, skip_, tickToTime( GetTickCount() - start_ ).c_str(),
                tickToTime( average ).c_str() );
        }

        return buf;
    }
    void update()
    {
        BW_GUARD;

        if( action_.empty() )
        {
            SendMessage( hwnd_, SB_SETTEXT, 0, (LPARAM)action_.c_str() );
            SendMessage( hwnd_, SB_SETTEXT, 1, (LPARAM)L"" );
            SendMessage( hwnd_, SB_SETTEXT, 2, (LPARAM)L"" );
            SendMessage( hwnd_, SB_SETTEXT, 3, (LPARAM)L"" );
            SendMessage( hwnd_, SB_SETTEXT, 4, (LPARAM)L"" );
        }
        else
        {
            if( chunk_.empty() )
                SendMessage( hwnd_, SB_SETTEXT, 0, (LPARAM)action_.c_str() );
            else
                SendMessage( hwnd_, SB_SETTEXT, 0, (LPARAM)( action_ + L": " + chunk_.substr( chunk_.size() - 9 ) ).c_str() );
            wchar_t buf[256];
            if( allProcessed_ == processed_ )
                bw_snwprintf(buf, ARRAY_SIZE(buf),  L"processed: %d", processed_ );
            else
                bw_snwprintf(buf, ARRAY_SIZE(buf),  L"processed: %d/%d", processed_, allProcessed_ );
            SendMessage( hwnd_, SB_SETTEXT, 1, (LPARAM)buf );
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"skipped: %d", skip_ );
            SendMessage( hwnd_, SB_SETTEXT, 2, (LPARAM)buf );
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"elapsed: %s", tickToTime( GetTickCount() - start_ ).c_str() );
            SendMessage( hwnd_, SB_SETTEXT, 3, (LPARAM)buf );
            DWORD average = ( GetTickCount() - start_ ) / ( processed_ ? processed_  : 1 );
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"average: %s", tickToTime( average ).c_str() );
            SendMessage( hwnd_, SB_SETTEXT, 4, (LPARAM)buf );
        }
    }
    BW::wstring tickToTime( DWORD tick ) const
    {
        BW_GUARD;

        static wchar_t time[32];// 00:00:00
        tick = tick / 1000;
        if( tick > 3600 )
        {
            bw_snwprintf( time, ARRAY_SIZE( time ), L"%d:%02d:%02d", tick / 3600, tick / 60 % 60, tick % 60 );
        }
        else if( tick > 60 )
        {
            bw_snwprintf( time, ARRAY_SIZE( time ), L"%d:%02d", tick / 60, tick % 60 );
        }
        else
        {
            bw_snwprintf( time, ARRAY_SIZE( time ), L"%d s", tick );
        }
        return time;
    }
    void setStatus( const BW::wstring& status )
    {
        BW_GUARD;

        SendMessage( hwnd_, SB_SETTEXT, 5, (LPARAM)status.c_str() );
        update();
    }
    operator HWND()
    {
        return hwnd_;
    }
}
g_statusWindow;

WaypointGenerator   g_wpg;
ChunkView           g_chunkView;
IWaypointView*      g_pView;
HWND                g_hWindow           = NULL;

std::auto_ptr<Renderer> g_pRenderer;

HWND                g_hMooWindow        = NULL;
HWND                g_hHelpWindow       = NULL;
HMENU               g_hMenu             = NULL;
HGLRC               g_glrc              = NULL;
HACCEL              g_accelerators      = NULL;
POINT               g_dragStart;
POINT               g_mooDragStart;
bool                g_dragging;
bool                g_annotate = false;
bool                g_mooDragging;
bool                g_viewAdjacencies;
bool                g_viewBSPNodes;
bool                g_viewPolygonArea;
bool                g_viewPolygonBorders;
int                 g_bspNodesDepth = 0;
long                g_statusWindowHeight = 0;

HWND                g_hInfoDialog;
HWND                g_hListView;
POINT               g_listViewOffset;

int                 g_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
int                 g_dz[8] = {1, 1, 0, -1, -1, -1, 0, 1};

float               g_virtualMinX = -100.0f;
float               g_virtualMaxX = 100.0f;
float               g_virtualMinY = -100.0f;
float               g_virtualMaxY = 100.0f;
float               g_centreX;
float               g_centreY;
float               g_radius;
float               g_radiusX;
float               g_radiusY;
float               g_viewX;
float               g_viewY;
int                 g_selectedIndex = -1;

BW::set<BW::string> g_chunkSet;

int                 g_selectedGirth = 0;
BW::map<float,Girth>  g_girthSpecs;          // value : width, height, depth from navgen_settings.xml.
                                              // this is a vector 'cause I'm a little worried about 
                                              // having a float as lookup value.
BW::vector<float>   g_girthsAlwaysGenerate;   // always generate girths with values in this list.
BW::vector<float>  g_girthsViewable;

BWLock::BWLockDConnection   g_conn;
BW::string          g_bigbangd;
BW::string          g_username;

bool                g_generating = false;

GeometryMapping     *g_mapping = NULL;
BW::set<Chunk *>   g_chunksLoaded;
BW::set<Chunk *>   g_calcChunksDone;
BW::set<Chunk *>   g_calcChunksNotDirty;

int                 g_processor = 1;

bool                g_writeTGAs = true;
bool                g_reannotation = false;

float               g_chunkLoadDistance = 500.0f;

Matrix              g_camMatrix;

/**
 *  Get/set parameters for the screen saver and power settings.
 */
const static std::pair<UINT, UINT> s_scrnSaverList[] = 
{
    std::make_pair<UINT, UINT>(SPI_GETLOWPOWERTIMEOUT  , SPI_SETLOWPOWERTIMEOUT  ),
    std::make_pair<UINT, UINT>(SPI_GETPOWEROFFTIMEOUT  , SPI_SETPOWEROFFTIMEOUT  ),
    std::make_pair<UINT, UINT>(SPI_GETSCREENSAVETIMEOUT, SPI_SETSCREENSAVETIMEOUT)
};
const static size_t s_scrnSaverListSz = sizeof(s_scrnSaverList)/sizeof(s_scrnSaverList[0]);

/**
 *  Guard class to keep a consitent value of g_generating.  It also disables
 *  the screen saver, power settings etc when calculating and restores the 
 *  values in the destructor.
 */
class NavGenGenerating
{
public:
    NavGenGenerating() 
    {
        BW_GUARD;

        oldGenerating_ = g_generating; 
        g_generating = true; 
        // Get the screen saver time out, the power settings etc and turn them
        // off:
        for (size_t i = 0; i < s_scrnSaverListSz; ++i)
        {
            SystemParametersInfo(s_scrnSaverList[i].first , 0, &scrnParams_[i], 0);
            SystemParametersInfo(s_scrnSaverList[i].second, 0, NULL           , 0);
        }    
    }

    ~NavGenGenerating() 
    {
        BW_GUARD;

        g_generating = oldGenerating_;
        // Restore the screen saver and power settings:
        for (size_t i = 0; i < s_scrnSaverListSz; ++i)
        {
            SystemParametersInfo(s_scrnSaverList[i].second, scrnParams_[i], NULL, 0);
        }
    }

private:
    bool        oldGenerating_;
    UINT        scrnParams_[s_scrnSaverListSz];
};

bool                g_pleaseShutdown = false;

BW::string          g_floodResultPath = "";     // where to put the flood result files

float               g_lastTick = 0.0f;

// space chooser stuffs
static BW::string g_currentSpace;
static BW::string g_currentNavmeshGenerator;
static HANDLE gSpaceLock = INVALID_HANDLE_VALUE;
bool changeSpace( const BW::string& space );
static SpaceNameManager *g_spaceManager = NULL;
SmartPointer<DataResource> g_navgenSettings;

namespace
{
    void refreshWindows( bool ignoreInput = true, bool ignoreMenu = false, bool canClose = true )
    {
        BW_GUARD;

        MSG msg;
        while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
        {
            if (msg.message == WM_QUIT)
            {
                g_pleaseShutdown = true;
                return;
            }
            if (ignoreInput)
            {
                if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
                {
                    continue;
                }
                if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
                {
                    continue;
                }
            }
            if (ignoreMenu)
            {
                if (msg.message == WM_COMMAND)
                {
                    continue;
                }
            }
            if (!canClose && msg.message == WM_CLOSE)
            {
                continue;
            }
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    bool isCorrectNavmeshGenerator()
    {
        return g_currentNavmeshGenerator.empty() || 
            g_currentNavmeshGenerator == "navgen";
    }

    bool checkAllowGenerate()
    {
        if (!isCorrectNavmeshGenerator())
        {
            MsgBox mb( L"NavGen",
                bw_utf8tow( "NavGen is in Read Only mode as this space is" 
                    " not configured to be generated using NavGen." ),
                L"OK" );
            mb.doModal( g_hWindow );
            return false;
        }
        else
        {
            return true;
        }
    }
}


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020a
#endif


/**
 *  Class that adds errors to the error log. Also turns on an error flag
 *  when an error is reported.
 */
static const int WM_SETSTATUSTEXT = WM_USER + 0x234;
class ErrorLogHandler:
    public AsyncMessage, public DebugMessageCallback
{
public:
    ErrorLogHandler()
        : error_( false )
    {
        BW_GUARD;

        DebugFilter::instance().addMessageCallback( this );
        print( "NavGen Run Started ...\n" );
        printMsg( (BW::wstring( L"Logging to : " ) + getLogFileName() + L"\n").c_str() );
    }
    ~ErrorLogHandler()
    {
        BW_GUARD;

        printMsg( L"NavGen Run Finished ...\n" );
        print();
        printMsg( L"\n\n\n\n" );
        if (DebugFilter::isCreated())
        {
            DebugFilter::instance().deleteMessageCallback( this );
        }
        
        for (BW::set<wchar_t*>::iterator message = unhandledMessages_.begin();
            message != unhandledMessages_.end(); ++message)
        {
            bw_free( *message );
        }
        
        unhandledMessages_.clear();
    }


    virtual bool handleMessage( DebugMessagePriority messagePriority,
        const char * /*pCategory*/, DebugMessageSource /*messageSource*/,
        const LogMetaData & /*metaData*/, const char * pFormat, va_list argPtr )
    {
        BW_GUARD;

        bool isLogMsg = messagePriority > MESSAGE_PRIORITY_WARNING;
        static const int MAX_MESSAGE_BUFFER = 10240;
        static char s1[ MAX_MESSAGE_BUFFER ];
        LogMsg::formatMessage( s1, MAX_MESSAGE_BUFFER, pFormat, argPtr );
        char s2[10240];

        if ( g_processor != 1 && isLogMsg )
        {        
            bw_snprintf( s2, ARRAY_SIZE(s2), "%d> %s\0", g_processor, s1);
        }
        else
        {
            strncpy( s2, s1, strlen( s1 ) + 1 );
        }

        BW::wstring ws2;
        bw_acptow( s2, ws2 );

        if (messagePriority > MESSAGE_PRIORITY_INFO)
        {
            bool severity = messagePriority > MESSAGE_PRIORITY_WARNING;
            reportMessage( ws2.c_str(),severity );
            error_ = error_ || messagePriority > MESSAGE_PRIORITY_WARNING;
        }
        else
        {
            BW::wstring message = BW::wstring( L"Generation Status: " ) + ws2;
            // sendMessage is given a pointer that must be freed, we become
            // the owner of the allocated memory. addMessage is used to track
            // these messages.
            wchar_t* sendMessage = bw_wcsdup( message.c_str() );
            addMessage( sendMessage );

            PostMessage( g_hWindow, WM_SETSTATUSTEXT, 0,
                (LPARAM) sendMessage );
        }
        return false;
    }

    bool getError() { return error_; }

    void resetError() { error_ = false; }

    void print( const BW::string& message = "", bool separator = true )
    {
        BW_GUARD;

        if (separator)
        {
            printMsg( L"---------------------------------------------------------------\n" );
        }

        if (!message.empty())
        {
            printMsg( bw_acptow( message ).c_str() );
        }
    }

    BW::wstring consumeMessage( wchar_t* message )
    {
        // consume a message sent via WM_SETSTATUSTEXT
        BW::wstring retMessage(message);
        unhandledMessagesMutex.grab();
        MF_ASSERT( unhandledMessages_.find(message) != unhandledMessages_.end() );
        unhandledMessages_.erase( message );
        bw_free( message );
        unhandledMessagesMutex.give();
        return retMessage;
    }

private:

    void addMessage( wchar_t* message )
    {
        // add a message sent via WM_SETSTATUSTEXT
        unhandledMessagesMutex.grab();
        MF_ASSERT( unhandledMessages_.find(message) == unhandledMessages_.end() );
        unhandledMessages_.insert( message );
        unhandledMessagesMutex.give();
    }

    bool error_;

    // Track the messages we have sent via WM_SETSTATUSTEXT
    // they might not get handled when the application quits 
    // so will leak memory.
    SimpleMutex unhandledMessagesMutex;
    BW::set<wchar_t*> unhandledMessages_;
}
ErrorLogHandler;


bool waitChunkLoaded( Chunk *chunk, bool neighbourhood );
bool isSetOfNavmeshVisible( int meshIndex );


struct ProgressMonitor: public WaypointGenerator::IProgress
{
    void onProgress(const wchar_t* phase, int curr)
    {
        BW_GUARD;

        wchar_t buf[256];
        bw_snwprintf(buf, ARRAY_SIZE(buf), L"Generating %s: %d", phase, curr);
        g_statusWindow.setStatus( buf );
    }
};

void setReady()
{
    BW_GUARD;

    g_statusWindow.setStatus( L"Ready." );
}

void setCalculating(size_t done, size_t todo, 
    const BW::string & operation = "Calculating")
{
    BW_GUARD;

    BW::string message;
    if (todo == 0)
        message = operation;
    else
        message = BW::string( sformat("{0} {1} of {2} chunks", operation, done, todo).c_str() );

    BW::wstring wmessage;
    bw_acptow( message, wmessage );
    g_statusWindow.setStatus( wmessage );
}

class ScopedSetReady
{
public:
    ~ScopedSetReady() { setReady(); }
};


void drawGrid()
{
    BW_GUARD;

    uint x, z, a;
    uint8 mask;

    glBegin(GL_LINES);

    for ( x = 0; x < g_pView->gridX(); ++x )
    {
        for ( z = 0; z < g_pView->gridZ(); ++z )
        {
            mask = g_pView->gridMask(x, z);

            for ( a = 0; a < 8; ++a )
            {
                if ( mask & (1 << a) )
                {
                    glVertex2f(GLfloat(x), GLfloat(z));
                    glVertex2f(GLfloat(x + g_dx[a]*0.3f), GLfloat(z + g_dz[a]*0.3f));
                }
            }
        }
    }

    glEnd();
}

void drawBSPNode( IWaypointView::BSPNodeInfo & bni )
{
    BW_GUARD;

    glBegin(GL_POLYGON);

    for ( uint v = 0; v < bni.boundary_.size(); ++v )
    {
        glVertex2f(bni.boundary_[v].x,bni.boundary_[v].y);
    }

    glEnd();
}

void drawBSPNodes()
{
    BW_GUARD;

    IWaypointView::BSPNodeInfo bni;

    bool lastWasWay = false;

    int cursor = 0;
    int cursorMax = 1 << g_bspNodesDepth;
    //dprintf( "depth %d cursorMax %d\n", g_bspNodesDepth, cursorMax );
    while ( cursor < cursorMax )
    {
        int depth = g_pView->getBSPNode( cursor, g_bspNodesDepth, bni );
        if ( depth == -1 ) break;
        cursor += 1 << depth;

        if ( !bni.internal_ && !bni.waypoint_ ) continue;
        if ( lastWasWay != bni.waypoint_ )
        {
            lastWasWay = bni.waypoint_;
            if (lastWasWay) glColor3f(0.0f, 0.4f, 0.7f);
            else glColor3f(0.0f, 0.7f, 0.2f);
        }

        drawBSPNode( bni );
    }
}

void drawPolygon(int index)
{
    BW_GUARD;

    int v, a, vertexCount = g_pView->getVertexCount( index );
    Vector2 v1;
    bool ac;

    if ( vertexCount )
    {
        glBegin(GL_POLYGON);

        for( v = 0; v < vertexCount; ++v )
        {
            g_pView->getVertex(index, v, v1, a, ac);
            glVertex2f(v1.x, v1.y);
        }

        glEnd();
    }
}

void drawPolygons()
{
    BW_GUARD;

    for( int i = 0; i < g_pView->getPolygonCount(); ++i )
    {
        if (!g_pView->equivGirth( i, g_girthsViewable[g_selectedGirth] )) continue;

        if (isSetOfNavmeshVisible( i ))
        {
            drawPolygon(i);
        }
    }
}

void drawStuff()
{
    BW_GUARD;

    if ( !g_pView )
    {
        glClearColor( 0.7f, 0.7f, 0.7f, 0.7f );
        glClear( GL_COLOR_BUFFER_BIT );
        glFlush();
        return;
    }

    if ( g_viewX < g_viewY )
    {
        g_radiusX = g_radius;
        g_radiusY = g_radius * g_viewY / g_viewX;
    }
    else
    {
        g_radiusY = g_radius;
        g_radiusX = g_radius * g_viewX / g_viewY;
    }

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(g_centreX - g_radiusX, g_centreX + g_radiusX,
            g_centreY - g_radiusY, g_centreY + g_radiusY,
            -1.0f, 1.0f);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear( GL_COLOR_BUFFER_BIT );

    if ( g_viewBSPNodes )
    {
        // First the BSP nodes
        glColor3f(0.0f, 0.7f, 0.2f);
        glLineWidth(2.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawBSPNodes();
    }

    if ( g_viewPolygonArea )
    {
        // Draw filled polygons
        glColor3f(0.0f, 0.4f, 0.7f);
        glLineWidth(2.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawPolygons();
    }

    if ( g_selectedIndex >= 0 && g_selectedIndex < g_pView->getPolygonCount() )
    {
        // Draw the selected polygon
        glColor3f(1.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawPolygon(g_selectedIndex);
    }

    if ( g_viewAdjacencies )
    {
        // Now the adjacency grid
        glColor3f(1.0f, 1.0f, 1.0f);
        glLineWidth(1.0f);
        drawGrid();
    }

    if ( g_viewPolygonBorders )
    {
        // Now polygon outlines
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(2.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawPolygons();
    }

    glFlush();
}


void updateTitle(const BW::string & spaceName)
{
    BW_GUARD;

    BW::wstring windowTitle;
    if (spaceName.empty())
    {
        windowTitle = L"NavPoly Generator";
    }
    else
    {
        bw_utf8tow( spaceName, windowTitle );
        windowTitle += L" - NavPoly Generator";
    }

#ifdef _DEBUG
    windowTitle += L" (Debug)";
#endif

    SetWindowText(g_hWindow,windowTitle.c_str());
    InvalidateRect(g_hWindow, NULL, FALSE);
}


// functions used to manage view->set menu
void populateViewSetMenu()
{
    if (HMENU menu = GetSubMenu( GetMenu( g_hWindow ), 1 )) // view)
    {
        if (g_pView)
        {
            EnableMenuItem( menu, 0, MF_BYPOSITION | MF_ENABLED );
            menu = GetSubMenu( menu, 0 ); // sets

            while (GetMenuItemCount( menu ) > 2)
            {
                DeleteMenu( menu, GetMenuItemCount( menu ) - 1, MF_BYPOSITION );
            }

            AppendMenu( menu, MF_SEPARATOR, 0, NULL );

            wchar_t s[1024];

            for (int i = 0; i < g_pView->getSets(); ++i)
            {
                bw_snwprintf( s, sizeof( s ) / sizeof( *s ), L"%d", i );

                AppendMenu( menu, MF_CHECKED | MF_STRING, ID_VIEW_SET_NONE + i + 1, s );
            }
        }
        else
        {
            EnableMenuItem( menu, 0, MF_BYPOSITION | MF_GRAYED );
        }
    }
}


void updateViewSetMenu( bool setAll )
{
    if (HMENU menu = GetSubMenu( GetMenu( g_hWindow ), 1 )) // view
    {
        menu = GetSubMenu( menu, 0 ); // sets

        if (g_pView)
        {
            for (int i = 0; i < g_pView->getSets(); ++i)
            {
                CheckMenuItem( menu, i + 3,
                    MF_BYPOSITION | ( setAll ? MF_CHECKED : MF_UNCHECKED ) );
            }
        }
    }
}


void toggleVisibleSet( int index )
{
    HMENU menu = GetSubMenu( GetMenu( g_hWindow ), 1 ); // view


    if (menu && g_pView)
    {
        menu = GetSubMenu( menu, 0 ); // sets

        BW::vector<int> result;

        bool checked = !( GetMenuState( menu, index + 3, MF_BYPOSITION ) & MF_CHECKED );

        CheckMenuItem( menu, index + 3, MF_BYPOSITION |
            ( checked ? MF_CHECKED : MF_UNCHECKED ) );
    }
}


bool isSetOfNavmeshVisible( int meshIndex )
{
    if (g_pView)
    {
        if (HMENU menu = GetSubMenu( GetMenu( g_hWindow ), 1 )) // view
        {
            int set = g_pView->getSet( meshIndex );

            menu = GetSubMenu( menu, 0 ); // sets

            return !!(GetMenuState( menu, set + 3, MF_BYPOSITION ) & MF_CHECKED);
        }
    }

    return false;
}


void doFileOpen( const char * inputFileName = NULL, Chunk * pChunk = NULL )
{
    BW_GUARD;

    wchar_t szFile[MAX_PATH];
    size_t len;

    if ( inputFileName == NULL )
    {
        OPENFILENAME ofn;

        szFile[0] = 0;
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = g_hWindow;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = ARRAY_SIZE(szFile);
        ofn.lpstrFilter = L"Adjacency Maps\0*.tga\0Chunk Files\0*.chunk\0All\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Display the Open dialog box.

        if(!GetOpenFileName(&ofn))
            return;
    }
    else
    {
        size_t srcLen = strlen( inputFileName );
        bw_utf8tow( inputFileName, srcLen, szFile, ARRAY_SIZE( szFile ) );
        szFile[ std::min( srcLen, ARRAY_SIZE( szFile ) ) ] = L'\0';
    }

    len = wcslen(szFile);
    BW::string nfilename;
    bw_wtoutf8( szFile, nfilename );

    if( len > 4 && _wcsnicmp(szFile + len - 4, L".tga", 4) == 0 )
    {
        // load TGA file.
        // TODO:UNICODE: use utf8 
        if(!g_wpg.readTGA(nfilename.c_str()))
            return;

        ProgressMonitor pm;
        g_wpg.setProgressMonitor(&pm);
        SetCursor(LoadCursor(NULL, IDC_WAIT));
        g_wpg.generate();
        //g_wpg.determineOneWayAdjacencies();
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        g_wpg.setProgressMonitor(NULL);
        setReady();
        g_pView = &g_wpg;
        populateViewSetMenu();
    }
    else
    {
        // if loading a chunk file, must have reference to camera chunk in
        // order to do local -> global coord transform.
        if (pChunk == NULL) 
        {
            WARNING_MSG( "doFileOpen: pChunk == NULL" );
            return;
        }
        MF_ASSERT( BWResolver::resolveFilename( pChunk->resourceID() ) ==
            nfilename );

        // load chunk file.
        if (!g_chunkView.load( pChunk ))
            return;

        g_pView = &g_chunkView;
        populateViewSetMenu();
    }

    g_virtualMinX = 0;
    g_virtualMinY = 0;
    g_virtualMaxX = float( g_pView->gridX() );
    g_virtualMaxY = float( g_pView->gridZ() );

    float width = g_virtualMaxX - g_virtualMinX;
    float height = g_virtualMaxY - g_virtualMinY;

    g_centreX = g_virtualMinX + width / 2;
    g_centreY = g_virtualMinY + height / 2;
    g_radius = width > height ? width / 2 : height / 2;
}

/**
 *  Helper class to traverse a chunk space.  We do this by getting all
 *  *.chunk files in the space's directory (and subdirectories).
 */
class ChunkSpaceTraverser
{
public:
    ChunkSpaceTraverser();

    Chunk * chunk() const;
    bool next();
    bool done() const;
    size_t numDone() const;
    size_t numTodo() const;

protected:
    void getChunk();

private:
    Chunk                           *chunk_;
    BW::set<BW::string>           chunkNames_;
    BW::set<BW::string>::iterator it_;
    size_t                          idx_;
};


ChunkSpaceTraverser::ChunkSpaceTraverser() :
    chunk_(NULL),
    idx_(0)
{
    BW_GUARD;

    if (g_mapping != NULL)
    {
        ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
        chunkNames_ = g_mapping->gatherInternalChunks();

        for( int i = g_mapping->minGridX(); i <= g_mapping->maxGridX(); ++i )
            for( int j = g_mapping->minGridY(); j <= g_mapping->maxGridY(); ++j )
                chunkNames_.insert( g_mapping->outsideChunkIdentifier( i, j ) );

        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    it_ = chunkNames_.begin();
    getChunk();
}

Chunk *ChunkSpaceTraverser::chunk() const
{ 
    return chunk_; 
}

bool ChunkSpaceTraverser::next()
{
    BW_GUARD;

    if (it_ == chunkNames_.end())
        return false;

    if (chunk_ != NULL)
        g_chunksLoaded.insert(chunk_);

    ++it_; 
    ++idx_;
    getChunk();

    return chunk_ != NULL;
}

bool ChunkSpaceTraverser::done() const
{
    return it_ == chunkNames_.end();
}

size_t ChunkSpaceTraverser::numDone() const
{
    return idx_;
}

size_t ChunkSpaceTraverser::numTodo() const
{
    return chunkNames_.size();
}

void ChunkSpaceTraverser::getChunk()
{
    BW_GUARD;

    // If already iterated through the list then do nothing.
    if (it_ == chunkNames_.end())
    {
        chunk_ = NULL;
        return;
    }

    // Load the chunk.    
    BW::string chunkName = *it_;    
    chunk_ = ChunkManager::instance().findChunkByName(chunkName, g_mapping);
}

Chunk *             g_currentChunk = NULL;
LRESULT             g_totalComputers = 1;
LRESULT             g_myIndex = 0;
int                 g_currentChunkStatus;
int                 g_maxFloodPoints;
int                 g_curFloodPoints;

static const int    s_chunkLoadRetryTicks  = 60000;
static const int    s_chunkLoadSleep    =  100;


void processHarmlessMessages()
{
    BW_GUARD;

    if ( g_pleaseShutdown )
        return;

    MSG msg;
    while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ))
    {
        if (!GetMessage(&msg, NULL, 0, 0)) break;
        if (!IsDialogMessage(g_hInfoDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


void drawGenerateAllProgress()
{
    BW_GUARD;

    if (!g_currentChunk)
    {
        return;
    }

    HDC hdc;
    PAINTSTRUCT ps;
    InvalidateRect(g_hWindow, NULL, FALSE);
    hdc = BeginPaint(g_hWindow, &ps);

    ChunkSpace * pSpace = g_currentChunk->space();

    // set up gl
    Vector3 viewCent = g_currentChunk->centre();
    Vector3 viewSize( 6000.f * g_viewX / g_viewY, 0.f, 6000.f );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewCent.x - viewSize.x/2, viewCent.x + viewSize.x/2,
            viewCent.z - viewSize.z/2, viewCent.z + viewSize.z/2,
            -1.0f, 1.0f);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(1.0f);

    ChunkMap & chunks = pSpace->chunks();

    // Draw the main grid of the space
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(0.2f, 0.2f, 0.2f);
    BoundingBox spacebb = pSpace->gridBounds();
    float gridSize = pSpace->gridSize();
    for (float x = spacebb.minBounds().x; x <= spacebb.maxBounds().x; x += gridSize)
    {
        glBegin(GL_LINES);
        glVertex2f(x, spacebb.minBounds().z);
        glVertex2f(x, spacebb.maxBounds().z);
        glEnd();
    }
    for (float z = spacebb.minBounds().z; z <= spacebb.maxBounds().z; z += gridSize)
    {
        glBegin(GL_LINES);
        glVertex2f(spacebb.minBounds().x, z);
        glVertex2f(spacebb.maxBounds().x, z);
        glEnd();
    }

    // Now draw all the undone chunks in the set
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(0.1f, 0.1f, 0.1f);
    for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
        BW::vector<Chunk*>& chunks = it->second;

        BW::vector<Chunk*>::iterator j = chunks.begin();
        for (; j != chunks.end(); ++j)
        {
            Chunk * pChunk = *j;

            if ( !pChunk->loaded() )
                continue;

            const BoundingBox & bb = pChunk->boundingBox();

            bool doneIt = (g_chunksLoaded.find( pChunk ) != g_chunksLoaded.end());

            if (!doneIt)
            {
                glBegin(GL_POLYGON);
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
                glEnd();
            }
        }
    }

    // draw done chunks
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(0.5f, 0.5f, 0.5f);
    for (ChunkMap::iterator it = chunks.begin(); it != chunks.end(); it++)
    {
        BW::vector<Chunk*>& chunks = it->second;

        BW::vector<Chunk*>::iterator j = chunks.begin();
        for (; j != chunks.end(); ++j)
        {
            Chunk * pChunk = *j;

            if ( !pChunk->loaded() )
                continue;

            const BoundingBox & bb = pChunk->boundingBox();

            bool doneIt = (g_chunksLoaded.find( pChunk ) != g_chunksLoaded.end());

            if (doneIt)
            {
                bool calced   = g_calcChunksDone.find( pChunk ) != g_calcChunksDone.end();
                bool notDirty = g_calcChunksNotDirty.find( pChunk ) != g_calcChunksNotDirty.end();

                if (!notDirty&&!calced)
                {
                    glColor3f(0.5f, 0.5f, 0.5f);    // mid-grey

                    glBegin(GL_POLYGON);
                    glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
                    glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
                    glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
                    glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
                    glEnd();
                }
            }
        }
    }

    BW::set<Chunk*> chunkSet( g_calcChunksDone );
    chunkSet.insert( g_calcChunksNotDirty.begin(), g_calcChunksNotDirty.end() );
    for( BW::set<Chunk*>::iterator it = chunkSet.begin(); it != chunkSet.end(); ++it )
    {
        const BoundingBox & bb = (*it)->boundingBox();

        bool notDirty = g_calcChunksNotDirty.find( *it ) != g_calcChunksNotDirty.end();

        if (notDirty)
            glColor3f(1.0f, 0.6f, 0.0f);    // orange
        else
            glColor3f(0.0f, 1.0f, 0.0f);    // green

        glBegin(GL_POLYGON);
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
        glEnd();
    }

    // now draw lines around them
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for ( ChunkMap::iterator it = chunks.begin(); it != chunks.end(); it++ )
    {
        BW::vector<Chunk*>& chunks = it->second;

        BW::vector<Chunk*>::iterator j = chunks.begin();
        for (; j != chunks.end(); ++j)
        {
            Chunk * pChunk = *j;

            if ( !pChunk->loaded() )
                continue;

            const BoundingBox & bb = pChunk->boundingBox();

            bool doneIt = (g_chunksLoaded.find( pChunk ) != g_chunksLoaded.end());

            if ( doneIt )
            {
                glColor3f(0.5f, 0.5f, 0.5f);

                glBegin(GL_POLYGON);
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
                glEnd();
            }
            else
            {
                glColor3f(1.0f, 1.0f, 1.0f);

                glBegin(GL_POLYGON);
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
                glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
                glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
                glEnd();
            }
        }
    }

    for( BW::set<Chunk*>::iterator it = chunkSet.begin(); it != chunkSet.end(); ++it )
    {
        const BoundingBox & bb = (*it)->boundingBox();

        glColor3f(0.5f, 0.5f, 0.5f);

        glBegin(GL_POLYGON);
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
        glEnd();

        if( !(*it)->loaded() )
        {
            glBegin(GL_POLYGON);
            glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
            glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
            glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
            glEnd();
            glBegin(GL_POLYGON);
            glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
            glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
            glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
            glEnd();
        }
    }

    // now draw the current chunk
    if ( g_currentChunk->loaded() )
    {
        const BoundingBox & bb = g_currentChunk->boundingBox();

        switch (g_currentChunkStatus)
        {
        case 0:     // loading - cyan
            glColor3f(0.0f, 1.0f, 1.0f);
            break;
        case 1:     // flooding - blue on black
            glColor3f(0.0f, 0.0f, 0.0f);
            break;
        case 2:     // generating - yellow
            glColor3f(1.0f, 1.0f, 0.0f);
            break;
        case 3:     // outputting - green
            glColor3f(0.0f, 1.0f, 0.0f);
            break;
        case 4:     // unmodified - orange
            glColor3f(1.0f, 0.6f, 0.0f);
            break;
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_POLYGON);
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
        glEnd();

        // if flooding draw as progress bar
        if (g_currentChunkStatus == 1)
        {
            float prop = float(g_curFloodPoints)/float(g_maxFloodPoints);
            if (prop > 1.f) prop = 1.f;
            float zmax = bb.minBounds().z * (1.f-prop) + bb.maxBounds().z * prop;

            glColor3f(0.1f, 0.1f, 1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBegin(GL_POLYGON);
            glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
            glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
            glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(zmax));
            glVertex2f(GLfloat(bb.minBounds().x), GLfloat(zmax));
            glEnd();
        }

        // and outline it in red
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin(GL_POLYGON);
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.minBounds().z));
        glVertex2f(GLfloat(bb.maxBounds().x), GLfloat(bb.maxBounds().z));
        glVertex2f(GLfloat(bb.minBounds().x), GLfloat(bb.maxBounds().z));
        glEnd();
    }

    glFlush();

    SwapBuffers(hdc);
    EndPaint(g_hWindow, &ps);
}

void updateMoo( float dTime, bool allowMovement = true );
void drawMoo(float dTime = 0.0f);

bool floodProgressCallback( int npoints )
{
    BW_GUARD;

    g_curFloodPoints = npoints;
    processHarmlessMessages();
    if ( g_pleaseShutdown )
    {
        return true;
    }
    drawGenerateAllProgress();

    return false;
}

int worldToGridCoord( float w, float gridSize )
{
    BW_GUARD;

    int g = int(w / gridSize);

    if ( w < 0.f )
        g--;

    return g;
}

bool isChunkLocked( Chunk * chunk )
{
    BW_GUARD;

    if( !g_chunkSet.empty() )
    {
        if( g_chunkSet.find( chunk->identifier() ) == g_chunkSet.end() )
            return false;
    }
    if (!g_conn.enabled())
        return true;

    if (!g_conn.connected())
        return false;

    // Use bb.centre, as the chunk may not be bound, which means its own
    // centre won't be valid
    Vector3 centre = chunk->boundingBox().centre();

    // ok, we've got got a coord between minX and maxX, it now needs to be 0 based
    ChunkSpace * space = chunk->space();
    float gridSize = space->gridSize();

    int gridX = worldToGridCoord( centre.x, gridSize );
    int gridY = worldToGridCoord( centre.z, gridSize );

    int w = space->maxGridX() - space->minGridX() + 1;
    int h = space->maxGridY() - space->minGridY() + 1;

    if (gridX < space->minGridX() || gridX > space->maxGridX() ||
        gridY < space->minGridY() || gridY > space->maxGridY())
    {
        ERROR_MSG( "isChunkLocked: chunk outside known grid "
            "(coords are %d, %d)\n", gridX, gridY );
        return false;
    }

    return g_conn.isLockedByMe( gridX, gridY );
}

BW::vector<float> compileGirthsList( Chunk * pChunk )
{
    BW_GUARD;

    // determine if we need to generate and wide girth nav polys for this chunk.

    BW::vector<float> girthsReturn = g_girthsAlwaysGenerate;

    BoundingBox cbb = pChunk->boundingBox();
    Vector3 mn = cbb.minBounds();
    Vector3 mx = cbb.maxBounds();

    NavGenUDO::GirthSeeds::iterator it = NavGenUDO::s_girthSeeds.begin();
    while (it != NavGenUDO::s_girthSeeds.end())
    {
        float girth = it->second.girth;
        float range = it->second.generateRange;

        const Vector3 & point = it->second.position;
        float x1 = point.x - range / 2.0f;
        float y1 = point.y - range / 2.0f;
        float z1 = point.z - range / 2.0f;
        float x2 = point.x + range / 2.0f;
        float y2 = point.y + range / 2.0f;
        float z2 = point.z + range / 2.0f;

        BoundingBox npBB( Vector3( x1, y1, z1 ), Vector3( x2, y2, z2 ) );

        // check to see if chunk boundary intersects with girth UDO bounding box.
        if ( npBB.intersects( pChunk->boundingBox() ) )
        {
            // if it does, is this girth already in the list? 
            int k;
            for (k=0; k<(int)girthsReturn.size(); ++k ) 
            {
                if (girth==girthsReturn[k])
                {
                    break;
                }
            }

            // no, then add it.
            if (k == girthsReturn.size() )
            {
                girthsReturn.push_back(girth);
            }

        }
        ++it;
    }

    INFO_MSG( "girths list size: %d\n", girthsReturn.size() );

    return girthsReturn;
}

class ScopedMenuDisabler
{
    HMENU menu_;
    int itemIndex_;
    bool byCommand_;
public:
    ScopedMenuDisabler( HMENU menu, int itemIndex, bool byCommand = true )
        :menu_( menu ), itemIndex_( itemIndex ), byCommand_( byCommand )
    {
        BW_GUARD;

        if (byCommand)
        {
            EnableMenuItem( menu_, itemIndex_, MF_BYCOMMAND | MF_GRAYED );
        }
        else
        {
            EnableMenuItem( menu_, itemIndex_, MF_BYPOSITION | MF_GRAYED );
        }
    }
    ~ScopedMenuDisabler()
    {
        BW_GUARD;

        if (byCommand_)
        {
            EnableMenuItem( menu_, itemIndex_, MF_BYCOMMAND | MF_ENABLED );
        }
        else
        {
            EnableMenuItem( menu_, itemIndex_, MF_BYPOSITION | MF_ENABLED );
        }
    }
};

class NavgenScopedMenuDisabler
{
    ScopedMenuDisabler openSpace_;
    ScopedMenuDisabler open_;
    ScopedMenuDisabler removeAll_;
    ScopedMenuDisabler generateAll_;
    ScopedMenuDisabler generateAllOverwrite_;
    ScopedMenuDisabler clusterGenerate_;

    ScopedMenuDisabler view_;
    ScopedMenuDisabler zoom_;
    ScopedMenuDisabler chunk_;

    bool showingMoo_;
public:
    NavgenScopedMenuDisabler() :
        openSpace_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_OPENSPACE ),
        open_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_OPEN1 ),
        removeAll_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_CLEAR_ALL ),
        generateAll_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_GENERATE_ALL ),
        generateAllOverwrite_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_GENERATE_ALL_OVERWRITE ),
        clusterGenerate_( GetSubMenu( GetMenu( g_hWindow ), 0 ), ID_FILE_CLUSTERGENERATE ),
        view_( GetMenu( g_hWindow ), 1, false ),
        zoom_( GetMenu( g_hWindow ), 2, false ),
        chunk_( GetMenu( g_hWindow ), 3, false ),
        showingMoo_( false )
    {
        BW_GUARD;

        Moo::ShadowManager* pShadowManager = 
            Renderer::instance().pipeline()->dynamicShadow();
        if (pShadowManager)
        {
            pShadowManager->set_semiEnabled( false );
        }

        disableMooWindowAndMenuItem();
        DrawMenuBar( g_hWindow );
    }
    ~NavgenScopedMenuDisabler()
    {
        BW_GUARD;

        enableMooWindowAndMenuItem();
        DrawMenuBar( g_hWindow );

        Moo::ShadowManager* pShadowManager = 
            Renderer::instance().pipeline()->dynamicShadow();
        if (pShadowManager)
        {
            pShadowManager->set_semiEnabled( true );
        }
    }

    /**
     *  Hide the Moo window so that we are not rendering, disable the render
     *  scene menu option.
     */
    void disableMooWindowAndMenuItem()
    {
        BW_GUARD;

        // Save current state
        showingMoo_ = isMooWindowShowing();

        // Hide if the window was visible
        if (showingMoo_)
        {
            ShowWindow( g_hMooWindow, SW_HIDE );
        }
        if (HMENU menu = GetMenu( g_hWindow ))
        {
            EnableMenuItem(
                menu, ID_VIEW_RENDEREDSCENE, MF_BYCOMMAND | MF_GRAYED );
        }
    }


    /**
     *  Restore the Moo window for rendering the scene, enable the render scene
     *  menu option.
     */
    void enableMooWindowAndMenuItem()
    {
        BW_GUARD;

        // Restore state
        if (showingMoo_ && !g_pleaseShutdown)
        {
            ShowWindow( g_hMooWindow, SW_SHOW );
        }
        if (HMENU menu = GetMenu( g_hWindow ))
        {
            EnableMenuItem(
                menu, ID_VIEW_RENDEREDSCENE, MF_BYCOMMAND | MF_ENABLED );
        }
    }

    /**
     *  Check if the render scene window is currently displayed.
     *  @return true if the Moo window is displayed.
     */
    static bool isMooWindowShowing()
    {
        BW_GUARD;
        const LONG styleBits = GetWindowLong( g_hMooWindow, GWL_STYLE );
        return ( styleBits & WS_VISIBLE ) > 0;
    }

};

// progress
bool generateOneProgress( int npoints )
{
    BW_GUARD;

    processHarmlessMessages();
    if ( g_pleaseShutdown )
    {
        return true;
    }

    return false;
}


// generate now calculates all girths required for the chunk, not just the selected one.
void doGenerate( Chunk * pChunk )
{
    BW_GUARD;

    if (!checkAllowGenerate())
    {
        return;
    }

    NavGenGenerating generating;

    NavgenScopedMenuDisabler md;

    if (!isChunkLocked( pChunk ))
    {
        MessageBox( g_hWindow,
            L"Cannot generate navmesh for this chunk because it is"
            L" not locked in bwlockd.",
            L"NavGen - Chunk not locked",
            MB_ICONWARNING | MB_OK | MB_APPLMODAL );
        return;
    }

    if (!waitChunkLoaded( pChunk, true ))
    {
        MessageBox( g_hWindow,
            L"Cannot load chunk and its neighbour for navmesh generation",
            L"NavPoly - Warnings/Errors occurred",
            MB_ICONWARNING | MB_OK | MB_APPLMODAL );

        g_currentChunk = NULL;
        return;
    }
    g_currentChunk = NULL;

    ModelessInfoDialog dlg( g_hWindow, L"Please Wait",
        L"Please wait while NavGen generates the navigation mesh.\n\n"
        L"This process can take several minutes, during which time the program may appear unresponsive.\n\n"
        L"Progress info is displayed in the status bar.",
        false /*okButton*/ );

    ChunkWaypointGenerator cwg( pChunk, g_floodResultPath );

    BW::vector<float> girthsToCalculate = compileGirthsList( pChunk );

    // find the chunk's section
    DataSectionPtr pChunkSect = BWResource::openSection(
        pChunk->resourceID() );

    ErrorLogHandler.resetError();
    ErrorLogHandler.print( "Start Generate Chunk: " + pChunk->identifier() + "\n" );

    // remove any existing waypoint sets (legacy) and navPolySets
    for ( int i = 0; i < pChunkSect->countChildren(); ++i )
    {
        DataSectionPtr ds = pChunkSect->openChild( i );
        if ( !((ds->sectionName() == "waypointSet") || 
            (ds->sectionName() == "navPolySet")) ) continue;
        pChunkSect->delChild( ds );
        --i;
    }

    for ( uint gi = 0; gi < girthsToCalculate.size(); ++gi )
    {
        if ( g_pleaseShutdown )
            return;

        Girth gSpec = g_girthSpecs.find( girthsToCalculate[gi] )->second;
        INFO_MSG( "Generating navPolys for girth size: %f (w: %f  h: %f  d: %f)\n",
            girthsToCalculate[gi], gSpec.width(), gSpec.height(), gSpec.depth() );

        cwg.flood( generateOneProgress, gSpec, g_writeTGAs );
        if ( g_pleaseShutdown )
            return;
        cwg.generate( g_annotate, gSpec );
        cwg.output( girthsToCalculate[gi], gi == 0 );
    }
    cwg.outputDirtyFlag( false );

    ErrorLogHandler.print( "Done\n", false );

    if ( ErrorLogHandler.getError() )
    {
        // errors occurred while processing the chunk. Display a message.
        MessageBox( g_hWindow,
            L"There were warnings/errors encountered during the process "
            L"that may or may not have affected the process region.\n"
            L"The chunk might still be missing navigation information.\n"
            L"Please check the errors log window for more information.",
            L"NavPoly - Warnings/Errors occurred",
            MB_ICONWARNING | MB_OK | MB_APPLMODAL );
    }
}

unsigned int hash( const char* str )
{
    BW_GUARD;

    static unsigned char hash[ 256 ];
    static bool inithash = true;
    if( inithash )
    {
        inithash = false;
        for( int i = 0; i < 256; ++i )
            hash[ i ] = i;
        int k = 7;
        for( int j = 0; j < 4; ++j )
            for( int i = 0; i < 256; ++i )
            {
                unsigned char s = hash[ i ];
                k = ( k + s ) % 256;
                hash[ i ] = hash[ k ];
                hash[ k ] = s;
            }
    }
    unsigned char result = ( 123 + strlen( str ) ) % 256;
    for( unsigned int i = 0; i < strlen( str ); ++i )
    {
        result = ( result + str[ i ] ) % 256;
        result = hash[ result ];
    }
    return result;
}


bool waitChunkLoaded(Chunk *chunk, bool neighbourhood)
{
    BW_GUARD;

    g_currentChunk = NULL;

    if (chunk == NULL)
    {
        return false;
    }

    if (!chunk->loaded())
    {
        ChunkManager::instance().loadChunkNow( chunk->identifier(), g_mapping );
        ChunkManager::instance().checkLoadingChunks();
    }
    MF_ASSERT( chunk->loaded() );
    MF_ASSERT( chunk->isBound() );

    g_camMatrix.translation( chunk->centre() );

    g_currentChunk = chunk;

    if (neighbourhood)
    {
        DWORD start = GetTickCount();

        while (!ChunkWaypointGenerator::canProcess(chunk))
        {
            if ( g_pleaseShutdown )
            {
                g_currentChunk = NULL;
                return false;           
            }

            processHarmlessMessages();
            drawGenerateAllProgress();
            Sleep( s_chunkLoadSleep );
        }

        if (GetTickCount() - start >= s_chunkLoadRetryTicks)
        {
            g_currentChunk = NULL;
            return false;
        }
    }
    ChunkManager::instance().camera( g_camMatrix, ChunkManager::instance().cameraSpace() );
    return true;
}

bool outsideChunkWritable( const BW::string& identifier )
{
    BW_GUARD;

    if (!g_conn.enabled())
        return true;

    if (!g_conn.connected())
        return false;

    if( *identifier.rbegin() == 'o' )
    {
        int16 gridX, gridZ;
        g_mapping->gridFromChunkName( identifier, gridX, gridZ );

        return g_conn.isLockedByMe( gridX, gridZ );
    }
    return false;
}


/**
 *  Unload all of the chunks in the space. Navgen does not mark/lock chunks.
 */
void unloadAllChunks()
{
    BW_GUARD;

    // Make sure that no chunks are locked

    // Stop loading more chunks
    ChunkManager::instance().cancelLoadingChunks();

    // As navgen does not lock any chunks, assert that nothing is loading now
    MF_ASSERT( !ChunkManager::instance().busy() );

    // Unload all chunks in the current space
    ChunkSpacePtr cameraSpace = ChunkManager::instance().cameraSpace();
    cameraSpace->unloadChunks();

    // All chunks in space should be unloaded
    MF_ASSERT( cameraSpace->boundChunkNum() == 0 );
}


void doClearAll()
{
    BW_GUARD;

    if (!checkAllowGenerate())
    {
        return;
    }

    NavgenScopedMenuDisabler md;

    g_statusWindow.begin( L"Clearing" );

    g_totalComputers = 1;
    g_myIndex = 0;

    ScopedSetReady scopedSetReady;
    ScopedSyncMode scopedSyncMode;

    unloadAllChunks();

    refreshWindows();

    // traverse over all the chunks in the space
    for (ChunkSpaceTraverser cst; !cst.done(); cst.next())
    {
        if (g_pleaseShutdown)
        {
            g_statusWindow.end();
            return;
        }

        // get the chunk and make a waypoint generator
        g_currentChunk = cst.chunk();

        if (hash( g_currentChunk->identifier().c_str() ) % g_totalComputers != g_myIndex)
        {
            g_statusWindow.skip();
            continue;
        }

        TRACE_MSG( "Current chunk: %s\n", g_currentChunk->resourceID().c_str() );        

        // find the chunk's section
        DataSectionPtr pChunkSect = BWResource::openSection(
            g_currentChunk->resourceID() );

        if (!pChunkSect)
        {
            ERROR_MSG( "Cannot load chunk file for : %s\n", g_currentChunk->resourceID().c_str() );
            continue;
        }

        if (*g_currentChunk->identifier().rbegin() == 'o')
        {
            if (!outsideChunkWritable( g_currentChunk->identifier() ))
            {
                g_statusWindow.skip();
                continue;
            }
        }
        else
        {
            if (!waitChunkLoaded( g_currentChunk, false )||!isChunkLocked(g_currentChunk))
            {
                g_statusWindow.skip();
                continue;
            }
        }

        g_statusWindow.process( g_currentChunk->identifier() );

        // remove any existing waypoint sets (legacy) and navPolySets
        // for this chunk.
        for (int i = 0; i < pChunkSect->countChildren(); i++)
        {
            DataSectionPtr ds = pChunkSect->openChild( i );

            if (!((ds->sectionName() == "waypoint") ||
                (ds->sectionName() == "waypointSet") ||
                (ds->sectionName() == "navPoly") ||
                (ds->sectionName() == "navmesh") ||
                (ds->sectionName() == "worldNavmesh") ||
                (ds->sectionName() == "navPolySet")))
            {
                continue;
            }

            pChunkSect->delChild( ds );
            i--;
        }

        pChunkSect->save();

        //Now lets clear the binary data as well...

        // get the chunk's cdata section
        DataSectionPtr pChunkBin = BWResource::openSection( g_currentChunk->binFileName(), true );

        pChunkBin->delChild( "navmesh" );
        pChunkBin->delChild( "worldNavmesh" );
        pChunkBin->delChild( "navmeshDirty" );

        if (DataSectionPtr auxData = pChunkBin->openSection( "auxData" ))
        {
            auxData->deleteSections( "worldNavmesh" );
        }

        pChunkBin->save();

        BWResource::instance().purge( g_currentChunk->binFileName(), true, pChunkBin );

        INFO_MSG( "Cleared: %d of %d\n", (int)cst.numDone(), (int)cst.numTodo() );

        refreshWindows();
    }

    // Now reload this to be current (doFileOpen also updates the 3dview)
    Chunk * pChunk = ChunkManager::instance().cameraChunk();

    if (pChunk != NULL)
    {
        BW::string filePath = BWResolver::resolveFilename( pChunk->resourceID() );
        doFileOpen( filePath.c_str(), pChunk );
    }

    // Redraw the chunk view to the current state
    drawStuff();
    g_currentChunk = NULL;
    g_statusWindow.end();
}

void doStatisticsAll()
{
    BW_GUARD;

    g_statusWindow.begin( L"Calculating statistics" );

    g_totalComputers = 1;
    g_myIndex = 0;

    ScopedSetReady scopedSetReady;
    ScopedSyncMode scopedSyncMode;

    unloadAllChunks();

    int totalChunk = 0;
    int navmeshChunk = 0;
    int size = 0;

    // traverse over all the chunks in the space
    for (ChunkSpaceTraverser cst; !cst.done(); cst.next())
    {
        if (g_pleaseShutdown)
        {
            g_statusWindow.end();
            return;
        }

        // get the chunk and make a waypoint generator
        g_currentChunk = cst.chunk();

        if (hash( g_currentChunk->identifier().c_str() ) % g_totalComputers != g_myIndex)
        {
            g_statusWindow.skip();
            continue;
        }

        TRACE_MSG( "Current chunk: %s\n", g_currentChunk->resourceID().c_str() );        

        if (*g_currentChunk->identifier().rbegin() == 'o')
        {
            if (!outsideChunkWritable( g_currentChunk->identifier() ))
            {
                g_statusWindow.skip();
                continue;
            }
        }
        else
        {
            if (!waitChunkLoaded( g_currentChunk, false )||!isChunkLocked(g_currentChunk))
            {
                g_statusWindow.skip();
                continue;
            }
        }

        g_statusWindow.process( g_currentChunk->identifier() );

        // remove any existing waypoint sets (legacy) and navPolySets
        // for this chunk.
        ++totalChunk;

        DataSectionPtr pChunkBin = BWResource::openSection( g_currentChunk->binFileName() );

        if (DataSectionPtr ds = pChunkBin->openSection( "auxData/worldNavmesh" ))
        {
            ++navmeshChunk;
            size += ds->asBinary()->len();
        }
    }

    // Now reload this to be current (doFileOpen also updates the 3dview)
    Chunk * pChunk = ChunkManager::instance().cameraChunk();

    if (pChunk != NULL)
    {
        BW::string filePath = BWResolver::resolveFilename( pChunk->resourceID() );
        doFileOpen( filePath.c_str(), pChunk );
    }

    INFO_MSG( "************* %d %d %d\n", totalChunk, navmeshChunk, size );
    // Redraw the chunk view to the current state
    drawStuff();
    g_currentChunk = NULL;
    g_statusWindow.end();
}

void doGenerateAll( bool overwrite = false )
{
    BW_GUARD;

    if (!checkAllowGenerate())
    {
        return;
    }

    if (overwrite)
    {
        doClearAll();
    }

    // Unload all of the chunks used during drawing
    unloadAllChunks();


    // set the loading distance to 0 before generating all
    // to load as few chunks as possible
    class GenerateAllEnvironmentSetter
    {
        bool showingMoo_;
    public:
        GenerateAllEnvironmentSetter()
        {
            BW_GUARD;

            ChunkManager::instance().autoSetPathConstraints( 0 );
            ChunkManager::instance().switchToSyncTerrainLoad( true );
        }
        ~GenerateAllEnvironmentSetter()
        {
            BW_GUARD;

            ChunkManager::instance().autoSetPathConstraints(
                g_chunkLoadDistance );
            ChunkManager::instance().switchToSyncTerrainLoad( false );
        }
    }
    generateAllEnvironmentSetter;

    g_pView = NULL;

    g_statusWindow.begin( L"Generating" );

    NavGenGenerating generating;
    ScopedSetReady scopedSetReady;

    NavgenScopedMenuDisabler md;

    g_calcChunksDone.clear();
    g_chunksLoaded.clear();
    g_calcChunksNotDirty.clear();

    bool errors = false;
    ErrorLogHandler.resetError();
    if (g_totalComputers == 1)
    {
        ErrorLogHandler.print( "Start Generate All Chunks:\n" );
    }
    else
    {
        BW::string msg( 
            sformat
            ( 
                "Start Cluster Generate (computer {0} of {1}):\n",
                g_myIndex + 1,
                g_totalComputers
            ).c_str() );
        ErrorLogHandler.print( msg );
    }

    // traverse over all the chunks in the space
    for (ChunkSpaceTraverser cst; !cst.done(); cst.next())
    {
        processHarmlessMessages();
        Moo::rc().preloadDeviceResources( 1000 );

        if (g_pleaseShutdown)
        {
            g_statusWindow.end();
            return;
        }

        // get the chunk and make a waypoint generator
        g_currentChunk = cst.chunk();

        DataSectionPtr pChunkSect = BWResource::openSection(
            g_currentChunk->resourceID() );

        if (!pChunkSect)
        {
            ERROR_MSG( "Cannot load chunk file for : %s\n", g_currentChunk->resourceID().c_str() );
            continue;
        }

        if( hash( g_currentChunk->identifier().c_str() ) % g_totalComputers != g_myIndex )
        {
            INFO_MSG( "Skip chunk %s because hash failed\n", g_currentChunk->identifier().c_str() );
            g_statusWindow.skip();
            drawGenerateAllProgress();
            continue;
        }

        // check if this outdoor chunk has been locked before load it because
        // outdoor chunks have their bounding box ready before being loaded
        if (g_currentChunk->isOutsideChunk() && !isChunkLocked( g_currentChunk ))
        {
            INFO_MSG( "Skip chunk %s because it is not locked\n", g_currentChunk->identifier().c_str() );
            g_statusWindow.skip();
            drawGenerateAllProgress();
            continue;
        }

        if (g_currentChunk->isOutsideChunk())
        {
            bool modified = true;
            DataSectionPtr chunkBinSection = BWResource::openSection( g_currentChunk->binFileName() );

            if (chunkBinSection)
            {
                modified = !chunkBinSection->openSection( "auxData/worldNavmesh" );
            }

            if (!modified)
            {
                g_calcChunksDone.insert( g_currentChunk );
                g_calcChunksNotDirty.insert( g_currentChunk );
                g_statusWindow.skip();
                drawGenerateAllProgress();
                continue;
            }
        }

        g_currentChunkStatus = 0;

        Chunk *loadingChunk = g_currentChunk; 
        bool loaded = waitChunkLoaded( g_currentChunk, true );

        drawGenerateAllProgress();  

        if( !loaded )
        {
            ERROR_MSG(
                "Could not load chunk %s: Maximum number of retries reached.\n"
                "No navigation information will be created for this chunk.",
                loadingChunk->identifier().c_str() );
            errors = true;
            g_statusWindow.skip();
            continue;
        }

        if (!g_currentChunk->isOutsideChunk() && !isChunkLocked( g_currentChunk ))
        {
            INFO_MSG( "Skip chunk %s because it is not locked\n", g_currentChunk->identifier().c_str() );
            g_statusWindow.skip();
            continue;
        }

        g_calcChunksDone.insert(g_currentChunk);
        TRACE_MSG( "Current chunk: %s\n", g_currentChunk->resourceID().c_str() );

        ChunkWaypointGenerator cwg( g_currentChunk, g_floodResultPath );

        if (!cwg.modified())
            g_calcChunksNotDirty.insert( g_currentChunk );

        // skip it if it hasn't been modified or if there was an error
        // loading the chunk
        if ( !cwg.modified() || !loaded )
        {
            g_currentChunkStatus = 4;
            drawGenerateAllProgress();
            MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
            updateMoo( 0.1f, false );
            INFO_MSG( "Skip chunk %s because it is not modified\n", g_currentChunk->identifier().c_str() );
            g_statusWindow.skip( !cwg.modified() );
            continue;
        }

        NoticeMsg().write( "%s\n",
            ( BW::string( "Generating chunk: " ) + g_currentChunk->identifier() + '\n' ).c_str()
        );

        g_statusWindow.process( g_currentChunk->identifier() );
        BW::vector<float> girthsToCalculate = compileGirthsList( g_currentChunk );

        for ( uint gi = 0; gi < girthsToCalculate.size(); ++gi )
        {
            if ( g_pleaseShutdown )
            {
                g_statusWindow.end();
                return;
            }

            Girth gSpec = g_girthSpecs.find( girthsToCalculate[gi] )->second;
            INFO_MSG( "Generating navPolys for girth size: %f (w: %f  h: %f  d: %f)\n",
                girthsToCalculate[gi], gSpec.width(), gSpec.height(), gSpec.depth() );

            // flood it
            g_currentChunkStatus = 1;
            g_maxFloodPoints = cwg.maxFloodPoints();
            g_curFloodPoints = 0;
            drawGenerateAllProgress();
            cwg.flood( floodProgressCallback, gSpec, g_writeTGAs );
            if ( g_pleaseShutdown )
            {
                g_statusWindow.end();
                return;
            }

            // generate it
            g_currentChunkStatus = 2;
            drawGenerateAllProgress();
            cwg.generate(g_annotate, gSpec);

            // and output it
            g_currentChunkStatus = 3;
            drawGenerateAllProgress();
            cwg.output( girthsToCalculate[gi], gi == 0 );
        }
        cwg.outputDirtyFlag( false );

        MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
        updateMoo( 0.1f, false );

        DataSectionCache::instance()->clear();
        DataSectionCensus::clear();
    }


    // Unload all of the chunks used during generation
    unloadAllChunks();

    setReady(); // don't display "Ready" if error message box is shown

    if ( errors || ErrorLogHandler.getError() )
    {
        ErrorLogHandler.print( "Done Generate All Chunks With Errors.\n", false );

        // errors occurred while processing the chunks. Display a message.
        BW::wstring msg = g_statusWindow.status() + 
            L"There were warnings/errors encountered during the process "
            L"that may or may not have affected the process region.\n"
            L"The chunk might still be missing navigation information.\n"
            L"Please check the errors log window for more information.";
        MessageBox( g_hWindow,
            msg.c_str(),
            L"NavPoly - Warnings/Errors occurred",
            MB_ICONWARNING | MB_OK | MB_APPLMODAL );
    }
    else
    {
        ErrorLogHandler.print( "Done Generate All Chunks.\n", false );
    }


    // Now reload this to be current (doFileOpen also updates the 3dview)
    Chunk * pChunk = ChunkManager::instance().cameraChunk();
    if (pChunk != NULL)
    {
        BW::string filePath = BWResolver::resolveFilename( pChunk->resourceID() );
        doFileOpen( filePath.c_str(), pChunk );
    }

    // Redraw the chunk view to the current state
    drawStuff();

    g_statusWindow.end();

    // Release the current chunk
    g_currentChunk = NULL;
}


void clampViewPosition()
{
    BW_GUARD;

    if ( g_centreX < g_virtualMinX )
        g_centreX = g_virtualMinX;

    if ( g_centreX > g_virtualMaxX )
        g_centreX = g_virtualMaxX;

    if ( g_centreY < g_virtualMinY )
        g_centreY = g_virtualMinY;

    if ( g_centreY > g_virtualMaxY )
        g_centreY = g_virtualMaxY;
}

void dragView()
{
    BW_GUARD;

    POINT curr;
    int dx, dy;

    GetCursorPos( &curr );
    dx = curr.x - g_dragStart.x;
    dy = curr.y - g_dragStart.y;

    g_centreX -= dx * (g_radiusX * 2 / g_viewX);
    g_centreY += dy * (g_radiusY * 2 / g_viewY);

    clampViewPosition();
    InvalidateRect( g_hWindow, NULL, FALSE );
    g_dragStart = curr;
}

INT_PTR CALLBACK StatsProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch ( msg )
    {
    case WM_INITDIALOG:
        {
            int bspCount = g_pView->getBSPNodeCount();
            int polyCount = g_pView->getPolygonCount();
            int vertexCount = 0;
            int binaryCount = 0;
            int adjCount = 0;
            int i, j, adj;
            Vector2 v;
            bool adjToAnotherChunk;


            for ( i = 0; i < polyCount; ++i )
            {
                vertexCount += g_pView->getVertexCount(i);

                for ( j = 0; j < g_pView->getVertexCount(i); ++j )
                {
                    g_pView->getVertex(i, j, v, adj, adjToAnotherChunk);
                    if(adj > 0)
                        adjCount++;
                }
            }

            // 4 bytes for each poly (count of vertices)
            binaryCount = 4 * polyCount;

            // 20 bytes for each vertex
            // (12 bytes for vector3, 8 bytes for adjacency info)
            binaryCount += (vertexCount * 20);

            SetDlgItemInt(hwnd, IDC_BSP_NODES, bspCount, FALSE);
            SetDlgItemInt(hwnd, IDC_WAYPOINT_POLYGONS, polyCount, FALSE);
            SetDlgItemInt(hwnd, IDC_WAYPOINT_VERTICES, vertexCount, FALSE);
            SetDlgItemInt(hwnd, IDC_WAYPOINT_ADJACENCIES, adjCount, FALSE);
            SetDlgItemInt(hwnd, IDC_BINARY_FILE_SIZE, binaryCount, FALSE);
            return TRUE;
        }


    case WM_COMMAND:
        if ( w == IDOK || w == IDCANCEL )
        {
            EndDialog(hwnd, w);
            return TRUE;
        }
    }

    return FALSE;
}

void zoom( float factor )
{
    BW_GUARD;

    g_radius *= factor;
    InvalidateRect( g_hWindow, NULL, FALSE );
}

void showCursorPos()
{
    BW_GUARD;

    wchar_t buf[256];
    POINT p;
    RECT r;

    Chunk * pChunk = ChunkManager::instance().cameraChunk();
    if ( pChunk )
    {

        GetCursorPos(&p);
        ScreenToClient(g_hWindow, &p);
        GetClientRect(g_hWindow, &r);
        r.bottom -= g_statusWindowHeight;

        if(r.right && r.bottom && g_radiusX > 0 && g_radiusY > 0)
        {
            float gridResolution = g_pView->gridResolution();
            float x1 = (g_centreX - g_radiusX) * gridResolution + g_pView->gridMin().x;
            float y1 = (g_centreY - g_radiusY) * gridResolution + g_pView->gridMin().z;
            float xs = gridResolution * g_radiusX * 2;
            float ys = gridResolution * g_radiusY * 2;

            float x = x1 + (p.x * xs / r.right);
            float y = y1 + ys - (p.y * ys / r.bottom);

            Vector3 in( x, 10.0f, y);
            Matrix m = pChunk->transformInverse();
            Vector3 out = m.applyPoint( in );
            float lx = out.x;
            float ly = out.z;

            int gx = int((x - g_pView->gridMin().x) / gridResolution + 0.5f);
            int gy = int((y - g_pView->gridMin().z) / gridResolution + 0.5f);

            bw_snwprintf(buf, ARRAY_SIZE(buf), L"Girth %g LPoint(%0.2f, %0.2f) Point (%0.2f, %0.2f) Grid (%d,%d)",
                g_girthsViewable[g_selectedGirth], lx, ly, x, y, gx, gy );

            g_statusWindow.setStatus( buf );
        }
    }

}


void selectNavPolyCommon( int newSel, bool showInfo )
{
    BW_GUARD;

    Chunk * pChunk = ChunkManager::instance().cameraChunk();
    if (!pChunk) 
    {
        return;
    }

    float gridResolution = g_pView->gridResolution();
    LVITEM item;
    int a, i, vertices;
    wchar_t buf[256];
    Vector2 v;
    bool adjToAnotherChunk;

    int oldSel = g_selectedIndex;
    g_selectedIndex = newSel;

    if ( g_selectedIndex != -1 )
    {
        SendDlgItemMessage(g_hInfoDialog, IDC_LIST1,
            LVM_DELETEALLITEMS, 0, 0);

        int set = g_pView->getSet( g_selectedIndex );

        bw_snwprintf(buf, ARRAY_SIZE(buf), L"%d-%d", set, g_selectedIndex + 1 );
        SetDlgItemText( g_hInfoDialog, IDC_WAYPOINT_ID, buf );
        bw_snwprintf(buf, ARRAY_SIZE(buf), L"%f to %f",
            g_pView->getMinHeight(g_selectedIndex), g_pView->getMaxHeight(g_selectedIndex) );
        SetDlgItemText( g_hInfoDialog, IDC_WAYPOINT_HEIGHT, buf );

        vertices = g_pView->getVertexCount( g_selectedIndex );

        for ( i = 0; i < vertices; ++i )
        {
            g_pView->getVertex(g_selectedIndex, i, v, a, adjToAnotherChunk);

            item.mask = LVIF_TEXT;
            item.iItem = i;
            item.iSubItem = 0;
            item.pszText = buf;
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"%d", i);
            SendMessage(g_hListView, LVM_INSERTITEM, 0, (LPARAM)&item);

            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"%f", v.x * gridResolution + g_pView->gridMin().x);
            item.iSubItem = 3;
            SendMessage(g_hListView, LVM_SETITEM, 0, (LPARAM)&item);

            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"%f", v.y * gridResolution + g_pView->gridMin().z);
            item.iSubItem = 4;
            SendMessage(g_hListView, LVM_SETITEM, 0, (LPARAM)&item);

            // now print out in local coords.
            Vector3 in( v.x * gridResolution + g_pView->gridMin().x,
                        10.0f,
                        v.y * gridResolution + g_pView->gridMin().z);

            Matrix m = m = pChunk->transformInverse();
            Vector3 out = m.applyPoint( in );

            item.iSubItem = 1;
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"%f", out.x);          
            SendMessage(g_hListView, LVM_SETITEM, 0, (LPARAM)&item);

            item.iSubItem = 2;
            bw_snwprintf(buf, ARRAY_SIZE(buf),  L"%f", out.z);          
            SendMessage(g_hListView, LVM_SETITEM, 0, (LPARAM)&item);

            // ---

            buf[0] = 0;

            if(adjToAnotherChunk)
            {
                bw_snwprintf(buf, ARRAY_SIZE(buf), L"Other Chunk");
            }
            if(a > 0)
            {
                bw_snwprintf(buf+wcslen(buf), ARRAY_SIZE(buf)-wcslen(buf), L"%d", a);
            }
            if(a < 0)
            {
                wchar_t * nbuf = buf+wcslen(buf);
                int adjFlags = -a;
                if (adjFlags == 0x0FFF)
                    wcscat( nbuf, L"Clear" );
                else if (adjFlags == 0x0333)
                    wcscat( nbuf, L"Cover" );
                else
                {
                    wchar_t ahtrans[] = L"SMRC";    // solid, mixed, reserved, clear
                    bw_snwprintf(nbuf, ARRAY_SIZE(buf)-wcslen(buf), L"Ahead: %c%c %c%c %c%c",
                        ahtrans[(adjFlags>>0)&3], ahtrans[(adjFlags>>2)&3],
                        ahtrans[(adjFlags>>4)&3], ahtrans[(adjFlags>>8)&3],
                        ahtrans[(adjFlags>>8)&3], ahtrans[(adjFlags>>10)&3] );
                }
            }

            if (buf[0])
            {
                item.iSubItem = 5;
                SendMessage(g_hListView, LVM_SETITEM, 0, (LPARAM)&item);
            }
        }

        if (g_selectedIndex == oldSel && showInfo)
        {
            ShowWindow(g_hInfoDialog, SW_SHOWNA);
        }
    }
    else
    {
        ShowWindow(g_hInfoDialog, SW_HIDE);
    }

    InvalidateRect(g_hWindow, NULL, FALSE);
}

void selectNavPoly( bool doubleClicked )
{
    BW_GUARD;

    POINT p;
    RECT r;
    float x1, y1, xs, ys, x, y;

    GetCursorPos(&p);
    ScreenToClient(g_hWindow, &p);
    GetClientRect(g_hWindow, &r);

    r.bottom -= g_statusWindowHeight;

    if(r.right && r.bottom && g_radiusX > 0 && g_radiusY > 0 && g_pView)
    {
        x1 = g_centreX - g_radiusX;
        y1 = g_centreY - g_radiusY;
        xs = g_radiusX * 2;
        ys = g_radiusY * 2;
        x = x1 + (p.x * xs / r.right);
        y = y1 + ys - (p.y * ys / r.bottom);

        Vector3 v( x, 0.f, y );
        v = g_pView->gridMin() + v * g_pView->gridResolution();
        static int lastNPFound = -1;
        BW::vector<int> npFound = g_pView->findWaypoints( v, g_girthsViewable[g_selectedGirth] );

        while (!npFound.empty())
        {
            if (isSetOfNavmeshVisible( npFound[0] ) )
            {
                break;
            }

            npFound.erase( npFound.begin() );
        }

        if (npFound.empty() && lastNPFound != -1 && doubleClicked)
            npFound.push_back( lastNPFound );
        else if (!npFound.empty())
            lastNPFound = npFound[0];
        else
            lastNPFound = -1;
        selectNavPolyCommon( lastNPFound, doubleClicked );
    }
}


enum ChunkOp
{
    CO_DISPLAY = 0,
    CO_GENERATE = 1,
    CO_REANNOTATE = 2
};

static void chunkOp( ChunkOp op );


INT_PTR CALLBACK infoDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch(msg)
    {
        case WM_INITDIALOG:
        {
            g_hListView = GetDlgItem(hwnd, IDC_LIST1);
            LVCOLUMN col;
            col.mask = LVCF_TEXT | LVCF_WIDTH;
            col.pszText = L"Index";
            col.cx = 50;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&col);

            col.pszText = L"world X";
            col.cx = 75;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 3, (LPARAM)&col);

            col.pszText = L"world Z";
            col.cx = 75;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 4, (LPARAM)&col);

            col.pszText = L"local X";
            col.cx = 75;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 1, (LPARAM)&col);

            col.pszText = L"local Z";
            col.cx = 75;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 2, (LPARAM)&col);

            col.pszText = L"AdjacentID";
            col.cx = 125;
            SendMessage(g_hListView, LVM_INSERTCOLUMN, 5, (LPARAM)&col);

            // Remember the offset of listview from bottom-right border.
            RECT r1, r2;
            GetClientRect(hwnd, &r1);
            GetWindowRect(g_hListView, &r2);
            g_listViewOffset.x = r1.right - (r2.right - r2.left);
            g_listViewOffset.y = r1.bottom - (r2.bottom - r2.top);
            return TRUE;
        }

        case WM_SIZE:
        {
            RECT r;
            GetClientRect(hwnd, &r);
            SetWindowPos(g_hListView, NULL, 0, 0,
                r.right - g_listViewOffset.x,
                r.bottom - g_listViewOffset.y,
                SWP_NOMOVE | SWP_NOZORDER);
            return TRUE;
        }

        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return TRUE;
    }

    return FALSE;
}


void closeHelpDialog()
{
    BW_GUARD;

    if (g_hHelpWindow != NULL)
    {
        DestroyWindow(g_hHelpWindow);
        g_hHelpWindow = NULL;
    }
}


INT_PTR CALLBACK helpDlgProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch(msg)
    {
        case WM_COMMAND:
            if (w == IDOK)
            {
                closeHelpDialog();
                return TRUE; // handled
            }
            break;
        case WM_CLOSE:
            closeHelpDialog();
            return TRUE; // handled
    }
    return FALSE; // not handled
}


void displayHelp()
{
    BW_GUARD;

    // Toggle the help window
    if (g_hHelpWindow != NULL)
    {
        closeHelpDialog();
    }
    else
    {
        g_hHelpWindow =
            CreateDialog
            (
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_HELP),
                g_hWindow,
                helpDlgProc
            );
        // Center the help window above the main window:
        RECT mainRect;
        GetWindowRect(g_hWindow, &mainRect);
        RECT helpRect;
        GetWindowRect(g_hHelpWindow, &helpRect);
        SetWindowPos
        (
            g_hHelpWindow, 
            HWND_TOPMOST,
            (mainRect.left + mainRect.right )/2 - (helpRect.right  - helpRect.left)/2,
            (mainRect.top  + mainRect.bottom)/2 - (helpRect.bottom - helpRect.top )/2,
            0,
            0,
            SWP_NOSIZE | SWP_SHOWWINDOW
        );       
    }
}

INT_PTR CALLBACK clusterDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch(msg)
    {
        case WM_CREATE:
            return TRUE;

        case WM_CLOSE:
            if ( !g_generating ||
                MessageBox( hwnd,
                    L"Closing NavPoly will stop any generation process in progress.\nAre you sure you want to close NavPoly?",
                    L"NavPoly",
                    MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
            {
                DestroyWindow(hwnd);
            }
            return TRUE;

        case WM_DESTROY:
            MsgBox::setDefaultParent( NULL );
            g_pleaseShutdown = true;
            PostQuitMessage(-1);
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
            hdc = BeginPaint(hwnd, &ps);
            if (g_currentChunk == NULL) drawStuff();
            SwapBuffers(hdc);
            EndPaint(hwnd, &ps);
            return TRUE;
        }

        case WM_LBUTTONDOWN:
            selectNavPoly( false );
            return TRUE;

        case WM_LBUTTONDBLCLK:
            selectNavPoly( true );
            return TRUE;

        case WM_RBUTTONDOWN:
            GetCursorPos(&g_dragStart);
            g_dragging = true;
            SetCapture(hwnd);
            return TRUE;

        case WM_MOUSEMOVE:
            if(g_dragging)
                dragView();
            else if (!g_generating)
                showCursorPos();
            return TRUE;

        case WM_RBUTTONUP:
            if(g_dragging)
            {
                g_dragging = false;
                ReleaseCapture();
                dragView();
            }
            return TRUE;

        case WM_MOUSEWHEEL:
            {
                double f = (short)HIWORD(w) / 120.0f;
                zoom(float(pow((double)ZOOM_FACTOR, -f)));
                return TRUE;
            }

        case WM_CHAR:
            switch(LOWORD(w))
            {
                case '+':
                    zoom(1 / ZOOM_FACTOR);
                    break;

                case '-':
                    zoom(ZOOM_FACTOR);
                    break;

                case ',':
                    g_bspNodesDepth = std::max(g_bspNodesDepth-1,0);
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    break;
                case '.':
                    g_bspNodesDepth = std::min(g_bspNodesDepth+1,31);
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    break;

                case ']':
                    // g_selectedGirth = std::max(g_selectedGirth,uint(1))-1;
                    g_selectedGirth += 1;
                    if (g_selectedGirth >= (int)g_girthsViewable.size())
                    {
                        g_selectedGirth = static_cast<int>(g_girthsViewable.size() - 1);
                    }
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    showCursorPos();
                    break;
                case '[':
                    // g_selectedGirth = std::min(g_selectedGirth+1,g_girths.size()-1);
                    g_selectedGirth -= 1;
                    if (g_selectedGirth < 0)
                    {
                        g_selectedGirth = 0;
                    }
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    showCursorPos();
                    break;
            }
            return TRUE;

        case WM_SIZE:
        {
            RECT r, r2;

            GetClientRect(g_hWindow, &r);

            GetWindowRect(g_statusWindow, &r2);
            g_statusWindowHeight = r2.bottom - r2.top;

            SetWindowPos(g_statusWindow, NULL, 0,
                r.bottom - g_statusWindowHeight,
                r2.bottom - r2.top,
                r.right, SWP_NOZORDER);


            r.bottom -= g_statusWindowHeight;
            glViewport(0, g_statusWindowHeight, r.right, r.bottom);

            g_viewX = float( r.right );
            g_viewY = float( r.bottom );

            InvalidateRect(g_hWindow, NULL, FALSE);
            return TRUE;
        }

        case WM_INITMENU:
            CheckMenuItem( g_hMenu, ID_VIEW_ERRORLOG,
                AsyncMessage().isShow() ? MF_CHECKED : MF_UNCHECKED );
            break;
        case WM_COMMAND:
        {
            if (LOWORD( w ) > ID_VIEW_SET_NONE &&
                LOWORD( w ) - ID_VIEW_SET_NONE - 1 < g_pView->getSets())
            {
                int index = LOWORD( w ) - ID_VIEW_SET_NONE - 1;

                toggleVisibleSet( index );

                if (!isSetOfNavmeshVisible( g_selectedIndex ))
                {
                    selectNavPolyCommon( -1, false );
                }
            }

            switch(LOWORD(w))
            {
            case ID_FILE_OPEN1:
                doFileOpen();
                break;

            case ID_FILE_OPENSPACE:
                {
                    BW::string space = g_spaceManager->browseForSpaces( g_hWindow );
                    space = BWResolver::dissolveFilename( space );
                    if( !space.empty() )
                        changeSpace( space );
                }
                break;

            case ID_FILE_CLEAR_ALL:
                if( ::MessageBox( GetForegroundWindow(),
                    L"Are you sure you want to clear out all the current navigation polygon data?\n"
                    L"This could take a long time to regenerate",
                    L"Confirm Clearing of Navigation Data", MB_YESNO ) == IDNO )
                        break;
                doClearAll();
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;

            case ID_FILE_GENERATE_ALL:
                doGenerateAll();
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;

            case ID_FILE_GENERATE_ALL_OVERWRITE:
                if( ::MessageBox( GetForegroundWindow(),
                    L"Are you sure you want to overwrite all the current navigation polygon data?\n"
                    L"This could take a long time to regenerate",
                    L"Confirm Overwriting of Navigation Data", MB_YESNO ) == IDNO )
                        break;
                doGenerateAll( true );
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;

            case ID_FILE_CLUSTERGENERATE:
                {
                    if( DialogBox( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_CLUSTERGENDIALOG ),
                        g_hWindow, clusterDialogProc ) == IDOK )
                    {
                        doGenerateAll( false );
                    }
                    g_totalComputers = 1;
                    g_myIndex = 0;
                }
                break;

            case ID_FILE_EXIT:
                DestroyWindow(hwnd);
                break;

            case ID_VIEW_SET_ALL:
                updateViewSetMenu( true );
                break;

            case ID_VIEW_SET_NONE:
                updateViewSetMenu( false );
                break;

            case ID_VIEW_ADJACENCIES:
                g_viewAdjacencies = !g_viewAdjacencies;
                CheckMenuItem(g_hMenu, LOWORD(w),
                    g_viewAdjacencies ? MF_CHECKED : MF_UNCHECKED);
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;
            case ID_VIEW_BSPNODES:
                g_viewBSPNodes = !g_viewBSPNodes;
                CheckMenuItem(g_hMenu, LOWORD(w),
                    g_viewBSPNodes ? MF_CHECKED : MF_UNCHECKED);
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;
            case ID_VIEW_POLYGONAREA:
                g_viewPolygonArea = !g_viewPolygonArea;
                CheckMenuItem(g_hMenu, LOWORD(w),
                    g_viewPolygonArea ? MF_CHECKED : MF_UNCHECKED);
                InvalidateRect(g_hWindow, NULL, FALSE);
                break;
            case ID_VIEW_POLYGONBORDERS:
                g_viewPolygonBorders = !g_viewPolygonBorders;
                CheckMenuItem(g_hMenu, LOWORD(w),
                    g_viewPolygonBorders ? MF_CHECKED : MF_UNCHECKED);

                InvalidateRect(g_hWindow, NULL, FALSE);
                break;

            case ID_VIEW_STATISTICS:
                if (g_pView != NULL)
                {
                    DialogBox(GetModuleHandle(NULL),
                        MAKEINTRESOURCE(IDD_STATISTICS),
                        hwnd, StatsProc);
                }
                break;

            case ID_VIEW_WAYPOINTINFO:
                if (g_pView != NULL)
                {
                    ShowWindow(g_hInfoDialog, SW_SHOW);
                }
                break;

            case ID_VIEW_RENDEREDSCENE:
                ShowWindow(g_hMooWindow, SW_SHOW);
                BringWindowToTop(g_hMooWindow);
                break;

            case ID_VIEW_ERRORLOG:
                CheckMenuItem( g_hMenu, LOWORD(w),
                    AsyncMessage().isShow() ? MF_UNCHECKED : MF_CHECKED );
                if( AsyncMessage().isShow() )
                    AsyncMessage().hide();
                else
                    AsyncMessage().show();
                break;
            case ID_ZOOM_IN:
                zoom(1 / ZOOM_FACTOR);
                break;
            case ID_ZOOM_OUT:
                zoom(ZOOM_FACTOR);
                break;

            case ID_CHUNK_DISPLAY:
                chunkOp( CO_DISPLAY );
                break;
            case ID_CHUNK_GENERATE:
                chunkOp( CO_GENERATE );
                break;
            case ID_CHUNK_GENERATE_SHRUNK:
                chunkOp( CO_GENERATE );
                break;
            case ID_CHUNK_REANNOTATE:
                if (g_reannotation)
                {
                    chunkOp( CO_REANNOTATE );
                }
                break;
            case ID_HELP_HELP:
                displayHelp();
                break;
            }
            break;
        }
        case WM_SETSTATUSTEXT:
            g_statusWindow.setStatus( 
                ErrorLogHandler.consumeMessage( (wchar_t*)l ) );
    }

    return DefWindowProc(hwnd, msg, w, l);
}



bool setupGL()
{
    BW_GUARD;

    PIXELFORMATDESCRIPTOR pfd;
    HDC hdc;
    int pixelFormat;

    hdc = GetDC(g_hWindow);

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 16;
    pfd.cAlphaBits = 0;
    pfd.cAccumBits = 0;
    pfd.cDepthBits = 0;
    pfd.cStencilBits = 0;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;

    if(!(pixelFormat = ChoosePixelFormat(hdc, &pfd)))
    {
        return false;
    }

    if(!SetPixelFormat(hdc, pixelFormat, &pfd))
    {
        return false;
    }

    if(!(g_glrc = wglCreateContext(hdc)))
    {
        return false;
    }

    wglMakeCurrent (hdc, g_glrc);
    ReleaseDC(g_hWindow, hdc);

    return true;
}

BW::string g_specialConsoleString;

extern int ChunkModel_token;
extern int ChunkLight_token;
extern int ChunkTerrain_token;
extern int ChunkTree_token;
static int s_chunkTokenSet = 
    ChunkModel_token | ChunkLight_token | 
    ChunkTerrain_token | ChunkTree_token ;

extern int ResMgr_token;
static int pythonTokenSet = ResMgr_token;

static BW::map<unsigned char, bool> s_mooKeyStates;

static float s_mooCamMoveRate = 5.f;

void clearMooKeys()
{
    BW_GUARD;
    s_mooKeyStates.clear();
}

void setupBWLockd( const BW::string& spacePath, float gridSize )
{
    BW_GUARD;

    struct WSAInit
    {
        WSAInit()
        {
            WSAStartup( MAKEWORD(1,1), &WSADATA() );
        }
    };
    static WSAInit WSAInit;

    if( !g_bigbangd.empty() && !g_username.empty() )
    {
        // TODO: This constant should be shared with bigbang
        static const float MAX_TERRAIN_SHADOW_RANGE = 500.f;
        static const int xExtent = (int)( ( MAX_TERRAIN_SHADOW_RANGE + 1.f ) / gridSize );

        if( !g_conn.connected() )
            g_conn.init( g_bigbangd, "NavGenApplication", xExtent, 1 );

        if (!g_conn.changeSpace( spacePath ))
        {
            NoticeMsg().write( "bwlockd: "
                "failed to connect to bwlockd server %s by username %s\n",
                g_bigbangd.c_str(), g_username.c_str() );
        }
        else
        {
            NoticeMsg().write( "bwlockd: "
                "connected to bwlockd server %s by username %s\n",
                g_bigbangd.c_str(), g_username.c_str() );
        }
    }
    else 
    {
        NoticeMsg().write( "bwlockd: not using bwlockd\n" );
    }
}

BOOL parseCommandLineMF( const char * commandLine, int& argc, char**& argv )
{
    BW_GUARD;

    if (!commandLine || strlen( commandLine ) == 0)
    {
        return TRUE;
    }

    // parse command line
    const int MAX_ARGS = 20;
    const char sepsArgs[] = " \t";
    const char sepsFile[] = "\"";

    MF_ASSERT( s_cmdLineBuffer == NULL );
    s_cmdLineBuffer = new char[ strlen( commandLine ) + 1 ];
    strcpy( s_cmdLineBuffer, commandLine );

    MF_ASSERT( s_argv == NULL );
    s_argv = new char*[ MAX_ARGS ];

    argv = s_argv;

    // parse the arguments
    argv[ argc ] = strtok( s_cmdLineBuffer, sepsArgs );

    while (argv[ argc ] != NULL)
    {
        if( ( strcmp( argv[ argc ], "-UID" ) == 0 ) ||
            ( strcmp( argv[ argc ], "-uid" ) == 0 ) )
        {
            ++argc;
            argv[ argc ] = strtok( NULL, sepsArgs );
            if ( argv[ argc ] == NULL )
            {
                ERROR_MSG( "NavGen::parseCommandLineMF: No user ID given\n" );
                return FALSE;
            }
        }
        else if( ( strcmp( argv[ argc ], "-r" ) == 0 ) ||
                 ( strcmp( argv[ argc ], "--res" ) == 0 ) ||
                 ( strcmp( argv[ argc ], "--settings" ) == 0 ) )
        {
            ++argc;
            char buff[MAX_PATH];
            char* str = strtok( NULL, sepsArgs );
            if(str != NULL && *str == '\"') //string is in quotes
            {
                if(str[strlen(str)-1] == '\"') //quoted string has no spaces
                {
                    str[strlen(str)-1] = '\0'; //remove last quote
                    strcpy(buff, str+1); //copy without first quote
                }
                else //quoted string contains spaces
                {
                    strcpy(buff, str+1); //copy without first quote
                    str = strtok( NULL, sepsFile );
                    strcat(buff, " "); //add space back in
                    strcat(buff, str);//append rest of string
                }
                argv[ argc ] = new char [ strlen(buff) ];
                strcpy( argv[ argc ], buff );
            }
            else if(str != NULL)
            {
                argv[ argc ] = str;
            }
            else
            {
                ERROR_MSG( "NavGen::parseCommandLineMF: No res paths given\n" );
                return FALSE;
            }
        }

        if (++argc >= MAX_ARGS)
        {
            ERROR_MSG( "NavGen::parseCommandLineMF: Too many arguments!!\n" );
            return FALSE;
        }

        argv[ argc ] = strtok( NULL, sepsArgs );
    }

    return true;
}

void processCommandLine(int argc, char **argv)
{
    BW_GUARD;
}


#if ENABLE_ASSET_PIPE
bool setupChunking( const BW::string& space, AssetClient * pAssetClient )
#else
bool setupChunking( const BW::string& space )
#endif
{
    BW_GUARD;

    // Disable water background calculations:
    Water::backgroundLoad(false);

    // Initialise python
    PyImportPaths paths;
    paths.addResPath( EntityDef::Constants::entitiesEditorPath() );

    if (!Script::init( paths, "navgen" ))
    {
        return false;
    }

    // Set callback for PyScript so it can know total game time
    Script::setTotalGameTimeFn( getTotalTime );

    if (!MaterialKinds::init())
    {
        ERROR_MSG( "setupChunking: failed to initialise MaterialKinds\n" );
        return false;
    }

    // Find the universe name   
    DataSectionPtr drRoot = g_navgenSettings->getRootSection();
    BW::string spacePath = drRoot->readString( "space/mru0" );
    if ( drRoot->findChild( "bwlockd" ) != NULL )
    {
        g_bigbangd = drRoot->readString( "bwlockd/host", g_bigbangd );
        g_username = drRoot->readString( "bwlockd/username", g_username );
    } 
    else 
    {
        g_bigbangd = drRoot->readString( "bigbangd/host", g_bigbangd );
        g_username = drRoot->readString( "bigbangd/username", g_username );
    }

    if( !g_bigbangd.empty() && g_username.empty() )
    {
        wchar_t name[1024];
        DWORD size = 1024;
        GetUserName( name, &size );
        bw_wtoutf8( name, g_username );
    }

    g_floodResultPath = drRoot->readString( "floodResultPath", g_floodResultPath );

    // Create the moo device

    g_pRenderer.reset( new Renderer );
    g_pRenderer->init( false, true );

#if ENABLE_ASSET_PIPE
    Moo::rc().createDevice( g_hMooWindow,
        0, 0, true, false, Vector2(0, 0), true, false, pAssetClient );
#else
    Moo::rc().createDevice( g_hMooWindow );
#endif

    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
    ShowCursor( TRUE );

    // Hide the 3D window to avoid it turning black from the clear device in
    // the following method
    ::ShowWindow( g_hMooWindow, SW_HIDE );

    s_pFontManager = FontManagerPtr( new FontManager() );

    ::ShowWindow( g_hMooWindow, SW_SHOW );

#if SPEEDTREE_SUPPORT
    speedtree::SpeedTreeRenderer::enviroMinderLighting(false);
#endif

    // Set its camera
    const float farPlaneDistance = g_chunkLoadDistance;
    Moo::Camera cam( 0.25f, farPlaneDistance, DEG_TO_RAD(60.f), 2.f );
    cam.aspectRatio(
        float(Moo::rc().screenWidth()) / float(Moo::rc().screenHeight()) );
    Moo::rc().camera( cam );

    // Set the camera matrix
    g_camMatrix = Matrix::identity;

    DataSectionPtr mcRoot = BWResource::instance().rootSection();
    Vector3 tran = mcRoot->readVector3(
            spacePath + "/space.settings/startPosition", Vector3( 1.f, 1.f, 1.f ) );
    g_camMatrix.translation( tran );
    Vector3 rot = mcRoot->readVector3( 
            spacePath + "/space.settings/startDirection" 
        );

    Matrix camRot;
    camRot.setRotate( rot[2], rot[1], rot[0] );
    g_camMatrix.preMultiply( camRot );

    // Init the terrain
    s_pTerrainManager = TerrainManagerPtr( new Terrain::Manager() );

    s_pTextureFeeds = TextureFeedsPtr( new TextureFeeds() );
    
    s_pLensEffectManager = LensEffectManagerPtr( new LensEffectManager() );

    SpaceManager::instance().init();
    ClientChunkSpaceAdapter::init();
    ChunkManager::instance().init();

    class NavGenMRUProvider : public MRUProvider
    {
        virtual void set( const BW::string& name, const BW::string& value )
        {
            g_navgenSettings->getRootSection()->writeString( name, value );
        }
        virtual const BW::string get( const BW::string& name ) const
        {
            return g_navgenSettings->getRootSection()->readString( name );
        }
    };
    static NavGenMRUProvider NavGenMRUProvider;
    g_spaceManager = new SpaceNameManager( NavGenMRUProvider );

    Waters::instance().init();

    EnviroMinder::init();

    bool quitNow = false;
    if( space.empty() )
    {
        if( !g_spaceManager->num() || !changeSpace( g_spaceManager->entry( 0 ) ) )
        {
            for(;;)
            {
                MsgBox mb( L"Open space",
                    L"NavGen cannot find a valid default space to open, do you want to open "
                    L"one or exit NavGen?",
                    L"&Open", L"E&xit" );
                int result = mb.doModal( g_hWindow );
                if( result == 0 )
                {
                    BW::string space = g_spaceManager->browseForSpaces( g_hWindow );
                    space = BWResolver::dissolveFilename( space );
                    if( !space.empty() && changeSpace( space ) )
                        break;
                }
                else
                {
                    quitNow = true;
                }
            }
        }
    }
    else if( !changeSpace( space ) )
    {
        quitNow = true;
    }

    if (quitNow)
    {
        ChunkManager::instance().fini();
        Waters::instance().fini();
        EnviroMinder::fini();
        s_pTextureFeeds.reset();
        s_pTerrainManager.reset();
        ExitProcess( 3 );
    }

    // This is not used by NavGen but needs to be initialised
    // in order to use EnviroMinder.  The fini method is called
    // at the end of WinMain
    FootPrintRenderer::init();

    // Set fogging
    float farFog = Moo::rc().camera().farPlane();
    float nearFog = 0;

    if (farFog > g_chunkLoadDistance)
    {
        ChunkManager::instance().cameraSpace()->enviro().setFarPlaneBaseLine(
            g_chunkLoadDistance );
        ChunkManager::instance().cameraSpace()->enviro().setFarPlane(
            g_chunkLoadDistance );
        farFog = g_chunkLoadDistance;
    }

    Moo::FogParams params = Moo::FogHelper::pInstance()->fogParams();
    params.m_enabled = 1.f; // 0.f means not enabled
    params.m_start = nearFog;
    params.m_end   = farFog;
    params.m_color = 0xFFFFFFFF;    
    Moo::FogHelper::pInstance()->fogParams(params);


    // Clear key states
    clearMooKeys();

    // Load girths
    Girths girths;

    for (uint i=0; i < girths.size(); ++i)
    {
        const Girth& girth = girths[ i ];

        g_girthSpecs.insert( std::make_pair( girth.girth(), girth ) );

        if (girth.always())
        {
            g_girthsAlwaysGenerate.push_back( girth.girth() );
        }

        g_girthsViewable.push_back( girth.girth() );
    }

    // is 0.5 in the list? if not, complain.
    if (g_girthSpecs.find( 0.5f ) == g_girthSpecs.end())
    {
        CRITICAL_MSG( "Girth = 0.5 specification not found.\n" );
    }

    // Manually set the far-plane to something more managable, we don't need
    // thousands of chunks in memory at any given moment.
    ChunkManager::instance().autoSetPathConstraints(g_chunkLoadDistance);

    // make chunk manager unload as many chunks as possible
    ChunkManager::instance().maxUnloadChunks( 100 );
    // And that's it
    return true;
}

void renderRompPreScene(float dTime)
{
    BW_GUARD;

    ChunkManager::instance().cameraSpace()->enviro().drawHind( dTime );
}

void renderRompDelayedScene( float dTime )
{
    BW_GUARD;

    ChunkManager::instance().cameraSpace()->enviro().drawHindDelayed( dTime );
}

void renderRompPostScene( float dTime )
{
    BW_GUARD;

    Moo::rc().MRTSupport().bind();

    Waters::instance().tick( dTime );

    Waters::instance().updateSimulations( dTime );

    Waters::instance().drawDrawList( dTime );

    Moo::rc().MRTSupport().unbind();

    LensEffectManager::instance().tick( dTime );

    LensEffectManager::instance().draw();
}

bool changeSpace( const BW::string& space )
{
    BW_GUARD;

    static int id = 1;

    if( g_currentSpace == space )
        return true;

    if( !BWResource::fileExists( space + '/' + SPACE_SETTING_FILE_NAME ) )
        return false;

    if( gSpaceLock != INVALID_HANDLE_VALUE )
        CloseHandle( gSpaceLock );
    BW::wstring wfilename;
    bw_utf8tow( BWResolver::resolveFilename( space + "/space.lck" ), wfilename );
    gSpaceLock = CreateFile( wfilename.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL );
    if( gSpaceLock == INVALID_HANDLE_VALUE )
    {
        MsgBox mb( Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OPEN_SPACE_TITLE"),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/UNABLE_TO_OPEN_SPACE", space ),
            Localise(L"WORLDEDITOR/WORLDEDITOR/BIGBANG/BIG_BANG/OK") );
        mb.doModal( g_hWindow );
        return false;
    }

    DataSectionPtr pSpaceSettings = 
        BWResource::openSection( space + "/space.settings" );
    if (!pSpaceSettings)
    {
        MsgBox mb( L"Open space", L"Failed to open space.settings.", L"OK" );
        mb.doModal( g_hWindow );
        return false;
    }

    g_currentNavmeshGenerator = 
        pSpaceSettings->readString( "navmeshGenerator" );
    if (!isCorrectNavmeshGenerator())
    {
        ErrorLogHandler.print( 
            "NOTE: '" + space + "' is not configured to be generated using NavGen" 
            " (generator is set to '" + g_currentNavmeshGenerator + "').", false );
        ErrorLogHandler.print( "NavGen is now in Read Only mode.", false );
    }

    const BW::string oldSpace = g_currentSpace;
    g_currentSpace = space;

    ChunkSpacePtr pOldSpace = ChunkManager::instance().cameraSpace();
    if (pOldSpace.getObject())
        pOldSpace->enviro().deactivate();

    SpaceManager::instance().clearSpaces();

    // flush all preloading requests to avoid using too much memory
    Moo::rc().preloadDeviceResources( 10000000 );

    ClientSpacePtr pSpace = SpaceManager::instance().findOrCreateSpace( id );
    ChunkSpacePtr chunkSpace = ChunkManager::instance().space( id, false );
    MF_ASSERT( chunkSpace );
    ++id;
    Matrix& nonConstIdentity = const_cast<Matrix&>(Matrix::identity);
    g_mapping = chunkSpace->addMapping( SpaceEntryID(), (float*)nonConstIdentity, space );
    if( !g_mapping )
    {
        g_spaceManager->removeSpaceFromRecent( space );
        g_navgenSettings->save();

        // Load the old space back
        g_currentSpace = "";
        MF_ASSERT( changeSpace( oldSpace ) );
        return false;
    }
    chunkSpace->terrainSettings()->defaultHeightMapLod( 0 );

    DataSectionPtr spaceSection = BWResource::openSection( space + "/space.localsettings" );
    if( spaceSection )
    {
        Vector3 dir, pos;
        pos = spaceSection->readVector3( "startPosition", Vector3( 0.f, 2.f, 0.f ) );
        dir = spaceSection->readVector3( "startDirection" );
        Matrix m;
        m.setIdentity();
        m.setRotate( dir[2], dir[1], dir[0] );
        m.translation( pos );
        g_camMatrix = m;
        m.invert();
        Moo::rc().view( m );
    }
    else
    {
        Vector3 pos = Vector3( 0.f, 2.f, 0.f );
        Matrix m;
        m.setIdentity();
        m.translation( pos );
        g_camMatrix = m;
        m.invert();
        Moo::rc().view( m );
    }

    DeprecatedSpaceHelpers::cameraSpace( pSpace );
    ChunkManager::instance().camera( g_camMatrix, chunkSpace );
    ChunkManager::instance().tick( 0.f );
    ChunkManager::instance().cameraSpace()->enviro().timeOfDay()->
        secondsPerGameHour(0.f);
    ChunkManager::instance().cameraSpace()->enviro().activate();

    g_spaceManager->addSpaceIntoRecent( space );
    g_navgenSettings->save();

    setupBWLockd( space, chunkSpace->gridSize() );

    // Manually set the far-plane to something more managable, we don't need
    // thousands of chunks in memory at any given moment.
    ChunkManager::instance().autoSetPathConstraints(g_chunkLoadDistance);

    updateTitle( space );

    for (;;)
    {
        Chunk* pChunk = ChunkManager::instance().cameraChunk();
        if (pChunk != NULL)
        {
            BW::string filePath = BWResolver::resolveFilename( pChunk->resourceID() );
            doFileOpen( filePath.c_str(), pChunk );
            break;
        }
        BgTaskManager::instance().tick();
        FileIOTaskManager::instance().tick();
        ChunkManager::instance().tick( 0.f );

        Sleep( 50 );
    }
    return true;
}

void moveMooCameraFromKeys( float dTime )
{
    BW_GUARD;

    Vector3 mvec( 0.f, 0.f, 0.f );
    if ( s_mooKeyStates[ 'W' ] )
        mvec.z += 1.f;
    if ( s_mooKeyStates[ 'S' ] )
        mvec.z -= 1.f;
    if ( s_mooKeyStates[ 'A' ] )
        mvec.x -= 1.f;
    if ( s_mooKeyStates[ 'D' ] )
        mvec.x += 1.f;
    if ( s_mooKeyStates[ 'Q' ] )
        mvec.y -= 1.f;
    if ( s_mooKeyStates[ 'E' ] )
        mvec.y += 1.f;

    float moveRate = s_mooCamMoveRate;
    if ( s_mooKeyStates[ VK_CONTROL ] )
        moveRate *= 5.f;
    if ( s_mooKeyStates[ VK_SHIFT ] )
        moveRate *= 5.f;
    if ( GetKeyState( VK_CAPITAL ) & 1 )
        moveRate *= 10.f;

    Vector3 newCamPos = g_camMatrix.applyToOrigin() +
        g_camMatrix.applyVector( mvec * dTime * moveRate );
    ChunkSpacePtr cameraSpace = ChunkManager::instance().cameraSpace();
    if( cameraSpace->gridBounds().intersects( newCamPos ) )
        g_camMatrix.translation( newCamPos );
}

/**
 *  Returns whether or not the camera is outside
 */
bool isCameraOutside()
{
    BW_GUARD;

    Chunk * pCC = ChunkManager::instance().cameraChunk();
    return pCC == NULL || pCC->isOutsideChunk();
}

void updateMoo( float dTime, bool allowMovement )
{
    BW_GUARD;

    g_lastTick = dTime;

    g_conn.tick();
    if (allowMovement) moveMooCameraFromKeys( dTime );

    Matrix viewMatrix; viewMatrix.invert( g_camMatrix );

    Moo::rc().reset();
    Moo::rc().view( viewMatrix );
    Moo::rc().updateProjectionMatrix();
    Moo::rc().updateViewTransforms();
    ChunkManager::instance().camera( Moo::rc().invView(), ChunkManager::instance().cameraSpace() );

    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

    Moo::rc().setFVF( D3DFVF_XYZRHW );
    Moo::rc().setVertexShader( NULL );

    BgTaskManager::instance().tick();
    FileIOTaskManager::instance().tick();
    ProviderStore::tick( dTime );
    ChunkManager::instance().tick( dTime );
    ChunkManager::instance().cameraSpace()->enviro().tick( dTime, isCameraOutside() );
    g_specialConsoleString = "";

    if (g_pView && !g_pView->identifier().empty())
    {
        updateTitle( g_currentSpace + " - " + g_pView->identifier() );
    }
    else
    {
        updateTitle( g_currentSpace );
    }
}

static BW::vector<Vector3> s_colTriDebug;


void drawMooPolygon( int pidx )
{
    BW_GUARD;

    int v, a, vertexCount = g_pView->getVertexCount(pidx);
    Vector2 v1;
    bool adjToAnotherChunk;

    if ( vertexCount <= 2 ) 
    {
        return;
    }

    float h = g_pView->getMaxHeight(pidx) + 0.05f;  // for z fighting

    static Moo::VertexXYZL polygonBuf[300];

    uint32 col = (pidx == g_selectedIndex ? 0x99ff0000 : 0x990000ff);

    for (v=0; v<vertexCount; v++)
    {
        g_pView->getVertex(pidx, v, v1, a, adjToAnotherChunk);
        polygonBuf[v].pos_ = Vector3( v1.x, h, v1.y );
        polygonBuf[v].colour_ = col;
    }

    if ( g_viewPolygonArea )
    {
        Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, vertexCount - 2,
            &polygonBuf[0], sizeof( Moo::VertexXYZL ) );
    }

    if ( g_viewPolygonBorders )
    {
        polygonBuf[vertexCount].pos_ = polygonBuf[0].pos_;
        for (v=0; v<=vertexCount; v++) polygonBuf[v].colour_ = 0x99ffff00;
        Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, vertexCount,
            &polygonBuf[0], sizeof( Moo::VertexXYZL ) );
    }
}

void drawMooBSPNodes()
{
    BW_GUARD;

    IWaypointView::BSPNodeInfo bni;
    Vector2 vp;

    Moo::VertexXYZL polygonBuf[32];

    int cursor = 0;
    int cursorMax = 1 << g_bspNodesDepth;
    while ( cursor < cursorMax )
    {
        int depth = g_pView->getBSPNode( cursor, g_bspNodesDepth, bni );
        if (depth == -1) break;
        cursor += 1 << depth;

        if (!bni.internal_ && !bni.waypoint_) continue;
        uint32 colA = bni.internal_ ? 0x00085511 : 0x00081155;
        uint32 colB = bni.internal_ ? 0x00083311 : 0x00081133;

        int nv = static_cast<int>(bni.boundary_.size());
        if (nv <= 2) continue;

        // first the top
        for (int v=0; v < nv; v++)
        {
            vp = bni.boundary_[v];
            polygonBuf[v].pos_ = Vector3( vp.x, bni.maxHeight_, vp.y );
            polygonBuf[v].colour_ = colA;
        }
        Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, nv - 2,
            &polygonBuf[0], sizeof( Moo::VertexXYZL ) );

        // now the bottom
        for (int v=0; v < nv; v++)
        {
            polygonBuf[v].pos_.y = bni.minHeight_;
            polygonBuf[v].colour_ = colB;
        }
        Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, nv - 2,
            &polygonBuf[0], sizeof( Moo::VertexXYZL ) );

        // and finally the sides
        for (int v=0; v <= nv; v++)
        {
            vp = bni.boundary_[v % nv];
            polygonBuf[v*2].pos_ = Vector3( vp.x, bni.minHeight_, vp.y );
            polygonBuf[v*2].colour_ = colA;
            polygonBuf[v*2+1].pos_ = Vector3( vp.x, bni.maxHeight_, vp.y );
            polygonBuf[v*2+1].colour_ = colB;
        }
        Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, nv*2,
            &polygonBuf[0], sizeof( Moo::VertexXYZL ) );
        // hmmm, could prolly do these will with the same buffer
        // using funky stride lengths... anyway, later.
    }
}

void drawMooGridPiece( int cursor )
{
    BW_GUARD;

    Vector3 pos;
    Vector4 ahgts;
    if (!g_pView->gridPoll( cursor, pos, ahgts )) return;

    uint32 col = 0x55ffffff;

    const int dx[4] = { 0, 1, 1, 1 };
    const int dz[4] = { 1, 1, 0, -1 };

    Moo::VertexXYZL lineBuf[8];
    int nlines = 0;
    for (int i = 0; i < 4; i++)
    {
        if (ahgts[i] <= -9999.f) continue;
        lineBuf[nlines*2].pos_ = pos;
        lineBuf[nlines*2].colour_ = col;
        lineBuf[nlines*2+1].pos_ = Vector3( pos.x + dx[i], ahgts[i], pos.z + dz[i] );
        lineBuf[nlines*2+1].colour_ = col;
        nlines++;
    }

    if (nlines > 0)
    {
        Moo::rc().drawPrimitiveUP(
            D3DPT_LINELIST, nlines, &lineBuf[0], sizeof( Moo::VertexXYZL ) );
    }
}

void drawMooColTri( Vector3 * pVertex )
{
    BW_GUARD;

    Moo::VertexXYZL triBuf[4];
    for (int v = 0; v < 4; v++)
    {
        triBuf[v].pos_ = pVertex[v%3];
        triBuf[v].colour_ = 0xaa00ff00;
    }

    Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 3,
        &triBuf[0], sizeof( Moo::VertexXYZL ) );
}

void drawMooWaypointView()
{
    if (!g_pView) return;

    Moo::rc().setRenderState( D3DRS_CLIPPING, FALSE );
    
    static Moo::Material polyMat;
    static bool polyMatInitted = false;
    if (!polyMatInitted)
    {
        polyMat.load( "helpers/materials/addvertex.mfm" );
        polyMatInitted = true;
        polyMat.fogged(false);
    }
    polyMat.set();
    Moo::rc().setVertexShader( NULL );
    Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
    Moo::rc().device()->SetPixelShader( NULL );
    Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

    float reso = g_pView->gridResolution();
    Vector3 min = g_pView->gridMin();
    Matrix resMatrix;
    resMatrix.setScale( reso, 1.f, reso );
    resMatrix.translation( Vector3( min.x, 0.f, min.z ) );
    Moo::rc().device()->SetTransform( D3DTS_WORLD, &resMatrix );
    Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
    Moo::rc().device()->SetTransform(D3DTS_PROJECTION, &Moo::rc().projection());


    if (g_viewAdjacencies)
    {
        for (int i = 0; i < g_pView->gridPollCount(); i++)
        {
            drawMooGridPiece(i);
        }
    }

    if (g_viewBSPNodes)
    {
        Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
        drawMooBSPNodes();
        Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
        Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    }

    if (g_viewPolygonArea || g_viewPolygonBorders)
    {
        for(int i = 0; i < g_pView->getPolygonCount(); i++)
        {
            if (!g_pView->equivGirth( i, g_girthsViewable[g_selectedGirth] )) 
                continue;

            if (isSetOfNavmeshVisible( i ))
            {
                drawMooPolygon(i);
            }
        }
    }

    Moo::rc().device()->SetTransform( D3DTS_WORLD, &Matrix::identity );
    Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );

    for (uint i = 0; i < s_colTriDebug.size(); i+=3)
    {
        drawMooColTri( &s_colTriDebug[i] );
    }

    Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );

    Moo::rc().setRenderState( D3DRS_CLIPPING, TRUE );
}

void drawMoo(float dTime)
{
    BW_GUARD;

    // Only draw if the render window is showing
    MF_ASSERT_DEV( NavgenScopedMenuDisabler::isMooWindowShowing() );

    InvalidateRect( g_hWindow, NULL, FALSE );

    if (!Moo::rc().checkDevice())
    {
        return;
    }

    Renderer::instance().tick(dTime);
    IRendererPipeline* rp = Renderer::instance().pipeline();
    
    ChunkManager::instance().updateAnimations();
    ChunkManager::instance().cameraSpace()->enviro().updateAnimations();

    // Update "tracking" objects after updating all nodes' positions
    ParticleSystemAction::flushLateUpdates();

    Moo::rc().beginScene();
    
    rp->begin();

    Moo::DrawContext shadowContext( Moo::RENDERING_PASS_SHADOWS );
    rp->beginCastShadows( shadowContext );
    rp->endCastShadows();

    //-- set up sun lighting.
    const ChunkSpacePtr& pSpace = ChunkManager::instance().cameraSpace();
    if (pSpace)
    {
        Moo::SunLight sun;
        sun.m_dir     = pSpace->sunLight()->worldDirection();
        sun.m_color   = pSpace->sunLight()->colour();
        sun.m_ambient = pSpace->ambientLight();

        Moo::rc().effectVisualContext().sunLight(sun);
    }

    renderRompPreScene(dTime);

    rp->beginOpaqueDraw();

    Moo::DrawContext navgenDrawContext( Moo::RENDERING_PASS_COLOR );
    navgenDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK );

    Moo::rc().effectVisualContext().initConstants();

    DX::Viewport viewport;
    viewport.Width = (DWORD)Moo::rc().screenWidth();
    viewport.Height = (DWORD)Moo::rc().screenHeight();
    viewport.MinZ = 0.f;
    viewport.MaxZ = 1.f;
    viewport.X = 0;
    viewport.Y = 0;
    Moo::rc().setViewport( &viewport );

    Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
        0x00ffffff, 1, 0 );
    Moo::rc().setWriteMask( 1, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_RED|
        D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_ALPHA );
    Moo::rc().nextFrame();
    Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | 
        D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

    if (Moo::rc().mixedVertexProcessing())
        Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_ALL);  
    
    ChunkManager::instance().draw( navgenDrawContext );

    // Update terrain lods
    Terrain::BasicTerrainLodController::instance().setCameraPosition( 
        Moo::rc().invView().applyToOrigin(), Moo::rc().zoomFactor() );
    Terrain::BaseTerrainRenderer::instance()->drawAll( 
        navgenDrawContext.renderingPassType() );

    ChunkManager::instance().cameraSpace()->enviro().drawFore(dTime);

    Chunks_drawCullingHUD();

    navgenDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
    navgenDrawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

    rp->endOpaqueDraw();

    //-- do lighting pass.
    rp->applyLighting();

    drawMooWaypointView();
    renderRompDelayedScene(dTime);

    rp->beginSemitransparentDraw();
    // note that we are not flushing opaque channel after this
    // if we hit assert because of it then we need to move renderRompPostScene higher
    navgenDrawContext.begin( Moo::DrawContext::ALL_CHANNELS_MASK ); 
    renderRompPostScene( dTime );
    navgenDrawContext.end( Moo::DrawContext::ALL_CHANNELS_MASK );
    navgenDrawContext.flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK | Moo::DrawContext::SHIMMER_CHANNEL_MASK );
    // flush main transparent data and romp transparent data
    rp->endSemitransparentDraw();

    rp->end();
    Moo::rc().endScene();
    Moo::rc().device()->Present( NULL, NULL, NULL, NULL );


    // The first time the view is shown, we should show the chunks with the
    // borders and polygon as though the user has just pressed enter.
    static bool firstRun = true;
    if (firstRun)
    {
        Chunk * pChunk = ChunkManager::instance().cameraChunk();
        if (pChunk != NULL) 
        {
            firstRun = false;
            chunkOp(CO_DISPLAY);
        }
    }
    Chunks_drawCullingHUD();
}


void dragMoo()
{
    BW_GUARD;

    POINT curr;
    GetCursorPos(&curr);

    const int dx = curr.x - g_mooDragStart.x;
    const int dy = curr.y - g_mooDragStart.y;

    float yaw = g_camMatrix.yaw();
    float pitch = g_camMatrix.pitch();
    const float roll = g_camMatrix.roll();

    if (fabs(roll) < MATH_PI / 2.0f)
    {
        yaw += float(dx) * 0.02f;
        pitch += float(dy) * 0.02f;
    }
    else
    {
        yaw -= float(dx) * 0.02f;
        pitch -= float(dy) * 0.02f;
    }

    Matrix newCam; 
    newCam.setRotate( yaw, pitch, roll );
    newCam.translation( g_camMatrix.applyToOrigin() );
    g_camMatrix = newCam;

    g_mooDragStart = curr;
}


void reannotate( Chunk * pChunk )
{
    BW_GUARD;

    ShowWindow( g_hMooWindow, SW_SHOW );
    BringWindowToTop(g_hMooWindow);
    float girth = g_girthsViewable[g_selectedGirth];

    // first load this chunk
    BW::string filePath = BWResolver::resolveFilename(
        pChunk->resourceID() );
    doFileOpen( filePath.c_str(), pChunk );
    if (!g_pView) return;

    LONG_PTR oldWndProc = SetWindowLongPtr( g_hMooWindow, GWLP_WNDPROC, (LONG_PTR)reannotatingWndProc );
    EnableWindow( g_hWindow, FALSE );

    // clear any existing annotations
    for (int p = 0; p < g_pView->getPolygonCount(); p++)
    {
        IWaypointView::PolygonRef pd( *g_pView, p );
        for (uint v = 0; v < pd.size(); v++)
        {
            if (pd[v].adjNavPoly() < 0) pd[v].adjNavPoly( 0 );
        }
    }

    // now re-annotate all the edges
    INFO_MSG( "Annotating polygons\n" );
    WaypointAnnotator wanno( g_pView, pChunk->space() );
    wanno.annotate( true, girth );

#if 0
    // find the chunk's section
    DataSectionPtr pChunkSect = BWResource::openSection(
        pChunk->resourceID() );

    // add the modified waypoints
    g_pView->writeToSection( pChunkSect, pChunk, girth, true );

    // and save it out!
    pChunkSect->save();
#else
    g_pView->saveOut( pChunk, girth, /*removeAllOld:*/true );
#endif

    // store debug triangles for display in 3D view
    s_colTriDebug = wanno.colTriDebug_;

    SetWindowLongPtr( g_hMooWindow, GWLP_WNDPROC, oldWndProc );
    EnableWindow( g_hWindow, TRUE );
}


void selectMooWaypoint( bool doubleClicked )
{
    BW_GUARD;

    POINT p;
    RECT r;

    GetCursorPos(&p);
    ScreenToClient(g_hMooWindow, &p);
    GetClientRect(g_hMooWindow, &r);

    if(r.right == 0 || r.bottom == 0 || !g_pView) return;

    Vector3 source = Moo::rc().invView().applyToOrigin();
    Vector3 dir = Moo::rc().invView().applyVector(
        Moo::rc().camera().nearPlanePoint(
            (float(p.x) / r.right)*2-1,
            1-(float(p.y) / r.bottom)*2 ) );

    dir.normalise();
    if (fabsf(dir.y) < 0.001f) return;

    Vector3 min = g_pView->gridMin();
    source.x -= min.x;
    source.z -= min.z;

    float reso = g_pView->gridResolution();
    source.x /= reso;
    source.z /= reso;
    dir.x /= reso;
    dir.z /= reso;

    int pc = g_pView->getPolygonCount();
    int pidx;
    for (pidx = 0; pidx < pc; pidx++)
    {
        if (!g_pView->equivGirth( pidx, g_girthsViewable[g_selectedGirth] )) 
            continue;

        if (!isSetOfNavmeshVisible( pidx ))
            continue;

        // project ray into plane of this height
        float h = g_pView->getMaxHeight(pidx);
        Vector3 wpt = source + dir * (h-source.y) / dir.y;
        Vector2 wpt2( wpt.x, wpt.z );

        // see if it's inside the polygon
        int vc = g_pView->getVertexCount(pidx);
        int v;
        for (v = 0; v < vc; v++)
        {
            Vector2 lastV, thisV;
            int adj;
            bool adjToAnotherChunk;
            g_pView->getVertex(pidx,v,lastV,adj,adjToAnotherChunk);
            g_pView->getVertex(pidx,(v+1)%vc,thisV,adj,adjToAnotherChunk);

            LineEq leq( lastV, thisV );
            if (leq.isInFrontOf( wpt2 )) 
                break;
        }

        if (v == vc && vc > 0) 
            break;
    }

    if (pidx == pc) pidx = -1;

    selectNavPolyCommon( pidx, doubleClicked );
}



/**
 *  Do an operation on the chunk the camera is currently in
 */
void chunkOp( ChunkOp op )
{
    BW_GUARD;

    Chunk * pChunk = ChunkManager::instance().cameraChunk();
    if (pChunk == NULL) 
        return;

    ScopedSetReady scopedSetReady;

    setCalculating(0, 0); // Display "Calculating" in the status bar
    switch (op)
    {
        case CO_DISPLAY:
        {
            BW::string filePath =
                BWResolver::resolveFilename(
                    pChunk->resourceID() );
            doFileOpen( filePath.c_str(), pChunk );
            break;
        }

        case CO_GENERATE:
            doGenerate( pChunk );
            break;

        case CO_REANNOTATE:
            reannotate( pChunk );
            break;
    }
}

LRESULT CALLBACK wndProcMoo(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch(msg)
    {
        case WM_CLOSE:
            clearMooKeys();
            ShowWindow(hwnd, SW_HIDE);
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return TRUE;
        }

        case WM_ACTIVATE:
            if ( LOWORD( w ) == WA_INACTIVE )
                clearMooKeys();
            return TRUE;

        case WM_LBUTTONDOWN:
            selectMooWaypoint( false );
            return TRUE;

        case WM_LBUTTONDBLCLK:
            selectMooWaypoint( true );
            return TRUE;


        case WM_RBUTTONDOWN:
            GetCursorPos(&g_mooDragStart);
            g_mooDragging = true;
            SetCapture(hwnd);
            return TRUE;

        case WM_MOUSEMOVE:
            if(g_mooDragging)
            {
                dragMoo();
            }
            return TRUE;

        case WM_RBUTTONUP:
            if(g_mooDragging)
            {
                g_mooDragging = false;
                ReleaseCapture();
                dragMoo();
            }
            return TRUE;

        case WM_KEYDOWN:
            s_mooKeyStates[static_cast<unsigned char>(w)] = true;
            break;

        case WM_KEYUP:
            s_mooKeyStates[static_cast<unsigned char>(w)] = false;
            break;

        case WM_CHAR:
            switch( LOWORD(w) )
            {
                case VK_RETURN:
                    chunkOp( CO_DISPLAY );
                    break;

                case '!':
                case '@':
                    chunkOp( CO_GENERATE );
                    break;

                case '*':
                    if (g_reannotation)
                    {
                        chunkOp( CO_REANNOTATE );
                    }
                    break;

                case ',':
                    g_bspNodesDepth = std::max(g_bspNodesDepth-1,0);
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    break;
                case '.':
                    g_bspNodesDepth = std::min(g_bspNodesDepth+1,31);
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    break;

                case '[':
//                  g_selectedGirth = std::max(g_selectedGirth,uint(1))-1;
                    g_selectedGirth -= 1;
                    if (g_selectedGirth < 0)
                    {
                        g_selectedGirth = 0;
                    }
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    showCursorPos();
                    break;
                case ']':
//                  g_selectedGirth = std::min(g_selectedGirth+1,g_girths.size()-1);
                    g_selectedGirth += 1;
                    if (g_selectedGirth >= (int)g_girthsViewable.size())
                    {
                        g_selectedGirth = static_cast<int>(g_girthsViewable.size() - 1);
                    }
                    InvalidateRect(g_hWindow, NULL, FALSE);
                    showCursorPos();
                    break;
            }
            return TRUE;
    }

    return DefWindowProc( hwnd, msg, w, l );
}

static bool getParam( BW::string* cmdLine,
    const BW::string& paramSwitch, BW::string* param )
{
    BW_GUARD;

    MF_ASSERT( cmdLine );
    MF_ASSERT( param );

    BW::string::iterator iter = std::search(
        cmdLine->begin(), cmdLine->end(), paramSwitch.begin(), paramSwitch.end() );

    if (iter != cmdLine->end())
    {
        BW::string::iterator paramStart = iter + paramSwitch.size();
        BW::string::iterator paramEnd;

        if (paramStart == cmdLine->end())
        {
            return false;
        }

        if (*paramStart == '\"')
        {
            ++paramStart;
            paramEnd = std::find( paramStart, cmdLine->end(), '\"' );

            if (paramEnd == cmdLine->end())
            {
                return false;
            }

            param->assign( paramStart, paramEnd );
            cmdLine->erase( iter, paramEnd + 1 );
        }
        else
        {
            paramEnd = std::find( paramStart, cmdLine->end(), ' ' );

            param->assign( paramStart, paramEnd );
            cmdLine->erase( iter, paramEnd );
        }
        return true;
    }
    return false;
}


int bwWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR commandLine, int)
{
    BW_GUARD;
    BW_SYSTEMSTAGE_MAIN();

    CStdMf cstdMf;

    GetSubMenu( NULL, 1 );

    BW::string cmdLine = bw_wtoutf8( commandLine );

    int argc = 0;
    char** argv = NULL;

    INITCOMMONCONTROLSEX initControls;
    initControls.dwSize = sizeof( initControls );
    initControls.dwICC = 0x8ffff; // EVERYTHING!!!!
    InitCommonControlsEx( &initControls );

    parseCommandLineMF( cmdLine.c_str(), argc, argv );

    // This local variable ensures scoped creation/destruction of the BWResource
    // instance.
    BWResource bwResourceHolder;

    // init BWResource
    BWResource::init( argc, (const char **)argv );

    // Resource glue initialisation
    if (!AutoConfig::configureAllFrom("resources.xml"))
    {
        CRITICAL_MSG( "Failed to load resources.xml. " );
        return false;
    }

    //Load the "engine_config.xml" file...
    DataSectionPtr configRoot = BWResource::instance().openSection( s_engineConfigXML.value() );
    
    if (!configRoot)
    {
        ERROR_MSG( "App::init: Couldn't open \"%s\"\n", s_engineConfigXML.value().c_str() );
        return false;
    }

    XMLSection::shouldReadXMLAttributes( configRoot->readBool( 
        "shouldReadXMLAttributes", XMLSection::shouldReadXMLAttributes() ));
    XMLSection::shouldWriteXMLAttributes( configRoot->readBool( 
        "shouldWriteXMLAttributes", XMLSection::shouldWriteXMLAttributes() ));

    // Read some options
    BW::string settingsFilename = BWResource::appDirectory() + "navgen_settings.xml";
    for (int i = 0; i < argc - 1; i++)
    {
        if (strcmp( "--settings", argv[i] ) == 0)
        {
            settingsFilename = argv[ i+1 ];
        }
    }
    if( !BWResource::fileAbsolutelyExists( settingsFilename ) )
    {
        CRITICAL_MSG( "Cannot find NavGen setting file : navgen_settings.xml\n" );
        return false;
    }
    g_navgenSettings = new DataResource( settingsFilename );
    if( !g_navgenSettings || !g_navgenSettings->getRootSection() )
    {
        CRITICAL_MSG( "Cannot open NavGen setting file : navgen_settings.xml\n" );
        return false;
    }

    g_annotate = g_navgenSettings->getRootSection()->readBool( "annotate", false );

    if( !s_LanguageFile.value().empty() )
        StringProvider::instance().load( BWResource::openSection( s_LanguageFile ) );

    BW::vector<DataSectionPtr> languages;
    g_navgenSettings->getRootSection()->openSections( "language", languages );
    if (!languages.empty())
    {
        for( BW::vector<DataSectionPtr>::iterator iter = languages.begin(); iter != languages.end(); ++iter )
            if( !(*iter)->asString().empty() )
                StringProvider::instance().load( BWResource::openSection( (*iter)->asString() ) );
    }
    else
    {
        StringProvider::instance().load( BWResource::openSection("helpers/languages/navgen_rc_en.xml" ));
        StringProvider::instance().load( BWResource::openSection("helpers/languages/files_en.xml"          ));
    }
    BW::wstring currentLanguage;
    BW::wstring currentCountry; 
    bw_utf8tow( g_navgenSettings->getRootSection()->readString( "currentLanguage", "" ), currentLanguage );
    bw_utf8tow( g_navgenSettings->getRootSection()->readString( "currentCountry", "" ), currentCountry );
    if ( currentLanguage != L"" )
        StringProvider::instance().setLanguages( currentLanguage, currentCountry );
    else
        StringProvider::instance().setLanguage();

    wchar_t s[1024];
    GetWindowText( ErrorLogHandler.handle(), s, ARRAY_SIZE( s ) );
    WindowTextNotifier::instance().set( ErrorLogHandler.handle(), s );
    WindowTextNotifier::instance().changed();

    WNDCLASS wc;
    MSG msg;

    processCommandLine( argc, argv );

    // From this point on, argc and argv are no longer valid.
    delete [] s_cmdLineBuffer;
    s_cmdLineBuffer = NULL;
    delete [] s_argv;
    s_argv = NULL;
    argc = 0;
    argv = NULL;

    wc.style = CS_DBLCLKS | CS_OWNDC;
    wc.lpfnWndProc = wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = L"navgen";

    if(!RegisterClass(&wc))
        return 0;

    wc.lpfnWndProc = wndProcMoo;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.lpszClassName = L"navgenMoo";
    wc.lpszMenuName = NULL;
    if(!RegisterClass(&wc))
        return 0;

    g_hWindow = CreateWindow(L"navgen", L"NavPoly Generator",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    if(!g_hWindow)
        return 0;

    MsgBox::setDefaultParent( g_hWindow );

    GetDC(g_hWindow);
    WindowTextNotifier::instance().set( GetMenu( g_hWindow ) );

    if( !g_statusWindow.create( g_hWindow ) )
        return 0;

    g_hMooWindow = CreateWindow( L"navgenMoo", L"NavPoly Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL);

    if (!g_hMooWindow)
        return 0;

    g_hInfoDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_WAYPOINT_INFO), g_hWindow,
        infoDialogProc);

    if(!g_hInfoDialog)
        return 0;

#if ENABLE_ASSET_PIPE
    typedef std::auto_ptr< AssetClient > AssetClientPtr;
    AssetClientPtr pAssetClient(new AssetClient);
#endif

    if(!setupGL())
        return 0;

    // Initialise Moo library
    if(!Moo::init())
        return 0;

    g_hMenu = GetMenu(g_hWindow);
    g_viewAdjacencies = (GetMenuState(g_hMenu, ID_VIEW_ADJACENCIES, MF_BYCOMMAND) & MF_CHECKED) != 0;
    g_viewBSPNodes = (GetMenuState(g_hMenu, ID_VIEW_BSPNODES, MF_BYCOMMAND) & MF_CHECKED) != 0;
    g_viewPolygonArea = (GetMenuState(g_hMenu, ID_VIEW_POLYGONAREA, MF_BYCOMMAND) & MF_CHECKED) != 0;
    g_viewPolygonBorders = (GetMenuState(g_hMenu, ID_VIEW_POLYGONBORDERS, MF_BYCOMMAND) & MF_CHECKED) != 0;

    g_processor = g_navgenSettings->getRootSection()->readInt("processor", 1);
    g_writeTGAs = g_navgenSettings->getRootSection()->readBool("writeTGAs", true);
    g_reannotation = g_navgenSettings->getRootSection()->readBool( "reannotation", false );
    DataSectionPtr graphicsPreferences =
        g_navgenSettings->getRootSection()->openSection("graphicsPreferences");
    if(graphicsPreferences.get())
        Moo::GraphicsSetting::init(graphicsPreferences);
    if (!g_reannotation)
    {
        DeleteMenu( g_hMenu, ID_CHUNK_REANNOTATE, MF_BYCOMMAND );
    }

    g_chunkLoadDistance = g_navgenSettings->getRootSection()->readFloat("loadDistance", g_chunkLoadDistance);
    BgTaskManager::init();
    BgTaskManager::instance().startThreads( "Chunk Loading Thread", 1 );

    FileIOTaskManager::init();
    FileIOTaskManager::instance().startThreads("File IO Thread", 1);

    InputDevices inputDevices;
    InputDevices::instance().init( hInstance, g_hWindow );

    bool workInCommandLine = false;

    if (strstr( cmdLine.c_str(), "/s" ))
    {
        workInCommandLine = true;

        BW::string space;

        if (!getParam( &cmdLine, "/s", &space ))
        {
            MessageBox( GetForegroundWindow(),
                L"Failed to parse command param /s",
                L"NavGen", MB_OK );
            return 3;
        }

        if( !BWResource::openSection( space + "/space.settings" ) )
        {
            BW::string msg = ( "Cannot find space " + space );
            BW::wstring wmsg;
            bw_utf8tow( msg, wmsg );
            MessageBox( GetForegroundWindow(), wmsg.c_str(),
                L"NavGen", MB_OK );
            return 3;
        }

        // TODO:UNICODE: Unicode cmdline?
        if (const char* p = strstr( cmdLine.c_str(), "/g" ))
        {
            BW::string file;

            if (!getParam( &cmdLine, "/g", &file ))
            {
                MessageBox( GetForegroundWindow(),
                    L"Failed to parse command param /g",
                    L"NavGen", MB_OK );
                return 3;
            }

            BW::wstring wfile;
            bw_utf8tow( file, wfile );
            DWORD attr = GetFileAttributes( wfile.c_str() );

            if( attr == INVALID_FILE_ATTRIBUTES || ( attr & FILE_ATTRIBUTE_DIRECTORY ) )
            {
                MessageBox( GetForegroundWindow(), 
                    ( L"Cannot open file " + wfile ).c_str(),
                    L"NavGen", MB_OK );
                return 3;
            }

            std::ifstream ifs( file.c_str() );
            BW::string chunkName;

            while( std::getline( ifs, chunkName ) )
            {
                while( !chunkName.empty() && isspace( *chunkName.begin() ) )
                    chunkName.erase( chunkName.begin() );
                while( !chunkName.empty() && isspace( *chunkName.rbegin() ) )
                    chunkName.resize( chunkName.size() - 1 );
                if( !chunkName.empty() )
                    g_chunkSet.insert( chunkName );
            }

            if (g_chunkSet.empty())
            {
                INFO_MSG( "Have nothing to generate automatically\n" );
                return 0;
            }
        }

#if ENABLE_ASSET_PIPE
        if (!setupChunking( space, pAssetClient.get() ))
            return 3;
#else
        if (!setupChunking( space ))
            return 3;
#endif          
    }
    else
    {
#if ENABLE_ASSET_PIPE
        if (!setupChunking( "", pAssetClient.get() ))
            return 0;
#else
        if (!setupChunking( "" ))
            return 0;
#endif
    }

    SetTimer( g_hMooWindow, 0, 1000, NULL ); 
    stampsPerSecond();

    ShowWindow(g_hWindow, SW_SHOW);
    BringWindowToTop(g_hWindow);
    setReady();

    g_accelerators = 
        LoadAccelerators(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ACCELERATORS));

    // By default enable the polygon area and borders:
    if (!g_viewPolygonArea)
    {
        ::SendMessage(g_hWindow, WM_COMMAND, ID_VIEW_POLYGONAREA, 0);
    }
    if (!g_viewPolygonBorders)
    {
        ::SendMessage(g_hWindow, WM_COMMAND, ID_VIEW_POLYGONBORDERS, 0);
    }

    if (workInCommandLine)
    {
        updateMoo( 0.f );
        doGenerateAll( !!strstr( cmdLine.c_str(), "/overwrite" ) );
    }
    else
    {
        Automation::parseCommandLine( commandLine );

        uint64 timeLast = timestamp();
        while(1)
        {
            if (g_pleaseShutdown)
                break;

            // process windows events
            while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ))
            {
                if (!GetMessage(&msg, NULL, 0, 0)) 
                    break;
                if (TranslateAccelerator(g_hWindow, g_accelerators, &msg))
                    continue;
                if (!IsDialogMessage(g_hInfoDialog, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            // find how long it's been since last time
            uint64 timeNow = timestamp();
            float dTime = float(
                double(timeNow-timeLast) / stampsPerSecondD() );
            timeLast = timeNow;

            MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

            // update script
            incrementTotalTime( dTime );
            Script::tick( getTotalTime() );

            // update the scene
            updateMoo( dTime );

            // draw the scene
            if (IsWindowVisible( g_hMooWindow ))
                drawMoo( dTime );
        }
    }

    doClearAll();
    MainLoopTasks::finiAll();

    BgTaskManager::instance().stopAll();    
    FileIOTaskManager::instance().stopAll();

    Moo::rc().releaseDevice();

    if(g_glrc)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_glrc);
    }

    if (ChunkManager::instance().cameraSpace().exists())
        ChunkManager::instance().cameraSpace()->enviro().deactivate();

    g_navgenSettings = NULL; // force cleanup here
    delete g_spaceManager;
    g_spaceManager = NULL;
    
    Script::fini();
    DeprecatedSpaceHelpers::cameraSpace( NULL );
    Waters::instance().fini();
    ChunkManager::instance().fini();
    SpaceManager::instance().fini();
    FootPrintRenderer::fini();
    EnviroMinder::fini();
    s_pTextureFeeds.reset();
    s_pTerrainManager.reset();
    MaterialKinds::fini();


    if( gSpaceLock != INVALID_HANDLE_VALUE )
        CloseHandle( gSpaceLock );

    s_pLensEffectManager.reset();
    s_pFontManager.reset();
    Moo::VertexDeclaration::fini();

    g_pRenderer.reset();

#if ENABLE_ASSET_PIPE
    pAssetClient.reset();
#endif

    Moo::fini();
    BWResource::fini();

    WindowTextNotifier::fini();
    BgTaskManager::fini();
    FileIOTaskManager::fini();

    DataSectionCensus::fini();

    return 0;
}


INT_PTR CALLBACK clusterDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    BW_GUARD;

    switch(msg)
    {
        case WM_INITDIALOG:
            for( int i = 2; i < 50; ++i )
            {
                wchar_t s[1024];
                bw_snwprintf( s, ARRAY_SIZE(s), L"%d", i );
                SendDlgItemMessage( hwnd, IDC_TOTALCOMPUTER, CB_ADDSTRING, 0, (LPARAM)s );
            }
            SendDlgItemMessage( hwnd, IDC_TOTALCOMPUTER, CB_SETCURSEL, 0, 0 );
            for( int i = 0; i < 2; ++i )
            {
                wchar_t s[1024];
                bw_snwprintf( s, ARRAY_SIZE(s), L"%d", i + 1 );
                SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_ADDSTRING, 0, (LPARAM)s );
            }
            SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_SETCURSEL, 0, 0 );
            g_totalComputers = 2;
            g_myIndex = 0;
            return TRUE;

        case WM_COMMAND:
            if( HIWORD( w ) == CBN_SELCHANGE )
            {
                if( LOWORD( w ) == IDC_TOTALCOMPUTER )
                {
                    SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_RESETCONTENT, 0, 0 );

                    g_totalComputers = SendDlgItemMessage( hwnd, IDC_TOTALCOMPUTER, CB_GETCURSEL, 0, 0 ) + 2;

                    for( int i = 0; i < g_totalComputers; ++i )
                    {
                        wchar_t s[1024];
                        bw_snwprintf( s, ARRAY_SIZE(s), L"%d", i + 1 );
                        SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_ADDSTRING, 0, (LPARAM)s );
                    }
                    SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_SETCURSEL, 0, 0 );
                    g_myIndex = 0;
                }
                else if( LOWORD( w ) == IDC_MYINDEX )
                {
                    g_myIndex = SendDlgItemMessage( hwnd, IDC_MYINDEX, CB_GETCURSEL, 0, 0 );
                }
            }
            else if( HIWORD( w ) == BN_CLICKED )
            {
                EndDialog( hwnd, LOWORD( w ) );
            }
            break;
    }

    return FALSE;
}

// This needs to be defined for navgen because duplo 
// actually depends on BigWorldClientScript::callNextFrame 
// is EDITOR_ENABLED is not defined and, you guess it right, 
// it isn't defined for navgen, causing unresolved external 
// symbol errors.
namespace BigWorldClientScript
{
    void callNextFrame( PyObject *, PyObject *, const char *, double )
    {}
}

BW_END_NAMESPACE


BW_USE_NAMESPACE

static PyObject *py_exit(PyObject * /*args*/)
{
    BW_GUARD;

    PostQuitMessage(-1);
    g_pleaseShutdown = true;

    Py_RETURN_NONE;
}
PY_MODULE_FUNCTION(exit, NavGen)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPWSTR commandLine, int cmdShow)
{
    return CallWithExceptionFilter( bwWinMain, hInstance, hPrev, commandLine, cmdShow );
}
