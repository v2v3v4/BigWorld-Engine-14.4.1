#include "pch.hpp"

#include "tracker.hpp"

// Standard MF Library Headers.
#include "moo/node.hpp"
#include "math/quat.hpp"
#include "math/mathdef.hpp"

// Application Specific Headers.
#include "duplo/pymodelnode.hpp"
#include "model/model.hpp"
#include "model/super_model.hpp"
#include "pymodel.hpp"
#include "pyscript/script.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 );

#define ACCELERATION_TIME 0.5f
#define MIN_ANGULAR_THRESHOLD 0.01f

#define ENABLE_TRACKER_DEBUG 1
#define TRACKER_DEBUG (ENABLE_TRACKER_DEBUG || !CLIENT_CONSUMER_BUILD)


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "tracker.ipp"
#endif

bool g_trace = false;
bool g_blendNodes = true;

namespace
{

/**
 *	This file static class is used to initialise the watchers associated with
 *	trackers.
 */
class WatcherInitialiser
{
public:
	WatcherInitialiser()
	{
		MF_WATCH("tracker/trace", g_trace, Watcher::WT_READ_WRITE, "Enables debugging output statements for trackers.");
		MF_WATCH("tracker/blendNodes", g_blendNodes, Watcher::WT_READ_WRITE, "Enables blending for the trackers, "
			"when enabled, the trackers will blend smoothly between different directions." );
	}
};

WatcherInitialiser s_watcherInitialiser;

} // anon namespace



// -----------------------------------------------------------------------------
// BaseNodeInfo:: Constructor(s) and Destructor.
// -----------------------------------------------------------------------------

/*~ attribute BaseNodeInfo minYaw
 *  The minimum yaw that this will allow a Tracker to apply to a Node, in
 *  radians, with its initial position as 0.
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo maxYaw
 *  The maximum yaw that this will allow a Tracker to apply to a Node, in
 *  radians, with its initial position as 0.
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo minPitch
 *  The minimum pitch that this will allow a Tracker to apply to a Node, in
 *  radians, with its initial position as 0.
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo maxPitch
 *  The maximum pitch that this will allow a Tracker to apply to a Node, in
 *  radians, with its initial position as 0.
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo angularVelocity
 *  The angular speed at which the nodes turn, in radians per second.
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo angularThreshold
 *  The angular threshold is a value used when blending the node rotation to
 *  switch between decay or angular modes. The decay mode blends 
 *  the node to the new angle using a decaying function by specifying the 
 *  halflife. The angular mode blends the node to its new angle using
 *  the angular velocity specified. The choice of modes depends on the difference 
 *  between the nodes old angle and new angle, if the difference is less than the 
 *  threshold then the decaying function else the angular velocity is used.
 *  The threshold is specified in radians.
 *  e.g.
 *  threshold &lt;= 0  : only use angular velocity (default value)
 *  threshold &lt; a (a &gt; 0)  : use angular decay if angle diff &lt; a else use angular velocity
 *  @type Read-Write float.
 */
/*~ attribute BaseNodeInfo angularHalflife
 *  The angular halflife determines the time of angular decay, specified in seconds.
 *  This value is to be used in conjunction with angular threshold and must be non zero
 *  for it to take effect.
 *  @type Read-Write float.
 */
PY_TYPEOBJECT( BaseNodeInfo)

PY_BEGIN_METHODS( BaseNodeInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BaseNodeInfo )
	PY_ATTRIBUTE ( minYaw )
	PY_ATTRIBUTE ( maxYaw )
	PY_ATTRIBUTE ( minPitch )
	PY_ATTRIBUTE ( maxPitch )
	PY_ATTRIBUTE ( angularVelocity )
	PY_ATTRIBUTE ( angularThreshold )
	PY_ATTRIBUTE ( angularHalflife )
PY_END_ATTRIBUTES()


/**
 *	Constructor
 */
BaseNodeInfo::BaseNodeInfo(
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		PyTypeObject *pType ) :
	PyObjectPlus( pType ),
	minPitch_( minPitch ),
	maxPitch_( maxPitch ),
	minYaw_( minYaw ),
	maxYaw_( maxYaw ),
	angularVelocity_( angularVelocity ),
	angularThreshold_( angularThreshold ),
	angularHalflife_( angularHalflife )
{
}


/**
 *	Constructor
 */
BaseNodeInfo::BaseNodeInfo(BaseNodeInfo const * t, PyTypeObject *pType) :
	PyObjectPlus( pType )
{
	BW_GUARD;
	if (t == NULL)
	{
		minPitch_ = 0.f;
		maxPitch_ = 0.f;
		minYaw_ = 0.f;
		maxYaw_ = 0.f;
		angularVelocity_ = 0.f;
		angularThreshold_ = -1.f;
		angularHalflife_ = 0.f;
		return;
	}

	minPitch_ = t->minPitch();
	maxPitch_ = t->maxPitch();
	minYaw_ = t->minYaw();
	maxYaw_ = t->maxYaw();
	angularVelocity_ = t->angularVelocity();
	angularThreshold_ = t->angularThreshold();
	angularHalflife_ = t->angularHalflife();
}

/**
 *	Constructor
 */
BaseNodeInfo::BaseNodeInfo( PyTypeObject *pType ) :
	PyObjectPlus( pType ),
	minPitch_( 0.f ),
	maxPitch_( 0.f ),
	minYaw_( 0.f ),
	maxYaw_( 0.f ),
	angularVelocity_( 0.f ),
	angularThreshold_( -1.f ),
	angularHalflife_( 0.f )
{
}

/**
 *	Destructor
 */
BaseNodeInfo::~BaseNodeInfo()
{
}


// -----------------------------------------------------------------------------
// TrackerNodeInfo:: Constructor(s) and Destructor.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( TrackerNodeInfo)

PY_BEGIN_METHODS( TrackerNodeInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TrackerNodeInfo )
PY_END_ATTRIBUTES()

/*~ function BigWorld TrackerNodeInfo
 *  Creates a new instance of the TrackerNodeInfo class. This describes how
 *  PyModelNode instances are affected by a Tracker. Note that the attributes
 *  specified by this function are not made accessible by script in the object
 *  it creates.
 *  @param model The PyModel that owns the nodes which trackers
 *  using this will affect.
 *  @param primaryNodeName A string containing the name of the primary node.
 *  The direction change applied to the primary node is not weighted.
 *  @param secondaryNodesList A list of tuples of the form ( nodeName, weight ),
 *  where nodeName is a string containing the name of a node to be affected
 *  by a tracker, and weight is a float describing the influence a tracker has
 *  over it.
 *  @param pointingNodeName A string containing the name of a node provides the
 *  frame of reference used for tracker manipulations. If the node specified is
 *  not present, the model's scene root is used instead.
 *  @param minPitch A float describing the maximum pitch variation in the
 *  negative direction (upwards) that a tracker can apply, in degrees.
 *  @param maxPitch A float describing the maximum pitch variation in the
 *  positive direction (downwards) that a tracker can apply, in degrees.
 *  @param minYaw A float describing the maximum yaw variation in the negative
 *  direction (to the left) that a tracker can apply, in degrees.
 *  @param maxYaw A float describing the maximum yaw variation in the positive
 *  direction (to the right) that a tracker can apply, in degrees.
 *  @param angularVelocity A float describing the speed at which a tracker will
 *  turn the PyModelNode instances that it influences, in degrees per second.
 *  @param angularThreshold (optional) A float describing the angle to check 
 *	if a decay or simple interpolation is used to blend the tracker node into 
 *	the correct angle, specified in degrees. For more information check 
 *	BaseNodeInfo. Defaults to -1.
 *  @param angularHalflife (optional) A float describing the angular rate of
 *  decay to blend the tracker node, specified in seconds. For more information
 *	check BaseNodeInfo. Defaults to 0.
 *  @return The new TrackerNodeInfo.
 */
PY_FACTORY( TrackerNodeInfo, BigWorld )


/**
 *	Constructor
 */
TrackerNodeInfo::TrackerNodeInfo( PyModel *pOwner,
		Moo::NodePtr pPrimaryNode,
		const BW::vector< std::pair< Moo::NodePtr, float > > &secondaryNodes,
		Moo::NodePtr pPointingNode,
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		PyTypeObject *pType ) :
	BaseNodeInfo(minPitch, maxPitch, minYaw, maxYaw, angularVelocity, angularThreshold, angularHalflife, pType ),
	pOwner_( pOwner ),
	// TODO: Store a reference to pOwner (inc refcount) or get it to tell us when
	// it goes away. Or get Tracker to tell us when we are selected into it.
	pPrimaryNode_( pPrimaryNode ),
	primaryNodeWeight_( 1.0f ),
	secondaryNodes_( secondaryNodes ),
	pPointingNode_( pPointingNode )
{
	BW_GUARD;
	// Normalise the weights so they add up to one. Start by summing the
	// weights. Add only the positive weights. Negative weights are
	// not part of the total.
	BW::vector< std::pair< Moo::NodePtr, float > >::iterator iter;
	float sumOfWeights = primaryNodeWeight_;
	for ( iter = secondaryNodes_.begin(); iter != secondaryNodes_.end();
		iter++ )
	{
		if ( iter->second > 0.0f )
		{
			sumOfWeights += iter->second;
		}
	}


	// Divide each weight by the sum.
	primaryNodeWeight_ /= sumOfWeights;
	for ( iter = secondaryNodes_.begin(); iter != secondaryNodes_.end();
		iter++ )
	{
		iter->second /= sumOfWeights;
	}
}


