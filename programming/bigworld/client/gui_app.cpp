#include "pch.hpp"
#include "gui_app.hpp"


#include "ashes/simple_gui.hpp"
#include "ashes/simple_gui_component.hpp"
#include "chunk/chunk_manager.hpp"
#include "cstdmf/base64.h"
#include "moo/draw_context.hpp"
#include "romp/flora.hpp"
#include "romp/progress.hpp"
#include "zip/zlib.h"
#include "scaleform/config.hpp"
#if SCALEFORM_SUPPORT
    #include "scaleform/manager.hpp"
#endif

#include "app.hpp"
#include "app_config.hpp"
#include "device_app.hpp"
#include "romp/watermark.hpp"
#include "moo/renderer.hpp"

#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"

// Define this to one to draw our watermark (BigWorld Technology logo)
#define DRAW_WATERMARK 0


BW_BEGIN_NAMESPACE

GUIApp GUIApp::instance;

int GUIApp_token = 1;



GUIApp::GUIApp()
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "GUI/App", NULL );
}


GUIApp::~GUIApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "GUI/App" );*/
}


bool GUIApp::init()
{
    BW_GUARD;
    if (App::instance().isQuiting())
    {
        return false;
    }
    // access simple gui
    SimpleGUI::instance().hInstance( DeviceApp::s_hInstance_ );
    SimpleGUI::instance().hwnd( DeviceApp::s_hWnd_ );

    DataSectionPtr configSection = AppConfig::instance().pRoot();
#if DRAW_WATERMARK
    //To generate this watermark just use the watermark_creator project
    //with a ppm in similar format to the example.ppm
    int widthNormalWaterMark = 208;
    int heightNormalWaterMark = 50;
    const char b64DataNormalWaterMark[] = "\
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";

    //Indie Water Mark
    int widthIndieWaterMark = 208;
    int heightIndieWaterMark = 64;
    const char b64DataIndieWaterMark[] = "\
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";

    //Academic Water Mark
    int widthAcademicWaterMark = 208;
    int heightAcademicWaterMark = 64;
    const char b64DataAcademicWaterMark[] = "\
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";


    int waterMarkMode = configSection->readInt("superShot/wmode", 1);
    BW::string txstr;
    if (waterMarkMode == 2)
    {
        txstr = decodeWaterMark (b64DataIndieWaterMark, widthIndieWaterMark, heightIndieWaterMark);
    }
    else if (waterMarkMode == 3)
    {
        txstr = decodeWaterMark (b64DataAcademicWaterMark, widthAcademicWaterMark, heightAcademicWaterMark);
    }
    //default is normal watermark
    else 
    {
        txstr = decodeWaterMark (b64DataNormalWaterMark, widthNormalWaterMark, heightNormalWaterMark);
    }

    SimpleGUIComponent * watermark = new SimpleGUIComponent( "" );
    PyTextureProvider * watermarkProv = new PyTextureProvider( NULL,
        Moo::TextureManager::instance()->getUnique( txstr ) );

    watermark->pySet_texture( watermarkProv );
    watermark->widthMode( SimpleGUIComponent::SIZE_MODE_CLIP );
    watermark->heightMode( SimpleGUIComponent::SIZE_MODE_CLIP );
    float ww = 0.3f;
    watermark->width( ww );
    watermark->height( ww / 3.f );
    watermark->filterType( SimpleGUIComponent::FT_LINEAR );
    Py_DECREF( watermarkProv );
    watermark->colour( 0x20ffffff );

    watermark->anchor( SimpleGUIComponent::ANCHOR_H_RIGHT,
        SimpleGUIComponent::ANCHOR_V_BOTTOM );
    watermark->position( Vector3( 0.99f, -0.99f, 0.1f ) );
    //mark that the watermark can't be deleted
    watermark->allowDelete( false );
    SimpleGUI::instance().addSimpleComponent( *watermark );
    Py_DECREF(watermark);
#endif

    return DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
}


void GUIApp::fini()
{
    BW_GUARD;

    //put here to avoid problems when client is shut down at weird spots
    // in the startup loop.
#if ENABLE_WATCHERS
    Watcher::fini();
#endif
    DeviceApp::instance.deleteGUI();
}


void GUIApp::tick( float /* dGameTime */, float dRenderTime )
{
    BW_GUARD_PROFILER( AppTick_GUI );
    static DogWatch dwGUI("GUI");
    dwGUI.start();

    // update the GUI components
    SimpleGUI::instance().update( dRenderTime );

    dwGUI.stop();

#if SCALEFORM_SUPPORT
    ScaleformBW::Manager::instance().tick( dRenderTime );
#endif

}


void GUIApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_GUI );
    static DogWatch dwGUI("GUI");
    dwGUI.start();

    // draw UI
    rp().beginGUIDraw();
    SimpleGUI::instance().draw();
    ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
    if (pSpace != NULL && pSpace->enviro().flora() != NULL)
    {
        pSpace->enviro().flora()->drawDebug();
    }

    rp().endGUIDraw();

    dwGUI.stop();

#if SCALEFORM_SUPPORT
    ScaleformBW::Manager::instance().draw();
#endif
}

BW_END_NAMESPACE


// gui_app.cpp
