#include "pch.hpp"
#include "camera_app.hpp"

#include "app.hpp"
#include "app_config.hpp"
#include "cstdmf/guard.hpp"
#include "camera/annal.hpp"
#include "camera/camera_control.hpp"
#include "camera/projection_access.hpp"
#include "chunk/chunk_manager.hpp"
#include "client_camera.hpp"
#include "device_app.hpp"
#include "pyscript/script.hpp"
#include "script_player.hpp"
#include "romp/progress.hpp"
#include "script/script_object.hpp"

#include "space/client_space.hpp"
#include "space/space_manager.hpp"
#include "space/deprecated_space_helpers.hpp"

BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "camera_app.ipp"
#endif

BW_INIT_SINGLETON_STORAGE( CameraApp )

CameraApp::CameraApp() :
	Singleton(),
	MainLoopTask(),
	entityPicker_(),
	pyAutoAim_( NULL ),
	pyTargeting_( NULL ),
	pyCameraSpeed_( NULL ),
	clientSpeedProvider_( NULL ),
	clientCamera_( NULL )
{
	BW_GUARD;
	
	MainLoopTasks::root().add( this, "Camera/App", NULL );
}


CameraApp::~CameraApp()
{
	BW_GUARD;
	/*MainLoopTasks::root().del( this, "Camera/App" );*/
}


bool CameraApp::init()
{
	BW_GUARD;
	if (App::instance().isQuiting())
	{
		return false;
	}

	pyAutoAim_ = AutoAimPtr( new AutoAim( entityPicker_ ),
		PyObjectPtr::STEAL_REFERENCE );
	pyTargeting_ = TargetingPtr(
		new Targeting( entityPicker_, this->autoAim() ),
		PyObjectPtr::STEAL_REFERENCE );
	pyCameraSpeed_ = CameraSpeedPtr( new CameraSpeed(),
		PyObjectPtr::STEAL_REFERENCE );

	clientSpeedProvider_ =
		new ClientSpeedProvider( this->targeting(), this->cameraSpeed() );
	clientCamera_ = new ClientCamera( this->clientSpeedProvider() );

#if ENABLE_WATCHERS
	DEBUG_MSG( "CameraApp::init: Initially using %d(~%d)KB\n",
		memUsed(), memoryAccountedFor() );
#endif

	DataSectionPtr rootSection = AppConfig::instance().pRoot();
	pyAutoAim_->readInitialValues( rootSection );
	pyTargeting_->readInitialValues( rootSection );
	pyCameraSpeed_->readInitialValues( rootSection );
	clientSpeedProvider_->readInitialValues( rootSection );
 	clientCamera_->init( rootSection );

	MF_WATCH( "Render/Near Plane",
		*clientCamera_->projAccess(),
		MF_ACCESSORS( float, ProjectionAccess, nearPlane ), "Camera near plane." );

	MF_WATCH( "Render/Far Plane",
		*clientCamera_->projAccess(),
		MF_ACCESSORS( float, ProjectionAccess, farPlane ),"Camera far plane." );

	MF_WATCH( "Render/Fov",
		*clientCamera_->projAccess(),
		MF_ACCESSORS( float, ProjectionAccess, fov ), "Field of view." );


	// Set up the camera
	float farPlaneDistance =
		rootSection->readFloat( "camera/farPlane", 250.f );
	Moo::Camera cam( 0.25f, farPlaneDistance, DEG_TO_RAD(60.f), 2.f );
	cam.aspectRatio(
		float(Moo::rc().screenWidth()) / float(Moo::rc().screenHeight()) );
	Moo::rc().camera( cam );

	// read in some ui settings
	CameraControl::strafeRate( rootSection->readFloat(
		"ui/straferate", 100.0 ) );

	CameraControl::initDebugInfo();

	// Add BigWorld.cameraSpeed to the BigWorld module
	ScriptModule pBigworldModule = ScriptModule::import( "BigWorld", ScriptErrorPrint() );
	MF_ASSERT( ScriptModule::check( pBigworldModule ) );

	pBigworldModule.setAttribute( "cameraSpeed",
		this->cameraSpeed(), ScriptErrorPrint() );

	// Add BigWorld.targeting to the BigWorld module
	pBigworldModule.setAttribute( "targeting",
		this->targeting(), ScriptErrorPrint() );

	return DeviceApp::s_pStartupProgTask_->step(APP_PROGRESS_STEP);
}


