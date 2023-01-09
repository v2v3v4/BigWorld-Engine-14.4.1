// camera_app.ipp

#ifdef CODE_INLINE
#define INLINE    inline
#else
#define INLINE
#endif

INLINE
EntityPicker& CameraApp::entityPicker()
{
	BW_GUARD;
	return entityPicker_;
}

INLINE
AutoAimPtr CameraApp::autoAim()
{
	BW_GUARD;
	MF_ASSERT( pyAutoAim_.hasObject() );
	return pyAutoAim_;
}

INLINE
TargetingPtr CameraApp::targeting()
{
	BW_GUARD;
	MF_ASSERT( pyTargeting_.hasObject() );
	return pyTargeting_;
}

INLINE
CameraSpeedPtr CameraApp::cameraSpeed()
{
	BW_GUARD;
	MF_ASSERT( pyCameraSpeed_.hasObject() );
	return pyCameraSpeed_;
}

INLINE
ClientSpeedProvider& CameraApp::clientSpeedProvider()
{
	BW_GUARD;
	MF_ASSERT( clientSpeedProvider_ != NULL );
	return *clientSpeedProvider_;
}

INLINE
ClientCamera& CameraApp::clientCamera()
{
	BW_GUARD;
	MF_ASSERT( clientCamera_ != NULL );
	return *clientCamera_;
}

INLINE
const EntityPicker& CameraApp::entityPicker() const
{
	BW_GUARD;
	return entityPicker_;
}

INLINE
const AutoAimPtr CameraApp::autoAim() const
{
	BW_GUARD;
	MF_ASSERT( pyAutoAim_.hasObject() );
	return pyAutoAim_;
}

INLINE
const TargetingPtr CameraApp::targeting() const
{
	BW_GUARD;
	MF_ASSERT( pyTargeting_.hasObject() );
	return pyTargeting_;
}

INLINE
const CameraSpeedPtr CameraApp::cameraSpeed() const
{
	BW_GUARD;
	MF_ASSERT( pyCameraSpeed_.hasObject() );
	return pyCameraSpeed_;
}

INLINE
const ClientSpeedProvider& CameraApp::clientSpeedProvider() const
{
	BW_GUARD;
	MF_ASSERT( clientSpeedProvider_ != NULL );
	return *clientSpeedProvider_;
}

INLINE
const ClientCamera& CameraApp::clientCamera() const
{
	BW_GUARD;
	MF_ASSERT( clientCamera_ != NULL );
	return *clientCamera_;
}

// camera_app.ipp
