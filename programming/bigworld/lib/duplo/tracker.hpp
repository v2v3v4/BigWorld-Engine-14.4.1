#ifndef TRACKER_HPP
#define TRACKER_HPP

#include "cstdmf/bw_vector.hpp"

#include "math/angle.hpp"
#include "math/vector3.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "duplo/py_transform_fashion.hpp"

BW_BEGIN_NAMESPACE

class Dresser;
class Entity;
class PyModel;
class Tracker;

namespace Moo
{
	class Node;
	typedef SmartPointer<Node> NodePtr;
}

/*~ class BigWorld.BaseNodeInfo
 *	This is the abstract base class for the TrackerNodeInfo class. It holds 
 *  information about how a Tracker affects PyModel Node instances, the yaw and
 *  pitch movement ranges that it permits, and the speed at which the nodes 
 *  rotate.
 *
 *  Refer to the Tracker class documentation for an overview of the tracker 
 *  system and related code samples.
 */
/**
 *	This base class for all tracker node infos. It holds information 
 *  about the Nodes that this tracker affects and the weightings for 
 *  each node and the limits of yaw and pitch.
 */
class BaseNodeInfo : public PyObjectPlus
{
	Py_Header( BaseNodeInfo, PyObjectPlus)

public:
	/// @name Constructor(s) and Destructor.
	//@{
	BaseNodeInfo( 
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		PyTypeObject *pType = &s_type_ );
	BaseNodeInfo( BaseNodeInfo const * t, PyTypeObject *pType = &s_type_ );
	BaseNodeInfo( PyTypeObject *pType = &s_type_ );
	virtual ~BaseNodeInfo();
	//@}

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( minYaw_, minYaw )
	PY_RW_ATTRIBUTE_DECLARE( minPitch_, minPitch )

	PY_RW_ATTRIBUTE_DECLARE( maxYaw_, maxYaw )
	PY_RW_ATTRIBUTE_DECLARE( maxPitch_, maxPitch )

	PY_RW_ATTRIBUTE_DECLARE( angularVelocity_, angularVelocity)
	PY_RW_ATTRIBUTE_DECLARE( angularThreshold_, angularThreshold)
	PY_RW_ATTRIBUTE_DECLARE( angularHalflife_, angularHalflife)

	//@}

	///	@name lookAt - applies transforms so that nodes look at the specified vector.
	//@{
	virtual void lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
		const float weighting = 1.0f ) = 0;
	//@}

	///	@name getTrackedDirection - sets direction to the current tracking direction
	//@{
	virtual void getTrackedDirection( Vector3 &direction ) = 0;
	//@}

	///	@name isPrimaryNodeAnimated - returns whether or not the primary node is animated.
	//@{
	virtual bool isPrimaryNodeAnimated() = 0;
	//@}

	///	@name size() - returns the size of nodeInfo, primarily for nested node infos like blend
	//@{
	virtual int size() = 0;
	//@}

	///	@name tick() - this is called every 'tick' of the tracker so
	/// the node info can do any updates it needs to
	/// @return A node info to replace itself, if not equal
	//@{
	virtual BaseNodeInfo * tick( const float dTime ) { return this; }

	///	@name Accessor Methods for Limits and properties.
	//@{
	float minPitch() const;
	void minPitch( float newMinimum );
	float maxPitch() const;
	void maxPitch( float newMaximum );

	float minYaw() const;
	void minYaw( float newMinimum );
	float maxYaw() const;
	void maxYaw( float newMaximum );

	float angularVelocity() const;
	void angularVelocity( float newVelocity );

	float angularThreshold() const;
	void angularThreshold( float newThreshold );

	float angularHalflife() const;
	void angularHalflife( float newHalflife );
	//@}

private:
	float minPitch_;
	float maxPitch_;
	float minYaw_;
	float maxYaw_;
	float angularVelocity_;
	float angularThreshold_;
	float angularHalflife_;
};

typedef SmartPointer<BaseNodeInfo> BaseNodeInfoPtr;


