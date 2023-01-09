#include "pch.hpp"
#include "finale_app.hpp"

#include "ashes/gui_progress.hpp"
#include "moo/streamed_animation_channel.hpp"
#include "moo/effect_manager.hpp"
#include "pyscript/personality.hpp"
#include "pyscript/py_data_section.hpp"
#include "romp/flora.hpp"
#include "romp/font_manager.hpp"
#include "romp/progress.hpp"
#include "moo/lod_settings.hpp"
#include "duplo/py_model_renderer.hpp"
#include "ashes/simple_gui_component.hpp"
#include "terrain/terrain_graphics_options.hpp"
#include "pyscript/py_callback.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "connection_control.hpp"
#include "device_app.hpp"
#include "entity_manager.hpp"
#include "script_bigworld.hpp"
#include "open_automate/open_automate_wrapper.hpp"

#include "moo/renderer.hpp"


BW_BEGIN_NAMESPACE

/// This is set to true just before Finale app calls start on the personality script
bool g_bAppStarted = false;

FinaleApp FinaleApp::instance;

int FinalApp_token = 1;


FinaleApp::FinaleApp() :    cancelPreloads_( false ),
                            disablePreloads_( false ),
                            donePreloads_( false )
{
    BW_GUARD;
    MainLoopTasks::root().add( this, "Finale/App", NULL );
}


FinaleApp::~FinaleApp()
{
    BW_GUARD;
    /*MainLoopTasks::root().del( this, "Finale/App" );*/
}