/**
 *	This method applies transforms so that nodes look at the specified vector.
 */
void TrackerNodeInfo::lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
	const float weighting )
{
	BW_GUARD;

	// Get angle and axis of rotation between the two direction
	// vectors.
	float angle = dirInWS.getUnitVectorAngle( lookAtInWS );

	angle *= weighting;

	// Optimisation: Only rotate if the angle is greater than a
	// certain threshold.
	if ( angle < 0.01f )
	{
		return;
	}

	Vector3 waxis = dirInWS.crossProduct( lookAtInWS );

	Quaternion rotation;
	Matrix rotateTransform;

	Matrix inverse = pPrimaryNode_->worldTransform();
	inverse.invert();

	// Convert axis vector into local coordinate space for the
	// primary node.
	Vector3 axis = inverse.applyVector( waxis );

#if 0
	if ( g_trace )
	{
		DEBUG_MSG("TrackerNodeInfo::lookAt - lookAtInWS=(%6.3f,%6.3f,%6.3f), angle = %6.3f\n",
			lookAtInWS.x, lookAtInWS.y, lookAtInWS.z, angle);
		DEBUG_MSG("TrackerNodeInfo::lookAt - dirInWS=(%6.3f,%6.3f,%6.3f)\n",
			dirInWS.x, dirInWS.y, dirInWS.z);
		DEBUG_MSG("TrackerNodeInfo::lookAt - waxis=(%6.3f,%6.3f,%6.3f)\n",
			waxis.x, waxis.y, waxis.z);
		DEBUG_MSG("TrackerNodeInfo::lookAt - axis=(%6.3f,%6.3f,%6.3f)\n",
			axis.x, axis.y, axis.z);
	}
#endif

	// Rotate the primary node.
	rotation.fromAngleAxis( angle * primaryNodeWeight_, axis );

	rotateTransform.setRotate( rotation );

	pPrimaryNode_->transform().preMultiply( rotateTransform );

	Matrix acc = rotateTransform;

	// Rotate the secondary nodes.
	BW::vector< std::pair< Moo::NodePtr, float > >::iterator iter;
	for ( iter = secondaryNodes_.begin();
		iter != secondaryNodes_.end();
		iter++ )
	{
		inverse = iter->first->worldTransform();
		inverse.preMultiply( acc );
		inverse.invert();
		axis = inverse.applyVector( waxis );

		rotation.fromAngleAxis( angle * iter->second, axis );
		rotateTransform.setRotate( rotation );
		iter->first->transform().preMultiply( rotateTransform );
		acc.postMultiply( rotateTransform );
	}
}


/**
 *	This method accepts a direction vector and updates it to the current
 *  direction of the manipulated bipeds. It uses the pointing node as the
 *  reference. If no pointing node is defined, it attempts to use the scene
 *  root for the model instead.
 *
 *	@param	direction	This vector is the result of the calculation.
 */
void TrackerNodeInfo::getTrackedDirection( Vector3 &direction )
{
	BW_GUARD;
	// Get the direction of the tracked biped nodes.
	Matrix primaryWS = pPrimaryNode_->worldTransform();

	if (pPointingNode_ == pPrimaryNode_)
	{
		Matrix	worldMatrix;
		worldMatrix.setRotateY( pOwner_->unitRotation() );
		worldMatrix.postMultiply( pOwner_->worldTransform() );
		direction = worldMatrix.applyToUnitAxisVector(2);
	}
	else if (pPointingNode_)
	{
		// If a pointing node was specified, get current node direction
		// in world space from the primary node and pointing node.
		Matrix pointingWS = pPointingNode_->worldTransform();
			//pOwner_->transformInWS( pPointingNode_ );
		direction = pointingWS.applyToOrigin() - primaryWS.applyToOrigin();
	}
	else
	{
		// Use the primary node's direction for orientation.
		direction = primaryWS.applyToUnitAxisVector(2);
	}
	direction.normalise();
}


/**
 *	This method returns whether or not the primary node has been animated or not
 *  @return true if node is animated, false otherwise.
 */
bool TrackerNodeInfo::isPrimaryNodeAnimated()
{
	BW_GUARD;
	// Check if the node has been animated or not. If not, just return
	// and do nothing further.
    if ( pPrimaryNode_->blend( Model::blendCookie() ) <= 0.0f )
	{
		return false;
	}
	else
	{
		return true;
	}
}



/**
 *	This method is for debugging. It returns the number of node info's this
 *	object has. In this case, it is always 1.
 */
int TrackerNodeInfo::size()
{
	return 1;
}


PyObject * TrackerNodeInfo::pyNew( PyObject * args )
{
	BW_GUARD;
	// first try to parse the arguments
	PyObject * pModelObj;
	char * primaryNodeName;
	PyObject * secondaryNodesList;
	char * pointingNodeName;
	float	minPitch;
	float	maxPitch;
	float	minYaw;
	float	maxYaw;
	float	angularVelocity;
	float	angularThreshold = -1.f;
	float	angularHalflife = 0.f;

	// first parse the easy arguments
	if (!PyArg_ParseTuple( args, "OsO!sfffff|ff",
		&pModelObj,
		&primaryNodeName,
		&PyList_Type,
		&secondaryNodesList,
		&pointingNodeName,
		&minPitch,
		&maxPitch,
		&minYaw,
		&maxYaw,
		&angularVelocity,
		&angularThreshold,
		&angularHalflife) || !PyModel::Check( pModelObj ))
	{
		PyErr_SetString( PyExc_TypeError, "TrackerNodeInfo() expects: "
			"PyModel ownerModel, " 
			"string PrimaryNodeName, "
			"[(string,float)] secondaryNodesList, "
			"string pointingNodeName, "
			"float minPitch, float maxPitch, "
			"float minYaw, float maxYaw, float angularVelocity and optional "
			"float angularThreshold, float angularHalflife");
		return NULL;
	}

	// make sure the model isn't blank
	PyModel * pModel = (PyModel*)pModelObj;
	SuperModel * pSuperModel = pModel->pSuperModel();
	if (pSuperModel == NULL)
	{
		PyErr_SetString( PyExc_TypeError,
			"TrackerNodeInfo() does not work on blank models" );
		return NULL;
	}

	// now the vector of secondary nodes
	BW::vector< std::pair< BW::string, float > >	secondaryNodes;

	Py_ssize_t listSize = PyList_Size( secondaryNodesList );
	for (Py_ssize_t i=0; i < listSize; i++)
	{
		char *	nodeName;
		float	nodeWeight;

		if (!PyArg_ParseTuple( PyList_GetItem( secondaryNodesList, i ),
			"sf", &nodeName, &nodeWeight ))
		{
			PyErr_SetString( PyExc_TypeError,
				"TrackerNodeInfo() expects: (string, float) pair "
				"in the secondaryNodesList" );
			return NULL;
		}

		secondaryNodes.push_back(
			std::make_pair( nodeName, nodeWeight ) );
	}

	// ok, argument parsing done, now for argument conversion.

	// Get primary node.
	Moo::NodePtr primaryNode = pSuperModel->findNode( primaryNodeName );
	if (!primaryNode)
	{
		PyErr_Format( PyExc_ValueError, "TrackerNodeInfo() "
			"could not find primary node named '%s' in the model '%s'\n",
			primaryNodeName, pModel->name().c_str() );
		return NULL;
	}

	// Get secondary nodes.
	BW::vector< std::pair< Moo::NodePtr, float > > nodes;
	BW::vector< std::pair< BW::string, float > >::const_iterator iter;
	for ( iter = secondaryNodes.begin(); iter != secondaryNodes.end(); iter++ )
	{
		Moo::NodePtr secondaryNode = pSuperModel->findNode( iter->first );

		if (!secondaryNode)
		{
			PyErr_Format( PyExc_ValueError, "TrackerNodeInfo() "
				"could not find secondary node named '%s' in the model '%s'\n",
				iter->first.c_str(), pModel->name().c_str() );
			return NULL;
		}

		std::pair< Moo::NodePtr, float > newNodePair;
		newNodePair.first = secondaryNode;
		newNodePair.second = iter->second;

		nodes.push_back( newNodePair );
	}

	// Get pointing node. This node can be NULL.
	Moo::NodePtr pointingNode = pSuperModel->findNode( pointingNodeName );

	// Convert the angle values into radians.
	minPitch = DEG_TO_RAD( minPitch );
	maxPitch = DEG_TO_RAD( maxPitch );
	minYaw = DEG_TO_RAD( minYaw );
	maxYaw = DEG_TO_RAD( maxYaw );
	angularVelocity = DEG_TO_RAD( angularVelocity );
	angularThreshold = DEG_TO_RAD( angularThreshold );

	// now actually make the tracker
	return new TrackerNodeInfo(	pModel,
		primaryNode,
		nodes,
		pointingNode,
		minPitch,
		maxPitch,
		minYaw,
		maxYaw,
		angularVelocity,
		angularThreshold,
		angularHalflife );
}


