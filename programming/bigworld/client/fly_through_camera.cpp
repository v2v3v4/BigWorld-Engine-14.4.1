#include "pch.hpp"

#include "fly_through_camera.hpp"

#include "chunk/user_data_object.hpp"
#include "pyscript/personality.hpp"
#include "filter.hpp" 
#include "script_player.hpp" 
#include "client_camera.hpp"
#include "app.hpp"
#include "camera_app.hpp"

BW_BEGIN_NAMESPACE

static const float CAMERA_HEIGHT = 0.0f;
static const float DEFAULT_SPEED = 5.0f;

// ----------------------------------------------------------------------------
// Local static methods
// ----------------------------------------------------------------------------


/**
 * Pull details of CameraNode UDOs starting at startNode.next until we see 
 * stopNode, then stop. If startNode is NULL, process stopNode and then 
 * continue with stopNode.next Updates startNode to be the last node we 
 * processed, for calling again later. Returns true if we hit stopNode, or
 * false if we simply couldn't find the next node.
 */
static bool readCameraNodes( PyObjectPtr & pStartNode, PyObjectPtr pStopNode,
	FlyThroughNodes & nodes )
{
	BW_GUARD;

	PyObject* pCameraNode;
	if (pStartNode == NULL)
	{
		pCameraNode = pStopNode.getObject();
	}
	else
	{
		pCameraNode = PyObject_GetAttrString( pStartNode.getObject(), "next" );
	}

	while (pCameraNode != NULL)
	{
		// In this case, pStartNode.next = pStopNode
		if (pStartNode != NULL && pCameraNode == pStopNode)
		{
			return true;
		}

		// Get the position
		PyObject* pPosition = 
			PyObject_GetAttrString( pCameraNode, "position" );

		if ( !pPosition )
		{
			// UDO not ready? Come back later.
			PyErr_Clear();
			return false;
		}

		pStartNode = pCameraNode;

		FlyThroughNode node;

		PyObject* pX = PyObject_GetAttrString( pPosition, "x" ); 
		PyObject* pY = PyObject_GetAttrString( pPosition, "y" );
		PyObject* pZ = PyObject_GetAttrString( pPosition, "z" );

		Vector3 position(	( float )PyFloat_AsDouble( pX ),
							( float )PyFloat_AsDouble( pY ) + CAMERA_HEIGHT,
							( float )PyFloat_AsDouble( pZ ) );

		Py_DECREF( pZ );
		Py_DECREF( pY );
		Py_DECREF( pX );
		Py_DECREF( pPosition );

		node.pos = position;
		
		// Get the direction of the camera
		PyObject* pRoll = PyObject_GetAttrString( pCameraNode, "roll" );
		PyObject* pPitch = PyObject_GetAttrString( pCameraNode, "pitch" );
		PyObject* pYaw = PyObject_GetAttrString( pCameraNode, "yaw" );

		Vector3 rot(	(float)PyFloat_AsDouble( pYaw ),
						(float)PyFloat_AsDouble( pPitch ),
						(float)PyFloat_AsDouble( pRoll ) );

		Py_DECREF( pRoll );
		Py_DECREF( pPitch );
		Py_DECREF( pYaw );

		node.rot = rot;

		float speed = 0.f;

		if (PyObject_HasAttrString( pCameraNode, "speed" )) {
			PyObject *pSpeed = PyObject_GetAttrString( pCameraNode, "speed" );
			speed = (float)PyFloat_AsDouble( pSpeed );
			Py_DECREF( pSpeed );
		}

		// Provide a fallback default speed if the start node doesn't have one.
		if (pCameraNode == pStopNode.getObject() && speed == 0.f)
		{
			speed = DEFAULT_SPEED;
		}

		node.speed = speed;
		nodes.push_back( node );

		// Grab a link to next
		pCameraNode = PyObject_GetAttrString( pCameraNode, "next" );
	}
	// Don't propagate Python errors
	PyErr_Clear();
	return false;
}