/*~ class BigWorld.TrackerNodeInfo
 *  This is a subclass of BaseNodeInfo. This describes a system by which how 
 *  PyModelNode instances can be affected by a Tracker, and the Node whose 
 *  frame of reference is used for tracker manipulations. These values are all
 *  provided to the factory method used to create this 
 *  (BigWorld.TrackerNodeInfo), and cannot be accessed afterwards.
 *
 *  Refer to the Tracker class documentation for an overview of the tracker 
 *  system and related code samples.
 */
/**
 *	This class holds information about the Nodes that this tracker 
 *  affects and the weightings for each node and the limits of
 *  yaw and pitch.
 */
class TrackerNodeInfo : public BaseNodeInfo
{
	Py_Header( TrackerNodeInfo, BaseNodeInfo )

public:
	TrackerNodeInfo( PyModel *pOwner,
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
		PyTypeObject *pType = &s_type_ );	

	/// @name BaseNodeInfo overrides
	//@{
	virtual void lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
		const float weighting = 1.0f );
	virtual void getTrackedDirection( Vector3 &direction );
	virtual bool isPrimaryNodeAnimated();
	virtual int size();
	//@}

	PY_FACTORY_DECLARE()

private:
	PyModel *pOwner_;
	Moo::NodePtr pPrimaryNode_;
	float primaryNodeWeight_;
	BW::vector< std::pair< Moo::NodePtr, float > > secondaryNodes_;
	Moo::NodePtr pPointingNode_;

};

/*~ class BigWorld.ArmTrackerNodeInfo
 *  This is a subclass of BaseNodeInfo. This describes a two bone arm system  
 *  used to either reach a specific direction or point in a specific direction. 
 *	The values are all provided to the factory method used to create this 
 *  (BigWorld.ArmTrackerNodeInfo).
 *
 *  Refer to the Tracker class documentation for an overview of the tracker 
 *  system and related code samples.
 */
/**
 *	This class holds information about the Nodes that this tracker 
 *  affects and the weightings for each node and the limits of
 *  yaw and pitch.
 */

class ArmTrackerNodeInfo : public BaseNodeInfo
{
	Py_Header( ArmTrackerNodeInfo, BaseNodeInfo )

public:
	ArmTrackerNodeInfo( PyModel *pOwner,
		Moo::NodePtr pUpperArmNode,
		Moo::NodePtr pForeArmNode,

		//Node direction clamps	
		float minShoulderPitch,
		float maxShoulderPitch,
		float minShoulderYaw,
		float maxShoulderYaw,
		float minShoulderRoll,
		float maxShoulderRoll,
		float minElbowYaw,
		float maxElbowYaw,

		//Overall direction clamps
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,

		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		int	  pointingAxis,

		PyTypeObject *pType = &s_type_ );	

	/// @name BaseNodeInfo overrides
	//@{
	virtual void lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS,
		const float weighting = 1.0f );
	virtual void getTrackedDirection( Vector3 &direction );
	virtual bool isPrimaryNodeAnimated();
	virtual int size();

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( minShoulderPitch_, minShoulderPitch )
	PY_RW_ATTRIBUTE_DECLARE( maxShoulderPitch_, maxShoulderPitch )
	PY_RW_ATTRIBUTE_DECLARE( minShoulderYaw_, minShoulderYaw )
	PY_RW_ATTRIBUTE_DECLARE( maxShoulderYaw_, maxShoulderYaw )
	PY_RW_ATTRIBUTE_DECLARE( minShoulderRoll_, minShoulderRoll )
	PY_RW_ATTRIBUTE_DECLARE( maxShoulderRoll_, maxShoulderRoll )
	PY_RW_ATTRIBUTE_DECLARE( minElbowYaw_, minElbowYaw )
	PY_RW_ATTRIBUTE_DECLARE( maxElbowYaw_, maxElbowYaw )
	PY_RW_ATTRIBUTE_DECLARE( pointingAxis_, pointingAxis )

private:
	PyModel *pOwner_;

	Moo::NodePtr pUpperArmNode_;
	Moo::NodePtr pForeArmNode_;

	float minShoulderPitch_;
	float maxShoulderPitch_;
	float minShoulderYaw_;
	float maxShoulderYaw_;
	float minShoulderRoll_;
	float maxShoulderRoll_;
	float minElbowYaw_;
	float maxElbowYaw_;

	int	  pointingAxis_;

};


