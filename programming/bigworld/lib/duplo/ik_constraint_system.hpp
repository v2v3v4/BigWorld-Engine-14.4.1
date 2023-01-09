#pragma once
#ifndef IK_CONSTRAINT_SYSTEM_HPP
#define IK_CONSTRAINT_SYSTEM_HPP

/** @file ik_constraint_system.hpp
 *	
 *	This file contains the class and supporting classes for IKConstraintSystem.
 *	This is a PyFashion that can be attached to models to apply animation
 *	constraints and IK.
 */

#include <memory>
#include <utility>
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_unordered_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "duplo/py_transform_fashion.hpp"
#include "math/vector3.hpp"
#include "model/fashion.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

class IKConstraintSystem;
class SuperModelNodeTree;


/**
 *	This abstract class is inherited from by classes in an
 *	IKConstraintSystem with the capability to apply changes to a SuperModel.
 */
class IKCSApplier
{
public:
	/**
	 *	@pre model.getInstanceNodeTree() isn't NULL.
	 *	@post Returned false only if no modifications were made.
	 */
	virtual bool apply( SuperModel & model, IKConstraintSystem & system ) = 0;
};


/**
 *	This class defines a type that provides the constraining matrix using by
 *	a Constraint. This constraining matrix is used by the Constraint object
 *	to constrain its target.
 */
class ConstraintSource
	 :
	 public PyObjectPlus
{
	Py_Header( ConstraintSource, PyObjectPlus )

public:
	explicit ConstraintSource( PyTypeObject * pType );
	virtual ~ConstraintSource(){}

	/**
	 *	Enumerates the possible source types.
	 */
	enum DependencyType
	{
		///A node in the associated model.
		MODEL_NODE,
		///A constraint node.
		CONSTRAINT,
		///Null type (source with no dependency).
		NONE
	};

	typedef std::pair<DependencyType, BW::string> Dependency;
	/**
	 *	@return The type of node this is dependent on and its name (name is in
	 *		the current model that this source depends on if MODEL_NODE, in this
	 *		system for CONSTRAINT or IK_HANDLE). NONE type indicates no
	 *		dependency.
	 */
	virtual Dependency getDependency() const = 0;

	/**
	 *	@pre True.
	 *	@post Returned the current matrix from the source, or identity if it
	 *		could not be found.
	 */
	virtual const Matrix & getMatrix( const SuperModel & model,
		const IKConstraintSystem & system ) = 0;

	/**
	 *	This function is only intended to be used for debugging purposes.
	 *	@return Debug description of the source.
	 */
	virtual BW::string desc() const = 0;
};
typedef SmartPointer<ConstraintSource> ConstraintSourcePtr;
typedef ConstSmartPointer<ConstraintSource> ConstConstraintSourcePtr;
PY_SCRIPT_CONVERTERS_DECLARE( ConstraintSource )


/*~ class BigWorld.ModelNodeConstraintSource
 *	@components{ client }
 *	A constraint source taking its transform from a node in the model
 *	the constraint is applied to.
 */
/**
 *	A constraint source taking its transform from a node in the model
 *	the constraint is applied to.
 */
class ModelNodeConstraintSource
	:
	public ConstraintSource
{
	Py_Header( ModelNodeConstraintSource, ConstraintSource )

public:
	explicit ModelNodeConstraintSource(
		const BW::string & sourceNodeIdentifier );

	virtual Dependency getDependency() const;

	virtual const Matrix & getMatrix( const SuperModel & model,
		const IKConstraintSystem & system );

	virtual BW::string desc() const;

	///	@name Python Methods.
	//@{
	PY_RO_ATTRIBUTE_DECLARE( identifier_.c_str(), identifier );

	PY_AUTO_FACTORY_DECLARE(
		ModelNodeConstraintSource, ARG( BW::string, END ) );
	static ModelNodeConstraintSource * New(
		const BW::string & sourceNodeIdentifier );
	//@}

private:
	///Identifier of the source node.
	const BW::string identifier_;
};


/*~ class BigWorld.MatrixProviderConstraintSource
 *	@components{ client }
 *	A constraint source returning a matrix from a MatrixProvider.
 */