// -----------------------------------------------------------------------------
// ArmTrackerNodeInfo:: Constructor(s) and Destructor.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ArmTrackerNodeInfo)

PY_BEGIN_METHODS( ArmTrackerNodeInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ArmTrackerNodeInfo )

	/*~	attribute ArmTrackerNodeInfo.minShoulderPitch
	 *	Describing the maximum pitch variation in the negative
	 *  direction (downwards) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( minShoulderPitch )

	/*~	attribute ArmTrackerNodeInfo.maxShoulderPitch
	 *	Describes the maximum pitch variation in the positive
	 *  direction (upwards) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( maxShoulderPitch )

	/*~	attribute ArmTrackerNodeInfo.minShoulderYaw
	 *	Describes the maximum yaw variation in the negative direction
	 *  (to the left) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( minShoulderYaw )

	/*~	attribute ArmTrackerNodeInfo.maxShoulderYaw
	 *	Describes the maximum yaw variation in the positive direction
	 *  (to the right) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( maxShoulderYaw )

	/*~	attribute ArmTrackerNodeInfo.minShoulderRoll
	 *	Describes the maximum roll variation in the negative
	 *	direction (clockwise) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( minShoulderRoll )

	/*~	attribute ArmTrackerNodeInfo.maxShoulderRoll
	 *	Describes the maximum roll variation in the positive
	 *  direction (anticlockwise) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( maxShoulderRoll )

	/*~	attribute ArmTrackerNodeInfo.minElbowYaw
	 *	Describes the maximum yaw variation in the negative direction
	 *  (to the left) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( minElbowYaw )

	/*~	attribute ArmTrackerNodeInfo.maxElbowYaw
	 *	Describes the maximum yaw variation in the positive direction
	 *  (to the right) that a tracker can apply, in degrees.
	 *	@type	float
	 */
	PY_ATTRIBUTE ( maxElbowYaw )

	/*~	attribute ArmTrackerNodeInfo.pointingAxis
	 *	Specifies the pointing axis of node (Xaxis, Yaxis or Zaxis).
	 *	@type	integer
	 */
	PY_ATTRIBUTE ( pointingAxis )

PY_END_ATTRIBUTES()

/*~ function BigWorld ArmTrackerNodeInfo
 *  Creates a new instance of the ArmTrackerNodeInfo class. This describes how
 *  PyModelNode instances are affected by a two bone IK Tracker.
 *  @param model The PyModel that owns the nodes which trackers
 *  using this will affect.
 *  @param pUpperArmNode A string containing the name of the upper arm node.
 *  @param pForeArmNode A string containing the name of the forearm node.
 *  @param minShoulderPitch A float describing the maximum pitch variation in the
 *  negative direction (downwards) that a tracker can apply, in degrees.
 *  @param maxShoulderPitch A float describing the maximum pitch variation in the
 *  positive direction (upwards) that a tracker can apply, in degrees.
 *  @param minShoulderYaw A float describing the maximum yaw variation in the negative
 *  direction (to the left) that a tracker can apply, in degrees.
 *  @param maxShoulderYaw A float describing the maximum yaw variation in the positive
 *  direction (to the right) that a tracker can apply, in degrees.
 *  @param minShoulderRoll A float describing the maximum roll variation in the
 *  negative direction (clockwise) that a tracker can apply, in degrees.
 *  @param maxShoulderRoll A float describing the maximum roll variation in the
 *  positive direction (anticlockwise) that a tracker can apply, in degrees.
 *  @param minElbowPitch A float describing the maximum pitch variation in the negative
 *  direction (downwards) that a tracker can apply, in degrees.
 *  @param maxElbowPitch A float describing the maximum pitch variation in the positive
 *  direction (upwards) that a tracker can apply, in degrees.
 *  @param minPitch A float describing the maximum pitch variation in the
 *  negative direction (downwards) that a tracker can apply, in degrees.
 *  @param maxPitch A float describing the maximum pitch variation in the
 *  positive direction (upwards) that a tracker can apply, in degrees.
 *  @param minYaw A float describing the maximum yaw variation in the negative
 *  direction (to the left) that a tracker can apply, in degrees.
 *  @param maxYaw A float describing the maximum yaw variation in the positive
 *  direction (to the right) that a tracker can apply, in degrees.
 *  @param pointingAxis An int specifying the pointing axis of node (Xaxis, Yaxis or Zaxis).
 *  @param angularVelocity A float describing the speed at which a tracker will
 *  turn the PyModelNode instances that it influences, in degrees per second.
 *  @param angularThreshold A float describing the angle to check if a decay or
 *  simple interpolation is used to blend the tracker node into the correct angle, 
 *  specified in degrees. For more information check BaseNodeInfo.
 *  @param angularHalflife A float describing the angular rate of decay to blend
 *  the tracker node, specified in seconds. For more information check BaseNodeInfo.
 *  @return The new ArmTrackerNodeInfo.
 */
PY_FACTORY( ArmTrackerNodeInfo, BigWorld )


/**
 *	Constructor
 */
ArmTrackerNodeInfo::ArmTrackerNodeInfo( PyModel *pOwner,
		Moo::NodePtr pUpperArmNode,
		Moo::NodePtr pForeArmNode,
		float minShoulderPitch,
		float maxShoulderPitch,
		float minShoulderYaw,
		float maxShoulderYaw,
		float minShoulderRoll,
		float maxShoulderRoll,
		float minElbowYaw,
		float maxElbowYaw,
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		int	  pointingAxis,
		PyTypeObject *pType ) :
	BaseNodeInfo(minPitch, maxPitch, minYaw, maxYaw, angularVelocity, angularThreshold, angularHalflife, pType ),
	pOwner_(pOwner),
	pUpperArmNode_( pUpperArmNode ),
	pForeArmNode_( pForeArmNode ),
	minShoulderPitch_(minShoulderPitch),
	maxShoulderPitch_(maxShoulderPitch),
	minShoulderYaw_(minShoulderYaw),
	maxShoulderYaw_(maxShoulderYaw),
	minShoulderRoll_(minShoulderRoll),
	maxShoulderRoll_(maxShoulderRoll),
	minElbowYaw_(minElbowYaw),
	maxElbowYaw_(maxElbowYaw),
	pointingAxis_(pointingAxis)
{
}


/**
 *	This method applies transforms so that nodes look at the specified vector.
 */
void ArmTrackerNodeInfo::lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
	const float weighting)
{
	BW_GUARD;
	//Need to invert look at direction?
	//lookAtInWS *= -1.0;

	//Calculate parent reference frame
	//Note: Access parent world transform directly sometimes returns
	//		identity matrix. probably need to re-traverse the node tree
	//		to be safe that all world transforms are correct when accessed.
	Matrix parentWS = pUpperArmNode_->transform();
	parentWS.invert();
	parentWS.postMultiply(pUpperArmNode_->worldTransform());

	/////////////////////////////
	//	Upper Arm Motion
	/////////////////////////////

	Matrix upperArmWS = pUpperArmNode_->worldTransform();
	Vector3 dirUpperArmInWS = upperArmWS.applyToUnitAxisVector(0);
	dirUpperArmInWS.normalise();

	Vector3 wantedUpperArmDirection;
	//Vector3 up(0.0, 1.0, 0.0);
	//wantedUpperArmDirection = lookAtInWS.crossProduct( up );

	//Clamp upper arm direction to models forward plane depending on which arm
	//  Left: pointingAxis_  = 0
	//	Rigth: pointingAxis_  = 1
 	Vector3 modelDir(pOwner_->worldTransform().applyToUnitAxisVector(0));
	modelDir.normalise();
	float yawComponent  = modelDir.dotProduct(lookAtInWS);
	if ((pointingAxis_ == 0 && yawComponent > 0) || (pointingAxis_ == 1 && yawComponent < 0))
		wantedUpperArmDirection = lookAtInWS - yawComponent * modelDir;
	else
		wantedUpperArmDirection = lookAtInWS;


	// Get angle and axis of rotation between the two direction
	// vectors.
	float angle = acosf(Math::clamp( -1.f, dirUpperArmInWS.dotProduct( wantedUpperArmDirection ), 1.f ) );
	angle *= weighting;

	Vector3 waxis = dirUpperArmInWS .crossProduct(wantedUpperArmDirection );

	Quaternion rotationLS;
	Quaternion rotationWS;
	Matrix rotateTransformLS;
	Matrix rotateTransformWS;

	Matrix inverse = pUpperArmNode_->worldTransform();
	inverse.invert();

	// Convert axis vector into local coordinate space for the
	// upper arm node
	Vector3 axis = inverse.applyVector( waxis );

	// Rotate the node.
	rotationLS.fromAngleAxis( angle , axis );
	rotateTransformLS.setRotate( rotationLS );

	rotationWS.fromAngleAxis( angle , waxis );
	rotateTransformWS.setRotate( rotationWS );

	pUpperArmNode_->transform().preMultiply( rotateTransformLS );


	/////////////////////////////////
	//	Forearm Motion
	/////////////////////////////////

	//Multiply world matrix by upper arm change.
	//Matrix foreArmWS = pUpperArmNode_->parent()->worldTransform();
	Matrix foreArmWS = parentWS;
	foreArmWS.preMultiply(pUpperArmNode_->transform());
	foreArmWS.preMultiply(pForeArmNode_->transform());

	Vector3 dirForeArmInWS = foreArmWS.applyToUnitAxisVector(0);

	dirForeArmInWS.normalise();

	// Get angle and axis of rotation between the two direction
	// vectors.
	angle = acosf(Math::clamp( -1.f, dirForeArmInWS.dotProduct( lookAtInWS ), 1.f ) );
	angle *= weighting;

	waxis =  dirForeArmInWS.crossProduct(lookAtInWS);

	inverse = foreArmWS;
	inverse.invert();

	// Convert axis vector into local coordinate space for the
	// primary node. This axis should be the fulcrum axis of the
	// forearm. Ths is a good place to debug tracker  behaviour
	axis = inverse.applyVector( waxis );


	// Rotate the primary node.
	rotationLS.fromAngleAxis( angle , axis );
	rotateTransformLS.setRotate( rotationLS );

	pForeArmNode_->transform().preMultiply( rotateTransformLS );
}