bool FinaleApp::init()
{
    BW_GUARD;
    if (App::instance().isQuiting())
    {
        return false;
    }
    DataSectionPtr configSection = AppConfig::instance().pRoot();

    // Run any preloads

    bool ret = true;

#if ENABLE_ASSET_PIPE
    if (configSection->readBool( "renderer/cacheEffects", true ))
    {
        // First cache all of the effects used by the system.
        BW::vector<BW::string> effectNames;
        this->findEffects( "shaders/std_effects/", effectNames );
        this->findEffects( "shaders/std_effects/shadows/", effectNames );
        this->findEffects( "shaders/post_processing/", effectNames );
        this->findEffects( "shaders/decal/", effectNames );
        this->findEffects( "shaders/environment/", effectNames );
        this->findEffects( "shaders/speedtree/", effectNames );
        this->findEffects( "shaders/flora/", effectNames );
        this->findEffects( "shaders/terrain/", effectNames );
        this->findEffects( "shaders/water/", effectNames );
        ret = this->cacheEffects( effectNames );
    }
    else
#endif // ENABLE_ASSET_PIPE
    {
        // Tick the progress bar because shader compilation / caching uses "1"
        if( DeviceApp::s_pProgress_ )
        {
            ret = DeviceApp::s_pStartupProgTask_->step(1.f) &&
                    DeviceApp::s_pProgress_->draw( true );
        }
    }

    // Register the graphics settings for the effect files.
    // This is done by loading up effect files that define the settings.
    if( DeviceApp::s_pProgress_ )
    {
        DeviceApp::s_pProgress_->add( "Registering graphics settings" );
    }

    BW::vector<BW::string> settingsEffects_;
    configSection->readStrings( "renderer/settings/effect", settingsEffects_ );
    for (uint32 i = 0; i < settingsEffects_.size(); i++)
    {
        Moo::ManagedEffectPtr pEffect = 
            Moo::EffectManager::instance().get( settingsEffects_[i] );

        if (pEffect.hasObject())
        {
            pEffect->registerGraphicsSettings( settingsEffects_[i] );
        }
    }

    DataSectionPtr pSection = BWResource::instance().openSection( s_graphicsSettingsXML );

    // setup far plane options
    DataSectionPtr farPlaneOptionsSec = pSection->openSection("farPlane");
    EnviroMinderSettings::instance().init(farPlaneOptionsSec);

    // setup flora options
    DataSectionPtr floraOptionsSec = pSection->openSection("flora");
    FloraSettings::instance().init(floraOptionsSec);

    // setup object lod options
    DataSectionPtr lodOptionsSec = pSection->openSection("objectLOD");
    LodSettings::instance().init(lodOptionsSec);

    Moo::StreamedAnimation::s_cacheMaxSize_ =
        ( configSection->readInt( "animation/streamCacheSizeKB", 1024 )<<10 );

    // TODO: Move this whole functionality into the personality script!
    // It just needs to be able to access a sequence of entity modules.
    if (ret && !(disablePreloads_ = configSection->readBool( "entities/disablePreloads", false )))
    {
#if ENABLE_WATCHERS
        DEBUG_MSG( "App::init: Before preloading using %d(~%d)KB\n",
            memUsed(), memoryAccountedFor() );
#endif

        if (DeviceApp::s_pGUIProgress_)
        {
            DeviceApp::s_pProgress_->add( "Preloading Resources" );
            this->runPreloads( *DeviceApp::s_pStartupProgTask_ );
        }
        else
        {
            ProgressTask    preloadTask =
                ProgressTask( DeviceApp::s_pProgress_, "Preloading Resources", 1.f );

            this->runPreloads( preloadTask );
        }

        Moo::EffectManager::instance().registerGraphicsSettings();

#if ENABLE_WATCHERS
        DEBUG_MSG( "App::init: After preloading using %d(~%d)KB\n",
            memUsed(), memoryAccountedFor() );

        DEBUG_MSG( "App::init: Preloaded %d PyObjects\n", preloadedPyObjects_.size() );
        DEBUG_MSG( "App::init: Preloaded %d DataSections\n", preloadedDataSections_.size() );   
        DEBUG_MSG( "App::init: Preloaded %d Textures\n", preloadedTextures_.size() );
        DEBUG_MSG( "App::init: Preloaded %d Effects\n", preloadedEffects_.size() );

#endif
    }

    // Import the personality script
    BW::string persName = configSection->readString(
        "personality", Personality::DEFAULT_NAME );

    if (!Personality::import( persName ))
    {
        criticalInitError(
                "App::initPersonality: Could not load personality module <%s>!",
                persName.c_str() );
        return false;
    }

    Personality::callOnInit( /* isReload */ false );

    if (App::instance().isQuiting())
    {
        return false;
    }
    if (DeviceApp::s_pStartupProgTask_)
    {
        ret = ret && DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
    }

    DataSectionPtr scriptsConfig;
    if (!s_usingDeprecatedBigWorldXML)
    {
        scriptsConfig = BWResource::instance().openSection(s_scriptsConfigXML);
        if (!scriptsConfig.exists())
        {
            criticalInitError(
                    "App::initPersonality: Could not load scripts config file <%s>!",
                    s_scriptsConfigXML.value().c_str() );

            return false;
        }
    }

    //  Call personality.init function
    int extraParam = s_usingDeprecatedBigWorldXML ? 0 : 2;
    PyObject * pTuple = NULL;
    if (DeviceApp::s_pGUIProgress_)
    {
        pTuple = PyTuple_New( 2 + extraParam );
        PyObject * gui = DeviceApp::s_pGUIProgress_->gui().getObject();
        Py_INCREF(gui);
        PyTuple_SetItem( pTuple, 1 + extraParam, &*gui );
    }
    else
    {
        pTuple = PyTuple_New( 1 + extraParam );
    }

    DataSectionPtr engineConfig = AppConfig::instance().pRoot();
    if (!s_usingDeprecatedBigWorldXML)
    {
        PyTuple_SetItem( pTuple, 0, new PyDataSection( scriptsConfig ) );
        PyTuple_SetItem( pTuple, 1, new PyDataSection( engineConfig ) );
        PyTuple_SetItem( pTuple, 2, new PyDataSection( s_scriptsPreferences ) );
    }
    else
    {
        PyTuple_SetItem( pTuple, 0, new PyDataSection( engineConfig ) );
    }

    Personality::instance().callMethod( "init",
        ScriptArgs( pTuple, ScriptObject::FROM_NEW_REFERENCE ),
        ScriptErrorPrint( "App::initPersonality: " ),
        /* allowNullMethod */ false );

    //  Load it into the __main__ modules list too
    persName = "import " + persName;
    PyRun_SimpleString( const_cast<char*>( persName.c_str() ) );

    g_bAppStarted = true;

    // And start the game!
    Personality::instance().callMethod( "start",
        ScriptErrorPrint( "App::init (start personality): " ),
        /* allowNullMethod */ false );

    return ret;
}


/**
 *  Finalises this FinaleApp.
 */
