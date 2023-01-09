#include "pch.hpp"
#include "facade_app.hpp"

#include "app.hpp"
#include "camera_app.hpp"
#include "canvas_app.hpp"
#include "connection_control.hpp"
#include "device_app.hpp"
#include "entity_picker.hpp"
#include "script_player.hpp"
#include "time_globals.hpp"
#include "world_app.hpp"

#include "chunk/chunk_manager.hpp"

#include "connection/smart_server_connection.hpp"

#include "duplo/foot_print_renderer.hpp"
#include "romp/lens_effect_manager.hpp"

#include "moo/draw_context.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_data_section.hpp"

#include "romp/distortion.hpp"
#include "moo/geometrics.hpp"
#include "romp/progress.hpp"
#include "romp/water.hpp"

#include "moo/renderer.hpp"

#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"


BW_BEGIN_NAMESPACE

static DogWatch g_sortedWatch( "DrawSorted" );


FacadeApp FacadeApp::instance;

int FacadeApp_token = 1;


/**
 *  This class notifies the game script to set the time of day on the server
 *  when it changes on the client.
 */
class ServerTODUpdater : public TimeOfDay::UpdateNotifier
{
public:
    ServerTODUpdater() {}

    virtual void updated( const TimeOfDay & tod )
    {
        ScriptObject personality = Personality::instance();
        if (!personality) return;

        personality.callMethod( "onTimeOfDayLocalChange",
            ScriptArgs::create( tod.gameTime(), tod.secondsPerGameHour() ),
            ScriptErrorPrint( "ServerTODUpdater notification: " ),
            /* allowNullMethod */ true );
    }
};



FacadeApp::FacadeApp() : dGameTime_( 0.f )
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "Facade/App", NULL );
}


FacadeApp::~FacadeApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "Facade/App" );*/
}


bool FacadeApp::init()
{
    BW_GUARD;
    CameraApp::instance().entityPicker().selectionFoV( DEG_TO_RAD( 10.f ) );
    CameraApp::instance().entityPicker().selectionDistance( 80.f );
    CameraApp::instance().entityPicker().deselectionFoV( DEG_TO_RAD( 80.f ) );

    todUpdateNotifier_ = new ServerTODUpdater();

    Waters::instance().init();
    FootPrintRenderer::init();

    if (App::instance().isQuiting())
    {
        return false;
    }

    return DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
}


void FacadeApp::fini()
{
    BW_GUARD;
    Waters::instance().fini();
    FootPrintRenderer::fini();

    if ( lastCameraSpace_.exists() )
    {
        EnviroMinder & enviro = lastCameraSpace_->enviro();
        enviro.deactivate();
    }

    // throw away this reference
    lastCameraSpace_ = NULL;
}


/*~ function BWPersonality.onCameraSpaceChange
 *  @components{ client }
 *  This callback method is called on the personality script when
 *  the camera has moved to a new space, and the space's environment
 *  has been set up.  The space ID and the space.settings file is
 *  passed in as arguments.
 */
void FacadeApp::tick( float dGameTime, float /* dRenderTime */ )
{
    BW_GUARD_PROFILER( AppTick_Facade );
    dGameTime_ = dGameTime;

    // update the weather
    ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
    if (!pSpace) return;

    // update the entity picker
    //  (must be here, after the camera position has been set)
    CameraApp::instance().entityPicker().update( dGameTime );

    EnviroMinder & enviro = pSpace->enviro();

    // if this space is new to us than transfer our Facade apparatus to it
    if (lastCameraSpace_ != pSpace)
    {
        // removing it from the old one first if necessary
        if (lastCameraSpace_)
        {
            EnviroMinder & oldEnviro = lastCameraSpace_->enviro();
            oldEnviro.deactivate();
            oldEnviro.timeOfDay()->delUpdateNotifier( todUpdateNotifier_ );
        }

        lastCameraSpace_ = pSpace;
        enviro.activate();
        enviro.timeOfDay()->addUpdateNotifier( todUpdateNotifier_ );

        // Probably not the ideal spot for this now.
        TimeGlobals::instance().setupWatchersFirstTimeOnly();

        // Inform the personality script that the space has changed.
        ScriptObject personality = Personality::instance();
        if (personality)
        {
            ScriptObject enviroData;
            if (enviro.pData())
            {
                enviroData = ScriptObject( new PyDataSection( enviro.pData() ),
                    ScriptObject::FROM_NEW_REFERENCE );
            }
            else
            {
                enviroData = ScriptObject::none();
            }

            personality.callMethod( "onCameraSpaceChange",
                ScriptArgs::create( pSpace->id(), enviroData ),
                ScriptErrorPrint( "Personality Camera Space Change notification: " ),
                /* allowNullMethod */ true );
        }
    }

    enviro.tick( dGameTime, isCameraOutside() );
}


void FacadeApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_Facade );
    GPU_PROFILER_SCOPE( AppDraw_Facade );

    IRendererPipeline* rp = Renderer::instance().pipeline();

    rp->beginSemitransparentDraw();

    // Draw the main batch of sorted triangles
    Moo::rc().setRenderState( D3DRS_FILLMODE,
        (WorldApp::instance.wireFrameStatus_ & 2) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

    if (WorldApp::instance.debugSortedTriangles_ % 4)
    {
        switch ( WorldApp::instance.debugSortedTriangles_ % 4 )
        {
        case 1:
            Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0x00, 1, 0 );

            Moo::Material::setVertexColour();
            Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
            Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
            Moo::rc().setTexture(0, Moo::TextureManager::instance()->get(s_blackTexture)->pTexture());
            Geometrics::texturedRect( Vector2(0.f,0.f),
                    Vector2(Moo::rc().screenWidth(),Moo::rc().screenHeight()),
                    Moo::Colour( 1.f,1.f,1.f, 0.75f ), true );
            break;
        case 2:
            Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0x00, 1, 0 );

            Moo::Material::setVertexColour();
            Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT );
            Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
            Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
            Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
            Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
            Moo::rc().setTexture(0, Moo::TextureManager::instance()->get(s_blackTexture)->pTexture());
            Geometrics::texturedRect( Vector2(0,0),
                    Vector2(Moo::rc().screenWidth(),Moo::rc().screenHeight()),
                    Moo::Colour( 0.f, 1.f, 1.f, 1.f ), true );
            Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
            break;
        case 3:
            Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0x00000000, 1, 0 );

            Moo::Material::setVertexColour();
            Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
            Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
            Moo::rc().setTexture(0, Moo::TextureManager::instance()->get(s_blackTexture)->pTexture());
            Geometrics::texturedRect( Vector2(0.f,0.f),
                    Vector2(Moo::rc().screenWidth(),Moo::rc().screenHeight()),
                    Moo::Colour( 1.f,1.f,1.f, 1.f ), true );
            break;
        }
    }
    Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

    ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
    if (pSpace)
    {
        // Draw waters
        Waters& phloem = Waters::instance();
        Entity * pPlayer = ScriptPlayer::entity();
         
        if (pPlayer != NULL)
        {
            phloem.playerPos( pPlayer->position() );
            WaterSceneRenderer::setPlayerModel( pPlayer->pPrimaryModel() );
        }

        // Update the water simulations
        phloem.updateSimulations( dGameTime_ );

        // Draw the distortion buffer (including water)
        CanvasApp::instance.updateDistortionBuffer();

        //-- draw overlays.
        pSpace->enviro().drawFore( dGameTime_, true, false, false, true, false );
    }

    // Draw the sorted triangles
    g_sortedWatch.start();
    {
        GPU_PROFILER_SCOPE( ColourDrawContext_sortedFlush );
        App::instance().drawContext( App::COLOUR_DRAW_CONTEXT ).flush( Moo::DrawContext::TRANSPARENT_CHANNEL_MASK );
    }
    g_sortedWatch.stop();
}

BW_END_NAMESPACE


// facade_app.cpp