/**
 *	This method accepts a direction vector and updates it to the current
 *  direction of the manipulated bipeds. In the case of the ArmTracker
 *	we need two directions so set these in the lookAt function
 *
 *	@param	direction	This vector is the result of the calculation.
 */
void ArmTrackerNodeInfo::getTrackedDirection( Vector3 &direction )
{
	BW_GUARD;
	// Get the direction of the tracked biped nodes.
	Matrix foreArmNodeWS = pForeArmNode_->worldTransform();

	// Use the primary node's direction for orientation.
	direction = foreArmNodeWS.applyToUnitAxisVector(0);

	direction.normalise();
}


/**
 *	This method returns whether or not the primary node has been animated or not
 *  @return true if node is animated, false otherwise.
 */
bool ArmTrackerNodeInfo::isPrimaryNodeAnimated()
{
	BW_GUARD;
	// Check if the node has been animated or not. If not, just return
	// and do nothing further.
    if ( pForeArmNode_->blend( Model::blendCookie() ) <= 0.0f )
	{
		return false;
	}
	else
	{
		return true;
	}
}



/**
 *	This method is for debugging. It returns the number of node info's this
 *	object has. In this case, it is always 1.
 */
int ArmTrackerNodeInfo::size()
{
	return 1;
}


PyObject * ArmTrackerNodeInfo::pyNew( PyObject * args )
{
	BW_GUARD;
	// first try to parse the arguments
	PyObject * pModelObj;
	char * upperArmNodeName;
	char * foreArmNodeName;
	float	minShoulderPitch;
	float	maxShoulderPitch;
	float	minShoulderYaw;
	float	maxShoulderYaw;
	float	minShoulderRoll;
	float	maxShoulderRoll;
	float	minElbowPitch;
	float	maxElbowPitch;
	float	minPitch;
	float	maxPitch;
	float	minYaw;
	float	maxYaw;
	float	angularVelocity;
	int		pointingAxis;
	float	angularThreshold = -1.f;
	float	angularHalflife = 0.f;

	// first parse the easy arguments
	if (!PyArg_ParseTuple( args, "Ossffffffffffffif|ff",
		&pModelObj,
		&upperArmNodeName,
		&foreArmNodeName,
		&minShoulderPitch,
		&maxShoulderPitch,
		&minShoulderYaw,
		&maxShoulderYaw,
		&minShoulderRoll,
		&maxShoulderRoll,
		&minElbowPitch,
		&maxElbowPitch,
		&minPitch,
		&maxPitch,
		&minYaw,
		&maxYaw,
		&pointingAxis,
		&angularVelocity,
		&angularThreshold,
		&angularHalflife) || !PyModel::Check( pModelObj ))
	{
		PyErr_SetString( PyExc_TypeError, "ArmTrackerNodeInfo() expects: "
			"PyModel, string UpperArmNodeName, "
			"string ForeArmNodeName, "
			"float minShoulderPitch, float maxShoulderPitch, "
			"float minShoulderYaw, float maxShoulderYaw, "
			"float minShoulderRoll, float maxShoulderRoll, "
			"float minElbowPitch, float maxElbowPitch, "
			"float minPitch, float maxPitch, "
			"float minYaw, float maxYaw, "
			"int   pointingAxis, "
			"float angularVelocity and optional "
			"float angularThreshold, float angularHalflife");
			return NULL;
	}

	// make sure the model isn't blank
	PyModel * pModel = (PyModel*)pModelObj;
	SuperModel * pSuperModel = pModel->pSuperModel();
	if (pSuperModel == NULL)
	{
		PyErr_SetString( PyExc_TypeError,
			"ArmTrackerNodeInfo() does not work on blank models" );
		return NULL;
	}

	// ok, argument parsing done, now for argument conversion.

	// Get upper arm node.
	Moo::NodePtr upperArmNode = pSuperModel->findNode( upperArmNodeName );
	if (!upperArmNode)
	{
		PyErr_Format( PyExc_ValueError, "ArmTrackerNodeInfo() "
			"could not find upper arm node named '%s' in the model '%s'\n",
			upperArmNodeName, pModel->name().c_str() );
		return NULL;
	}

	// Get forearm node.
	Moo::NodePtr foreArmNode = pSuperModel->findNode( foreArmNodeName );
	if (!upperArmNode)
	{
		PyErr_Format( PyExc_ValueError, "ArmTrackerNodeInfo() "
			"could not find forearm node named '%s' in the model '%s'\n",
			foreArmNodeName, pModel->name().c_str() );
		return NULL;
	}

	// Convert the angle values into radians.
	minShoulderPitch = DEG_TO_RAD( minShoulderPitch );
	maxShoulderPitch = DEG_TO_RAD( maxShoulderPitch );
	minShoulderYaw = DEG_TO_RAD( minShoulderYaw );
	maxShoulderYaw = DEG_TO_RAD( maxShoulderYaw );
	minShoulderRoll = DEG_TO_RAD( minShoulderRoll );
	maxShoulderRoll = DEG_TO_RAD( maxShoulderRoll );
	minElbowPitch = DEG_TO_RAD( minElbowPitch );
	maxElbowPitch = DEG_TO_RAD( maxElbowPitch );

	minPitch = DEG_TO_RAD( minPitch );
	maxPitch = DEG_TO_RAD( maxPitch );
	minYaw = DEG_TO_RAD( minYaw );
	maxYaw = DEG_TO_RAD( maxYaw );
	angularVelocity = DEG_TO_RAD( angularVelocity );
	angularThreshold = DEG_TO_RAD( angularThreshold );


	// now actually make the tracker
	return new ArmTrackerNodeInfo(	pModel,
		upperArmNode,
		foreArmNode,
		minShoulderPitch,
		maxShoulderPitch,
		minShoulderYaw,
		maxShoulderYaw,
		minShoulderRoll,
		maxShoulderRoll,
		minElbowPitch,
		maxElbowPitch,
		minPitch,
		maxPitch,
		minYaw,
		maxYaw,
		angularVelocity,
		angularThreshold,
		angularHalflife,
		pointingAxis
		);
}


// -----------------------------------------------------------------------------
// Section: OrthogonalNodeInfo
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( OrthogonalNodeInfo)