/*~ class BigWorld.OrthogonalNodeInfo
 *  This is a subclass of BaseNodeInfo. An Instance of this class describes  
 *  a system by which PyModelNode instances can be affected by a Tracker. 
 *  Such an instance can be used by a Tracker to apply pitch changes to one 
 *  model node, and yaw changes to another. This can be used very effectively 
 *  with models which represent machinery that turns on swivel joints. The 
 *  values which control this behaviour are all provided to the factory method 
 *  used to create this (BigWorld.OrthogonalNodeInfo).  Once created, only the 
 *	pitch and yaw axis can be accessed and/or changed.
 *
 *  Refer to the Tracker class documentation for an overview of the tracker 
 *  system and related code samples.
 */
/**
 *	This class holds information about the Nodes that this tracker affects. It
 *	has one node whose pitch is changed, one node whose yaw is changed and a
 *	node that it will try and point towards the target.
 */
class OrthogonalNodeInfo : public BaseNodeInfo
{
	Py_Header( OrthogonalNodeInfo, BaseNodeInfo )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	OrthogonalNodeInfo( PyModel *pOwner,
		Moo::NodePtr pPitchNode,
		Moo::NodePtr pYawNode,
		Moo::NodePtr pDirectionNode,
		float minPitch,
		float maxPitch,
		float minYaw,
		float maxYaw,
		int pitchAxis,
		int yawAxis,
		float angularVelocity,
		float angularThreshold,
		float angularHalflife,
		PyTypeObject *pType = &s_type_ );	
	//@}

	/// @name BaseNodeInfo overrides
	//@{
	virtual void lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS, 
		const float weighting = 1.0f );
	virtual void getTrackedDirection( Vector3 &direction );
	virtual bool isPrimaryNodeAnimated();
	virtual int size();
	//@}

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( yawAxis_, yawAxis )
	PY_RW_ATTRIBUTE_DECLARE( pitchAxis_, pitchAxis )

private:
	PyModel *pOwner_;
	Moo::NodePtr pPitchNode_;
	Moo::NodePtr pYawNode_;
	Moo::NodePtr pDirectionNode_;	
	int pitchAxis_;
	int yawAxis_;

};

/*~ class BigWorld.BlendNodeInfo
 *  This is a subclass of BaseNodeInfo. Instances of this class are used by the
 *  Tracker system to blend between BaseNodeInfo objects over 0.3 seconds. This
 *  is automatically done whenever the nodeInfo or directionProvider properties
 *  of a Tracker are changed. Instances of this class cannot be created or
 *  accessed via script.
 */
/**
 *	This class allows you to blend between two BaseNodeInfos
 */
class BlendNodeInfo : public BaseNodeInfo
{
	Py_Header( BlendNodeInfo, BaseNodeInfo)

public:
	/// @name Constructor(s) and Destructor.
	//@{
	BlendNodeInfo(
		BaseNodeInfoPtr pOld,
		BaseNodeInfoPtr pNew,
		float blendTime = 0.3f);
	//@}

	/// @name BaseNodeInfo overrides
	//@{
	virtual void lookAt( const Vector3& lookAtInWS, const Vector3& dirInWS, 
		const float weighting = 1.0f);
	virtual void getTrackedDirection( Vector3 &direction );
	virtual bool isPrimaryNodeAnimated();
	virtual int size();
	virtual BaseNodeInfo * tick( const float dTime );
	//@}

private:
	float totalTime_;
	const float blendTime_;
	BaseNodeInfoPtr pOld_;
	BaseNodeInfoPtr pNew_;
};