/**
 *	A constraint source returning a matrix from a MatrixProvider.
 */
class MatrixProviderConstraintSource
	:
	public ConstraintSource
{
	Py_Header( MatrixProviderConstraintSource, ConstraintSource )

public:
	explicit MatrixProviderConstraintSource( MatrixProviderPtr & provider );

	virtual Dependency getDependency() const;

	virtual const Matrix & getMatrix( const SuperModel & model,
		const IKConstraintSystem & system );

	virtual BW::string desc() const;

	///	@name Python Methods.
	//@{
	PY_AUTO_FACTORY_DECLARE(
		MatrixProviderConstraintSource, ARG( MatrixProviderPtr, END ) );
	static MatrixProviderConstraintSource * New(
		MatrixProviderPtr & provider );
	//@}

private:
	///Supplies the world-space transform.
	MatrixProviderPtr provider_;
	///Stores the last matrix generated.
	Matrix lastMatrix_;
};


/*~ class BigWorld.ConstraintTarget
 *	@components{ client }
 *	This class defines a type that identifies the target of a constraint.
 *	The target is always in the current IKConstraintSystem or the owning
 *	SuperModel.
 */
/**
 *	This class defines a type that identifies the target of a constraint.
 *	The target is always in the current IKConstraintSystem or the owning
 *	SuperModel.
 */
class ConstraintTarget
	 :
	 public PyObjectPlus
{
	Py_Header( ConstraintTarget, PyObjectPlus )

public:
	/**
	 *	Enumerates the possible types of target.
	 */
	enum Type
	{
		///A node in the associated model.
		MODEL_NODE,
		///An IK handle node's target.
		IK_HANDLE_TARGET,
		///An IK handle node's pole vector.
		IK_HANDLE_POLE_VECTOR
	};


	ConstraintTarget( Type type, const BW::string & identifier );
	virtual ~ConstraintTarget(){}

	bool operator == ( const ConstraintTarget & rhs ) const;

	///	@name Python Methods.
	//@{
	PY_RO_ATTRIBUTE_DECLARE( type_, type )
	PY_RO_ATTRIBUTE_DECLARE( identifier_.c_str(), identifier )
	
	PY_AUTO_FACTORY_DECLARE(
		ConstraintTarget, ARG( BW::string, ARG( BW::string, END ) ) );
	static ConstraintTarget * New(
		const BW::string & strType, const BW::string & identifier );
	//@}
	
	const Matrix & getMatrix(
		const SuperModel & model, const IKConstraintSystem & system ) const;
	Type getType() const;
	const BW::string & getIdentifier() const;

	BW::string desc() const;

private:
	const Type type_;
	const BW::string identifier_;
};
typedef SmartPointer<ConstraintTarget> ConstraintTargetPtr;
typedef ConstSmartPointer<ConstraintTarget> ConstConstraintTargetPtr;
PY_SCRIPT_CONVERTERS_DECLARE( ConstraintTarget )


/*~ class BigWorld.Constraint
 *	@components{ client }
 *	Defines the interface for a constraint to be applied to a node in a model.
 */
/**
 *	Defines the interface for a constraint to be applied to a node in a model.
 */