PY_BEGIN_METHODS( OrthogonalNodeInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( OrthogonalNodeInfo )

	/*~	attribute OrthogonalNodeInfo.yawAxis
	 *
	 *	Describes which axis to rotate along to change yaw.  This value was
	 *	provided upon creation (see BigWorld.OrthogonalNodeInfo()).  Values
	 *	of 1, 2 and 3 represent the x, y and z axis repectively.  Negative
	 *	values cause the desired axis to be inverted.
	 *
	 *	@type int
	 */
	PY_ATTRIBUTE ( yawAxis )

	/*~	attribute OrthogonalNodeInfo.pitchAxis
	 *
	 *	Describes which axis to rotate along to change pitch.  This value was
	 *	provided upon creation (see BigWorld.OrthogonalNodeInfo()).  Values
	 *	of 1, 2 and 3 represent the x, y and z axis repectively.  Negative
	 *	values cause the desired axis to be inverted.
	 *
	 *	@type int
	 */
	PY_ATTRIBUTE ( pitchAxis )

PY_END_ATTRIBUTES()

/*~ function BigWorld OrthogonalNodeInfo
 *  This creates a new instance of the OrthogonalNodeInfo class which describes
 *  how PyModelNode instances can be affected by a Tracker. Note that the
 *  attributes specified by this function are not made accessable by script in
 *  the object it creates. Instances of this particular NodeInfo class can be
 *  used by a Tracker to apply pitch changes to one model node, and yaw changes
 *  to another. This can be used very effectively with models which represent
 *  machinery that turns on swivel joints.
 *  @param model The PyModel owning the nodes that will be affected by any
 *  Tracker which makes use of the returned OrthogonalNodeInfo.
 *  @param pitchNode A string containing the name of the pitch node.
 *  This is the node to which a Tracker applies pitch changes.
 *  If this does not match the name of a node of the model specified by the
 *  model argument then a ValueError is thrown.
 *  @param yawNode A string containing the name of the yaw node.
 *  This is the node to which yaw changes are applied.
 *  If this does not match the name of a node in the model specified by the
 *  model argument then a ValueError is thrown.
 *  @param directionNode A string containing the name of a node that can be
 *  assumed to have undergone both the pitch and yaw translations after they've
 *  been applied. This should be a descendent of both the pitchNode and the
 *  yawNode.
 *  If this does not match the name of a node in the model specified by the
 *  model argument then a ValueError is thrown.
 *  @param minPitch A float describing the maximum pitch variation (in degrees)
 *  in the negative direction (downwards) that can be applied by a Tracker.
 *  @param maxPitch A float describing the maximum pitch variation (in degrees)
 *  in the positive direction (upwards) that can be applied by a Tracker.
 *  @param minYaw A float describing the maximum yaw variation (in degrees)
 *  in the negative direction (to the left) that can be applied by a Tracker.
 *  @param maxYaw A float describing the maximum yaw variation (in degrees)
 *  in the positive direction (to the right) that can be applied by a Tracker.
 *  @param yawAxis An int describing which axis to rotate along to change yaw
 *  1=x-axis, 2=y-axis, 3=z-axis, negative values means invert the axis.
 *  @param pitchAxis An int describing which axis to rotate along to change pitch
 *  1=x-axis, 2=y-axis, 3=z-axis, negative values means invert the axis.
 *  @param angularVelocity A float describing the speed (in degrees per second)
 *  that a Tracker will rotate any PyModelNode instances that it influences.
 *  @param angularThreshold A float describing the angle to check if a decay or
 *  simple interpolation is used to blend the tracker node into the correct angle, 
 *  specified in degrees. For more information check BaseNodeInfo.
 *  @param angularHalflife A float describing the angular rate of decay to blend
 *  the tracker node, specified in seconds. For more information check BaseNodeInfo.
 *  @return The new OrthogonalNodeInfo.
 */

PY_FACTORY( OrthogonalNodeInfo, BigWorld )


/**
 *	Constructor
 */
OrthogonalNodeInfo::OrthogonalNodeInfo( PyModel *pOwner,
		Moo::NodePtr pPitchNode,
		Moo::NodePtr pYawNode,
		Moo::NodePtr pDirectionNode,
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		int yawAxis,
		int pitchAxis,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		PyTypeObject *pType ) :
	BaseNodeInfo( minPitch, maxPitch, minYaw, maxYaw, angularVelocity, angularThreshold, angularHalflife, pType ),
	pOwner_( pOwner ),
	pPitchNode_( pPitchNode ),
	pYawNode_( pYawNode ),
	pDirectionNode_( pDirectionNode ),
	yawAxis_( yawAxis ),
	pitchAxis_( pitchAxis )
{
	// TODO:PM We should try and be consistent with the ordering of yaw, pitch
	// and roll. In general, prefer yaw, pitch and then roll which these
	// arguments are not.
}


/**
 *	This method applies transforms so that nodes look at the specified vector.
 */
void OrthogonalNodeInfo::lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
	const float weighting )
{
	BW_GUARD;
	float yaw   = weighting * Angle(lookAtInWS.yaw()   - dirInWS.yaw());
	float pitch = weighting * Angle(lookAtInWS.pitch() - dirInWS.pitch());

	Matrix rotation;
	if( yawAxis_ < 0) yaw = -yaw;
	switch( yawAxis_ )
	{
		case 1: case -1:
			rotation.setRotateX( yaw );
			break;
		case 3: case -3:
			rotation.setRotateZ( yaw );
			break;
		default:
			rotation.setRotateY( yaw );
	}
	pYawNode_->transform().preMultiply( rotation );

	if( pitchAxis_ < 0) pitch = -pitch;
	switch( pitchAxis_  )
	{
		case 2: case -2:
			rotation.setRotateY( pitch );
			break;
		case 3: case -3:
			rotation.setRotateZ( pitch );
			break;
		default:
			rotation.setRotateX( pitch );
	}
	pPitchNode_->transform().preMultiply( rotation );
}


/**
 *	This method accepts a direction vector and updates it to the current
 *  direction of the manipulated bipeds. It uses the pointing node as the
 *  reference. If no pointing node is defined, it attempts to use the scene
 *  root for the model instead.
 *
 *	@param	direction	This vector is the result of the calculation.
 */
void OrthogonalNodeInfo::getTrackedDirection( Vector3 &direction )
{
	BW_GUARD;
	// Get the direction of the tracked biped nodes.
	direction =
		pDirectionNode_->worldTransform().applyToUnitAxisVector( Z_AXIS );
	direction.normalise();
}


/**
 *	This method returns whether or not the primary node has been animated or not
 *  @return true if node is animated, false otherwise.
 */
bool OrthogonalNodeInfo::isPrimaryNodeAnimated()
{
	BW_GUARD;
	// TODO:PM What's the deal with this? Check this.
	// Check if the node has been animated or not. If not, just return and do
	// nothing further.
    if (pDirectionNode_->blend( Model::blendCookie() ) <= 0.0f)
	{
		return false;
	}
	else
	{
		return true;
	}
}



/**
 *	This method is for debugging. It returns the number of node info's this
 *	object has. In this case, it is always 1.
 */
int OrthogonalNodeInfo::size()
{
	return 1;
}


/**
 *	This method is used to implement the Python constructor for this object.
 */
PyObject * OrthogonalNodeInfo::pyNew( PyObject * args )
{
	BW_GUARD;
	// first try to parse the arguments
	PyObject * pModelObj;
	char * pitchNodeName;
	char * yawNodeName;
	char * directionNodeName;
	float	minPitch;
	float	maxPitch;
	float	minYaw;
	float	maxYaw;
	int pitchAxis;
	int yawAxis;
	float	angularVelocity;
	float	angularThreshold = -1.f;
	float	angularHalflife	= 0.f;

	// first parse the easy arguments
	if (!PyArg_ParseTuple( args, "Osssffffiif|ff",
		&pModelObj,
		&pitchNodeName,
		&yawNodeName,
		&directionNodeName,
		&minPitch,
		&maxPitch,
		&minYaw,
		&maxYaw,
		&yawAxis,
		&pitchAxis,
		&angularVelocity,
		&angularThreshold,
		&angularHalflife))
	{
		PyErr_SetString( PyExc_TypeError, "OrthogonalNodeInfo() expects: "
			"PyModel, string pitchNodeName, "
			"string yawNodeName, "
			"string directionNodeName, "
			"float minPitch, float maxPitch, "
			"float minYaw, float maxYaw, "
			"float minElbowPitch, float maxElbowPitch, "
			"float yawAxis, float pitchAxis, "
			"float angularVelocity and optional "
			"float angularThreshold, float angularHalflife");
		return NULL;
	}

	if (!PyModel::Check( pModelObj ))
	{
		PyErr_SetString( PyExc_TypeError,
			"OrthogonalNodeInfo() expects a PyModel as the first argument" );
		return NULL;
	}

	// make sure the model isn't blank
	PyModel * pModel = (PyModel*)pModelObj;
	SuperModel * pSuperModel = pModel->pSuperModel();
	if (pSuperModel == NULL)
	{
		PyErr_SetString( PyExc_TypeError,
			"OrthogonalNodeInfo() does not work on blank models" );
		return NULL;
	}

	// Get pitch node.
	Moo::NodePtr pitchNode = pSuperModel->findNode( pitchNodeName );
	if (!pitchNode)
	{
		PyErr_Format( PyExc_ValueError, "OrthogonalNodeInfo() "
			"could not find pitch node named '%s' in the model '%s'\n",
			pitchNodeName, pModel->name().c_str() );
		return NULL;
	}

	// Get yaw node.
	Moo::NodePtr yawNode = pSuperModel->findNode( yawNodeName );
	if (!yawNode)
	{
		PyErr_Format( PyExc_ValueError, "OrthogonalNodeInfo() "
			"could not find yaw node named '%s' in the model '%s'\n",
			yawNodeName, pModel->name().c_str() );
		return NULL;
	}

	// Get direction node.
	Moo::NodePtr directionNode = pSuperModel->findNode( directionNodeName );
	if (!directionNode)
	{
		PyErr_Format( PyExc_ValueError, "OrthogonalNodeInfo() "
			"could not find direction node named '%s' in the model '%s'\n",
			directionNodeName, pModel->name().c_str() );
		return NULL;
	}

	// TODO:PM I think it'd be better to keep everything in radians. The tools
	// can expose things as degrees if necessary.

	// Convert the angle values into radians.
	minPitch = DEG_TO_RAD( minPitch );
	maxPitch = DEG_TO_RAD( maxPitch );
	minYaw = DEG_TO_RAD( minYaw );
	maxYaw = DEG_TO_RAD( maxYaw );
	angularVelocity = DEG_TO_RAD( angularVelocity );
	angularThreshold = DEG_TO_RAD( angularThreshold );

	// now actually make the tracker
	return new OrthogonalNodeInfo( pModel,
		pitchNode,
		yawNode,
		directionNode,
		minPitch, maxPitch,
		minYaw, maxYaw,
		yawAxis, pitchAxis,
		angularVelocity,
		angularThreshold,
		angularHalflife);
}


// -----------------------------------------------------------------------------
// BlendNodeInfo:: Constructor(s) and Destructor.
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( BlendNodeInfo )

PY_BEGIN_METHODS( BlendNodeInfo )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( BlendNodeInfo )
PY_END_ATTRIBUTES()

