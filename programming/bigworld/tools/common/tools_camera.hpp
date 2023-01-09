#ifndef TOOLS_CAMERA_HPP
#define TOOLS_CAMERA_HPP

#include "guimanager/gui_manager.hpp"
#include "appmgr/options.hpp"
#include "math/boundbox.hpp"
#include "orbit_camera.hpp"
#include "mouse_look_camera.hpp"

BW_BEGIN_NAMESPACE

class KeyEvent;
class MouseEvent;

/**
 * This class extends the functionality of a mouse look camera that
 * can be driven around using the keyboard through adding the 
 * ability to lock the camera in one axis
 */

class ToolsCamera : public PyObjectPlus
{
	Py_Header( ToolsCamera, PyObjectPlus )

public:
	ToolsCamera();
	~ToolsCamera();

	static ToolsCamera & instance() { return *s_instance; }

	virtual void update( float dTime = 0.f, bool activeInputHandler = true );

	void save();

	enum CameraMode
	{
		CameraMode_Free = 0,
		CameraMode_X,
		CameraMode_Y,
		CameraMode_Z,
		CameraMode_Orbit
	};

	void boundingBox( const BoundingBox& bb, bool updateCentre = true );

	void setAnimateZoom( bool animate ) { zoomAnim_ = animate; }
		
	void mode ( unsigned mode )
	{
		if (cameraMode_ == CameraMode_Orbit)
		{
			cameraMatrixFree_ = lastView_;

			// Make orbit button act like a toggle
			if (mode == CameraMode_Orbit) 
			{
				mode = CameraMode_Free; 
			}
		}
		// Make free button act like a toggle
		else if (cameraMode_ == CameraMode_Free && mode == CameraMode_Free)
		{
			mode = CameraMode_Orbit;
		}

		cameraMode_ = mode;

		modeChange_ = true;
		
		update();

		if (GUI::Manager::pInstance() != NULL) 
		{
			// Update the main toolbar if the app is already inited.
			GUI::Manager::instance().update();
		}
	}
	int mode()
	{
		return (unsigned)cameraMode_;
	}
	
	bool handleKeyEvent( const KeyEvent & ke );
	bool handleMouseEvent( const MouseEvent & me );
	void render( float dt = 0.f );

	void windowHandle( HWND hwnd );

	void origin( const Vector3& val );
	void view( const Matrix& val );
	const Vector3& origin() const;
	const Matrix & view() const;

	float minZoomIn() const;
	void minZoomIn(float minv);
    float maxZoomOut() const;
    void maxZoomOut(float maxv);
	
	//These are exposed to python
	void invert( bool val );
	void speed( float val );
	void turboSpeed( float val );
	void rotDir( int dir );
	void orbitSpeed( float speed );
	
	bool invert();
	float speed();
	float turboSpeed();
	int rotDir();
	float orbitSpeed();

	void zoomToExtents( bool animate, const BoundingBox & box = BoundingBox::s_insideOut_ );
	
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, invert, invert )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, speed, speed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, turboSpeed, turboSpeed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( unsigned, mode, mode )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, rotDir, rotDir )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, orbitSpeed, orbitSpeed )

private:
	static ToolsCamera * s_instance;

	float getLodDist();
	
	MouseLookCameraPtr mouseLookCamera_;
	OrbitCameraPtr orbitCamera_;

	BoundingBox bb_;
	
	unsigned	cameraMode_;
	bool		modeChange_;
	
	float		zoomDuration_;
	bool		zoomAnim_;
	Vector3		zoomStart_;
	Vector3		zoomFinish_;
	float		zoomTime_;
	Vector3		centreStart_;
	Vector3		centreFinish_;
	
	Matrix		lastView_;
	
	Matrix		cameraMatrixFree_;
	Vector3		cameraPositionX_;
	Vector3		cameraPositionY_;
	Vector3		cameraPositionZ_;
	
	float       minZoomIn_;
	float       maxZoomOut_;
};

typedef SmartPointer<ToolsCamera> ToolsCameraPtr;

BW_END_NAMESPACE
#endif // TOOLS_CAMERA_HPP