void FinaleApp::fini()
{
    BW_GUARD;

    Script::clearTimers();

    // Notify connection control that we are shutting down here
    // in case the personality script calls BigWorld.disconnect()
    ConnectionControl::instance().notifyOfShutdown();
    // Disconnect before the fini in script has been called during Script::call.
    // Otherwise disconnect will call script functions that use objects
    // that are destroyed. Furthermore, if we don't pass false into disconnect
    // it will call some python callback that use destroyed C++ timer objects.
    ConnectionControl::instance().disconnect(false);

    // Tell the personality script about it
    if (Personality::instance())
    {
        Personality::instance().callMethod( "fini",
            ScriptErrorPrint( "App::~App: " ),
            /* allowNullMethod */ false );
    }

    while (preloadedPyObjects_.size())
    {
        Py_DECREF(preloadedPyObjects_.back());
        preloadedPyObjects_.pop_back();

    }

    preloadedTextures_.clear();
    preloadedDataSections_.clear();
    preloadedEffects_.clear();

    PyModelRenderer::fini();
}


void FinaleApp::tick( float /* dGameTime */, float /* dRenderTime */ )
{
}


void FinaleApp::draw()
{
    BW_GUARD_PROFILER( AppDraw_Finale );

    // end rendering
    static DogWatch endSceneWatch("EndScene");
    endSceneWatch.start();

    Renderer::instance().pipeline()->end();

    Moo::rc().endScene();
    //For open automate benchmark we need to call openautomate 
    if  (BWOpenAutomate::s_runningBenchmark == true)
    {
        oaDisplayFrame( oaFloat( App::instance().getRenderTimeFrameStart() ) );
    }
    Moo::rc().present();

    endSceneWatch.stop();

    // Only care on warning about loading of files
    // in the main thread if we are drawing the world
    BWResource::watchAccessFromCallingThread(gWorldDrawEnabled);
}


bool FinaleApp::handleKeyEvent(const KeyEvent & event)
{
    BW_GUARD;
    bool handled = false;

    // check if this is a key up event
    if (event.isKeyDown())
    {
        if ( InputDevices::isKeyDown( KeyCode::KEY_JOYBACK ) )
            this->cancelPreloads_ = true;
        if ( InputDevices::isKeyDown( KeyCode::KEY_JOYSTART ) )
            this->cancelPreloads_ = true;
        if ( InputDevices::isKeyDown( KeyCode::KEY_ESCAPE ) )
            this->cancelPreloads_ = true;
        if ( InputDevices::isKeyDown( KeyCode::KEY_APPS ) )
            this->cancelPreloads_ = true;
    }

    return 1;
}


/**
 *  This method runs any preload activities
 */