BlendNodeInfo::BlendNodeInfo(
	    BaseNodeInfoPtr pOld,
		BaseNodeInfoPtr pNew,
		float blendTime ) :
	BaseNodeInfo(&*pNew),
	pOld_(pOld),
	pNew_(pNew),
	totalTime_(0.0),
	blendTime_(blendTime)
{
}


/**
 *  This method is called every tracker tick so the node info can do any necessary updates
 *		@param dTime - time since last tick.
 */
BaseNodeInfo * BlendNodeInfo::tick( const float dTime )
{
	BW_GUARD;
	BaseNodeInfo * pSubNI;
	if (pOld_ && (pSubNI = pOld_->tick( dTime )) != &*pOld_) pOld_ = pSubNI;
	if (pNew_ && (pSubNI = pNew_->tick( dTime )) != &*pNew_) pNew_ = pSubNI;

	totalTime_ += dTime;
	return (totalTime_ <= blendTime_) ? this : pNew_.getObject();
}


/**
 *	This method applies transforms so that nodes look at the specified vector.
 */
void BlendNodeInfo::lookAt( const Vector3& lookAtInWS, const Vector3& /*dirInWS*/,
	const float weighting)
{
	BW_GUARD;
	float newW = min( totalTime_ / blendTime_, 1.f );
	float oldW = 1.0f - newW;

	if (pOld_)
	{
		Vector3 oldDirInWS;
		pOld_->getTrackedDirection(oldDirInWS);
		pOld_->lookAt( lookAtInWS, oldDirInWS, oldW * weighting );
	}

	if (pNew_)
	{
		Vector3 newDirInWS;
		pNew_->getTrackedDirection(newDirInWS);
		pNew_->lookAt( lookAtInWS, newDirInWS, newW * weighting );
	}
}


/**
 *	This method accepts a direction vector and updates it to the current
 *  direction of the manipulated bipeds. It uses the pointing node as the
 *  reference. If no pointing node is defined, it attempts to use the scene
 *  root for the model instead.
 *
 *	@param	direction	This vector is the result of the calculation.
 */
void BlendNodeInfo::getTrackedDirection( Vector3 &direction )
{
	BW_GUARD;
	if (pNew_)
		pNew_->getTrackedDirection(direction);
	else if (pOld_)
		pOld_->getTrackedDirection(direction);
}


/**
 *	This method returns whether or not the primary node has been animated or not
 *  @return true if node is animated, false otherwise.
 */
bool BlendNodeInfo::isPrimaryNodeAnimated()
{
	BW_GUARD;
	if (pNew_)
	{
		if (pOld_)
		{
			return pNew_->isPrimaryNodeAnimated() && pOld_->isPrimaryNodeAnimated();
		}
		else
		{
			return pNew_->isPrimaryNodeAnimated();
		}
	}
	else
	{
		if (pOld_)
		{
			return pOld_->isPrimaryNodeAnimated();
		}
		else
		{
			return false;
		}
	}
}

int BlendNodeInfo::size()
{
	BW_GUARD;
	int size = 0;
	if (pNew_)
		size += pNew_->size();
	else
		size++;

	if (pOld_)
		size += pOld_->size();
	else
		size++;
	return size;
}


// -----------------------------------------------------------------------------
// Tracker:: Python Methods.
// -----------------------------------------------------------------------------

/*~ function Tracker debug
 *  This is used to obtain debug information regarding nodeinfo sizes. This
 *  function is not supported and is not intended to be used by game code.
 *  @return A string containing debug information.
 */
/*~ attribute Tracker relativeProvider
 *  If this is non-zero then the nodes are turned to face the target direction
 *  in model space. Otherwise world space is used. Note that this
 *  is set to zero whenever directionProvider is changed.
 *  @type Read-Write Integer.
 */
/*~ attribute Tracker directionProvider
 *  This supplies the pitch and yaw targets. Note that assigning to this
 *  attribute also sets the relativeProvider attribute to false.
 *  @type Read-Write MatrixProvider.
 */
/*~ attribute Tracker nodeInfo
 *  This specifies which nodes this tracker affects, and to what
 *  extent it affects them.
 *  @type Read-Write BaseNodeInfo.
 */
/*~ attribute Tracker cachedNodeInfo
 *  This is the previous nodeInfo, used by the node info blending code. This is
 *  provided for debugging purposes, and is not expected to be used by a game.
 *  @type Read-Only BaseNodeInfo.
 */
/*~ attribute Tracker yaw
 *  The yaw currently applied by the tracker. Assigning a value to this
 *  causes the model to snap to the specified yaw then turn back towards the
 *  yaw supplied by the directionProvider attribute. Note that the model will
 *  not be affected if a directionProvider has not been specified, and that the
 *  value of this is relative to the world coordinate system regardless of the
 *  value of the relativeProvider attribute.
 *  @type Read-Write Float.
 */
/*~ attribute Tracker pitch
 *  The pitch currently applied by the tracker. Assigning a value to this
 *  causes the model to snap to the specified pitch then turn back towards the
 *  pitch supplied by the directionProvider attribute. Note that the model will
 *  not be affected if a directionProvider has not been specified, and that the
 *  value of this is relative to the world coordinate system regardless of the
 *  value of the relativeProvider attribute.
 *  @type Read-Write Float.
 */
PY_TYPEOBJECT( Tracker )

PY_BEGIN_METHODS( Tracker )
	PY_METHOD( debug )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Tracker )
	PY_ATTRIBUTE ( relativeProvider )
	PY_ATTRIBUTE ( directionProvider )
	PY_ATTRIBUTE ( nodeInfo )
	PY_ATTRIBUTE ( cachedNodeInfo )
	PY_ATTRIBUTE ( yaw )
	PY_ATTRIBUTE ( pitch )
PY_END_ATTRIBUTES()

/*~ function BigWorld Tracker
 *  Creates a new instance of the Tracker class. This is a subclass of
 *  PyFashion which manipulates yaw and pitch of the PyModelNode instances in a
 *  PyModel so as to make them face towards (or "track") a target.
 *  @return The new Tracker object.
 */
PY_FACTORY( Tracker, BigWorld )



/**
 *	Helper class to dress the supermodel in our fashion
 */
class Dresser: public TransformFashion
{
public:
	Dresser( Tracker & t ) : t_( t ) {}

	/**
	 *	Dresser requires models to re-calculate their transform state after
	 *	this fashion is applied only if the tracker is currently tracking
	 *	something.
	 *	@return true if the tracker is currently tracking something, otherwise
	 *		false.
	 */
	bool needsRedress() const { return t_.effect(); }

private:
	virtual void dress( SuperModel & sm, Matrix& world ) {}

	Tracker & t_;
};


/**
 *	Constructor
 */
Tracker::Tracker( PyObject * pHitLimitCallBack, PyTypeObject * pType ) :
	PyTransformFashion( pType ),
	pOwner_( NULL ),
	pDresser_( new Dresser( *this ) ),
	pitch_( 0.0f ),
	yaw_( 0.0f ),
	deltaPitch_( 0.0f ),
	deltaYaw_( 0.0f ),
	maxLod_( 150.f ),
	loddedOut_( false ),
	lookAtInWS_( 0.0f, 0.0f, 1.0f ),
	tTime_( 0.0f ),
	pHitLimitCallBack_( pHitLimitCallBack ),
	lookAtInitialised_( false ),
	trackingNothing_(false),
	relativeProvider_(false),
	pNodeInfo_(NULL),
	pDirectionProvider_(NULL),
	pCachedNodeInfo_(NULL),
	maxNodeInfoSize_(0)
{
}

/**
 *	Destructor
 */
INLINE Tracker::~Tracker()
{
}

/**
 * This method returns the current Node Info to a python script
 */
PyObject * Tracker::pyGet_nodeInfo()
{
	BW_GUARD;
	return Script::getData( pNodeInfo_.getObject() );
}

/**
 * This method returns the current Node Info to a python script
 */
PyObject * Tracker::pyGet_cachedNodeInfo()
{
	BW_GUARD;
	return Script::getData( pCachedNodeInfo_.getObject() );
}

/**
 * This method allows a python script to set the Node Info
 */
int Tracker::pySet_cachedNodeInfo( PyObject * value )
{
	BW_GUARD;
	PyErr_SetString( PyExc_AttributeError,
		"can't set tracker.cachedNodeInfo" );
	return -1;
}

/**
 * This method allows a python script to set the Node Info
 */