/*~ class BigWorld.Tracker
 *	Tracker is a subclass of PyFashion that manipulates the yaw and pitch of 
 *  Node instances in a PyModel to make them face towards (or "track") a 
 *  target. The target may be another PyEntity / PyModel, an entity's 
 *  auxiliary data, a specific direction, a point moving through a range of 
 *  angles (allowing the PyModel to scanning back & forth), or anything else 
 *  that can be represented by a MatrixProvider.
 *
 *  The following steps set up and use a Tracker:
 *  
 *  Create a Tracker object. A new Tracker object is create by calling
 *  BigWorld.Tracker(). The factory method doesn't take any parameters,
 *  so the attributes of the new object need to be set.
 *
 *  Assign a direction provider. This is the matrix provider which tells the
 *  Tracker which direction it should try to orient the PyModelNode objects 
 *  that it affects. If this is not assigned or is set to None then the
 *  Tracker will have no effect, even if the pitch and yaw attributes are 
 *  manually altered. 
 *
 *  Specify whether the direction provider provides an absolute direction, or a
 *  direction relative to the model's orientation. Note that this should be set
 *  after the direction provider, as this is set to false each time the 
 *  direction provider is changed.
 *
 *  Specify a NodeInfo (an instance of a child class of BaseNodeInfo). This 
 *  object contains data used by the Tracker to determine which PyModelNode 
 *  objects to affect, and to what degree to affect them.
 *
 *  Apply the Tracker to a PyModel. This is achieved by assigning the Tracker
 *  object to a member variable belonging to the model. This variable need not
 *  exist beforehand, however it can be referenced afterwards (to remove the
 *  Tracker, for example).
 *
 *  Code Example: Turning a character model's head to point in the direction 
 *  the player is facing. This demonstrates a simple use for the
 *  EntityDirProvider class.
 *  @{
 *	# NOTE: model refers to the character model to which this is applied
 *	# NOTE: ent refers to the entity whose direction model will turn to face
 *
 *	# create a new Tracker
 *	tracker = BigWorld.Tracker()
 *
 *	# create an EntityDirProvider  which follows the entity's facing and
 *  # apply it to the Tracker
 *	facingDirProvider = BigWorld.EntityDirProvider( ent, 1, 0 )
 *	tracker.directionProvider = facingDirProvider
 *
 *	# create list of secondary nodes
 *	secondaryNodes = [ ( "biped Neck", 0.5 ) ]
 *
 *	# create TrackerNodeInfo & apply to Tracker
 *	nodeInfo = BigWorld.TrackerNodeInfo( model, "biped Head", secondaryNodes,
 *                                       "None", -100, 100, -100, 100, 100 )
 *	tracker.nodeInfo = nodeInfo
 *
 *	# apply tracker to the model
 *	model.facingTracker = tracker
 *  @}
 *  
 *  Code Example: Turning a character model's head to watch another entity.
 *  This demonstrates a simple use for the DiffDirProvider class.
 *  @{
 *  # NOTE: model refers to the character model to which this is applied
 *  # NOTE: sourceEntity refers to the entity which owns model
 *  # NOTE: targetEntity is the entity to be watched by model
 *  
 *  # create a new Tracker
 *  tracker = BigWorld.Tracker()
 *  
 *  # create a DiffDirProvider & apply it to the Tracker
 *  diffDirProvider = BigWorld.DiffDirProvider( sourceEntity.matrix, 
 *                                              targetEntity.matrix )
 *  tracker.directionProvider = diffDirProvider
 *  
 *  # create list of secondary nodes
 *  secondaryNodes = [ ( "biped Neck", 0.5 ) ]
 *  
 *  # create TrackerNodeInfo & apply to Tracker
 *  nodeInfo = BigWorld.TrackerNodeInfo( model, "biped Head", secondaryNodes,
 *                                       "None", -100, 100, -100, 100, 100 )
 *  tracker.nodeInfo = nodeInfo
 *  
 *  # apply tracker to the model
 *  model.diffDirTracker = tracker
 *  @}
 *
 *  Code Example: Swinging a character model's arms idly back & forth. This 
 *  demonstrates a use for the ScanDirProvider class, and also the use of 
 *  negative weightings for secondary nodes. By applying negative weightings
 *  to children of the primary node, this example effectively stops the Tracker
 *  from applying its transformation to all subsequent nodes. This is shown in
 *  the example by the head remaining stationary despite the movement which
 *  the Tracker creates lower down the spine.
 *  @{
 *  # NOTE: model refers to the character model to which this is applied
 *  
 *  # create a new Tracker
 *  tracker = BigWorld.Tracker()
 *  
 *  # create a ScanDirProvider & apply it to the Tracker
 *  scanDirProvider = BigWorld.ScanDirProvider( 1, 5 ) # this scans +/-1 radian
 *                                                     # every 5 seconds
 *  tracker.directionProvider = scanDirProvider
 *  tracker.relativeProvider = 1
 *  
 *  # create list of secondary nodes
 *  secondaryNodes = []
 *  secondaryNodes.append( ( "biped Neck", -0.5 ) )
 *  secondaryNodes.append( ( "biped Head", -0.5 ) )
 *  secondaryNodes.append( ( "biped Spine1", 0.5 ) )
 *  
 *  # create a TrackerNodeInfo & apply to the tracker
 *  nodeInfo = BigWorld.TrackerNodeInfo( model, "biped Spine1", secondaryNodes,
 *                                       "None", -100, 100, -100, 100, 100 )
 *  tracker.nodeInfo = nodeInfo
 *  
 *  # apply tracker to the model
 *  model.armSwingTracker = tracker
 *  @}
 */
