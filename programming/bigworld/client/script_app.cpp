#include "pch.hpp"

#include "script_app.hpp"

#include "action_matcher.hpp"
#include "app.hpp"
#include "app_config.hpp"
#include "connection_control.hpp"
#include "device_app.hpp"
#include "entity_manager.hpp"
#include "physics.hpp"
#include "portal_state_changer.hpp"
#include "script_bigworld.hpp"

#include "ashes/gui_progress.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/simple_gui_component.hpp"

#include "client/bw_winmain.hpp"

#include "connection_model/bw_connection.hpp"

#include "cstdmf/bwversion.hpp"
#include "duplo/pymodelnode.hpp"

#include "math/colour.hpp"

#include "pyscript/personality.hpp"
#include "pyscript/py_callback.hpp"



BW_BEGIN_NAMESPACE

ScriptApp ScriptApp::instance;

int ScriptApp_token = 1;



ScriptApp::ScriptApp()
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Script/App", NULL );
}


ScriptApp::~ScriptApp()
{
	BW_GUARD;
	/*MainLoopTasks::root().del( this, "Script/App" );*/
}


bool ScriptApp::init()
{
	BW_GUARD;
#if ENABLE_WATCHERS
	DEBUG_MSG( "ScriptApp::init: Initially using %d(~%d)KB\n",
		memUsed(), memoryAccountedFor() );
#endif

	if (!BigWorldClientScript::init( AppConfig::instance().pRoot() ))
	{
		criticalInitError( "App::init: BigWorldClientScript::init() failed!" );

		return false;
	}

	// TODO: Other things (e.g. client camera) are depending on Entity Manager being initialised here. This needs to be tidied up.
	EntityManager::instance();

#if ENABLE_WATCHERS
	MF_WATCH( "Client Settings/Entity Collisions", ActionMatcher::globalEntityCollision_,
		Watcher::WT_READ_WRITE, "Enables entity - entity collision testing" );
#endif // ENABLE_WATCHERS

	DataSectionPtr pConfigSection = AppConfig::instance().pRoot();
	SimpleGUI::init( pConfigSection );

	//Now the scripts are setup, we can load a progress bar
	DeviceApp::s_pGUIProgress_ = new GUIProgressDisplay( loadingScreenGUI, BWProcessOutstandingMessages );
	if (DeviceApp::s_pGUIProgress_->gui())
	{
		if (!DeviceApp::s_pGUIProgress_->gui()->script().exists())
		{
			SimpleGUI::instance().addSimpleComponent( 
				*DeviceApp::s_pGUIProgress_->gui() );
		}
		DeviceApp::s_pProgress_    = DeviceApp::s_pGUIProgress_;
	}
	else
	{
		bw_safe_delete(DeviceApp::s_pGUIProgress_);
		// Legacy Progress bar
		//NOTE : only reading the loading screen from ui/loadingScreen
		//for legacy support.  loading screen is not AutoConfig'd from
		//resources.xml in the system/loadingScreen entry.
		if (loadingScreenName.value() == "")
		{
			pConfigSection->readString( "ui/loadingScreen", "" );
		}
		Vector3 colour = pConfigSection->readVector3( "ui/loadingText", Vector3(255,255,255) );
		DeviceApp::s_pProgress_ = new ProgressDisplay( NULL, displayLoadingScreen, Colour::getUint32( colour, 255 ) );
	}

	char buf[ 256 ];
	bw_snprintf( buf, sizeof(buf), "Build: %s %s. Version: %s",
		configString, App::instance().compileTime(), BWVersion::versionString().c_str() );
	DeviceApp::s_pProgress_->add( buf );

	if (DeviceApp::s_pGUIProgress_)
	{
		DeviceApp::s_pStartupProgTask_ = new ProgressTask( DeviceApp::s_pProgress_, "App Startup", PROGRESS_TOTAL );
	}
	else
	{
		//subtract one from progress total because there is no personality script part to it.
		DeviceApp::s_pStartupProgTask_ = new ProgressTask( DeviceApp::s_pProgress_, "App Startup", PROGRESS_TOTAL - 1 );
	}
	if (App::instance().isQuiting())
	{
		return false;
	}
	return DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
}


void ScriptApp::fini()
{
	BW_GUARD;	
#if ENABLE_WATCHERS
	Watcher::fini();
#endif
	DeviceApp::instance.deleteGUI();
	SimpleGUI::instance().clearSimpleComponents();
	BgTaskManager::instance().stopAll();
	BigWorldClientScript::fini();
	SimpleGUI::fini();
}


void ScriptApp::tick( float dGameTime, float /* dRenderTime */ )
{
	BW_GUARD_PROFILER( AppTick_Script );
	double gameTime = App::instance().getGameTimeFrameStart();

	static DogWatch	dwScript("Script");
	dwScript.start();

	BigWorldClientScript::tick();

	// tick all vector4 / matrix providers.  do this before scripts
	// are run, so they can access up-to-date values, and any new
	// providers created are not ticked immediately.
	ProviderStore::tick( dGameTime );

	// very first call any timers that have gone off
	// (including any completion procedures stored by action queues
	//  on their last tick)
	{
		SCOPED_DISABLE_FRAME_BEHIND_WARNING;
		Script::tick( gameTime );
	}

	static DogWatch	dwPhysics("Physics");
	dwPhysics.start();

	// TODO: There is a weird bug related to the following line of code. It
	//	shows up when total time is very large. Try it by setting
	//	App::totalTime_ to something larger than 600,000. It looks as though
	//	the arguments to the following call are rounded to floats. This causes
	//	rounding problems.

	// call all the wards ticks for controlled entities
	// BWEntities::update() has already applied filter outputs,
	// but Physics applies positions with BWEntity::onMoveLocally, which
	// outputs immediately.
	// also note: physics may call callbacks of its own
	Physics::tickAll( gameTime, gameTime - dGameTime );

	dwPhysics.stop();

	static DogWatch dwEntity("Entity");
	dwEntity.start();

	EntityManager::instance().tick( gameTime, gameTime - dGameTime );

	// tell the server anything we've been waiting to
	BWConnection * pConnection = ConnectionControl::instance().pConnection();

	if (pConnection != NULL)
	{
		pConnection->updateServer();
	}

	dwEntity.stop();

	// see if the player's environment has changed
	static int oldPlayerEnvironment = -1;
	int newPlayerEnvironment = !isPlayerOutside();

	if (oldPlayerEnvironment != newPlayerEnvironment)
	{
		Personality::instance().callMethod( "onChangeEnvironments",
			ScriptArgs::create( newPlayerEnvironment ),
			ScriptErrorPrint( "Personality onChangeEnvironments: " ),
			/* allowNullMethod */ false );

		oldPlayerEnvironment = newPlayerEnvironment;
	}

	PortalStateChanger::tick( dGameTime );

	dwScript.stop();
}

void ScriptApp::inactiveTick( float dGameTime, float dRenderTime )
{
	BW_GUARD;
	this->tick( dGameTime, dRenderTime );
}


void ScriptApp::draw()
{

}

BW_END_NAMESPACE

// script_app.cpp
