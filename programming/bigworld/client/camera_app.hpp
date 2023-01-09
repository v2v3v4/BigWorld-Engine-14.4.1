#ifndef CAMERA_APP_HPP
#define CAMERA_APP_HPP

#include "client_camera.hpp"
#include "entity_picker.hpp"
#include "cstdmf/init_singleton.hpp"
#include "cstdmf/main_loop_task.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Camera task
 */
class CameraApp : public Singleton<CameraApp>, public MainLoopTask
{
public:
	CameraApp();
	~CameraApp();

	virtual bool init();
	virtual void fini();
	virtual void tick( float dGameTime, float dRenderTime );
	virtual void draw();
	virtual void inactiveTick( float dGameTime, float dRenderTime );

	EntityPicker& entityPicker();
	AutoAimPtr autoAim();
	TargetingPtr targeting();
	CameraSpeedPtr cameraSpeed();
	ClientSpeedProvider& clientSpeedProvider();
	ClientCamera& clientCamera();

	const EntityPicker& entityPicker() const;
	const AutoAimPtr autoAim() const;
	const TargetingPtr targeting() const;
	const CameraSpeedPtr cameraSpeed() const;
	const ClientSpeedProvider& clientSpeedProvider() const;
	const ClientCamera& clientCamera() const;

private:
	EntityPicker entityPicker_;
	AutoAimPtr pyAutoAim_;
	TargetingPtr pyTargeting_;
	CameraSpeedPtr pyCameraSpeed_;
	ClientSpeedProvider* clientSpeedProvider_;
	ClientCamera* clientCamera_;
};

#ifdef CODE_INLINE
#include "camera_app.ipp"
#endif

BW_END_NAMESPACE

#endif // CAMERA_APP_HPP

// camera_app.hpp