/**
 *	This class manipulates the nodes of a model to track a target.
 *	The target may be another entity / model, an entity's auxiliary
 *	data, a specific direction, or a point moving through a range of
 *	angles (for scanning).
 */
class Tracker : public PyTransformFashion
{
	Py_Header( Tracker, PyTransformFashion )

public:
	/// @name Constructor and Destructor.
	//@{
	Tracker( PyObject *pHitLimitCallBack = NULL,
			 PyTypeObject *pType = &Tracker::s_type_ );
	~Tracker();
	//@}

	///	@name Python Methods.
	//@{
	PyObject * pyGet_directionProvider();
	int pySet_directionProvider( PyObject * value );

	PyObject * pyGet_nodeInfo();
	int pySet_nodeInfo( PyObject * value );

	PyObject * pyGet_cachedNodeInfo();
	int pySet_cachedNodeInfo( PyObject * value );

	PY_METHOD_DECLARE( py_debug );

	PY_RW_ATTRIBUTE_DECLARE( relativeProvider_, relativeProvider )
	PY_RW_ATTRIBUTE_DECLARE( yaw_, yaw )
	PY_RW_ATTRIBUTE_DECLARE( pitch_, pitch )

	PY_RW_ATTRIBUTE_DECLARE( maxLod_, maxLod )

	static Tracker * New( SmartPointer<PyObject> pHitLimitCallback )
		{ return new Tracker( &*pHitLimitCallback ); }
	PY_AUTO_FACTORY_DECLARE( Tracker,
		OPTARG( SmartPointer<PyObject>, NULL, END ) )
	//@}

	/// @name PyFashion Methods.
	//@{
	virtual void tick( float dTime, float lod );
	const TransformFashionPtr transformFashion() const;
	TransformFashionPtr transformFashion();
	bool isPreTransform() const;
	bool isAffectingTransformThisFrame() const;

	virtual Tracker * makeCopy( PyModel * pModel, const char * attrName );
	virtual void disowned();
	//@}

	bool effect();

	BaseNodeInfo* getCachedNodeInfo()const ;
	void setCachedNodeInfo(BaseNodeInfo* );

	BaseNodeInfo* getNodeInfo() const;
	void setNodeInfo(BaseNodeInfo* );


private:
	///	@name Private Helper Methods.
	//@{
	void getTrackedDirection( Vector3 &direction );
	//@}


	///	@name Data associated with Tracking Behaviour.
	//@{
	PyModel		* pOwner_;
	SmartPointer<TransformFashion> pDresser_;

	float pitch_;
	float yaw_;

	Angle deltaPitch_;
	Angle deltaYaw_;

	float	maxLod_;
	bool	loddedOut_;

	SmartPointer<PyObject>	pHitLimitCallBack_;

	Vector3 lookAtInWS_;
	float tTime_;

	bool lookAtInitialised_;
	bool trackingNothing_;

	MatrixProviderPtr pDirectionProvider_;
	BaseNodeInfoPtr pNodeInfo_;

	//used to keep the script assigned nodeInfo,because if pDirectionProvider_ is set as NULL, 
	//pNodeInfo_ will be set as NULL, when pDirectionProvider_ is set as not NULL again, 
	//pNodeInfo_ will be restored to the script assigned value (from pCachedNodeInfo_ ) 
	BaseNodeInfoPtr pCachedNodeInfo_;

	bool relativeProvider_;

	int maxNodeInfoSize_;
	//@}
};


#ifdef CODE_INLINE
#include "tracker.ipp"
#endif

BW_END_NAMESPACE

#endif
/* tracker.hpp */