void FinaleApp::runPreloads( ProgressTask & preloadTask )
{
    BW_GUARD;

    PyObject * pPreloadList = EntityType::getPreloads();

    Py_ssize_t listlen = PyList_Size( pPreloadList );

    float dStep = 1.f / (listlen + 2.f);
    preloadTask.step(dStep);

    for (int i=0; i < listlen; i++, preloadTask.step(dStep) )
    {
        InputDevices::processEvents( *this );
        if (this->cancelPreloads_ || App::instance().isQuiting())
            break;

        // make sure it's reasonable
        PyObject * pString = PyList_GetItem( pPreloadList, i );
        if (!PyString_Check( pString ) && !PyTuple_Check( pString ))
        {
            WARNING_MSG( "App::init: Couldn't preload resource named "
                "in element %d of list because it's not a string "
                "or a tuple\n", i );
            continue;
        }

        // see if it's a texture
        if (PyString_Check( pString ))
        {
            char * str = PyString_AsString( pString );
            if ( strlen( str ) == 0 )
            {
                WARNING_MSG( "App::init: Empty preload resource "
                    "in element %d of list\n", i );
                continue;
            }

            char * pend = strrchr( str, '.' );

            if (pend != NULL && (!strcmp( pend, ".bmp" ) ||
                !strcmp( pend, ".tga" ) || !strcmp( pend, ".dds" )))
            {
                Moo::BaseTexturePtr pTex =
                    Moo::TextureManager::instance()->get( str );
                if (!pTex)
                {
                    WARNING_MSG( "App::init: "
                        "Could not preload texture '%s'\n", str );
                }
                else
                {
                    preloadedTextures_.push_back( pTex );

                    //TRACE_MSG( "App::init: "
                    //  "After loading texture %s using %dKB\n",
                    //  str, memUsed() );
                }
                continue;
            }
            else if ((pend != NULL && !(strcmp( pend, ".font" ) )))
            {
                FontManager::instance().get( str );
                //TRACE_MSG( "App::init: "
                //      "After loading font %s using %dKB\n",
                //      str, memUsed() );
                continue;
            }
            else if ((pend != NULL && !(strcmp( pend, ".fx" ) )))
            {               
                DataSectionPtr pSect = BWResource::instance().openSection( str );
                if (pSect)
                {
                    Moo::ManagedEffectPtr pEffect = 
                        Moo::EffectManager::instance().get( str );

                    if (pEffect.hasObject())
                    {
                        preloadedEffects_.push_back( pEffect );
                        //TRACE_MSG( "App::init: "
                        //  "Preloaded effect file %s.  Mem use now %dKB\n",
                        //  str, memUsed() );
                    }
                    else
                    {
                        WARNING_MSG( "App::init: "
                            "Could not preload effect '%s'\n", str );
                    }
                }
                continue;
            }   
            else if (!(pend != NULL && !(strcmp( pend, ".model" ) )))
            {
                DataSectionPtr pSect = BWResource::instance().openSection( str );
                if (pSect)
                {
                    preloadedDataSections_.push_back( pSect );
                    //TRACE_MSG( "App::init: "
                    //  "After loading resource %s using %dKB\n",
                    //  str, memUsed() );
                }
                continue;
            }                   

        }

        // ok, assume it's a model then
        SmartPointer<PyObject> pModelStr( PyObject_Str( pString ), true );
        if (!pModelStr)
        {
            WARNING_MSG( "App::init: "
                "Couldn't do str() on elt %d of preload list!\n", i );
            PyErr_PrintEx(0);
            PyErr_Clear();

            continue;
        }

        // find out what we should call it
        char * str = pModelStr ?
            PyString_AsString( pModelStr.getObject() ) : NULL;

        // figure out what arguments to pass to the factory method
        bool isString = PyString_Check( pString );
        PyObject * pArgs = pString;
        if (isString)
        {
            pArgs = PyTuple_New( 1 );
            PyTuple_SetItem( pArgs, 0, pString );
            Py_INCREF( pString );
        }

        // make the model!
        PyObject * pModel = PyModel::pyNew( pArgs );

        // decref the tuple if we made one
        if (isString) Py_DECREF( pArgs );

        // complain if we couldn't load it
        if (!pModel)
        {
            WARNING_MSG( "App::init: "
                "Couldn't preload a model %s\n", str );
            PyErr_PrintEx(0);
            PyErr_Clear();
            continue;
        }

        preloadedPyObjects_.push_back( pModel );

        // leak this reference so the model stays loaded
        //Py_DECREF( pModel );

        // and that's all there is to it
        //TRACE_MSG( "App::init: "
        //  "After loading model %s using %dKB\n", str, memUsed() );
    }

    Py_DECREF( pPreloadList );
    preloadTask.step(dStep);
    donePreloads_ = true;
}


void FinaleApp::findEffects( const BW::StringRef & folderName,
    BW::vector< BW::string > & ret ) const
{
    BW_GUARD;
    DataSectionPtr pFolder = BWResource::openSection( folderName );
    if (pFolder)
    {
        static const StringRef FX_EXTN = "fx";
        char resourceID[BW_MAX_PATH];
        bw_str_copy( resourceID, ARRAY_SIZE( resourceID ), folderName );
        char * pathEnd = resourceID + folderName.size();
        const size_t remainingSize = ARRAY_SIZE( resourceID ) - std::min(
            ARRAY_SIZE( resourceID ), folderName.size() );

        BW::string childName;
        int numChildren = pFolder->countChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            childName = pFolder->childSectionName( i );
            if (BWResource::getExtension( childName ) == FX_EXTN)
            {
                bw_str_copy( pathEnd, remainingSize,
                    childName );
                ret.push_back( resourceID );
            }
        }
    }
}


bool FinaleApp::cacheEffects( const BW::vector<BW::string> & effects ) const
{
    BW_GUARD;
    //TODO: if this system has alot of ram... preload the shaders as well as compiling? 

    DeviceApp::s_pProgress_->add( "Caching effects" );
    DeviceApp::s_pProgress_->draw( true );

    float step = 1.f / (float)(effects.size() + 1);
    for (uint i=0; i<effects.size(); i++)
    {
        Moo::EffectManager::instance().ensureCompiled(effects[i]);
        if( DeviceApp::s_pProgress_ )
        {
            DeviceApp::s_pStartupProgTask_->step(step);
            DeviceApp::s_pProgress_->draw( true );
        }
    }

    if( DeviceApp::s_pProgress_ )
    {
        DeviceApp::s_pStartupProgTask_->step(step);
        DeviceApp::s_pProgress_->draw( true );
    }

    return true;
}

BW_END_NAMESPACE

// finale_app.cpp
