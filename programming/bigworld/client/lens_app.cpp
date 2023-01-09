#include "pch.hpp"
#include "lens_app.hpp"


#include "romp/lens_effect_manager.hpp"
#include "romp/progress.hpp"


#include "app.hpp"
#include "canvas_app.hpp"
#include "device_app.hpp"

#include "moo/renderer.hpp"
#include "moo/custom_AA.hpp"
#include "moo/ppaa_support.hpp"


BW_BEGIN_NAMESPACE

LensApp LensApp::instance;

int LensApp_token = 1;


LensApp::LensApp() : 
    dGameTime_( 0.f )
{ 
    BW_GUARD;
    MainLoopTasks::root().add( this, "Lens/App", NULL ); 
}

LensApp::~LensApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "Lens/App" );*/ 
}

bool LensApp::init()
{
    BW_GUARD;
    bool result = false;
    if (DeviceApp::s_pStartupProgTask_)
    {
        result = DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
    }

    if (App::instance().isQuiting())
    {
        result = false;
    }
    return result;
}


void LensApp::fini()
{
    BW_GUARD;
}


void LensApp::tick( float dGameTime, float /* dRenderTime */ )
{
    // I'm not sure this is the correct thing to be ticking
    // LensEffectManager by, but on inspection it's not used anyway.
    // See LensEffect::tick
    dGameTime_ = dGameTime;
}


void LensApp::draw()
{
    BW_GUARD;
    GPU_PROFILER_SCOPE(LensApp_draw);
    if(!gWorldDrawEnabled)
        return;

    IRendererPipeline* rp = Renderer::instance().pipeline();
    rp->endSemitransparentDraw();

    LensEffectManager::instance().tick( dGameTime_ );
    LensEffectManager::instance().draw();

    //-- resolve fsaa buffer.
    Moo::rc().customAA().ppaa().apply();

    // Finish off the back buffer filters now
    CanvasApp::instance.finishFilters();
    // Note: I have moved this after drawFore, because I reckon the things
    // in there (seas, rain) prolly want to be affected by the filters too.

}

BW_END_NAMESPACE

// lens_app.cpp