class Constraint
	:
	public PyObjectPlus
{
	Py_Header( Constraint, PyObjectPlus )

public:
	Constraint(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target,
		PyTypeObject * pType );
	virtual ~Constraint(){}

	///	@name Python Methods.
	//@{
	PY_RO_ATTRIBUTE_DECLARE( identifier_.c_str(), identifier )
	PY_RW_ATTRIBUTE_DECLARE( weight_, weight )
	//@}

	const BW::string & getIdentifier() const;
	ConstraintSourcePtr getSource() const;
	ConstraintTargetPtr getTarget() const;
	float getWeight() const;
	void setWeight( float weight );

	Matrix getMatrix( const SuperModel & model,
		const IKConstraintSystem & system ) const;


	/**
	 *	Enumerates the type of constraint output for a constraint.
	 *	Constraints are interpolated based on weight only with others
	 *	of the same type. Additionally, ROTATION and POSITION constraints
	 *	can simultaneously constrain the same target, but
	 *	if a target has any PARENT constraints it must
	 *	only have PARENT constraints.
	 *	This behaviour mirrors that of Maya.
	 */
	enum Type
	{
		///This constraint applies to rotation.
		ROTATION,
		///This constraint applies to position.
		POSITION,
		///This is a parent constraint that applies position and rotation.
		PARENT
	};
	/**
	 *	@return The type of output from this constraint.
	 */
	virtual Type getType() const = 0;


	/**
	 *	This function is only intended to be used for debugging purposes.
	 *	@return Debug description of the constraint.
	 */
	virtual BW::string desc() const = 0;

protected:
	BW::string baseConstraintMemberDesc() const;

private:
	/**
	 *	For the node with an unconstrained transform of @a unconstrained,
	 *	and a transform for the constraint source of @a sourceMatrix, this
	 *	function returns the result contribution for this.
	 */
	virtual Matrix constrain(
		const Matrix & unconstrained, const Matrix & sourceMatrix ) const = 0;


	///The identifier used for this constraint.
	BW::string identifier_;
	///Source of constraint transforms.
	ConstraintSourcePtr source_;
	///Identifies the target to be constrained.
	ConstraintTargetPtr target_;
	///The weight for this constraint, which controls the proportion of this
	///transform used when there are multiple constraints targeting one node.
	float weight_;
};
typedef SmartPointer<Constraint> ConstraintPtr;
typedef ConstSmartPointer<Constraint> ConstConstraintPtr;
PY_SCRIPT_CONVERTERS_DECLARE( Constraint )


/*~ class BigWorld.ConstraintConstraintSource
 *	@components{ client }
 *	A constraint source returning a matrix from a Constraint.
 */
/**
 *	A constraint source returning a matrix from a Constraint.
 */
class ConstraintConstraintSource
	:
	public ConstraintSource
{
	Py_Header( ConstraintConstraintSource, ConstraintSource )

public:
	explicit ConstraintConstraintSource( ConstraintPtr & constraint );

	virtual Dependency getDependency() const;

	virtual const Matrix & getMatrix(
		const SuperModel & model, const IKConstraintSystem & system );

	virtual BW::string desc() const;

	///	@name Python Methods.
	//@{
	PY_AUTO_FACTORY_DECLARE(
		ConstraintConstraintSource, ARG( ConstraintPtr, END ) );
	static ConstraintConstraintSource * New(
		ConstraintPtr & constraint );
	//@}

private:
	///The constraint we're sourcing the matrix from.
	ConstraintPtr constraint_;
	///Stores the last matrix generated.
	Matrix lastMatrix_;
};


/*~ class BigWorld.PointConstraint
 *	@components{ client }
 *	A constraint applying the source position to the target.
 */
/**
 *	A constraint applying the source position to the target.
 */