// CatmullRomFlyThroughSpline class
// ----------------------------------------------------------------------------
/*
 * This is a Catmull-Rom spline.
 * It sets endOfPath() to true when it passes nodes[nodeLen-2], but will
 * loop through the two control nodes nodes[nodeLen-1] and nodes[0] if allowed.
 */
class CatmullRomFlyThroughSpline : public FlyThroughSpline 
{
public:
	CatmullRomFlyThroughSpline() :
		camera_( Matrix::identity ),
		endOfPath_( false ),
		nextNodeId_( 1 ),
		edgeTimeElapsed_( 0.f ),
		speed_( 0.f ),
		edgeTimeTotal_( 0.f ),
		reset_( true )
	{};

	bool update( float dTime, const FlyThroughNodes & nodes );

	void reset();

	bool endOfPath() const { return endOfPath_; }

	const Matrix & camera() const { return camera_; }

private:
	///	@name Utility methods
	//@{
	void updateEdges( const FlyThroughNodes & nodes );
	Vector3 getSplinePos( float position ) const;
	Vector3 getSplineRot( float position ) const;
	void updateView();
	//@}

	///	@name Data representing where we are on the path
	//@{
	Matrix camera_;
	bool endOfPath_;
	//@}

	///	@name Data controlling where we are on the path
	//@{
	// The id of the node at the end of the current edge
	unsigned int nextNodeId_;
	// Time spent on the current edge in seconds
	float edgeTimeElapsed_;
	// Details about the current edge, and the edges either side.
	Vector3 p0_, p1_, p2_, p3_;
	Vector3 r0_, r1_, r2_, r3_;
	float speed_;
	// How long we expect to spend on this edge in seconds.
	float edgeTimeTotal_;
	bool reset_;
	//@}

	static void clampAngles( const Vector3 & start, Vector3 & end );
};

/**
 * A helper function to clamp rotation to the acute angles between 2 angles.
 */
void CatmullRomFlyThroughSpline::clampAngles( const Vector3 & start, Vector3 & end )
{
	BW_GUARD;

	Vector3 rot = end - start;
	if (rot[0] <= -MATH_PI)
	{
		end[0] += MATH_2PI; 
	} 
	else if (rot[0] > MATH_PI) 
	{
		end[0] -= MATH_2PI;
	}

	if (rot[1] <= -MATH_PI)
	{
		end[1] += MATH_2PI; 
	}
	else if (rot[1] > MATH_PI) 
	{
		end[1] -= MATH_2PI;
	}

	if (rot[2] <= -MATH_PI) 
	{
		end[2] += MATH_2PI; 
	}
	else if (rot[2] > MATH_PI)
	{
		end[2] -= MATH_2PI;
	}
}

bool CatmullRomFlyThroughSpline::update( float dTime, 
	const FlyThroughNodes & nodes )
{
	BW_GUARD;
	static DogWatch dwUpdate( "CatmullRomFlyThroughSpline" );
	ScopedDogWatch dwUpdateWathcer( dwUpdate );

	/* We need at least four nodes */
	if (nodes.size() < 4 )
	{
		return false;
	}

	endOfPath_ = false;

	if (reset_)
	{
		reset_ = false;
		speed_ = nodes[ 0 ].speed;
		MF_ASSERT( speed_ > 0.f );

		// First time. We should start the node[1] to node[2] edge now.
		// Treat the node[0] to node[1] edge as dTime long.
		nextNodeId_ = 1;
		edgeTimeElapsed_ = 0;
		edgeTimeTotal_ = dTime;
	}

	edgeTimeElapsed_ += dTime;

	const size_t nodeCount = nodes.size();

	while (edgeTimeElapsed_ >= edgeTimeTotal_)
	{
		edgeTimeElapsed_ -= edgeTimeTotal_;
		nextNodeId_ += 1;

		if (nextNodeId_ >= nodeCount - 1) 
		{
			endOfPath_ = true;
		}
		updateEdges( nodes );
	}
	updateView();
	return true;
}

void CatmullRomFlyThroughSpline::reset() 
{
	BW_GUARD;

	reset_ = true;
}