void CameraApp::fini()
{
	BW_GUARD;

	DeprecatedSpaceHelpers::cameraSpace( NULL );

	ScriptModule pBigworldModule = ScriptModule::import( "BigWorld", ScriptErrorPrint() );
	MF_ASSERT( ScriptModule::check( pBigworldModule ) );

	pBigworldModule.setAttribute("targeting",
		ScriptObject::none(), ScriptErrorPrint() );
	pBigworldModule.setAttribute("cameraSpeed",
		ScriptObject::none(), ScriptErrorPrint() );

	bw_safe_delete( clientCamera_ );
	bw_safe_delete( clientSpeedProvider_ );

	pyCameraSpeed_ = ScriptObject();
	pyTargeting_ = ScriptObject();
	pyAutoAim_ = ScriptObject();
}

void CameraApp::tick( float /* dGameTime */, float dRenderTime )
{
	BW_GUARD_PROFILER( AppTick_Camera );
	static DogWatch	dwCameras("Cameras");
	dwCameras.start();

	clientCamera_->update( dRenderTime );

	const BaseCameraPtr pCamera = clientCamera_->camera();
	Matrix cameraMatrix = pCamera->view();

	// and apply the change
	Moo::rc().reset();
	Moo::rc().view( cameraMatrix );
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();
	Moo::rc().updateLodViewMatrix();

	// tell the chunk manager about it too
	ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();

	if (pCamera && pCamera->spaceID() != 0)
	{
		pSpace = SpaceManager::instance().space( pCamera->spaceID() );
	}
	else if (ScriptPlayer::entity() && ScriptPlayer::entity()->pSpace())
	{
		pSpace = ScriptPlayer::entity()->pSpace();
	}

	if (pSpace)
	{
		DeprecatedSpaceHelpers::cameraSpace( pSpace );
		ChunkSpacePtr chunkSpace = 
			ChunkManager::instance().space( pSpace->id(), false );
		if (chunkSpace)
		{
			ChunkManager::instance().camera( Moo::rc().invView(), chunkSpace );
		}
		else
		{
			ChunkManager::instance().camera( Matrix::identity, NULL );
		}
	}
	else
	{
		DeprecatedSpaceHelpers::cameraSpace( NULL );
		ChunkManager::instance().camera( Matrix::identity, NULL );
	}

	static SpaceID lsid = -1;
	SpaceID tsid = pSpace ? pSpace->id() : -1;
	if (lsid != tsid)
	{
		lsid = tsid;

		INFO_MSG("CameraApp::tick: Camera space id is %d\n", lsid );
	}

	dwCameras.stop();
}

void CameraApp::inactiveTick( float dGameTime, float dRenderTime )
{
	BW_GUARD;
	this->tick( dGameTime, dRenderTime );
}

void CameraApp::draw()
{
	BW_GUARD;
}

// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

/*~	attribute BigWorld.cameraSpeed
 *	Stores the CameraSpeed object which is used to describe how a camera can
 *	move.
 *  @type CameraSpeed.
 */
// Gets added to BigWorld module in CameraApp::CameraApp

/*~ attribute BigWorld.targeting
 *	Stores a Targeting object, which is used to configure the BigWorld Targeting
 *	System.
 */
// Gets added to BigWorld module in CameraApp::CameraApp

/*~ function BigWorld.camera
 *
 *	This function is used to get and set the camera that is used by BigWorld
 *	to render the world.  It can be called with either none or one arguments.
 *	If it has no arguments, then it returns the camera currently in use.
 *	If it is called with one argument, then that argument should be a camera
 *	and it is set to be the active camera.  In this case the function returns
 *	None.
 *
 *	@param	cam		a Camera object, which is set to be the current camera.
 *					If this is not specified, the current camera will be returned.
 *
 *	@return			A Camera object or None
 */
/**
 *	Get or set the app's camera
 */
static PyObject * camera( BaseCameraPtr pCam )
{
	BW_GUARD;
	if (!pCam)
	{
		// Getting 
		return CameraApp::instance().clientCamera().camera().newRef();
	}
	else
	{
		// Setting
		CameraApp::instance().clientCamera().camera( pCam );
		Py_RETURN_NONE;
	}
}
PY_AUTO_MODULE_FUNCTION(
	RETOWN, camera, OPTARG( BaseCameraPtr, BaseCameraPtr(), END ), BigWorld )


/*~ function BigWorld.projection
 *
 *	This function retrieves the ProjectionAccess object for the current camera,
 *  allowing access to such things as the fov and the clip planes.
 *
 *	@return		ProjectionAccess
 */
/**
 *	Get the app's projection access object
 */
static PyObject * projection()
{
	BW_GUARD;
	ProjectionAccess * pProjection = 
		BW_NAMESPACE CameraApp::instance().clientCamera().projAccess();
	pProjection->incRef(); // Inc-ref as we return own
	return pProjection;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, projection, END, BigWorld )

BW_END_NAMESPACE

// camera_app.cpp