class PointConstraint
	:
	public Constraint
{
	Py_Header( PointConstraint, Constraint )

public:
	PointConstraint(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( minTranslation_, minTranslation )
	PY_RW_ATTRIBUTE_DECLARE( maxTranslation_, maxTranslation )
	PY_RW_ATTRIBUTE_DECLARE( offset_, offset )

	PY_AUTO_FACTORY_DECLARE(
		PointConstraint, ARG( BW::string,
			ARG( ConstraintSourcePtr, ARG( ConstraintTargetPtr, END ) ) ) );
	static PointConstraint * New(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );
	//@}
	
	const Vector3 & getMinTranslation() const;
	const Vector3 & getMaxTranslation() const;
	const Vector3 & getOffset() const;
	void setOffset( const Vector3 & offset );
	
	virtual Constraint::Type getType() const;
	virtual BW::string desc() const;

private:
	virtual Matrix constrain(
		const Matrix & unconstrained, const Matrix & sourceMatrix ) const;

	///Minimum translation to be applied by this constraint.
	Vector3 minTranslation_;
	///Maximum translation to be applied by this constraint.
	Vector3 maxTranslation_;
	///(x, y, z) offset this constraint aims to maintain between the constraint
	///source and the target. They are applied in the order x, y, z.
	Vector3 offset_;
};


/*~ class BigWorld.RotationConstraint
 *	@components{ client }
 *	An abstract class representing a constraint applying a rotation to the
 *	target.
 */
/**
 *	An abstract class representing a constraint applying a rotation to the
 *	target.
 */
class RotationConstraint
	:
	public Constraint
{
	Py_Header( RotationConstraint, Constraint )

public:
	RotationConstraint(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target,
		PyTypeObject * pType );

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( minRotations_, minRotations )
	PY_RW_ATTRIBUTE_DECLARE( maxRotations_, maxRotations )
	PY_RW_ATTRIBUTE_DECLARE( offset_, offset )
	//@}
	
	const Vector3 & getMinRotations() const;
	const Vector3 & getMaxRotations() const;
	const Vector3 & getOffset() const;
	void setOffset( const Vector3 & offset );
	
	virtual Constraint::Type getType() const;

protected:
	BW::string baseRotationConstraintMemberDesc() const;
	bool limitRotations( Matrix & input ) const;

private:
	///Minimum rotation amounts (in radians) for x, y, z.
	Vector3 minRotations_;
	///Maximum rotation amounts (in radians) for x, y, z.
	Vector3 maxRotations_;
	///(x, y, z) rotations (in radians) this constraint aims to maintain
	///between the constraint source and the target.
	Vector3 offset_;
};


/*~ class BigWorld.OrientConstraint
 *	@components{ client }
 *	A constraint applying the source rotation to the target.
 */
/**
 *	A constraint applying the source rotation to the target.
 */
class OrientConstraint
	:
	public RotationConstraint
{
	Py_Header( OrientConstraint, RotationConstraint )

public:
	OrientConstraint(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );

	///	@name Python Methods.
	//@{
	PY_AUTO_FACTORY_DECLARE(
		OrientConstraint, ARG( BW::string,
			ARG( ConstraintSourcePtr, ARG( ConstraintTargetPtr, END ) ) ) );
	static OrientConstraint * New(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );
	//@}

	virtual BW::string desc() const;

private:
	virtual Matrix constrain(
		const Matrix & unconstrained, const Matrix & sourceMatrix ) const;
};


/*~ class BigWorld.AimConstraint
 *	@components{ client }
 *	A constraint applying a rotation to the target so that it will face
 *	towards the source.
 */
/**
 *	A constraint applying a rotation to the target so that it will face
 *	towards the source.
 */
class AimConstraint
	:
	public RotationConstraint
{
	Py_Header( AimConstraint, RotationConstraint )

public:
	AimConstraint(
		const BW::string & identifier,
		ConstraintSourcePtr source,
		ConstraintTargetPtr target );

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( aimVector_, aimVector )
	PY_RW_ATTRIBUTE_DECLARE( upVector_, upVector )
	PY_RW_ATTRIBUTE_DECLARE( worldUpVector_, worldUpVector )

	PY_AUTO_FACTORY_DECLARE(
		AimConstraint, ARG( BW::string,
			ARG( ConstraintSourcePtr, ARG( ConstraintTargetPtr, END ) ) ) );
	static AimConstraint * New(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );
	//@}

	virtual BW::string desc() const;
	
	const Vector3 & getAimVector() const;
	void setAimVector( const Vector3 & aimVector );
	const Vector3 & getUpVector() const;
	void setUpVector( const Vector3 & upVector );
	const Vector3 & getWorldUpVector() const;
	void setWorldUpVector( const Vector3 & worldUpVector );

private:
	virtual Matrix constrain(
		const Matrix & unconstrained, const Matrix & sourceMatrix ) const;

	///This vector (defined in model space) is aimed at the target.
	Vector3 aimVector_;
	///The up vector in model space.
	Vector3 upVector_;
	///The up vector will be aligned as closely as possible with this vector
	///in world space.
	Vector3 worldUpVector_;
};


/*~ class BigWorld.ParentConstraint
 *	@components{ client }
 *	A constraint applying transforms as if the target were weighted 100%
 *	to the source node.
 */
/**
 *	A constraint applying transforms as if the target were weighted 100%
 *	to the source node.
 */
class ParentConstraint
	:
	public Constraint
{
	Py_Header( ParentConstraint, Constraint )

public:
	ParentConstraint(
		const BW::string & identifier,
		ConstraintSourcePtr source,
		ConstraintTargetPtr target );

	///	@name Python Methods.
	//@{
	PY_RW_ATTRIBUTE_DECLARE( translationOffset_, translationOffset )
	PY_RW_ATTRIBUTE_DECLARE( rotationOffset_, rotationOffset )

	PY_AUTO_FACTORY_DECLARE(
		ParentConstraint, ARG( BW::string,
			ARG( ConstraintSourcePtr, ARG( ConstraintTargetPtr, END ) ) ) );
	static ParentConstraint * New(
		const BW::string & identifier,
		ConstraintSourcePtr & source,
		ConstraintTargetPtr & target );
	//@}
	
	const Vector3 & getTranslationOffset() const;
	void setTranslationOffset( const Vector3 & offset );
	const Vector3 & getRotationOffset() const;
	void setRotationOffset( const Vector3 & offset );
	
	virtual Constraint::Type getType() const;
	virtual BW::string desc() const;

private:
	virtual Matrix constrain(
		const Matrix & unconstrained, const Matrix & sourceMatrix ) const;

	///(x, y, z) translation offset this constraint aims to maintain between
	///the constraint source and the target.
	Vector3 translationOffset_;
	///(x, y, z) rotation offsets this constraint aims to maintain between
	///the constraint source and the target.
	///They are applied in the order x, y, z and are in radians.
	Vector3 rotationOffset_;
};


/**
 *	This class is used to group constraints that apply to the same target.
 *	This is useful for dependency calculations and because the results will
 *	need to be interpolated depending on the constraint weights.
 */
class ConstraintGroup
	: public IKCSApplier
{
public:
	ConstraintGroup() {}
	explicit ConstraintGroup( BW::vector<ConstraintPtr> & constraints );
	virtual ~ConstraintGroup() {}

	/**
	 *	@return The target of the constraints in this group.
	 */
	ConstConstraintTargetPtr getTarget() const;

	///	@name IKCSApplier Methods.
	//@{
	virtual bool apply( SuperModel & model, IKConstraintSystem & system );
	//@}

	const BW::vector<ConstraintPtr> & getConstraints() const;

private:
	///The constraints in this group.
	BW::vector<ConstraintPtr> constraints_;

	///Stores output (weight, rotation) pairs for this group on each time
	///it's applied.
	BW::vector< std::pair<float, Quaternion> > outputRot_;
	///Stores output (weight, scale) pairs for this group on each time
	///it's applied. This will be the same size as outputRot_ since
	///it just stores the scale component of the input matrices.
	BW::vector< std::pair<float, Vector3> > outputScale_;
	///Stores output (weight, position) pairs for this group on each time
	///it's applied.
	BW::vector< std::pair<float, Vector3> > outputPos_;
};


/*~ class BigWorld.IKHandle
 *	@components{ client }
 *	Defines the interface for an IK handle to be applied to nodes in a model.
 *
 *	Solvers optionally use a pole vector in order to control the plane the
 *	joints lie on (similar to the rotate plane solver in Maya).
 */
/**
 *	Defines the interface for an IK handle to be applied to nodes in a model.
 *
 *	Solvers optionally use a pole vector in order to control the plane the
 *	joints lie on (similar to the rotate plane solver in Maya).
 *	
 *	It is expected that IKHandle::apply will be called on the affected nodes
 *	before animation is propagated to them. This is so the IKHandle
 *	doesn't need to worry about updating all children. It may need to
 *	propagate transforms to those it is directly modifying during calculations
 *	though.
 *	Following the application of the IK solver, the world transform for the
 *	affected nodes will be up-to-date (animation propagated along the chain
 *	and IK solver modifications applied).
 */
class IKHandle
	:
	public PyObjectPlus
	, public IKCSApplier
{
	Py_Header( IKHandle, PyObjectPlus )

public:
	IKHandle( const BW::string & identifier, PyTypeObject * pType );
	virtual ~IKHandle(){}

	///	@name Python Methods.
	//@{
	PY_RO_ATTRIBUTE_DECLARE( identifier_.c_str(), identifier )
	PY_RW_ATTRIBUTE_DECLARE( target_, target )
	PY_RW_ATTRIBUTE_DECLARE( drawTarget_, drawTarget )
	PY_RW_ATTRIBUTE_DECLARE( poleVector_, poleVector )
	PY_RW_ATTRIBUTE_DECLARE( poleVectorIsFromModelPoint_,
		poleVectorIsFromModelPoint )
	PY_RW_ATTRIBUTE_DECLARE( drawPoleVector_, drawPoleVector )
	//@}

	const BW::string & getIdentifier() const;
	const Vector3 & getTarget() const;
	void setTarget( const Vector3 & target );
	bool getDrawTarget() const;
	void setDrawTarget( bool enable );
	void setPoleVector(
		const Vector3 & poleVector, bool poleVectorIsFromModelPoint );
	const Vector3 & getPoleVector() const;
	bool getPoleVectorIsFromModelPoint() const;
	bool getDrawPoleVector() const;
	void setDrawPoleVector( bool enable );

	/**
	 *	@return The bone indices that are targeted (i.e., the indices in
	 *		model.getInstanceNodeTree() of bones that will be modified by
	 *		this handle). The final element is the end bone (that aims to
	 *		move to the target). The first element is that start bone. This
	 *		result will only remain valid until
	 *		model.getNumInstanceNodeTreeRegens() changes. It will be empty
	 *		if there are errors (specified nodes don't exist in model, etc).
	 */
	virtual const BW::vector<int> & getTargetBoneIndices(
		const SuperModel & model ) = 0;

	/**
	 *	This function notifies this handle that any cached id values should be
	 *	considered invalid. Should be called when associated with a new
	 *	IK system or the system is associated with a new model. This needn't
	 *	be called when the SuperModel's SuperModelNodeTree changes (the IK
	 *	handle will detect that internally).
	 */
	virtual void resetCachedIds() = 0;

	/**
	 *	This function is only intended to be used for debugging purposes.
	 *	@return Debug description of the constraint.
	 */
	virtual BW::string desc() const = 0;

protected:
	BW::string baseIKHandleMemberDesc() const;

private:
	///The identifier of this IK handle.
	const BW::string identifier_;

	///The model-space position for target point.
	Vector3 target_;
	///If this is true, the target position will be displayed.
	bool drawTarget_;

	///The pole vector that influences the solution (in model space).
	Vector3 poleVector_;
	///	If this is true, the pole vector value used is currently getting
	///interpreted as a model-space point (not directly as a vector). The
	///pole vector used for the IK will go from the start joint position to
	///this point.
	bool poleVectorIsFromModelPoint_;
	///If this is true, the pole vector will be displayed (converted
	///as appropriate if poleVectorIsFromModelPoint_ is true).
	bool drawPoleVector_;
};
typedef SmartPointer<IKHandle> IKHandlePtr;
typedef ConstSmartPointer<IKHandle> ConstIKHandlePtr;
PY_SCRIPT_CONVERTERS_DECLARE( IKHandle )


/*~ class BigWorld.CCDChainIKHandle
 *	@components{ client }
 *	An IK solver that uses cyclic coordinate descent (CCD) to calculate
 *	joint rotations.
 */
/**
 *	An IK solver that uses cyclic coordinate descent (CCD) to calculate
 *	joint rotations.
 */
class CCDChainIKHandle
	:
	public IKHandle
{
	Py_Header( CCDChainIKHandle, IKHandle )

public:
	CCDChainIKHandle(
		const BW::string & identifier,
		const BW::string & startModelNodeIdentifier,
		const BW::string & endModelNodeIdentifier );

	///	@name Python Methods.
	//@{
	PY_RO_ATTRIBUTE_DECLARE(
		startModelNodeIdentifier_.c_str(), startModelNodeIdentifier )
	PY_RO_ATTRIBUTE_DECLARE(
		endModelNodeIdentifier_.c_str(), endModelNodeIdentifier )
	PY_RW_ATTRIBUTE_DECLARE( maxNumIterations_, maxNumIterations )
	PY_RW_ATTRIBUTE_DECLARE( targetDistTolerance_, targetDistTolerance )

	PY_AUTO_FACTORY_DECLARE(
		CCDChainIKHandle, ARG( BW::string,
			ARG( BW::string, ARG( BW::string, END ) ) ) );
	static CCDChainIKHandle * New(
		const BW::string & identifier,
		const BW::string & startModelNodeIdentifier,
		const BW::string & endModelNodeIdentifier );
	//@}

	///	@name IKHandle Methods.
	//@{
	virtual const BW::vector<int> & getTargetBoneIndices(
		const SuperModel & model );
	virtual bool apply( SuperModel & model, IKConstraintSystem & system );
	virtual void resetCachedIds();
	virtual BW::string desc() const;
	//@}
	
	uint32 getMaxNumIterations() const;
	void setMaxNumIterations( uint32 maxNumIterations );
	float getTargetDistTolerance() const;
	void setTargetDistTolerance( float tolerance );

private:
	///Start joint of the IK chain.
	const BW::string startModelNodeIdentifier_;
	///End joint of the IK chain (chases target).
	const BW::string endModelNodeIdentifier_;
	
	///The maximum number of solver iterations to be applied trying to
	///pull the end node towards the target.
	uint32 maxNumIterations_;
	///The iterations will end before maxNumIterations_ have completed if
	///the end joint is already within this distance of the target.
	float targetDistTolerance_;

	///Value returned from getNumInstanceNodeTreeRegens on the model when
	///targetBones_ was calculated. If it changes, need to regenerate
	///targetBones_.
	uint32 targetBonesTreeRegens_;
	///The node indices in the SuperModelNodeTree for the targeted joints.
	///Stored from start to end.
	BW::vector<int> targetBones_;
	///Scratch space used during apply. Contains targetBones_.size() matrices.
	BW::vector<Matrix> scratchAdjustmentTransforms_;
	///Scratch space used during apply. Contains
	///targetBones_.size() + 1 matrices.
	BW::vector<Matrix> scratchWorldTransforms_;
};


/**
 *	Class of IKCSApplier that will propagate transforms from parent to child.
 */
class ParentTransformApplier
	:
	public IKCSApplier
{
public:
	/**
	 *	@param nodeTreeIndex The index of the node in the SuperModelNodeTree
	 *		for which this will get the parent's transform and apply it to
	 *		this node's relative transform. If the node has no parent, it will
	 *		not do anything.
	 *	@pre @a nodeTreeIndex is positive.
	 *	@post Created object to apply the propagation described above.
	 */
	ParentTransformApplier( int nodeTreeIndex );
	virtual ~ParentTransformApplier(){}

	///	@name IKCSApplier Methods.
	//@{
	virtual bool apply( SuperModel & model, IKConstraintSystem & system );
	//@}

private:
	///Node to propagate parent transform onto.
	int nodeTreeIndex_;
};


/*~ class BigWorld.IKConstraintSystem
 *	@components{ client }
 *	Implements a system of constraints and IK handles that will modify the
 *	nodes for a model.
 */
/**
 *	Implements a system of constraints and IK handles that will modify the
 *	nodes for a model.
 */
class IKConstraintSystem
	:
	public PyTransformFashion
{
	Py_Header( IKConstraintSystem, PyTransformFashion )

public:
	/**
	 *	Creates a system with no constraints, no IK handles, and not
	 *	associated with any PyModel.
	 */
	IKConstraintSystem();
	virtual ~IKConstraintSystem();


	/// @name PyFashion Methods.
	//@{
	virtual void tick( float dTime, float lod );

	virtual IKConstraintSystem * makeCopy(
		PyModel * pModel, const char * attrName );
	virtual void disowned();
	//@}


	///	@name PyTransformFashion Methods.
	//@{
	virtual const TransformFashionPtr transformFashion() const;
	virtual TransformFashionPtr transformFashion();
	virtual bool isPreTransform() const;
	virtual bool isAffectingTransformThisFrame() const;
	//@}


	///	@name Python Methods.
	//@{
	PY_METHOD_DECLARE( py_debug );

	PY_METHOD_DECLARE( py_setConstraint );
	PY_METHOD_DECLARE( py_removeConstraint );
	PY_METHOD_DECLARE( py_getConstraint );
	PY_METHOD_DECLARE( py_getConstraints );

	PY_METHOD_DECLARE( py_setIKHandle );
	PY_METHOD_DECLARE( py_removeIKHandle );
	PY_METHOD_DECLARE( py_getIKHandle );
	PY_METHOD_DECLARE( py_getIKHandles );

	PY_RW_ATTRIBUTE_DECLARE( maxLod_, maxLod );

	static IKConstraintSystem * New();
	PY_AUTO_FACTORY_DECLARE( IKConstraintSystem, END )
	//@}


	bool apply();

	void setConstraint( ConstraintPtr constraint );
	bool removeConstraint( const BW::string & identifier );
	ConstraintPtr getConstraint( const BW::string & identifier );
	ConstConstraintPtr getConstraint( const BW::string & identifier ) const;

	void setIKHandle( IKHandlePtr ikHandle );
	bool removeIKHandle( const BW::string & identifier );
	IKHandlePtr getIKHandle( const BW::string & identifier );
	ConstIKHandlePtr getIKHandle( const BW::string & identifier ) const;

	void compile();
	PY_AUTO_METHOD_DECLARE( RETVOID, compile, END )

	const Matrix & getWorldTransform() const;

	BW::string desc() const;

private:
	void compileInternal( const SuperModel & model,
		const SuperModelNodeTree & tree );
	void generateApplyOrder( const SuperModel & model,
		const SuperModelNodeTree & tree );


	///Used to dress a SuperModel in this fashion.
	class IKCSDresser;

	///Current owner of this system (NULL if not associated).
	PyModel * pOwner_;
	///If true, it indicates that the model this is associated with has been
	///changed since the last call to apply(). This implies that
	///dependencies need recalculating.
	bool newModelSet_;
	///Used to apply this to the model.
	TransformFashionPtr pDresser_;

	///Set to true when this system has been disabled due to LOD.
	bool loddedOut_;
	///The maximum LOD at which this system would be applied.
	float maxLod_;

	typedef BW::unordered_map<BW::string, ConstraintPtr> ConstraintMap;
	///Constraint descriptions.
	ConstraintMap constraints_;

	typedef BW::unordered_map<BW::string, IKHandlePtr> IKHandleMap;
	///Constraint descriptions.
	IKHandleMap ikHandles_;

	///This is set to true to indicate that the dependencies need to be
	///recalculated. This will be true when the set of IK handles and
	///constraints has been changed, the model has been changed, etc.
	bool calculateDependencies_;
	///Value returned from getNumInstanceNodeTreeRegens on the SuperModel when
	///dependencies were last calculated. If it changes, need to regenerate
	///dependencies.
	uint32 lastDependencyCalcTreeRegens_;

	typedef
		std::pair<ConstraintTarget::Type, BW::string>
		ConstraintTargetKey;
	typedef
		BW::unordered_map<ConstraintTargetKey, ConstraintGroup>
		ConstraintGroupMap;
	///Groups of constraints applying to the same target. The map key
	///is the target of the constraint group.
	ConstraintGroupMap constraintGroups_;
	///Appliers to apply the animation propagation from parent to child
	///for world transforms, for every node in the model. The indices for
	///elements here match their index in the SuperModelNodeTree, and this
	///collection is regenerated when that tree changes.
	BW::vector<ParentTransformApplier> animPropagators_;
	///Appliers in the order they should be applied to the model.
	BW::vector<IKCSApplier *> appliers_;
};
typedef SmartPointer<IKConstraintSystem> IKConstraintSystemPtr;
typedef ConstSmartPointer<IKConstraintSystem> ConstIKConstraintSystemPtr;
PY_SCRIPT_CONVERTERS_DECLARE( IKConstraintSystem )

BW_END_NAMESPACE

#endif //IK_CONSTRAINT_SYSTEM_HPP