void CatmullRomFlyThroughSpline::updateEdges( const FlyThroughNodes & nodes ) 
{
	BW_GUARD;

	const size_t nodeCount = nodes.size();
	nextNodeId_ = nextNodeId_ % nodeCount;

	// Get the interpolation samples, treating the node list
	// as a loop.
	const size_t n0 = ( nextNodeId_ + nodeCount - 2 ) % nodeCount;
	const size_t n1 = ( nextNodeId_ + nodeCount - 1 ) % nodeCount;
	const size_t n2 = ( nextNodeId_ ) % nodeCount;
	const size_t n3 = ( nextNodeId_ + 1 ) % nodeCount;

	p0_ = nodes[ n0 ].pos;
	r0_ = nodes[ n0 ].rot;

	p1_ = nodes[ n1 ].pos;
	r1_ = nodes[ n1 ].rot;

	p2_ = nodes[ n2 ].pos;
	r2_ = nodes[ n2 ].rot;

	p3_ = nodes[ n3 ].pos;
	r3_ = nodes[ n3 ].rot;

	//Make sure the rotations only result in acute turns
	clampAngles( r0_, r1_ );
	clampAngles( r1_, r2_ );
	clampAngles( r2_, r3_ );

	if ( nodes[ n1 ].speed > 0.f )
	{
		speed_ = nodes[ n1 ].speed;
	}

	Vector3 start;
	Vector3 end = getSplinePos( 0.f );
	const int steps = 4;
	const float perStep = 1.f/steps;
	float roughLen = 0;
	for (int i = 1; i <= steps; ++i )
	{
		start = end;
		end = getSplinePos( i * perStep );
		roughLen += (end-start).length();
	}

	edgeTimeTotal_ = roughLen / speed_;
	/*
	DEBUG_MSG( "%s: Edge to node %d - gives straight-line "
		"length %f, rough length %f over %d steps and edgeTime %f at"
		" speed %f\n", __FUNCTION__, nextNodeId_, (p2_ - p1_).length(), 
		roughLen, steps, edgeTimeTotal_, speed_ );
	*/
}

Vector3 CatmullRomFlyThroughSpline::getSplinePos( float position ) const 
{
	BW_GUARD;

	Vector3 result;
	D3DXVec3CatmullRom( &result, &p0_, &p1_, &p2_, &p3_, position );
	return result;
}

Vector3 CatmullRomFlyThroughSpline::getSplineRot( float position ) const 
{
	BW_GUARD;

	Vector3 result;
	D3DXVec3CatmullRom( &result, &r0_, &r1_, &r2_, &r3_, position );
	return result;
}

void CatmullRomFlyThroughSpline::updateView() 
{
	BW_GUARD;

	const float position = edgeTimeElapsed_/edgeTimeTotal_;

	MF_ASSERT( position >= 0.f );
	MF_ASSERT( position < 1.f );

	const Vector3 pos = getSplinePos( position );
	const Vector3 rot = getSplineRot( position );

	//Work out the direction and up vector
	Matrix m = Matrix::identity;
	m.preRotateY( rot[0] );
	m.preRotateX( rot[1] );
	m.preRotateZ( rot[2] );

	const Vector3 dir = m.applyToUnitAxisVector( Z_AXIS );
	const Vector3 up = m.applyToUnitAxisVector( Y_AXIS );

	//Update the transform
	camera_.lookAt( pos, dir, up );
}

// ----------------------------------------------------------------------------
// FlyThroughCamera class
// ----------------------------------------------------------------------------

/**
 *	Find a UserDataObject whose "name" attribute matches the supplied nodeName.
 */