int Tracker::pySet_nodeInfo( PyObject * value )
{
	BW_GUARD;
	// first make sure we're setting it to a model
	if (value == Py_None)
	{
		if (g_blendNodes)
		{
			SmartPointer<BlendNodeInfo> blender(
				new BlendNodeInfo( pNodeInfo_, NULL ),
				true );
			int bSize = blender->size();
#if TRACKER_DEBUG
			if (g_trace)
			{
				DEBUG_MSG("Tracker::set_directionProvider - old=%p, blender=%p, b_size=%d\n", &*pNodeInfo_.getObject(), &*blender, bSize);
			}
#endif
			if (bSize > maxNodeInfoSize_)
				maxNodeInfoSize_ = bSize;
			pNodeInfo_ = blender;
		}
		else
		{
			pNodeInfo_ = NULL;
		}
	}
	else
	{
			// first make sure we're setting it to a model
		if (value == NULL || (!BaseNodeInfo::Check( value )) )
		{
			PyErr_SetString( PyExc_TypeError,
				"Tracker.nodeInfo must be set to a BaseNodeInfo" );
			return -1;
		}

		BaseNodeInfo* newNodeInfo = static_cast<BaseNodeInfo*>(value);

		if (newNodeInfo == pNodeInfo_)
		{
			return 0;
		}

		if (g_blendNodes)
		{
			SmartPointer<BlendNodeInfo> blender(
				new BlendNodeInfo( pNodeInfo_, newNodeInfo ),
				true );
			int bSize = blender->size();
	#if TRACKER_DEBUG
			if(g_trace)
			{
				DEBUG_MSG("Tracker::set_nodeInfo : blender created - old=%p, new=%p, blender=%p, b_size=%d\n", pNodeInfo_.getObject(), newNodeInfo, blender.getObject(), bSize);
			}
	#endif
			if (bSize > maxNodeInfoSize_)
				maxNodeInfoSize_ = bSize;

			pNodeInfo_ = blender;
		}
		else
		{
	#if TRACKER_DEBUG
			if(g_trace)
			{
				DEBUG_MSG("Tracker::set_nodeInfo : new NodeInfo = %p\n", newNodeInfo);
			}
	#endif
			pNodeInfo_ = newNodeInfo;
		}
	}

	//keep the script assigned nodeInfo,because if pDirectionProvider_ is set as NULL, 
	//pNodeInfo_ will be set as NULL, when pDirectionProvider_ is set as not NULL again, 
	//pNodeInfo_ will be restored to the script assigned value (from pCachedNodeInfo_ ) 
	pCachedNodeInfo_ = pNodeInfo_;


	return 0;
}


/**
 * This method returns the current Direction Provider to a python script
 */
PyObject * Tracker::pyGet_directionProvider()
{
	BW_GUARD;
	return Script::getData( pDirectionProvider_ );
}


/**
 * This method allows a python script to set the Direction Provider
 */
