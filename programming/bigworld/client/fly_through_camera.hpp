#ifndef FLY_THROUGH_CAMERA_HPP
#define FLY_THROUGH_CAMERA_HPP

#include "cstdmf/smartpointer.hpp"
#include "camera/base_camera.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class FlyThroughCamera;
typedef SmartPointer<FlyThroughCamera> FlyThroughCameraPtr;

typedef void (*FlyThroughCompleteCallback)( void );

struct FlyThroughNode {
	Vector3 pos;
	Vector3 rot;
	float speed;
};
typedef BW::vector<FlyThroughNode> FlyThroughNodes;

/* This is the callback class used by FlyThroughCamera to determine
 * its position and direction
 */
class FlyThroughSpline : public SafeReferenceCount {
public:
	/* For safe subclassing */
	virtual ~FlyThroughSpline() {};
	/* Advance the camera Matrix and update endOfPath() */
	/* Returns false if we can't use the give nodes list */
	virtual bool update( float dTime, const FlyThroughNodes & nodes ) = 0;

	/* Restart the spline */
	virtual void reset() = 0;

	/* Did the last update reach or pass the end of the loop */
	virtual bool endOfPath() const = 0;
	/* The current point on the spline */
	virtual const Matrix & camera() const = 0;
};
typedef SmartPointer<FlyThroughSpline> FlyThroughSplinePtr;

/*~ class BigWorld.FlyThroughCamera
 *
 * This is a BaseCamera which wraps an existing baseCamera,
 * and follows the path of CameraNode UDOs (duck-typed, any UDO
 * with 'name' and 'next' will do)
 */
class FlyThroughCamera : public BaseCamera 
{

	Py_Header( FlyThroughCamera, BaseCamera )

public:
	///	@name Methods associated with running the profiler
	//@{
	static PyObjectPtr findCameraNode( const BW::string & nodeName );
	//@}

	///	@name Constructor and destructor
	//@{
	explicit FlyThroughCamera( PyObjectPtr startNode, 
		FlyThroughSplinePtr spline = NULL, PyTypePlus * pType = &s_type_ );

	~FlyThroughCamera();
	//@}

	///	@name Methods associated with transforms and the 3D world.
	//@{
	virtual void set( const Matrix & viewMatrix );
	virtual void update( float deltaTime );
	//@}

	/// @name Methods associated with Camera Status.
	//@{
	bool running();
	void restart();
	void complete( const ScriptArgs& result, bool resetPlayerState );
	//@}

	/// @name Public Accessors
	//@{
	FlyThroughCompleteCallback callback() { return callback_; } const
	void callback( FlyThroughCompleteCallback value ) { callback_ = value; }

	bool loop() { return loop_; } const
	void loop( bool value ) { loop_ = value; }

	float fixedFrameTime() { return fixedFrameTime_; } const
	void fixedFrameTime( float value ) { fixedFrameTime_ = value; }

	BaseCameraPtr origCamera() { return origCamera_; }
	//@}

	///	@name Python stuff
	//@{
	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( loop_, loop );
	PY_RW_ATTRIBUTE_DECLARE( fixedFrameTime_, fixedFrameTime );
	//@}


private:
	/// @name Data associated with our own behaviour
	//@{
	FlyThroughCompleteCallback callback_;
	FlyThroughSplinePtr pSpline_;
	bool loop_;
	float fixedFrameTime_;
	//@}

	///	@name Data associated with the path being followed
	//@{
	PyObjectPtr firstCameraNode_;
	PyObjectPtr lastCameraNode_;
	bool complete_;
	FlyThroughNodes cameraInfoNodes_;
	//@}

	///	@name Data associated with the camera we replaced
	//@{
	BaseCameraPtr origCamera_;

	//@}

	///	@name Data associated with the player when we started
	//@{
	bool setValue_;
	Position3D origPos_;
	Direction3D origDir_;
	bool origFall_;
	//@}

	///	@name Private Methods that are made private to reserve their use.
	//@{
	/// Don't allow default-construction
	FlyThroughCamera( PyTypePlus * pType = &s_type_ );

	///	The copy constructor for FlyThroughCamera.
	FlyThroughCamera( const FlyThroughCamera & );

	///	The assignment operator for FlyThroughCamera.
	FlyThroughCamera &operator=( const FlyThroughCamera & );
	//@}
};

BW_END_NAMESPACE

#endif // FLY_THROUGH_CAMERA_HPP

// fly_through_camera.hpp