/*static*/ PyObjectPtr FlyThroughCamera::findCameraNode( 
	const BW::string & nodeName )
{
	BW_GUARD;

	PyObject* pBigWorld = PyImport_AddModule( "BigWorld" );

	// Find the first camera node
	PyObject* pDict = PyObject_GetAttrString( pBigWorld, "userDataObjects" );
	PyObject* pUdoList = PyMapping_Values( pDict );

	PyObject* pIter = PyObject_GetIter( pUdoList );
	PyObject* pCameraNode = NULL;

	char namedNode[256] = "";

	while ( PyObject* pUdo = PyIter_Next( pIter ) )
	{
		//If we don't have a name move on to the next
		if (!PyObject_HasAttrString( pUdo, "name" ))
		{
			continue;
		}
		
		PyObject* pName = PyObject_GetAttrString( pUdo, "name" );
		
		if (pName == NULL)
		{
			continue;
		}
		
		if ( strcmp( PyString_AsString( pName ), "" ) != 0 )
		{
			strncpy( namedNode, PyString_AsString( pName ), 255 );
			namedNode[255] = 0;
		}

		if ( strcmp( nodeName.c_str(), PyString_AsString( pName ) ) == 0 )
		{
			pCameraNode = pUdo;		// This will DECREF later
			Py_DECREF( pName );
			break;
		}
		
		Py_DECREF( pName );
		Py_DECREF( pUdo );
	}
	
	Py_DECREF( pIter );
	Py_DECREF( pUdoList );
	Py_DECREF( pDict );

	if (pCameraNode == NULL)
	{
		PyErr_Format( PyExc_StandardError,
			 "Unable to locate camera node [%s], did you mean [%s]?",
			nodeName.c_str(), namedNode );
	}
	
	return pCameraNode;
}

FlyThroughCamera::FlyThroughCamera( PyObjectPtr startNode, 
	FlyThroughSplinePtr spline, PyTypePlus * pType ) :
	callback_( NULL ),
	pSpline_( spline ),
	loop_( false ),
	fixedFrameTime_( 0.f ),
	complete_( false ),
	firstCameraNode_( startNode ),
	lastCameraNode_( NULL ),
	origCamera_( CameraApp::instance().clientCamera().camera() ),
	setValue_( ScriptPlayer::entity() && ScriptPlayer::entity()->pPhysics() ),
	BaseCamera( pType )
{
	BW_GUARD;

	MF_ASSERT( firstCameraNode_ );

	if (!pSpline_)
	{
		pSpline_ = new CatmullRomFlyThroughSpline();
	}

	//Store the original position
	if (setValue_) 
	{
		origPos_ = ScriptPlayer::entity()->position();
		origDir_ = ScriptPlayer::entity()->direction();
		origFall_ = ScriptPlayer::entity()->pPhysics()->fall();
		//set fall to false so player will not fall during fly through
		ScriptPlayer::entity()->pPhysics()->fall(false);
	}
}

FlyThroughCamera::~FlyThroughCamera()
{
	BW_GUARD;
}

void FlyThroughCamera::set( const Matrix & viewMatrix )
{
	BW_GUARD;
	// Ignore the set.
}

void FlyThroughCamera::update( float deltaTime )
{
	BW_GUARD;

	if (!complete_)
	{
		complete_ = readCameraNodes( lastCameraNode_, firstCameraNode_, 
			cameraInfoNodes_ );
	}

	float timeScale = (fixedFrameTime_ != 0.f) ? fixedFrameTime_ : deltaTime;

	if (!pSpline_->update( timeScale, cameraInfoNodes_ ))
	{
		// Abort the FlyThroughCamera.
		ERROR_MSG( "%s: CameraNode loop invalid or not enough"
			" nodes visible, aborting FlyThroughCamera\n",
			__FUNCTION__ );
		this->complete( ScriptArgs(), true );
		return;
	}

	if (pSpline_->endOfPath()) 
	{
		if (!complete_)
		{
			// We ran out of nodes, so fail.
			ERROR_MSG( "%s: Aborting flyThrough, flight ended "
				"before all nodes loaded\n",
				__FUNCTION__ );
			if (callback_ != NULL)
			{
				callback_();
			}
			else
			{
				this->complete( ScriptArgs(), true );
			}
			return;
		} 
		else if ( !loop_ ) 
		{
			if (callback_ != NULL)
			{
				callback_();
			}
			else
			{
				this->complete( ScriptArgs(), true );
			}
			return;
		}
	}

	view_ = pSpline_->camera();
	invView_.invert( view_ );

	//Update player transform too. 
	if (ScriptPlayer::entity() && ScriptPlayer::entity()->pPhysics() &&
		ScriptPlayer::entity()->isPhysicsAllowed())
 	{
		double timeNow = App::instance().getGameTimeFrameStart();
		SpaceID spaceID = ScriptPlayer::entity()->spaceID();
		Direction3D newDir( Vector3::zero() );
		
		Vector3 dir2D = invView_.applyToUnitAxisVector( Z_AXIS );
		dir2D.y = 0.f;
		dir2D.normalise();
		Vector3 playerLocation = invView_.applyToOrigin() - dir2D * 3.f;
		ScriptPlayer::entity()->onMoveLocally( timeNow, playerLocation,
			NULL_ENTITY_ID, /* is2DPosition */ false, newDir );
 	}
}