int Tracker::pySet_directionProvider( PyObject * value )
{
	BW_GUARD;
	// first make sure we're setting it to a model
	if (value == Py_None)
	{
		pDirectionProvider_ = NULL;

		//blend pNodeInfo_ to NULL, so that Tracker doesn't effect
		if (g_blendNodes)
		{
			SmartPointer<BlendNodeInfo> blender(
				new BlendNodeInfo( pNodeInfo_, NULL ),
				true );
			int bSize = blender->size();
#if TRACKER_DEBUG
			if (g_trace)
			{
				DEBUG_MSG("Tracker::set_directionProvider - old=%p, blender=%p, b_size=%d\n", pNodeInfo_.getObject(), blender.getObject(), bSize);
			}
#endif
			if (bSize > maxNodeInfoSize_)
				maxNodeInfoSize_ = bSize;
			pNodeInfo_ = blender;
		}
		else
		{
			pNodeInfo_ = NULL;
		}
	}
	else if (value == NULL || !MatrixProvider::Check( value ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Tracker.directionProvider must be set to a MatrixProvider" );
		return -1;
	}
	else
	{
		//previously there was not pDirectionProvider_,pNodeInfo_ was NULL, now restore pNodeInfo_ 
		if (!pDirectionProvider_ )
		{
			if(pCachedNodeInfo_!= pNodeInfo_)
			{
				if (g_blendNodes)
				{
					SmartPointer<BlendNodeInfo> blender(
						new BlendNodeInfo( pNodeInfo_, pCachedNodeInfo_ ),
						true );
					int bSize = blender->size();
#if TRACKER_DEBUG
					if(g_trace)
					{
						DEBUG_MSG("Tracker::set_directionProvider - Cached blender created - pNodeInfo=%p, pCachedNode=%p blender=%p, b_size=%d\n", pNodeInfo_.getObject(), pCachedNodeInfo_.getObject(), &*blender, blender->size());
					}
#endif
					if (bSize > maxNodeInfoSize_)
						maxNodeInfoSize_ = bSize;

					pNodeInfo_ = blender;

				}
				else
				{
					pNodeInfo_ = pCachedNodeInfo_;
				}
			}
		}

		pDirectionProvider_ = static_cast<MatrixProvider*>(value);

		//TODO: maybe some better way to do this. We may accidentally
		//		forget to set it in script
		relativeProvider_ = 0;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Tracker:: Methods dictating Tracking Behaviour.
// -----------------------------------------------------------------------------


/**
 *	Calculates the amount of rotation required by the model for the given
 *	frame. This is stored as deltas to the pitch and yaw values.
 *
 *	@param	dTime	Elapsed time in seconds, since the last frame.
 *	@param	lod		LoD at which model is to be drawn (so calculation can
 *		be abbreviated if the nodes will not be drawn)
 */
void Tracker::tick( float dTime, float lod )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pOwner_ != NULL)
	{
		return;
	}

	loddedOut_ = maxLod_ > 0.f && lod >= maxLod_;

	// tick our node infos and let them rearrange themselves
	if (pNodeInfo_)
	{
		BaseNodeInfo * pSubNI = pNodeInfo_->tick( dTime );
		if (pSubNI != &*pNodeInfo_) pNodeInfo_ = pSubNI;
	}
	if (pCachedNodeInfo_)
	{
		BaseNodeInfo * pSubNI = pCachedNodeInfo_->tick( dTime );
		if (pSubNI != &*pCachedNodeInfo_) pCachedNodeInfo_ = pSubNI;
	}

	// now figure out which way we should be pointing
	if (pNodeInfo_)
	{
		//lodded out so just call tick on nodeInfo and return
		if (loddedOut_) return;

		// Step 1: Find out where the model is facing.
		Vector3 modelDir(pOwner_->worldTransform().applyToUnitAxisVector(2));
		float modelPitch = modelDir.pitch();
		float modelYaw = modelDir.yaw();

		float minPitch = pNodeInfo_->minPitch();
		float maxPitch = pNodeInfo_->maxPitch();
		float minYaw   = pNodeInfo_->minYaw();
		float maxYaw   = pNodeInfo_->maxYaw();

		// Step 2: Determine the direction of the target in world-space in
		// terms of pitch and yaw.
		Angle newPitch;
		Angle newYaw;

		if (pDirectionProvider_)
		{
			Matrix directionMatrix;

			{
				SCOPED_DISABLE_FRAME_BEHIND_WARNING;
				pDirectionProvider_->matrix(directionMatrix);
			}

			if (relativeProvider_)
			{
				newPitch =  modelPitch + directionMatrix.pitch();
				newYaw   =  modelYaw   + directionMatrix.yaw();
			}
			else
			{
				newPitch =  directionMatrix.pitch();
				newYaw   =  directionMatrix.yaw();
			}
		}
		else
		{
			return;
		}

		// Step 3: Clamp the new angles to their limits.
		if (newPitch.clampBetween( modelPitch + minPitch, modelPitch + maxPitch ))
		{
			if (pHitLimitCallBack_)
			{
				int callbackValue;
				// Call the hit-limit call-back function with
				if (Angle::turnRange(modelPitch, newPitch) > MATH_PI)
				{
					callbackValue = 0; //exceed upper pitch limit
				}
				else
				{
					callbackValue = 1; //exceed lower pitch limit
				}
				Py_XINCREF( pHitLimitCallBack_.getObject() );
				PyObject* pResult = PyObject_CallFunction(
						pHitLimitCallBack_.getObject(),
						"i", callbackValue);
				PY_ERROR_CHECK();
				Py_XDECREF(pResult);
			}
		}

		if (newYaw.clampBetween( modelYaw + minYaw, modelYaw + maxYaw ))
		{
			if (pHitLimitCallBack_)
			{
				int callbackValue;
				// Call the hit-limit call-back function with
				if (Angle::turnRange(modelYaw, newYaw) > MATH_PI)
				{
					callbackValue = 2; //exceed upper yaw limit
				}
				else
				{
					callbackValue = 3; //exceed lower yaw limit
				}
				Py_XINCREF( pHitLimitCallBack_.getObject() );
				PyObject* pResult = PyObject_CallFunction(
						pHitLimitCallBack_.getObject(),
						"i", callbackValue);
				PY_ERROR_CHECK();
				Py_XDECREF(pResult);
			}
		}

		// Step 4: Rotate our updated pitch and yaw values to the new
		// direction.
		float desiredDeltaPitch = Angle::sameSignAngle( pitch_, newPitch ) -
				pitch_;
		float desiredDeltaYaw = Angle::sameSignAngle( yaw_, newYaw ) - yaw_;

		// Determine our velocity.
		if ( tTime_ < ACCELERATION_TIME )
		{
			tTime_ += dTime;
			if ( tTime_ > ACCELERATION_TIME )
			{
				tTime_ = ACCELERATION_TIME;
			}
		}


		// Since angular velocity is a velocity, we need to proportion it
		// for the pitch and yaw components.

		float angularVelocity = pNodeInfo_->angularVelocity();
		float pitchVelocity = 0.0f;
		float yawVelocity = 0.0f;
		float deltaYaw = 0.0f;
		float deltaPitch = 0.0f;
		float totalDesiredDeltaAngle = fabsf( desiredDeltaYaw ) +
			fabsf( desiredDeltaPitch );
		if ( totalDesiredDeltaAngle > 0.0f )
		{
			// Break-down the angular velocity into its two components.
			pitchVelocity = angularVelocity * fabsf( desiredDeltaPitch ) /
				totalDesiredDeltaAngle;
			yawVelocity = angularVelocity * fabsf( desiredDeltaYaw ) /
				totalDesiredDeltaAngle;

			// Adjust velocity by the acceleration period.
			float ratio = 1.0f - ( ACCELERATION_TIME - tTime_ ) /
				ACCELERATION_TIME;
			pitchVelocity *= ratio;
			yawVelocity *= ratio;

			float threshold = pNodeInfo_->angularThreshold();
			float halflife = pNodeInfo_->angularHalflife();

			if ( desiredDeltaYaw < 0.0f )
			{
				if ( halflife != 0 && desiredDeltaYaw > -threshold )
				{
					if ( desiredDeltaYaw > -MIN_ANGULAR_THRESHOLD )
						yaw_ += desiredDeltaYaw;
					else
						yaw_ = Math::decay( yaw_, yaw_ + desiredDeltaYaw, halflife, dTime );
				}
				else
				{
					deltaYaw = -yawVelocity * dTime;
					if ( deltaYaw < desiredDeltaYaw )
						deltaYaw = desiredDeltaYaw;
					yaw_ = Angle( yaw_ + deltaYaw );
				}
			}
			else
			{
				if ( halflife != 0 && desiredDeltaYaw < threshold )
				{
					if ( desiredDeltaYaw < MIN_ANGULAR_THRESHOLD )
						yaw_ += desiredDeltaYaw;
					else
						yaw_ = Math::decay( yaw_, yaw_ + desiredDeltaYaw, halflife, dTime );
				}
				else
				{
					deltaYaw = yawVelocity * dTime;
					if ( deltaYaw > desiredDeltaYaw )
						deltaYaw = desiredDeltaYaw;
					yaw_ = Angle( yaw_ + deltaYaw );
				}	
			}

			if ( desiredDeltaPitch < 0.0f )
			{
				if ( halflife != 0 && desiredDeltaPitch > -threshold )
				{
					if ( desiredDeltaPitch > -MIN_ANGULAR_THRESHOLD )
						pitch_ += desiredDeltaPitch;
					else
						pitch_ = Math::decay( pitch_, pitch_ + desiredDeltaPitch, halflife, dTime );
				}
				else
				{
					deltaPitch = -pitchVelocity * dTime;
					if ( deltaPitch < desiredDeltaPitch )
						deltaPitch = desiredDeltaPitch;
					pitch_ = Angle( pitch_ + deltaPitch );
				}
			}
			else
			{
				if ( halflife != 0 && desiredDeltaPitch < threshold )
				{
					if ( desiredDeltaPitch < MIN_ANGULAR_THRESHOLD )
						pitch_ += desiredDeltaPitch;
					else
						pitch_ = Math::decay( pitch_, pitch_ + desiredDeltaPitch, halflife, dTime );
				}
				else
				{
					deltaPitch = pitchVelocity * dTime;
					if ( deltaPitch > desiredDeltaPitch )
						deltaPitch = desiredDeltaPitch;
					pitch_ = Angle( pitch_ + deltaPitch );
				}	
			}

#if TRACKER_DEBUG
			if (g_trace && (deltaPitch < -4.0 || deltaPitch > 4.0))
			{
				DEBUG_MSG("Tracker::tick ratio=%6.3f, a.v=%6.3f, "
						"deltaPitch=%6.3f, desired dP=%6.3f, "
						"pitchVelocity=%6.3f, dTime=%6.3f\n",
					ratio, angularVelocity, deltaPitch, desiredDeltaPitch, pitchVelocity, dTime);

			}

			if (g_trace && (deltaYaw < -4.0 || deltaYaw > 4.0))
			{
				DEBUG_MSG("Tracker::tick ratio=%6.3f, a.v=%6.3f, "
						"deltaYaw=%6.3f, desired dY=%6.3f, yawVelocity=%6.3f, "
						"dTime=%6.3f\n",
					ratio, angularVelocity, deltaYaw, desiredDeltaYaw, yawVelocity, dTime);
			}
#endif
		}

		// Step 5: Convert direction angles into a direction vector.
		lookAtInWS_.setPitchYaw( pitch_, yaw_ );

#if TRACKER_DEBUG
		if (g_trace)
		{
			DEBUG_MSG("Tracker::tick new yaw_=%6.3f, dY=%6.3f, new pitch_=%6.3f, dP=%6.3f, dTime=%6.3f\n",
				yaw_, deltaYaw, pitch_, deltaPitch, dTime);
		}
#endif
	}
}


/**
 *	This method returns a smart pointer to the underlying fashion.
 *	@return the wrapped TransformFashion.
 */
const TransformFashionPtr Tracker::transformFashion() const
{
	BW_GUARD;
	return pDresser_;
}


/**
 *	This method returns a smart pointer to the underlying fashion.
 *	@return the wrapped TransformFashion.
 */
TransformFashionPtr Tracker::transformFashion()
{
	BW_GUARD;
	return pDresser_;
}


/**
 *	This method checks if this is an animation to be applied first
 *	or a tracker to be applied after animating.
 *	@return true because we are a tracker.
 */
bool Tracker::isPreTransform() const
{
	BW_GUARD;
	return false;
}


/**
 *	This method check if this animation is being applied this frame or
 *	if it has been turned off. Eg. due to lodding.
 *	@return true if the transform is actively being applied,
 *		false if lodded out.
 */
bool Tracker::isAffectingTransformThisFrame() const
{
	BW_GUARD;
	return !loddedOut_;
}


/**
 *	This method causes the tracker to effect its changes on a model's nodes.
 *	@return	True if local transforms were changed
 *		and the model needs re-traversing
 */
bool Tracker::effect()
{
	BW_GUARD;
	bool ret = false;

	if ( pOwner_ == NULL ) return ret;

	if (!pNodeInfo_) return ret;

	if (!pNodeInfo_->isPrimaryNodeAnimated()) return ret;

	if (!lookAtInitialised_)
	{
		pNodeInfo_->getTrackedDirection( lookAtInWS_ );

		pitch_ = lookAtInWS_.pitch();
		yaw_   = lookAtInWS_.yaw();
		lookAtInitialised_ = true;
	}


	// Get the direction of the tracked biped nodes.
	Vector3 dirInWS;
	pNodeInfo_->getTrackedDirection( dirInWS );
	pNodeInfo_->lookAt( lookAtInWS_, dirInWS );
	ret = true;

	return ret;
}


/**
 *	This method tells us that the given model wants to own us.
 *	Currently we do not casually make a copy of ourselves if another model
 *	has already claimed us, rather we return an error.
 */
Tracker * Tracker::makeCopy( PyModel * pModel, const char * attrName )
{
	BW_GUARD;
	if (pOwner_ != NULL)
	{
		PyErr_Format( PyExc_ValueError, "Cannot attach given Tracker to Model "
			"as '%s' because it is already attached elsewhere.", attrName );
		return NULL;
	}

	pOwner_ = pModel;
	loddedOut_ = false;

	Py_INCREF( this );
	return this;
}

/**
 *	Lets the Tracker know that its owner is no longer around.
 */
void Tracker::disowned()
{
	// TODO: tick nodeInfo's a lot?
	pOwner_ = NULL;
}

/**
 * This method returns the pointer to CachedNodeInfo
 */
BaseNodeInfo* Tracker::getCachedNodeInfo() const
{
	return pCachedNodeInfo_.getObject();
}

/**
 * This method sets pCachedNodeInfo
 */
void Tracker::setCachedNodeInfo(BaseNodeInfo* nodeInfo)
{
	BW_GUARD;	
#if TRACKER_DEBUG
	/*
	if(g_trace)
	{
		DEBUG_MSG("Tracker::set_nodeInfo : Cached Node Info being changed from 0x%08x to 0x%08x\n", (int)pNodeInfo_.getObject(), (int)nodeInfo);
	}
	*/
#endif
	if(pCachedNodeInfo_.getObject() != nodeInfo)
		pCachedNodeInfo_ = nodeInfo;
}


/**
 * This method returns the pointer to NodeInfo
 */
BaseNodeInfo* Tracker::getNodeInfo() const
{
	BW_GUARD;
	return pNodeInfo_.getObject();
}

/**
 * This method sets pNodeInfo
 */
void Tracker::setNodeInfo(BaseNodeInfo* nodeInfo)
{
	BW_GUARD;	
#if TRACKER_DEBUG
	/*
	if(g_trace)
	{
		DEBUG_MSG("Tracker::setNodeInfo: node Info being changed\n");
	}
	*/
#endif

	if(pNodeInfo_.getObject() != nodeInfo)
		pNodeInfo_ = nodeInfo;
}


/**
 *	This method allows scripts to get a debug string
 */
PyObject * Tracker::py_debug( PyObject * args )
{
	BW_GUARD;
	char msg[256];
	bw_snprintf(msg, sizeof(msg), "Tracker(%p):maxNodeInfoSize=%d, nodeInfo(%p):size=%d",
			this, maxNodeInfoSize_, pNodeInfo_.getObject(), (pNodeInfo_?pNodeInfo_->size():0) );
	return Script::getData(msg);
	Py_RETURN_NONE;
}

BW_END_NAMESPACE

// tracker.cpp