bool FlyThroughCamera::running()
{
	BW_GUARD;

	return CameraApp::instance().clientCamera().camera().getObject() == this;
}

void FlyThroughCamera::restart()
{
	BW_GUARD;

	// This has the effect of moving the flythrough 
	// to nextNodeId_ 2, edgePosition_ = 0.
	pSpline_->reset();
}

void FlyThroughCamera::complete( const ScriptArgs& result, 
	bool resetPlayerState )
{
	BW_GUARD;

	if (resetPlayerState && 
		setValue_ && 
		ScriptPlayer::entity() && 
		ScriptPlayer::entity()->pPhysics())
	{
		Vector3 teleportDir( origDir_.yaw, origDir_.pitch, origDir_.roll );
		ScriptPlayer::entity()->pPhysics()->teleport( origPos_, teleportDir );
		ScriptPlayer::entity()->pPhysics()->fall( origFall_ );
	} 
	else if (resetPlayerState) 
	{
		WARNING_MSG( "Failed to reset player state to "
			"pre-flyThrough position\n" );
	}

	if (running())
	{
		// TODO: Do I need to tick this camera?
		CameraApp::instance().clientCamera().camera( origCamera_ );
	}

	Personality::instance().callMethod( 
		"onFlyThroughFinished", 
		result ? result : ScriptArgs::create( ScriptObject::none() ),
		ScriptErrorPrint( "Personality onFlyThroughFinished" ),
		/* allowNullMethod: */ true );
}

// ----------------------------------------------------------------------------
// Python stuff
// ----------------------------------------------------------------------------

PY_TYPEOBJECT( FlyThroughCamera )

PY_BEGIN_METHODS( FlyThroughCamera )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FlyThroughCamera )
	/*~ attribute FlyThroughCamera.loop
	 *
	 *	This attribute specifies whether this flyThrough loops, or calls
	 *	Personality.onFlyThroughComplete when it hits the end of its path.
	 *
	 *	@type	bool
	 */
	PY_ATTRIBUTE(loop)
	/*~ attribute FlyThroughCamera.fixedFrameTime
	 *
	 *	This attribute specifies whether to move the camera at a fixed
	 *	delta-time per frame or at the wall-clock delta-time (if 0.f)
	 *
	 *	@type	float (seconds per frame or 0)
	 */
	PY_ATTRIBUTE(fixedFrameTime)
PY_END_ATTRIBUTES()

/*~ function BigWorld.FlyThroughCamera
 *
 *	This function creates a new FlyThroughCamera, which will follow the 
 *	CameraNode loop starting from the supplied CameraNode.
 *
 *	@param	startNode	A CameraNode UserDataObject to start following the 
 *  loop from.
 *
 *	@return a new FlyThroughCamera
 */
PY_FACTORY( FlyThroughCamera, BigWorld )

/**
 *	Python factory method
 */
PyObject * FlyThroughCamera::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject *udo;
	if (!PyArg_ParseTuple( args, "O!:BigWorld.FlyThroughCamera",
		&UserDataObject::s_type_, &udo ))
	{
		return NULL;
	}
	return new FlyThroughCamera( udo );
}

/*~ function BigWorld.runFlyThrough
 *	@components{ client }
 *
 *	Legacy function to setup a FlyThrough Camera on the loop from
 *	the named CameraNode.
 *
 *	Functionally equivalent to the following Python function:
 *	def runFlyThrough( cameraNodeName, loop = False):
 *		cameraNode = [udo for udo in BigWorld.userDataObjects.values() 
 *  		if hasattr( udo, "name" ) and udo.name == cameraNodeName][0]
 *		camera = BigWorld.FlyThroughCamera( cameraNode )
 *		camera.loop = loop
 *		BigWorld.camera( camera )
 *
 *	@param	cameraNodeName  A string naming the CameraNode UDO to start from
 *	@param	loop (optional) A bool of whether to loop repeatedly or to trigger
 *                          Personality.onFlyThroughFinished when the last node
 *                          is hit.
 *							Defaults to False
 *	@param	speed (optional) A float affecting the speed of the 
 *                           FlyThroughCamera. Defaults to 20.0.
 */
static void runFlyThrough( const BW::string & cameraNodeName, bool loop = false)
{
	PyObjectPtr cameraNode = 
		FlyThroughCamera::findCameraNode( cameraNodeName );
	if (!cameraNode)
	{
		return;
	}

	FlyThroughCameraPtr camera = new FlyThroughCamera( cameraNode );
	camera->loop( loop );
	CameraApp::instance().clientCamera().camera( 
		BaseCameraPtr( camera.get() ) );
}
PY_AUTO_MODULE_FUNCTION( RETVOID, runFlyThrough, ARG( BW::string, 
	OPTARG( bool, false, END ) ), BigWorld )

/*~	function BigWorld.flyThroughRunning
 *	@components{ client }
 *
 *	@return True if the currently-running camera is a FlyThroughCamera
 */
static bool flyThroughRunning( void )
{
	BaseCameraPtr pCamera = CameraApp::instance().clientCamera().camera();
	FlyThroughCameraPtr pFlyThroughCamera = 
		dynamic_cast<FlyThroughCamera *>( pCamera.get() );
	if (!pFlyThroughCamera)
	{
		return false;
	}
	return true;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, flyThroughRunning, END, BigWorld )

/*~	function BigWorld.cancelFlyThrough
 *	@components{ client }
 *
 *	Aborts and disconnects currently-running FlyThroughCamera
 *
 *	If no FlyThroughCamera is running, this call has no effect.
 *
 *	This will work for FlyThroughCameras added both through the normal
 *	BigWorld.camera interface, or using the profiler's BigWorld.runProfiler
 *	and BigWorld.runSoakTest interface.
 *
 *	The PlayerAvatar position and direction, and the Camera that was in
 *	use at the time the FlyThroughCamera was created, are restored.
 *
 *	It does trigger the Personality.onFlyThroughFinished callback.
 */
 // TODO later: Restoring the camera and cancelling a fly through are really
 // aspects of the ProfilerApp's use of the FlyThroughCamera.
 // Ideally, changing away from a FlyThroughCamera should be all that's needed
 // in the non-ProfilerApp case. And ProfilerApp also notices if that happens.
 // However, before this can be changed, FlyThroughCamera in FantasyDemo needs
 // to be able to be used safely without relying on calling 
 // BigWorld.cancelFlyThrough to restore the old camera and player position.
static void cancelFlyThrough( void )
{
	BaseCameraPtr pCamera = CameraApp::instance().clientCamera().camera();
	FlyThroughCameraPtr pFlyThroughCamera = 
		dynamic_cast<FlyThroughCamera *>( pCamera.get() );
	if (!pFlyThroughCamera)
	{
		return;
	}

	pFlyThroughCamera->complete( ScriptArgs(), true );
	return;
}
PY_AUTO_MODULE_FUNCTION( RETVOID, cancelFlyThrough, END, BigWorld )

BW_END_NAMESPACE

// fly_through_camera.cpp
