#include "pch.hpp"

#include "ik_constraint_system.hpp"

#include <algorithm>
#include <limits>
#include <functional>
#include <cstddef>
#if !CONSUMER_CLIENT_BUILD
#include <ostream>
#endif

#include "cstdmf/guard.hpp"
#include "cstdmf/dependency_graph.hpp"
#include "cstdmf/bw_unordered_set.hpp"
#include "cstdmf/bw_hash.hpp"
#include "math/mathdef.hpp"
#include "math/planeeq.hpp"
#include "math/quat.hpp"
#include "model/model.hpp"
#include "model/super_model.hpp"
#include "moo/node.hpp"
#include "pymodel.hpp"
#include "pyscript/script.hpp"
#include "moo/line_helper.hpp"
#include "moo/geometrics.hpp"

DECLARE_DEBUG_COMPONENT2( "IKConstraintSystem", 0 );


BW_BEGIN_NAMESPACE

namespace
{
	/**
	 *	@return The world space transform for the node with identifier
	 *		@a identifier from @a model, or if it doesn't exist returned
	 *		I.
	 */
	const Matrix & getMatrixSafe(
		const SuperModel & model, const BW::string & identifier )
	{
		//The conditions checked here are to make sure we satisfy the
		//preconditions in SuperModel. Those preconditions are so it's
		//easy to change later to allow constraints from other models.
		
		const SuperModelNodeTree * tree = model.getInstanceNodeTree();

		if (tree != NULL)
		{
			const int treeNodeIndex = tree->getNodeIndex( identifier );

			if (treeNodeIndex < 0)
			{
				WARNING_MSG( "Tree node with identifier '%s' "
					"does not exist.\n", identifier.c_str() );
			}
			else
			{
				return model.getNodeWorldTransform( treeNodeIndex );
			}
		}
		else
		{
			WARNING_MSG( "SuperModelNodeTree not available.\n" );
		}

		return Matrix::identity;
	}
	
	
	/**
	 *	@pre All weights in @a rotations are positive.
	 *	@post Interpolated the rotations in @a rotations proportionally
	 *		based on their weight, and assigned the result into
	 *		@a output.
	 */
	void interpolate( Quaternion & output,
		const BW::vector< std::pair<float, Quaternion> > & rotations )
	{
		MF_ASSERT( !rotations.empty() );

		output = rotations.front().second;
		if (rotations.size() < 2)
		{
			return;
		}
		float weightSoFar = rotations.front().first;
		
		for (BW::vector< std::pair<float, Quaternion> >::const_iterator itr =
				rotations.begin() + 1;
			itr != rotations.end();
			++itr)
		{
			MF_ASSERT( itr->first >= 0.0f );

			if (itr->first > 0.0f)
			{
				output.slerp( output, itr->second,
					itr->first / (weightSoFar + itr->first) );

				weightSoFar += itr->first;
			}
		}
	}


	/**
	 *	@pre All weights in @a positions are positive.
	 *	@post Interpolated the positions in @a positions proportionally
	 *		based on their weight, and assigned the result into
	 *		@a output.
	 */
	void interpolate( Vector3 & output,
		const BW::vector< std::pair<float, Vector3> > & positions )
	{
		MF_ASSERT( !positions.empty() );

		output = positions.front().second;
		if (positions.size() < 2)
		{
			return;
		}
		float weightSoFar = positions.front().first;
		
		for (BW::vector< std::pair<float, Vector3> >::const_iterator itr =
				positions.begin() + 1;
			itr != positions.end();
			++itr)
		{
			MF_ASSERT( itr->first >= 0.0f );

			if (itr->first > 0.0f)
			{
				output.lerp( output, itr->second,
					itr->first / (weightSoFar + itr->first) );

				weightSoFar += itr->first;
			}
		}
	}


	/**
	 *	@pre @a tree is the current instance node tree for @a model.
	 *	@pre @a nodeIndex is a valid node index in @a tree.
	 *	@return The world transform for nodeIndex's parent (or if it's a root,
	 *		the base world transform).
	 */
	Matrix getNodeParentWorldTransform(
		const SuperModel & model,
		const SuperModelNodeTree & tree, int nodeIndex )
	{
		MF_ASSERT( &tree == model.getInstanceNodeTree() );
		MF_ASSERT( 0 <= nodeIndex );
		MF_ASSERT( nodeIndex < tree.getNumNodes() );

		const int parentIndex = tree.getNode( nodeIndex ).getParentIndex();
		if (parentIndex >= 0)
		{
			return model.getNodeWorldTransform( parentIndex );
		}
		else
		{
			Matrix result =
				model.getNodeRelativeTransform( nodeIndex );
			result.invert();
			result.postMultiply(
				model.getNodeWorldTransform( nodeIndex ) );
			return result;
		}
	}


	/**
	 *	@pre @a model has an instance node tree.
	 *	@pre @a nodeIndex is a valid node index in @a model's instance
	 *		node tree.
	 *	@post Configured node @a nodeIndex in @a model's instance node tree to
	 *		have its relative transform set appropriately so that the output
	 *		world transform will equate to @a worldTransform, and set the
	 *		node's world transform to be @a worldTransform.
	 */
	void applyWorldTransformToModelNode(
		SuperModel & model, int nodeIndex, const Matrix & worldTransform )
	{
		const SuperModelNodeTree * tree = model.getInstanceNodeTree();
		MF_ASSERT( tree != NULL );

		MF_ASSERT( nodeIndex >= 0 );
		MF_ASSERT( nodeIndex < tree->getNumNodes() );

		const int parentIndex = tree->getNode( nodeIndex ).getParentIndex();
		if (parentIndex >= 0)
		{
			Matrix relTransform( model.getNodeWorldTransform( parentIndex ) );
			relTransform.invert();//is now parent world transform inverse.
			relTransform.preMultiply( worldTransform );

			model.setNodeRelativeTransform( nodeIndex, relTransform );
		}
		else
		{
			//Previous world matrix set directly on this node (it's a root).
			//We get it out and then apply the inverse of it so then we can
			//apply our replacement.

			Matrix relTransform =
				model.getNodeWorldTransform( nodeIndex );
			relTransform.invert();
			relTransform.postMultiply(
				model.getNodeRelativeTransform( nodeIndex ) );
			relTransform.preMultiply( worldTransform );
			
			model.setNodeRelativeTransform( nodeIndex, relTransform );
		}
		
		model.setNodeWorldTransform( nodeIndex, worldTransform );
	}


	/**
	 *	@pre @a model has an instance node tree.
	 *	@post If @a model has a node with identifier @a nodeIdentifier,
	 *		called the other applyWorldTransformToModelNode function to
	 *		set its world transform.
	 */
	void applyWorldTransformToModelNode(
		SuperModel & model,
		const BW::string & nodeIdentifier,
		const Matrix & worldTransform )
	{
		const SuperModelNodeTree * tree = model.getInstanceNodeTree();
		MF_ASSERT( tree != NULL );

		const int nodeIndex = tree->getNodeIndex( nodeIdentifier );
		if (nodeIndex < 0)
		{
			WARNING_MSG(
				"Target model node '%s' doesn't exist.\n",
				nodeIdentifier.c_str() );
			return;
		}

		applyWorldTransformToModelNode(
			model, nodeIndex, worldTransform );
	}


	/**
	 *	@return Matrix that will rotate @a upVect about @a aimVect so that it
	 *		lies as close as possible to @a worldUpVect.
	 */
	Matrix rotateUpVectAboutAimAxis( const Vector3 & upVect,
		const Vector3 & worldUpVect, const Vector3 & aimVect )
	{
		const float ZERO_LENGTH_SQ_EPSILON = 0.001f;

		if (aimVect.lengthSquared() < ZERO_LENGTH_SQ_EPSILON)
		{
			return Matrix::identity;
		}

		Vector3 upOnPlane = upVect.projectOntoPlane( aimVect );
		Vector3 wupOnPlane = worldUpVect.projectOntoPlane( aimVect );

		//Must check vector length. Maya does this if the user has specified an
		//up vector parallel to the aim vector, so AimConstraint must also work
		//in this circumstance.
		const float upVectPlaneLenSq = upOnPlane.lengthSquared();
		const float wupVectPlaneLenSq = wupOnPlane.lengthSquared();
		if ((upVectPlaneLenSq < ZERO_LENGTH_SQ_EPSILON) ||
			(wupVectPlaneLenSq < ZERO_LENGTH_SQ_EPSILON))
		{
			return Matrix::identity;
		}

		upOnPlane /= sqrt( upVectPlaneLenSq );
		wupOnPlane /= sqrt( wupVectPlaneLenSq );

		const float upRotAngle = upOnPlane.getUnitVectorAngle( wupOnPlane );
		
		const Vector3 planeRotAxis( upOnPlane.crossProduct( wupOnPlane ) );

		//Apply up vector rotation to the result.
		Quaternion rot;
		rot.fromAngleAxis( upRotAngle, planeRotAxis );
		Matrix result;
		result.setRotate( rot );
		return result;
	}


	///Used to make debug printing more convenient.
	std::ostream & operator << (
		std::ostream & output, ConstraintSource::DependencyType depType )
	{
		const char * str = NULL;
		switch (depType)
		{
		case ConstraintSource::CONSTRAINT:
			str = "constraint";
			break;

		case ConstraintSource::MODEL_NODE:
			str = "modelnode";
			break;

		case ConstraintSource::NONE:
			str = "none";
			break;

		default:
			MF_ASSERT( false );
			str = "[unhandled ConstraintSource::DependencyType]";
			break;
		}

		return output << str;
	}


	///Used to make debug printing more convenient.
	std::ostream & operator << (
		std::ostream & output, ConstraintTarget::Type type )
	{
		const char * str = NULL;
		switch (type)
		{
		case ConstraintTarget::MODEL_NODE:
			str = "modelnode";
			break;

		case ConstraintTarget::IK_HANDLE_TARGET:
			str = "ikhandletarget";
			break;

		case ConstraintTarget::IK_HANDLE_POLE_VECTOR:
			str = "ikhandlepolevector";
			break;

		default:
			MF_ASSERT( false );
			str = "[unhandled ConstraintTarget::Type]";
			break;
		}

		return output << str;
	}


	/**
	 *	@return A debug print string for a Vector3 that will replace
	 *		max values with the string "max" for easier reading.
	 */
	BW::string getVector3DebugStr( const Vector3 & vect )
	{
		BW::ostringstream str;
		str << "(";
		for (int i = 0; i < 3; ++i)
		{
			if (vect[i] == std::numeric_limits<float>::max())
			{
				str << "max";
			}
			else if (vect[i] == -std::numeric_limits<float>::max())
			{
				str << "-max";
			}
			else
			{
				str << vect[i];
			}

			if (i < 2)
			{
				str << ", ";
			}
		}
		str << ")";
		return str.str();
	}
}


// -----------------------------------------------------------------------------
// Section: ConstraintSource
// -----------------------------------------------------------------------------
// 
PY_TYPEOBJECT( ConstraintSource)

PY_BEGIN_METHODS( ConstraintSource )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ConstraintSource )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ConstraintSource )


ConstraintSource::ConstraintSource( PyTypeObject * pType )
	:
	PyObjectPlus( pType )
{
}


// -----------------------------------------------------------------------------
// Section: ModelNodeConstraintSource
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ModelNodeConstraintSource)

PY_BEGIN_METHODS( ModelNodeConstraintSource )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ModelNodeConstraintSource )
	/*~	attribute ModelNodeConstraintSource.identifier
	 *	The identifier of the source node.
	 *	@type string
	 */
	PY_ATTRIBUTE ( identifier )
PY_END_ATTRIBUTES()

PY_FACTORY( ModelNodeConstraintSource, BigWorld )


/**
 *	@param identifier The identifier of the node that this will source
 *		transforms from.
 */
ModelNodeConstraintSource::ModelNodeConstraintSource(
	const BW::string & identifier )
	:
	ConstraintSource( &s_type_ )
	, identifier_( identifier )
{
}


ModelNodeConstraintSource::Dependency
	ModelNodeConstraintSource::getDependency() const
{
	BW_GUARD;

	return Dependency( ConstraintSource::MODEL_NODE, identifier_ );
}


const Matrix & ModelNodeConstraintSource::getMatrix( const SuperModel & model,
	const IKConstraintSystem & system )
{
	BW_GUARD;

	return getMatrixSafe( model, identifier_ );
}


BW::string ModelNodeConstraintSource::desc() const
{
	BW_GUARD;

	return BW::string("ModelNodeConstraintSource '") + identifier_ + "'";
}


ModelNodeConstraintSource * ModelNodeConstraintSource::New(
	const BW::string & sourceNodeIdentifier )
{
	BW_GUARD;

	return new ModelNodeConstraintSource( sourceNodeIdentifier );
}



// -----------------------------------------------------------------------------
// Section: MatrixProviderConstraintSource
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( MatrixProviderConstraintSource)

PY_BEGIN_METHODS( MatrixProviderConstraintSource )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixProviderConstraintSource )
PY_END_ATTRIBUTES()

PY_FACTORY( MatrixProviderConstraintSource, BigWorld )


/**
 *	@param provider The provider of the world space transform that
 *		this will return.
 */
MatrixProviderConstraintSource::MatrixProviderConstraintSource(
	MatrixProviderPtr & provider )
	:
	ConstraintSource( &s_type_ )
	, provider_( provider )
{
}


MatrixProviderConstraintSource::Dependency
	MatrixProviderConstraintSource::getDependency() const
{
	BW_GUARD;

	return Dependency( ConstraintSource::NONE, "" );
}


const Matrix & MatrixProviderConstraintSource::getMatrix(
	const SuperModel & model, const IKConstraintSystem & system )
{
	BW_GUARD;

	provider_->matrix( lastMatrix_ );
	return lastMatrix_;
}


BW::string MatrixProviderConstraintSource::desc() const
{
	BW_GUARD;

	return BW::string("MatrixProviderConstraintSource type=") +
		provider_->typeName();
}


MatrixProviderConstraintSource * MatrixProviderConstraintSource::New(
	MatrixProviderPtr & provider )
{
	BW_GUARD;

	return new MatrixProviderConstraintSource( provider );
}



// -----------------------------------------------------------------------------
// Section: ConstraintTarget
// -----------------------------------------------------------------------------
// 
PY_TYPEOBJECT( ConstraintTarget)

PY_BEGIN_METHODS( ConstraintTarget )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ConstraintTarget )
	/*~	attribute ConstraintTarget.type
	 *	The type of the target.
	 *	@type int for type
	 */
	PY_ATTRIBUTE ( type )
	/*~	attribute ConstraintTarget.identifier
	 *	The identifier of the target.
	 *	@type string
	 */
	PY_ATTRIBUTE ( identifier )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ConstraintTarget )

PY_FACTORY( ConstraintTarget, BigWorld )


/**
 *	@param type The type of object targeted by this constraint.
 *	@param identifier The id of the object targeted by this constraint.
 *	@pre True.
 *	@post Created the target description.
 */
ConstraintTarget::ConstraintTarget(
	ConstraintTarget::Type type, const BW::string & identifier )
	:
	PyObjectPlus( &s_type_ )
	, type_( type )
	, identifier_( identifier )
{
}


bool ConstraintTarget::operator == ( const ConstraintTarget & rhs ) const
{
	return (type_ == rhs.type_) && (identifier_ == rhs.identifier_);
}


/**
 *	@return The matrix sourced from this target, or identity on error.
 */
const Matrix & ConstraintTarget::getMatrix(
	const SuperModel & model, const IKConstraintSystem & system ) const
{
	switch( getType() )
	{
	case ConstraintTarget::MODEL_NODE:
		return getMatrixSafe( model, getIdentifier() );
		
	case ConstraintTarget::IK_HANDLE_TARGET:
	case ConstraintTarget::IK_HANDLE_POLE_VECTOR:
		//IKHandle transform would never matter.
		return Matrix::identity;

	default:
		WARNING_MSG( "Unsupported constraint target source used. \n" );
		return Matrix::identity;
	}
}


/**
 *	@return Type of target for the constraint.
 */
ConstraintTarget::Type ConstraintTarget::getType() const
{
	return type_;
}


/**
 *	@return Identifier of the target for the constraint.
 */
const BW::string & ConstraintTarget::getIdentifier() const
{
	return identifier_;
}


/**
 *	This function is only intended to be used for debugging purposes.
 *	@return A debug string describing the object.
 */
BW::string ConstraintTarget::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "ConstraintTarget: identifier='" << identifier_ <<
		"', type=" << type_;
	return resultStr.str();
}


ConstraintTarget * ConstraintTarget::New(
	const BW::string & strType, const BW::string & identifier )
{
	BW_GUARD;

	ConstraintTarget::Type type = ConstraintTarget::MODEL_NODE;
	if (strType == "model_node")
	{
		type = ConstraintTarget::MODEL_NODE;
	}
	else if (strType == "ik_handle_target")
	{
		type = ConstraintTarget::IK_HANDLE_TARGET;
	}
	else if (strType == "ik_handle_pole_vector")
	{
		type = ConstraintTarget::IK_HANDLE_POLE_VECTOR;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError,
			"ConstraintTarget expects a type of 'model_node', "
			"'ik_handle_target', or 'ik_handle_pole_vector'. " );
		return NULL;
	}

	return new ConstraintTarget( type, identifier );
}




// -----------------------------------------------------------------------------
// Section: Constraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( Constraint)

PY_BEGIN_METHODS( Constraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Constraint )
	/*~	attribute Constraint.weight
	 *	The amount of contribution from this constraint where multiple
	 *	constrainers are applying to the same node.
	 *	@type float
	 */
	PY_ATTRIBUTE ( weight )
	/*~	attribute Constraint.identifier
	 *	The name used to identify this constraint.
	 *	@type string
	 */
	PY_ATTRIBUTE ( identifier )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( Constraint )


/**
 *	Creates a constraint sourcing data from @a source, applying it to
 *	@a target.
 */
Constraint::Constraint(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target,
	PyTypeObject *pType )
	:
	PyObjectPlus( pType )
	, identifier_( identifier )
	, source_( source )
	, target_( target )
	, weight_( 1.0f )
{
}


const BW::string & Constraint::getIdentifier() const
{
	return identifier_;
}


ConstraintSourcePtr Constraint::getSource() const
{
	return source_;
}


ConstraintTargetPtr Constraint::getTarget() const
{
	return target_;
}


float Constraint::getWeight() const
{
	return weight_;
}


void Constraint::setWeight( float weight )
{
	weight_ = weight;
}


/**
 *	@pre True.
 *	@post Returned the current matrix for this constraint (must be
 *		weighted with others using getWeight()). If there are problems
 *		sourcing the matrix, identity will be returned.
 */
Matrix Constraint::getMatrix( const SuperModel & model,
	const IKConstraintSystem & system ) const
{
	const Matrix & targetMat = target_->getMatrix( model, system);
	const Matrix & sourceMat = source_->getMatrix( model, system );
	const Matrix result = constrain( targetMat, sourceMat );
	//DEBUG_MSG( "IMCS T %s.\n", targetMat.desc().c_str() );
	//DEBUG_MSG( "IMCS S %s.\n", sourceMat.desc().c_str() );
	//DEBUG_MSG( "IMCS R %s.\n", result.desc().c_str() );
	return result;
}


/**
 *	This function is only intended to be used for debugging purposes.
 *	@return Details for the members of the base constraint that's
 *		intended to be used in constraint desc implementations.
 */
BW::string Constraint::baseConstraintMemberDesc() const
{
	BW::ostringstream resultStr;
	resultStr << "identifier='" << identifier_ << "', "
		"source={" << source_->desc() << "}, "
		"target={" << target_->desc() << "}, "
		"weight=" << weight_;
	return resultStr.str();
}



// -----------------------------------------------------------------------------
// Section: ConstraintConstraintSource
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ConstraintConstraintSource)

PY_BEGIN_METHODS( ConstraintConstraintSource )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ConstraintConstraintSource )
PY_END_ATTRIBUTES()

PY_FACTORY( ConstraintConstraintSource, BigWorld )


/**
 *	@param constraint The constraint providing the source matrix for
 *		this constraint.
 */
ConstraintConstraintSource::ConstraintConstraintSource(
	ConstraintPtr & constraint )
	:
	ConstraintSource( &s_type_ )
	, constraint_( constraint )
{
}


ConstraintConstraintSource::Dependency
	ConstraintConstraintSource::getDependency() const
{
	BW_GUARD;

	return Dependency(
		ConstraintSource::CONSTRAINT, constraint_->getIdentifier() );
}


const Matrix & ConstraintConstraintSource::getMatrix(
	const SuperModel & model, const IKConstraintSystem & system )
{
	BW_GUARD;

	lastMatrix_ = constraint_->getMatrix( model, system );
	return lastMatrix_;
}


BW::string ConstraintConstraintSource::desc() const
{
	BW_GUARD;

	return BW::string("ConstraintConstraintSource constraint={") +
		constraint_->desc() + "}";
}


ConstraintConstraintSource * ConstraintConstraintSource::New(
	ConstraintPtr & constraint )
{
	BW_GUARD;

	return new ConstraintConstraintSource( constraint );
}



// -----------------------------------------------------------------------------
// Section: PointConstraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PointConstraint)

PY_BEGIN_METHODS( PointConstraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PointConstraint )
	/*~	attribute PointConstraint.minTranslation
	 *	Minimum translation limit. The offset is applied after this.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( minTranslation )
	/*~	attribute PointConstraint.maxTranslation
	 *	Maximum translation limit. The offset is applied after this.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( maxTranslation )
	/*~	attribute PointConstraint.offset
	 *	This offset is maintained when the point constraint is applied.
	 *	Set this to the initial offset between the target and the source
	 *	if it should just copy relative translation.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( offset )
PY_END_ATTRIBUTES()

PY_FACTORY( PointConstraint, BigWorld )


PointConstraint::PointConstraint(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
	:
	Constraint( identifier, source, target, &s_type_ )
	,  minTranslation_(
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max() )
	,  maxTranslation_(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max() )
	, offset_( Vector3::ZERO )
{
}


PointConstraint * PointConstraint::New(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
{
	BW_GUARD;
	
	return new PointConstraint( identifier, source, target );
}


/**
 *	@return Minimum translation limit.
 */
const Vector3 & PointConstraint::getMinTranslation() const
{
	return minTranslation_;
}


/**
 *	@return Maximum translation limit.
 */
const Vector3 & PointConstraint::getMaxTranslation() const
{
	return maxTranslation_;
}


/**
 *	@return Position offset for this constraint.
 */
const Vector3 & PointConstraint::getOffset() const
{
	return offset_;
}


/**
 *	@pre True.
*	@post Set the position offset to @a offset.
 */
void PointConstraint::setOffset( const Vector3 & offset )
{
	offset_ = offset;
}


Constraint::Type PointConstraint::getType() const
{
	return Constraint::POSITION;
}


BW::string PointConstraint::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "PointConstraint: " <<
		baseConstraintMemberDesc() <<
		", offset=" << offset_ << ", "
		"minTranslation=" << getVector3DebugStr( minTranslation_ ) << ", "
		"maxTranslation=" << getVector3DebugStr( maxTranslation_ );
	return resultStr.str();
}


Matrix PointConstraint::constrain(
	const Matrix & unconstrained, const Matrix & sourceMatrix ) const
{
	Matrix result( unconstrained );

	//Apply relative offset after the unconstrained transform.
	Vector3 translation = sourceMatrix.applyToOrigin() -
		unconstrained.applyToOrigin();
	translation.clamp( minTranslation_, maxTranslation_ );
	translation += getOffset();
	result.postTranslateBy( translation );
	
	return result;
}



// -----------------------------------------------------------------------------
// Section: RotationConstraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( RotationConstraint)

PY_BEGIN_METHODS( RotationConstraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( RotationConstraint )
	/*~	attribute RotationConstraint.minRotations
	 *	Minimum rotation limits (radians). This is applied to the source
	 *	rotation (which will then have any offset applied). Limits below
	 *	-Pi mean no limit.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( minRotations )
	/*~	attribute RotationConstraint.maxRotations
	 *	Maximum rotation limits (radians). This is applied to the source
	 *	rotation (which will then have any offset applied). Limits above
	 *	Pi mean no limit.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( maxRotations )
	/*~	attribute RotationConstraint.offset
	 *	This offset gives (x, y, z) rotations maintained when the rotation
	 *	constraint is applied. They are applied in the order x, y, z.
	 *	Set this to the initial rotation offset between the target and the
	 *	source if it should apply relative rotation.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( offset )
PY_END_ATTRIBUTES()


RotationConstraint::RotationConstraint(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target,
	PyTypeObject *pType )
	:
	Constraint( identifier, source, target, pType )
	,  minRotations_(
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max(),
		-std::numeric_limits<float>::max() )
	,  maxRotations_(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max() )
	, offset_( Vector3::ZERO )
{
}


/**
 *	@return Minimum rotation limits (radians).
 */
const Vector3 & RotationConstraint::getMinRotations() const
{
	return minRotations_;
}


/**
 *	@return Maximum rotation limits (radians).
 */
const Vector3 & RotationConstraint::getMaxRotations() const
{
	return maxRotations_;
}


/**
 *	@return Rotation offsets for each axis for this constraint (radians).
 */
const Vector3 & RotationConstraint::getOffset() const
{
	return offset_;
}


/**
 *	@pre True.
*	@post Set the rotation offset (for each axis, in radians) to @a offset.
 */
void RotationConstraint::setOffset( const Vector3 & offset )
{
	offset_ = offset;
}


Constraint::Type RotationConstraint::getType() const
{
	return Constraint::ROTATION;
}


/**
 *	This function is only intended to be used for debugging purposes.
 *	@return Details for the members of the base RotationConstraint that's
 *		intended to be used in rotation constraint desc implementations.
 */
BW::string RotationConstraint::baseRotationConstraintMemberDesc() const
{
	BW::ostringstream resultStr;
	resultStr << baseConstraintMemberDesc() << ", "
		"offset=" << offset_ << ", "
		"minRotation=" << getVector3DebugStr( minRotations_ ) << ", "
		"maxRotation=" << getVector3DebugStr( maxRotations_ );
	return resultStr.str();
}


/**
 *	@param input The source rotations for the constraint, without offsets
 *		applied.
 *	@return @a input with rotations limited according to
 *		the min and max rotation settings.
 */
bool RotationConstraint::limitRotations( Matrix & input ) const
{
	return false;
}




// -----------------------------------------------------------------------------
// Section: OrientConstraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( OrientConstraint)

PY_BEGIN_METHODS( OrientConstraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( OrientConstraint )
PY_END_ATTRIBUTES()

PY_FACTORY( OrientConstraint, BigWorld )


OrientConstraint::OrientConstraint(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
	:
	RotationConstraint( identifier, source, target, &s_type_ )
{
}


OrientConstraint * OrientConstraint::New(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
{
	BW_GUARD;
	
	return new OrientConstraint( identifier, source, target );
}


BW::string OrientConstraint::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "OrientConstraint: " <<
		baseRotationConstraintMemberDesc();
	return resultStr.str();
}


namespace
{
	/**
	 *	@param v1, v2, v3 The vectors to orthogonalise.
	 *	@param e1, e2, e3 Output orthogonalised vectors (corresponding to
	 *		the input arguments).
	 *	@pre @a v1, @a v2, and @a v3 are linearly independent and non-zero.
	 *	@post Assigned orthogonalised versions of the inputs to @a e1,
	 *		@a e2, and @a e3.
	 */
	void orthogonaliseOnly(
		const Vector3 & v1, const Vector3 & v2, const Vector3 & v3,
		Vector3 & e1, Vector3 & e2, Vector3 & e3 )
	{
		BW_GUARD;

		//Apply Gramm-Schmidt orthogonalisation.
		e1 = v1; 

		Vector3 p1; 
		p1.projectOnto( v2, v1 );
		e2 = v2 - p1;
	
		Vector3 p2; 
		p2.projectOnto( v3, v1 );
		Vector3 p3; 
		p3.projectOnto( v3, e2 );
		e3 = v3 - p2 - p3;

		//MF_ASSERT( almostZero( e1.dotProduct( e2 ) ) );
		//MF_ASSERT( almostZero( e1.dotProduct( e3 ) ) );
		//MF_ASSERT( almostZero( e2.dotProduct( e3 ) ) );
	}


	/**
	 *	@pre @a mat contains only rotation, scale, and translation
	 *		components.
	 *	@post Returned the rotation component of @a mat (if it contains only
	 *		translation, scale, and rotation).
	 */
	Matrix getRotation( const Matrix & mat )
	{
		Matrix result( Matrix::identity );

		//Matrix[] is described as returning the row of the matrix as a
		//Vector3, but it seems conceptually here to be the column.
		orthogonaliseOnly(
			mat[0], mat[1], mat[2], result[0], result[1], result[2] );
		result[0].normalise();
		result[1].normalise();
		result[2].normalise();

		return result;
	}


	/**
	 *	@pre @a mat contains only rotation, scale, and translation
	 *		components.
	 *	@post Returned the scale component of @a mat (if it contains only
	 *		translation, scale, and rotation).
	 */
	Vector3 getScale( const Matrix & mat )
	{
		Vector3 c0;
		Vector3 c1;
		Vector3 c2;

		orthogonaliseOnly(
			mat[0], mat[1], mat[2], c0, c1, c2 );

		return Vector3( c0.length(), c1.length(), c2.length() );
	}


	/**
	 *	@pre @a mat contains only rotation, scale, and translation
	 *		components.
	 *	@post Returned the scale component of @a mat (if it contains only
	 *		translation, scale, and rotation).
	 */
	void getRotationAndScale( const Matrix & mat,
		Matrix & rotation, Vector3 & scale )
	{
		rotation = Matrix::identity;

		orthogonaliseOnly(
			mat[0], mat[1], mat[2], rotation[0], rotation[1], rotation[2] );

		for (int i = 0; i < 3; ++i)
		{
			scale[i] = rotation[i].length();

			//Vector3 length to be considered zero vector.
			const float VEC_LEN_EPSILON = 0.00000001f;

			if (!almostZero( scale[i], VEC_LEN_EPSILON ) )
			{
				rotation[i] *= (1.0f / scale[i]);
			}
		}

#if 0
		//Used for testing only.
		DEBUG_MSG( "MatInput=%s\n", mat.desc().c_str() );
		DEBUG_MSG( "rotation=%s\n", rotation.desc().c_str() );
		DEBUG_MSG( "scale=%s\n", scale.desc().c_str() );
		DEBUG_MSG( "translation=%s\n", translation.desc().c_str() );

		//Mul should be the same as the input matrix.
		Matrix mul;
		Matrix scaleMat;
		scaleMat.setScale( scale );
		mul.multiply( rotation, scaleMat );
		DEBUG_MSG( "rotation*scale=%s\n", mul.desc().c_str() );
		
		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < 3; ++j)
			{
				MF_ASSERT( abs(mat(i, j) - mul(i, j)) <= 0.001f );
			}
		}
#endif
	}
	///Same as the other getRotationAndScale, with the rotation a Quaternion.
	void getRotationAndScale( const Matrix & mat,
		Quaternion & rotation, Vector3 & scale )
	{
		Matrix rotationMatrix;
		getRotationAndScale( mat, rotationMatrix, scale );
		rotation.fromMatrix( rotationMatrix );
	}


	/**
	 *	@param mat The matrix to decompose.
	 *	@param rotation/scale/translation Will be assigned the corresponding
	 *		component from @a mat.
	 *	@pre @a mat contains only rotation, scale, and translation
	 *		components.
	 *	@post Decomposed @a mat and stored the corresponding components into
	 *		@a rotation, @a scale, and @a translation.
	 */
	void decomposeMatrix( const Matrix & mat,
		Matrix & rotation, Vector3 & scale, Vector3 & translation )
	{
		getRotationAndScale( mat, rotation, scale );
		translation = mat[3];
	}
	///Same as the other decomposeMatrix, but the rotation is a Quaternion.
	void decomposeMatrix( const Matrix & mat,
		Quaternion & rotation, Vector3 & scale, Vector3 & translation )
	{
		getRotationAndScale( mat, rotation, scale );
		translation = mat[3];
	}
}


Matrix OrientConstraint::constrain(
	const Matrix & unconstrained, const Matrix & sourceMatrix ) const
{
	//We must remove any scale component from the matrix.
	//It's difficult to know if this will give the same result as Maya
	//in all circumstances (e.g., combinations of negative scales could
	//just as easily be rotation), but it works for data seen with uniform
	//positive scales.
	Matrix result( getRotation( sourceMatrix ) );

	limitRotations( result );
	
	result.preRotateZ( getOffset()[2] );
	result.preRotateY( getOffset()[1] );
	result.preRotateX( getOffset()[0] );

	//Preserve the scale component of the input; we only want to replace the
	//rotation component.
	Matrix scaleMat;
	scaleMat.setScale( getScale( unconstrained ) );
	result.preMultiply( scaleMat );

	//Equivalent to post-multiplying by a matrix applying just the translation
	//from unconstrained (i.e., apply this translation after doing the
	//rotation/scale).
	result[3] = unconstrained[3];

	return result;
}



// -----------------------------------------------------------------------------
// Section: AimConstraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( AimConstraint)

PY_BEGIN_METHODS( AimConstraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( AimConstraint )
	/*~	attribute AimConstraint.aimVector
	 *	This vector (defined in model space) is aimed at the target.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( aimVector )
	/*~	attribute AimConstraint.upVector
	 *	The up vector in model space.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( upVector )
	/*~	attribute AimConstraint.worldUpVector
	 *	The up vector will be aligned as closely as possible with this vector
	 *	in world space.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( worldUpVector )
PY_END_ATTRIBUTES()

PY_FACTORY( AimConstraint, BigWorld )


AimConstraint::AimConstraint(
	const BW::string & identifier,
	ConstraintSourcePtr source,
	ConstraintTargetPtr target )
	:
	RotationConstraint( identifier, source, target, &s_type_ )
	, aimVector_( Vector3::I )
	, upVector_( Vector3::J )
	, worldUpVector_( Vector3::J )
{
}


AimConstraint * AimConstraint::New(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
{
	BW_GUARD;
	
	return new AimConstraint( identifier, source, target );
}


BW::string AimConstraint::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "AimConstraint: " <<
		baseRotationConstraintMemberDesc() << ", "
		"aimVector=" << aimVector_ << ", "
		"upVector=" << upVector_ << ", "
		"worldUpVector=" << worldUpVector_;
	return resultStr.str();
}


const Vector3 & AimConstraint::getAimVector() const
{
	return aimVector_;
}


void AimConstraint::setAimVector( const Vector3 & aimVector )
{
	aimVector_ = aimVector;
}


const Vector3 & AimConstraint::getUpVector() const
{
	return upVector_;
}


void AimConstraint::setUpVector( const Vector3 & upVector )
{
	upVector_ = upVector;
}


const Vector3 & AimConstraint::getWorldUpVector() const
{
	return worldUpVector_;
}


void AimConstraint::setWorldUpVector( const Vector3 & worldUpVector )
{
	worldUpVector_ = worldUpVector;
}


Matrix AimConstraint::constrain(
	const Matrix & unconstrained, const Matrix & sourceMatrix ) const
{
	Matrix uncRotation;
	Vector3 uncScale;
	Vector3 uncTranslation;
	decomposeMatrix( unconstrained, uncRotation, uncScale, uncTranslation );

	const Vector3 nodePos = uncTranslation;
	const Vector3 nodeToTarget(
		sourceMatrix.applyToOrigin() - nodePos );

	//Rotate aimVector_ to be aligned with vectorToTarget.
	const Vector3 inputAim = uncRotation.applyVector( aimVector_ );
	const Vector3 rotAxis = inputAim.crossProduct( nodeToTarget );
	const float rotAngle = inputAim.getAngle( nodeToTarget );

	//Prepare the result up to the aim vector rotation.
	Quaternion rot;
	rot.fromAngleAxis( rotAngle, rotAxis );
	Matrix rotMat;
	rotMat.setRotate( rot );

	Matrix result = uncRotation;
	result[3] = uncTranslation;
	result.preRotateZ( getOffset()[2] );
	result.preRotateY( getOffset()[1] );
	result.preRotateX( getOffset()[0] );

	result.postTranslateBy( -nodePos );
	result.postMultiply( rotMat );

	//Now need to rotate so that upVector_ points in the same position as
	//worldUpVector_ if they're projected onto the plane with normal
	//defined by the aim vector.
	const Matrix rotMat2 = rotateUpVectAboutAimAxis(
		result.applyVector( upVector_ ), worldUpVector_, nodeToTarget );
	result.postMultiply( rotMat2 );

	//Potentially apply rotation limits.
	rotMat.postMultiply( rotMat2 );
	if (limitRotations( rotMat ))
	{
		//Redo with limited rotation.
		result = uncRotation;
		result[3] = uncTranslation;
		result.preRotateZ( getOffset()[2] );
		result.preRotateY( getOffset()[1] );
		result.preRotateX( getOffset()[0] );
		result.postTranslateBy( -nodePos );
		result.postMultiply( rotMat );
	}
	
	result.postTranslateBy( nodePos );
	
	Matrix uncScaleMat;
	uncScaleMat.setScale( uncScale );
	result.preMultiply( uncScaleMat );

	return result;
}



// -----------------------------------------------------------------------------
// Section: ParentConstraint
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ParentConstraint)

PY_BEGIN_METHODS( ParentConstraint )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ParentConstraint )
	/*~	attribute ParentConstraint.offset
	 *	This offset is maintained when the constraint is applied.
	 *	Set this to the initial offset between the target and the source
	 *	if it should just copy relative translation.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( translationOffset )
	/*~	attribute ParentConstraint.offset
	 *	This rotation offset (in radians; x, y, then z) is maintained when
	 *	the constraint is applied. Set this to the initial offset between the
	 *	target and the source f it should just apply relative rotation.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( rotationOffset )
PY_END_ATTRIBUTES()

PY_FACTORY( ParentConstraint, BigWorld )


ParentConstraint::ParentConstraint(
	const BW::string & identifier,
	ConstraintSourcePtr source,
	ConstraintTargetPtr target )
	:
	Constraint( identifier, source, target, &s_type_ )
	,  translationOffset_( Vector3::ZERO )
	,  rotationOffset_( Vector3::ZERO )
{
}


ParentConstraint * ParentConstraint::New(
	const BW::string & identifier,
	ConstraintSourcePtr & source,
	ConstraintTargetPtr & target )
{
	BW_GUARD;
	
	return new ParentConstraint( identifier, source, target );
}


/**
 *	@return Position offset for this constraint.
 */
const Vector3 & ParentConstraint::getTranslationOffset() const
{
	return translationOffset_;
}


/**
 *	@pre True.
*	@post Set the translation offset to @a offset.
 */
void ParentConstraint::setTranslationOffset( const Vector3 & offset )
{
	translationOffset_ = offset;
}


/**
 *	@return Rotation offset for this constraint (radians).
 */
const Vector3 & ParentConstraint::getRotationOffset() const
{
	return rotationOffset_;
}


/**
 *	@pre True.
*	@post Set the rotation offset (for each axis, in radians) to @a offset.
 */
void ParentConstraint::setRotationOffset( const Vector3 & offset )
{
	rotationOffset_ = offset;
}


Constraint::Type ParentConstraint::getType() const
{
	return Constraint::PARENT;
}


BW::string ParentConstraint::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "ParentConstraint: " <<
		baseConstraintMemberDesc() << ", "
		"translationOffset=" << translationOffset_ << ", "
		"rotationOffset=" << rotationOffset_;
	return resultStr.str();
}


Matrix ParentConstraint::constrain(
	const Matrix & unconstrained, const Matrix & sourceMatrix ) const
{
	BW_GUARD;

	Matrix result;
	//Parent relationship needs to preserve the scale of the target.
	result.setScale( getScale( unconstrained) );

	result.postRotateX( getRotationOffset()[0] );
	result.postRotateY( getRotationOffset()[1] );
	result.postRotateZ( getRotationOffset()[2] );

	Matrix srcRot;
	Vector3 srcScale;
	Vector3 srcTransl;
	decomposeMatrix( sourceMatrix, srcRot, srcScale, srcTransl );
	
	//Scale of the source is applied to the translation offset.
	result[3] = getTranslationOffset() * srcScale;

	//Only transform translation and rotation from the source, not the scale.
	Matrix sNoScale = srcRot;
	sNoScale[3] = srcTransl;
	result.postMultiply( sNoScale );

	return result;
}



// -----------------------------------------------------------------------------
// Section: ConstraintGroup
// -----------------------------------------------------------------------------


/**
 *	@pre All constraints in @a constraints have the same target.
 *	@post Created a group of constraints from @a constraints. The input
 *		vector @a constraints is now empty.
 */
ConstraintGroup::ConstraintGroup( BW::vector<ConstraintPtr> & constraints )
{
	if (constraints.empty())
	{
		return;
	}
	constraints_.swap( constraints );
	
	//Set to true if we are using any PARENT constraints.
	bool usingParentConstraint = false;

	std::size_t numRot = 0;
	std::size_t numPos = 0;

	ConstConstraintTargetPtr target = getTarget();
	for( BW::vector<ConstraintPtr>::const_iterator itr = constraints_.begin();
		itr != constraints_.end();
		++itr )
	{
		MF_ASSERT( (* target) == (*(*itr)->getTarget()) );

		const bool isParentConstraint =
			((*itr)->getType() == Constraint::PARENT);
		if (itr != constraints_.begin())
		{
			if (usingParentConstraint != isParentConstraint)
			{
				WARNING_MSG( "Cannot have the same target constrained by "
					"a parent constraint as well as other types of "
					"constraint. " );
				constraints_.clear();
				return;
			}
		}
		else
		{
			usingParentConstraint = isParentConstraint;
		}

		switch( (*itr)->getType() )
		{
		case Constraint::ROTATION:
			MF_ASSERT( !usingParentConstraint );
			++numRot;
			break;

		case Constraint::POSITION:
			MF_ASSERT( !usingParentConstraint );
			++numPos;
			break;

		case Constraint::PARENT:
			MF_ASSERT( usingParentConstraint );
			++numRot;
			++numPos;
			break;

		default:
			MF_ASSERT( false && "Unhandled constraint type. " );
		}
	}

	outputRot_.resize( numRot );
	outputScale_.resize( numRot );
	outputPos_.resize( numPos );
	
	MF_ASSERT( usingParentConstraint
		||
		((numRot + numPos) == constraints_.size()) );
	MF_ASSERT( !usingParentConstraint
		||
		((numRot == numPos) && (numRot == constraints_.size())) );
}


ConstConstraintTargetPtr ConstraintGroup::getTarget() const
{
	if (!constraints_.empty())
	{
		return constraints_.front()->getTarget();
	}
	return ConstraintTargetPtr();
}


/**
 *	@pre True.
 *	@post On true return: Applied the constraints from this group to the
 *			target.
 *		On false: no changes applied. Could be zero weights or invalid
 *			constraint grouping.
 */
bool ConstraintGroup::apply(
	SuperModel & model, IKConstraintSystem & system )
{
	MF_ASSERT(
		//Applying position and/or rotations.
		(constraints_.size() == (outputRot_.size() + outputPos_.size()))
		||
		//Applying parent constraints.
		((constraints_.size() == outputRot_.size())
			&& (constraints_.size() == outputPos_.size())) );
	MF_ASSERT( outputScale_.size() == outputRot_.size() );
	MF_ASSERT( model.getInstanceNodeTree() != NULL );

	if (constraints_.empty())
	{
		return false;
	}
	
	std::size_t rotIndex = 0;
	float rotWeightSum = 0.0f;
	std::size_t posIndex = 0;
	float posWeightSum = 0.0f;

	//For every constraint, get the currently applying matrix.
	for( BW::vector<ConstraintPtr>::size_type i = 0;
		i < constraints_.size();
		++i )
	{
		switch( constraints_[i]->getType() )
		{
		case Constraint::ROTATION:
			{
				MF_ASSERT( rotIndex < outputRot_.size() );
				outputRot_[rotIndex].first = outputScale_[rotIndex].first =
					std::max( 0.0f, constraints_[i]->getWeight() );
				if (outputRot_[rotIndex].first > 0.0f)
				{
					rotWeightSum += outputRot_[rotIndex].first;

					getRotationAndScale(
						constraints_[i]->getMatrix( model, system ),
						outputRot_[rotIndex].second,
						outputScale_[rotIndex].second );
				}
				++rotIndex;
			}
			break;

		case Constraint::POSITION:
			{
				MF_ASSERT( posIndex < outputPos_.size() );
				outputPos_[posIndex].first =
					std::max( 0.0f, constraints_[i]->getWeight() );
				if (outputPos_[posIndex].first > 0.0f)
				{
					posWeightSum += outputPos_[posIndex].first;
					outputPos_[posIndex].second = constraints_[i]->getMatrix(
						model, system ).applyToOrigin();
				}
				++posIndex;
			}
			break;

		case Constraint::PARENT:
			{
				MF_ASSERT( rotIndex == posIndex );
				outputRot_[rotIndex].first =
					outputScale_[rotIndex].first =
					outputPos_[posIndex].first =
						std::max( 0.0f, constraints_[i]->getWeight() );
				if (outputPos_[posIndex].first > 0.0f)
				{
					rotWeightSum += outputRot_[rotIndex].first;
					posWeightSum += outputPos_[posIndex].first;

					const Matrix constraintMat =
						constraints_[i]->getMatrix( model, system );

					decomposeMatrix(
						constraintMat,
						outputRot_[rotIndex].second,
						outputScale_[rotIndex].second,
						outputPos_[posIndex].second );
				}
				++rotIndex;
				++posIndex;
			}
			break;

		default:
			MF_ASSERT( false && "Unhandled constraint type. " );
		}
	}
	if ((rotWeightSum <= 0.0f) && (posWeightSum <= 0.0f))
	{
		return false;
	}

	//Get the matrix to apply.
	const ConstConstraintTargetPtr target = getTarget();

	Quaternion rotation;
	Vector3 scale;
	if ((rotIndex > 0) && (rotWeightSum > 0.0f))
	{
		interpolate( rotation, outputRot_ );
		interpolate( scale, outputScale_ );
	}
	else
	{
		getRotationAndScale(
			target->getMatrix( model, system ),
			rotation, scale );
	}

	Vector3 position;
	if ((posIndex > 0) && (posWeightSum > 0.0f))
	{
		interpolate( position, outputPos_ );
	}
	else
	{
		position = target->getMatrix( model, system ).applyToOrigin();
	}

	Matrix toApply;
	toApply.setRotate( rotation );
	Matrix scaleMat;
	scaleMat.setScale( scale );
	toApply.preMultiply( scaleMat );
	toApply[3] = position;


	//Apply the output matrix.
	switch( target->getType() )
	{
	case ConstraintTarget::MODEL_NODE:
		applyWorldTransformToModelNode(
			model, target->getIdentifier(), toApply );
		break;
		
	case ConstraintTarget::IK_HANDLE_TARGET:
	case ConstraintTarget::IK_HANDLE_POLE_VECTOR:
		{
			IKHandlePtr handle = system.getIKHandle( target->getIdentifier() );

			if( handle.hasObject() )
			{
				//Need to take points to model space.
				Matrix worldTransformInv = system.getWorldTransform();
				worldTransformInv.invert();

				if ( target->getType() == ConstraintTarget::IK_HANDLE_TARGET )
				{
					handle->setTarget( worldTransformInv.applyPoint(
						toApply.applyToOrigin() ) );
				}
				else
				{
					MF_ASSERT( target->getType() ==
						ConstraintTarget::IK_HANDLE_POLE_VECTOR );
					handle->setPoleVector(
						worldTransformInv.applyPoint( toApply.applyToOrigin() ),
						true );
				}
			}
			else
			{
				WARNING_MSG( "ConstraintGroup IKHandle target '%s' cannot be "
					"found.\n",
					target->getIdentifier().c_str() );
			}
		}
		break;

	default:
		MF_ASSERT( false && "Unsupported constraint target type. " );
		break;
	}

	return true;
}


/**
 *	@return The collection of constraints in this group.
 */
const BW::vector<ConstraintPtr> & ConstraintGroup::getConstraints() const
{
	return constraints_;
}



// -----------------------------------------------------------------------------
// Section: IKHandle
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( IKHandle)

PY_BEGIN_METHODS( IKHandle )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( IKHandle )
	/*~	attribute IKHandle.identifier
	 *	This name identifying this handle.
	 *	@type string
	 */
	PY_ATTRIBUTE ( identifier )
	/*~	attribute IKHandle.target
	 *	The target position (in model space) for the IK handle.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( target )
	/*~	attribute IKHandle.drawTarget
	 *	Set to true to visualise the target position.
	 *	@type bool
	 */
	PY_ATTRIBUTE ( drawTarget )
	/*~	attribute IKHandle.poleVector
	 *	The solver forces all joints in the chain
	 *	to lie on a plane. The plane contains the vector from the start joint
	 *	to the target (termed "handle vector" in Maya), and this pole vector.
	 *	This vector is in model space. It is ignored if it's a zero vector
	 *	and poleVectorIsFromModelPoint is false.
	 *	@type Vector3
	 */
	PY_ATTRIBUTE ( poleVector )
	/*~	attribute IKHandle.poleVectorIsFromModelPoint
	 *	If this is true, the pole vector value used is currently getting
	 *	interpreted as a model-space point (not directly as a vector). The
	 *	pole vector used for the IK will go from the start joint position to
	 *	this point.
	 *	@type bool
	 */
	PY_ATTRIBUTE ( poleVectorIsFromModelPoint )
	/*~	attribute IKHandle.drawPoleVector
	 *	Set to true to visualise the pole vector.
	 *	@type bool
	 */
	PY_ATTRIBUTE ( drawPoleVector )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( IKHandle )


IKHandle::IKHandle(
	const BW::string & identifier, PyTypeObject * pType )
	:
	PyObjectPlus( pType )
	, identifier_( identifier )
	, target_( Vector3::ZERO )
	, drawTarget_( false )
	, poleVector_( Vector3::ZERO )
	, poleVectorIsFromModelPoint_( false )
	, drawPoleVector_( false )
{
}


/**
 *	@return Name identifying this solver.
 */
const BW::string & IKHandle::getIdentifier() const
{
	return identifier_;
}


/**
 *	@return Target point in model space for the IK solver.
 */
const Vector3 & IKHandle::getTarget() const
{
	return target_;
}


/**
 *	@pre True.
 *	@post Set the target point for the IK solver.
 */
void IKHandle::setTarget( const Vector3 & target )
{
	target_ = target;
}


/**
 *	@return True if the target position will be drawn.
 */
bool IKHandle::getDrawTarget() const
{
	return drawTarget_;
}


/**
 *	@pre True.
 *	@post Set whether debug drawing of the target position is enabled.
 */
void IKHandle::setDrawTarget( bool enable )
{
	drawTarget_ = enable;
}


/**
 *	Sets the pole vector and conversion flag.
 *	@see poleVector attribute.
 *	@see poleVectorIsFromModelPoint attribute.
 */
void IKHandle::setPoleVector(
	const Vector3 & poleVector, bool poleVectorIsFromModelPoint )
{
	poleVector_ = poleVector;
	poleVectorIsFromModelPoint_ = poleVectorIsFromModelPoint;
}


/**
 *	@return Pole vector in model space for the IK solver.
 */
const Vector3 & IKHandle::getPoleVector() const
{
	return poleVector_;
}


/**
 *	@return True if the pole vector stores a model-space point, and
 *		the vector used is actually from the start joint to this point.
 */
bool IKHandle::getPoleVectorIsFromModelPoint() const
{
	return poleVectorIsFromModelPoint_;
}


/**
 *	@return True if the pole vector will be drawn.
 */
bool IKHandle::getDrawPoleVector() const
{
	return drawPoleVector_;
}


/**
 *	@pre True.
 *	@post Set whether debug drawing of the pole vector is enabled.
 */
void IKHandle::setDrawPoleVector( bool enable )
{
	drawPoleVector_ = enable;
}


/**
 *	This function is only intended to be used for debugging purposes.
 *	@return Details for the members of the base RotationConstraint that's
 *		intended to be used in rotation constraint desc implementations.
 */
BW::string IKHandle::baseIKHandleMemberDesc() const
{
	BW::ostringstream resultStr;
	resultStr <<
		"identifier=" << identifier_ << ", "
		"target=" << target_ << ", "
		"poleVector=" << poleVector_;
	return resultStr.str();
}




// -----------------------------------------------------------------------------
// Section: CCDChainIKHandle
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( CCDChainIKHandle)

PY_BEGIN_METHODS( CCDChainIKHandle )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( CCDChainIKHandle )
	/*~	attribute CCDChainIKHandle.startModelNodeIdentifier
	 *	The identifier of the start model node in the chain.
	 *	@type string
	 */
	PY_ATTRIBUTE ( startModelNodeIdentifier )
	/*~	attribute CCDChainIKHandle.endModelNodeIdentifier
	 *	The identifier of the end model node in the chain (chases target).
	 *	@type string
	 */
	PY_ATTRIBUTE ( endModelNodeIdentifier )
	/*~	attribute CCDChainIKHandle.maxNumIterations
	 *	The maximum number of iterations performed by the solver.
	 *	@type uint32
	 */
	PY_ATTRIBUTE ( maxNumIterations )
	/*~	attribute CCDChainIKHandle.targetDistTolerance
	 *	The solver will exit when the end node is within this distance of the
	 *	target.
	 *	@type float
	 */
	PY_ATTRIBUTE ( targetDistTolerance )
PY_END_ATTRIBUTES()

PY_FACTORY( CCDChainIKHandle, BigWorld )


/**
 *	@param identifier The identifier of the node that this will source
 *		transforms from.
 */
CCDChainIKHandle::CCDChainIKHandle(
	const BW::string & identifier,
	const BW::string & startModelNodeIdentifier,
	const BW::string & endModelNodeIdentifier )
	:
	IKHandle( identifier, &s_type_ )
	, startModelNodeIdentifier_( startModelNodeIdentifier )
	, endModelNodeIdentifier_( endModelNodeIdentifier )
	, maxNumIterations_( 10 )
	, targetDistTolerance_( 0.0001f )
	, targetBonesTreeRegens_( std::numeric_limits<uint32>::max() )
{
}


CCDChainIKHandle * CCDChainIKHandle::New(
	const BW::string & identifier,
	const BW::string & startModelNodeIdentifier,
	const BW::string & endModelNodeIdentifier )
{
	BW_GUARD;
	
	return new CCDChainIKHandle(
		identifier, startModelNodeIdentifier, endModelNodeIdentifier );
}


const BW::vector<int> & CCDChainIKHandle::getTargetBoneIndices(
	const SuperModel & model )
{
	const SuperModelNodeTree * nodeTree = model.getInstanceNodeTree();
	if ( nodeTree == NULL )
	{
		targetBones_.clear();
		scratchAdjustmentTransforms_.clear();
		scratchWorldTransforms_.resize( 1 );
	}
	else if (model.getNumInstanceNodeTreeRegens() != targetBonesTreeRegens_)
	{
		//Get the chain from the start to the end from SuperModelNodeTree.
		const int startNodeIndex =
			nodeTree->getNodeIndex( startModelNodeIdentifier_ );
		const int endNodeIndex =
			nodeTree->getNodeIndex( endModelNodeIdentifier_ );

		if (startNodeIndex < 0)
		{
			WARNING_MSG( "CCDChainIKHandle start node '%s' cannot be found.\n",
				startModelNodeIdentifier_.c_str() );
			targetBones_.clear();
		}
		else if (endNodeIndex < 0)
		{
			WARNING_MSG( "CCDChainIKHandle end node '%s' cannot be found.\n",
				endModelNodeIdentifier_.c_str() );
			targetBones_.clear();
		}
		else if (!nodeTree->getPathIndices(
			targetBones_, startNodeIndex, endNodeIndex ))
		{
			MF_ASSERT( targetBones_.empty() );
			WARNING_MSG( "CCDChainIKHandle chain from the start node '%s' "
				"to the end node '%s' cannot be found.\n",
				startModelNodeIdentifier_.c_str(),
				endModelNodeIdentifier_.c_str() );
		}
		else
		{
			targetBonesTreeRegens_ =
				model.getNumInstanceNodeTreeRegens();
			MF_ASSERT( !targetBones_.empty() );
			MF_ASSERT( targetBones_.front() == startNodeIndex );
			MF_ASSERT( targetBones_.back() == endNodeIndex );
		}
		
		scratchAdjustmentTransforms_.resize( targetBones_.size() );
		scratchWorldTransforms_.resize( targetBones_.size() + 1 );
	}

	return targetBones_;
}


bool CCDChainIKHandle::apply( SuperModel & model, IKConstraintSystem & system )
{
	MF_ASSERT( model.getInstanceNodeTree() != NULL );

	const float DRAW_STAR_SIZE = 0.125f;

	const Matrix & worldTransform = system.getWorldTransform();

	//Must take the position to world space.
	const Vector3 targetPos = worldTransform.applyPoint( getTarget() );
	if (getDrawTarget())
	{
		LineHelper::instance().drawStar( targetPos, DRAW_STAR_SIZE,
			Moo::PackedColours::BLUE, Moo::PackedColours::RED );
	}

	const BW::vector<int> & chainBones = getTargetBoneIndices( model );
	if (chainBones.empty())
	{
		return false;
	}

	const SuperModelNodeTree * nodeTree = model.getInstanceNodeTree();
	MF_ASSERT( nodeTree != NULL );

	//World transform of the start joint's parent is needed because
	//animation propagation will need to be performed.
	const Matrix startJointParentWorld =
		getNodeParentWorldTransform( model, *nodeTree, chainBones[0] );

	//Should not be possible if dependencies were calculated properly.
	MF_ASSERT(
		nodeTree->isDescendantOf( chainBones.front(), chainBones.back() )
		&&
		"CCDChainIKHandle end must be a descendant of the start node.\n" );
	
	MF_ASSERT( scratchAdjustmentTransforms_.size() == chainBones.size() );
	MF_ASSERT( scratchWorldTransforms_.size() == (chainBones.size() + 1) );
	//First element is parent of the start node.
	scratchWorldTransforms_[0] = startJointParentWorld;

	//Travel along the chain calculating the initial world transforms.
	//This needs to be done because the dependencies are configured so
	//that IKHandle::apply runs before animation propagation.
	//This is necessary because otherwise the IKHandle would need to
	//take care of propagating to all children for every node
	//updated - it's easier and more efficient to apply IK before the
	//ParentTransformApplier appliers are executed for the nodes.
	for(std::size_t chainIndex = 0;
		chainIndex < chainBones.size();
		++chainIndex)
	{
		scratchWorldTransforms_[chainIndex + 1] =
			scratchWorldTransforms_[chainIndex];
		scratchWorldTransforms_[chainIndex + 1].preMultiply(
			model.getNodeRelativeTransform( chainBones[chainIndex] ) );
	}
	
	const bool usePoleVector =
		getPoleVectorIsFromModelPoint() || (getPoleVector() != Vector3::ZERO);
	if (usePoleVector)
	{
		//Must take the vector to world space.
		Vector3 poleVect;
		if (getPoleVectorIsFromModelPoint())
		{
			poleVect = worldTransform.applyPoint( getPoleVector() );
		}
		else
		{
			poleVect = worldTransform.applyVector( getPoleVector() );
		}

		MF_ASSERT( scratchWorldTransforms_.size() >= 2 );
		const Vector3 & startJointPos =
			scratchWorldTransforms_[1].applyToOrigin();

		if (getPoleVectorIsFromModelPoint())
		{
			poleVect -= startJointPos;
		}

		const Vector3 handleVector = targetPos - startJointPos;

		if (getDrawPoleVector())
		{
			LineHelper::instance().drawLine(
				startJointPos, startJointPos + poleVect,
				Moo::PackedColours::YELLOW );
			LineHelper::instance().drawLine(
				startJointPos, startJointPos + handleVector,
				Moo::PackedColours::CYAN );
		}

		//Rotate the joints onto the plane. We rotate about the handle vector.
		Vector3 poleVectorPlaneNormal = poleVect.crossProduct( handleVector );
		//PlaneEq plane( startJointPos, poleVectorPlaneNormal );
		
		Matrix adjustRotationMatSoFar = Matrix::identity;
		for (std::size_t chainIndex = 0;
			(chainIndex + 1) < chainBones.size();
			++chainIndex)
		{
			//Get current joint details.
			const Vector3 currentPos = adjustRotationMatSoFar.applyPoint(
				scratchWorldTransforms_[chainIndex + 1].applyToOrigin() );
			const Vector3 nextPos = adjustRotationMatSoFar.applyPoint(
				scratchWorldTransforms_[chainIndex + 2].applyToOrigin() );
			
			Matrix adjustRotationMat = rotateUpVectAboutAimAxis(
				nextPos - currentPos, poleVect, handleVector );
			
			//This rotation will take the bone to the desired location,
			//but it can flip the bone over the handle vector, which results
			//in an undesirable twist on the joint. Therefore, we apply a
			//second rotation about the bone to remove any such twist that's
			//added.
			const Vector3 boneVector =
				adjustRotationMat.applyVector( nextPos - currentPos );

			adjustRotationMat.postMultiply( rotateUpVectAboutAimAxis(
				adjustRotationMat.applyVector( poleVectorPlaneNormal ),
				poleVectorPlaneNormal, boneVector ) );
			
			adjustRotationMat.preTranslateBy( -currentPos );
			adjustRotationMat.postTranslateBy( currentPos );

			adjustRotationMatSoFar.postMultiply( adjustRotationMat );
			scratchWorldTransforms_[chainIndex + 1].postMultiply(
				adjustRotationMatSoFar );
		}
		scratchWorldTransforms_.back().postMultiply( adjustRotationMatSoFar );
	}
	
	const float targetDistToleranceSq =
		targetDistTolerance_ * targetDistTolerance_;

	Matrix adjustRotationMat;
	float bestTargetDistSq = std::numeric_limits<float>::max();
	int32 iterationsSinceImprovement = 0;
	for (uint32 iteration = 0; iteration < maxNumIterations_; ++iteration)
	{
		Vector3 endPos = scratchWorldTransforms_.back().applyToOrigin();
		scratchAdjustmentTransforms_.back() = Matrix::identity;

		//For every node in the chain other than the end node, in reverse
		//starting from one before the end node.
		for(std::size_t chainIndex = chainBones.size() - 1;
			chainIndex > 0;
			)
		{
			--chainIndex;

			//Get current joint details.
			const Vector3 & currentPos =
				scratchWorldTransforms_[chainIndex + 1].applyToOrigin();

			//Get vectors for rotating towards the target.
			const Vector3 cToT = targetPos - currentPos;
			const Vector3 cToE = endPos - currentPos;
			const Vector3 rotAxis = cToE.crossProduct( cToT );
			const float rotAngle = cToE.getAngle( cToT );

			//Create world-space rotation to apply.
			Quaternion rot;
			rot.fromAngleAxis( rotAngle, rotAxis );
			adjustRotationMat.setRotate( rot );
			scratchAdjustmentTransforms_[chainIndex] = adjustRotationMat;

			adjustRotationMat.preTranslateBy( -currentPos );
			adjustRotationMat.postTranslateBy( currentPos );

#if 0
			//Blue is before correction.
			LineHelper::instance().drawStar(
				endPos, DRAW_STAR_SIZE,
				Moo::PackedColours::BLUE, Moo::PackedColours::BLUE );
			//Red is after correction.
			LineHelper::instance().drawStar(
				adjustRotationMat.applyPoint( endPos ), DRAW_STAR_SIZE * 1.1f,
				Moo::PackedColours::RED, Moo::PackedColours::RED );
#endif

			endPos = adjustRotationMat.applyPoint( endPos );
		}

		//Now we propagate the adjustments back along the chain
		//(above loop is end-to-start, we need to do this start-to-end).
		scratchWorldTransforms_[1].postMultiply( adjustRotationMat );
		scratchAdjustmentTransforms_[0] = adjustRotationMat;
		Matrix parentWorld = scratchWorldTransforms_[1];
		for (std::size_t chainIndex = 1;
			(chainIndex + 1) < chainBones.size();
			++chainIndex)
		{
			//Parent rotated, do update for position.
			const Vector3 currentPos =
				scratchAdjustmentTransforms_[chainIndex - 1].applyPoint(
					scratchWorldTransforms_[chainIndex + 1].applyToOrigin() );

			//Update the adjustment rotation. It is currently a rotation
			//implicitly understood to occur about the joint, but we change it
			//to a world-space rotation.
			Matrix & adjustMat = scratchAdjustmentTransforms_[chainIndex];
			adjustMat.preTranslateBy( -currentPos );
			adjustMat.postTranslateBy( currentPos );

			adjustMat.preMultiply(
				scratchAdjustmentTransforms_[chainIndex - 1] );

			//Propagate.
			scratchWorldTransforms_[chainIndex + 1].postMultiply(
				adjustMat );
		}
		if (chainBones.size() > 1)
		{
			//The last adjustment is always identity, so we can just apply the
			//same rotation as for the second-last node.
			scratchWorldTransforms_.back().postMultiply(
				scratchAdjustmentTransforms_[chainBones.size() - 2] );
		}

		const float distSq = endPos.distanceSquared( targetPos );
		if ( distSq <= targetDistToleranceSq )
		{
			break;
		}

		const int32 NON_CONVERGING_ITERATION_LIMIT = 5;
		if (distSq >= bestTargetDistSq)
		{
			if ((++iterationsSinceImprovement) >=
				NON_CONVERGING_ITERATION_LIMIT)
			{
				//The solution hasn't been converging further, so no point
				//performing more iterations.
				break;
			}
		}
		else
		{
			bestTargetDistSq = distSq;
			iterationsSinceImprovement = 0;
		}
	}

	//Configure our calculated world-space transforms on the model nodes.
	for(std::size_t chainIndex = 0;
		chainIndex < chainBones.size();
		++chainIndex)
	{
		applyWorldTransformToModelNode(
			model, chainBones[chainIndex],
			scratchWorldTransforms_[chainIndex + 1] );
	}

	return !chainBones.empty();
}


void CCDChainIKHandle::resetCachedIds()
{
	targetBonesTreeRegens_ = std::numeric_limits<uint32>::max();
}


BW::string CCDChainIKHandle::desc() const
{
	BW::ostringstream resultStr;
	resultStr << "CCDChainIKHandle: " <<
		baseIKHandleMemberDesc() << ", "
		"start=" << startModelNodeIdentifier_ << ", "
		"end=" << endModelNodeIdentifier_;
	return resultStr.str();
}


/**
 *	@return The maximum number of solver iterations to be used.
 */
uint32 CCDChainIKHandle::getMaxNumIterations() const
{
	return maxNumIterations_;
}


/**
 *	
 */
void CCDChainIKHandle::setMaxNumIterations( uint32 maxNumIterations )
{
	maxNumIterations_ = maxNumIterations;
}


/**
 *	@return Solver iterations stop early if within this distance of the target.
 */
float CCDChainIKHandle::getTargetDistTolerance() const
{
	return targetDistTolerance_;
}


/**
 *	@pre tolerance is positive.
 *	@post Set the target distance tolerance (@see getTargetDistTolerance).
 */
void CCDChainIKHandle::setTargetDistTolerance( float tolerance )
{
	MF_ASSERT( tolerance >= 0.0f );
	targetDistTolerance_ = tolerance;
}



// -----------------------------------------------------------------------------
// Section: ParentTransformApplier
// -----------------------------------------------------------------------------
ParentTransformApplier::ParentTransformApplier( int nodeTreeIndex )
	:
	nodeTreeIndex_( nodeTreeIndex )
{
	MF_ASSERT( nodeTreeIndex_ >= 0 );
}
	

bool ParentTransformApplier::apply( SuperModel & model,
	IKConstraintSystem & system )
{
	const SuperModelNodeTree * tree = model.getInstanceNodeTree();
	MF_ASSERT( tree != NULL );

	if (nodeTreeIndex_ >= tree->getNumNodes())
	{
		WARNING_MSG( "Tree node index for animation propagation '%d' "
			"is out of range.\n", nodeTreeIndex_ );
		return false;
	}

	const SuperModelNode & node = tree->getNode( nodeTreeIndex_ );
	const int parentIndex = node.getParentIndex();
	if (parentIndex < 0)
	{
		//Do nothing if no parent; there's nothing to propagate.
		return false;
	}

	const Matrix & relTransform(
			model.getNodeRelativeTransform( nodeTreeIndex_ ) );
	const Matrix & parentWorldTransform(
		model.getNodeWorldTransform( parentIndex ) );
	Matrix newWorld;
	newWorld.multiply( relTransform, parentWorldTransform );
	model.setNodeWorldTransform( nodeTreeIndex_, newWorld );

	return true;
}





// -----------------------------------------------------------------------------
// Section: IKConstraintSystem
// -----------------------------------------------------------------------------
PY_TYPEOBJECT( IKConstraintSystem )

PY_BEGIN_METHODS( IKConstraintSystem )
	/*~ function IKConstraintSystem.setConstraint
	 *	Sets the constraint. If there's already a constraint with the same
	 *	identifier, it will replace that constraint.
	 *	@param constraint The Constraint to set.
	 */
	PY_METHOD( setConstraint )
	/*~ function IKConstraintSystem.removeConstraint
	 *	@return True returned: remove the constraint with the specified
	 *			identifier.
	 *		False returned: no such constraint exists in this system.
	 */
	PY_METHOD( removeConstraint )
	/*~ function IKConstraintSystem.getConstraint
	 *	@return The Constraint associated with @a identifier (or None if not
	 *		set).
	 */
	PY_METHOD( getConstraint )
	/*~ function IKConstraintSystem.getConstraints
	 *	@return A list of all constraint names in the system.
	 */
	PY_METHOD( getConstraints )
	/*~ function IKConstraintSystem.setIKHandle
	 *	Sets the IK handle. If there's already an IK handle with the same
	 *	identifier, it will replace that IK handle.
	 *	@param ikHandle The IK handle to set.
	 */
	PY_METHOD( setIKHandle )
	/*~ function IKConstraintSystem.removeIKHandle
	 *	@return True returned: remove the IK handle with the specified
	 *			identifier.
	 *		False returned: no such IK handle exists in this system.
	 */
	PY_METHOD( removeIKHandle )
	/*~ function IKConstraintSystem.getIKHandle
	 *	@return The IKHandle associated with @a identifier (or None if not
	 *		set).
	 */
	PY_METHOD( getIKHandle )
	/*~ function IKConstraintSystem.getIKHandles
	 *	@return A list of all IK handle names in the system.
	 */
	PY_METHOD( getIKHandles )
	/*~ function IKConstraintSystem debug
	 *	@return A string containing debug information regarding things such as
	 *		the applied constraints and IK handles.
	 */
	PY_METHOD( debug )
	/*~	function IKConstraintSystem compile
	 *
	 *	After setting or removing IK handles or constraints, or attaching to
	 *	a model, the dependencies need recalculating. This is relatively
	 *	expensive. It will happen automatically the next time the system is
	 *	applied, but to avoid a delay later on this method can be called to
	 *	incur the cost straight away.
	 */
	PY_METHOD( compile )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( IKConstraintSystem )
	/*~ attribute IKConstraintSystem.maxLod
	 *
	 *	Sets the maximum LOD before the processing for the system is disabled.
	 *
	 *	@type	bool
	 */
	PY_ATTRIBUTE ( maxLod )
PY_END_ATTRIBUTES()

/*~ function BigWorld IKConstraintSystem
 *  Creates a new instance of the IKConstraintSystem class. This is a subclass
 *  of PyFashion which manipulates the nodes in a PyModel based on a set of
 *  constrainers and IK handles.
 *  
 *  @return The new IKConstraintSystem object.
 */
PY_FACTORY( IKConstraintSystem, BigWorld )

PY_SCRIPT_CONVERTERS( IKConstraintSystem )


class IKConstraintSystem::IKCSDresser
	:
	public TransformFashion
	, private BW::NonCopyable
{
public:
	IKCSDresser( IKConstraintSystem & ikcs )
		:
		ikcs_( ikcs )
	{
	}

	virtual ~IKCSDresser()
	{
	}

	virtual void dress( SuperModel & sm, Matrix& world )
	{
		BW_GUARD;
		PROFILER_SCOPED( IKConstraintSystem_dress );

		ikcs_.apply();
	}

	virtual bool needsRedress() const
	{
		//No redress needed since the IKConstraintSystem
		//propagates transforms down the hierarchy itself.
		return true;//TEMP: NECESSARY FOR NOW ON LOCAL REPO.false;
	}

private:
	IKConstraintSystem & ikcs_;
};


IKConstraintSystem::IKConstraintSystem()
	:
	PyTransformFashion( &IKConstraintSystem::s_type_ )
	, pOwner_()
	, newModelSet_( true )
	, pDresser_( new IKConstraintSystem::IKCSDresser( *this ) )
	, loddedOut_( false )
	, maxLod_( 150.0f )
	, calculateDependencies_( true )
	, lastDependencyCalcTreeRegens_( std::numeric_limits<uint32>::max() )
{
}


IKConstraintSystem::~IKConstraintSystem()
{
}


void IKConstraintSystem::tick( float dTime, float lod )
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( pOwner_ != NULL)
	{
		return;
	}

	loddedOut_ = (maxLod_ > 0.f) && (lod >= maxLod_);
}


IKConstraintSystem * IKConstraintSystem::makeCopy(
	PyModel * pModel, const char * attrName )
{
	BW_GUARD;

	if (pOwner_ != NULL)
	{
		PyErr_Format( PyExc_ValueError, "Cannot attach given "
			"IKConstraintSystem to Model as '%s' because it is "
			"already attached elsewhere. ", attrName );
		return NULL;
	}

	pOwner_ = pModel;
	loddedOut_ = false;
	newModelSet_ = true;
	calculateDependencies_ = true;
	lastDependencyCalcTreeRegens_ = std::numeric_limits<uint32>::max();

	Py_INCREF( this );
	return this;
}


void IKConstraintSystem::disowned()
{
	BW_GUARD;

	pOwner_ = NULL;
}


const TransformFashionPtr IKConstraintSystem::transformFashion() const
{
	BW_GUARD;

	return pDresser_;
}


TransformFashionPtr IKConstraintSystem::transformFashion()
{
	BW_GUARD;

	return pDresser_;
}


bool IKConstraintSystem::isPreTransform() const
{
	BW_GUARD;

	return false;
}


bool IKConstraintSystem::isAffectingTransformThisFrame() const
{
	BW_GUARD;

	return !loddedOut_;
}


IKConstraintSystem * IKConstraintSystem::New()
{
	BW_GUARD;

	return new IKConstraintSystem();
}


/**
 *	@return A debug string for scripts.
 */
PyObject * IKConstraintSystem::py_debug( PyObject * args )
{
	BW_GUARD;

	return Script::getData( desc() );
}


/**
 *	Python-accessible version of setConstraint.
 */
PyObject * IKConstraintSystem::py_setConstraint( PyObject *args )
{
	BW_GUARD;

	PyObject * constraint = NULL;

	if (!PyArg_ParseTuple( args, "O", &constraint )
		||
		!Constraint::Check( constraint ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.setConstraint expects: "
			"Constraint constraint" );
		return NULL;
	}

	ConstraintPtr constraintTyped( static_cast<Constraint *>(constraint) );
	setConstraint( constraintTyped );
	return Py_None;
}


/**
 *	Python-accessible version of removeConstraint.
 */
PyObject * IKConstraintSystem::py_removeConstraint( PyObject *args )
{
	BW_GUARD;

	const char * identifier = NULL;

	if (!PyArg_ParseTuple( args, "s", &identifier ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.removeConstraint expects: "
			"string identifier" );
		return NULL;
	}

	return (removeConstraint( identifier ) ? Py_True : Py_False);
}


/**
 *	Python-accessible version of getConstraint.
 */
PyObject * IKConstraintSystem::py_getConstraint( PyObject *args )
{
	BW_GUARD;

	const char * identifier = NULL;

	if (!PyArg_ParseTuple( args, "s", &identifier ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.getConstraint expects: "
			"string identifier" );
		return NULL;
	}

	MF_ASSERT( identifier != NULL );
	ConstraintPtr result = getConstraint( identifier );
	if (result.hasObject())
	{
		result->incRef();
		return static_cast<PyObject *>(result.get());
	}
	else
	{
		return Py_None;
	}
}


/**
 *	@return Python list of current constraint names.
 */
PyObject * IKConstraintSystem::py_getConstraints( PyObject *args )
{
	BW_GUARD;

	PyObject* result = PyList_New( 0 );
	MF_ASSERT( result != NULL)
	
	for (ConstraintMap::const_iterator itr = constraints_.begin();
		itr != constraints_.end();
		++itr)
	{
		PyObject * str = PyString_FromString( itr->first.c_str() );
		PyList_Append( result, str );
		Py_XDECREF( str );
	}

	return result;
}


/**
 *	Python-accessible version of setIKHandle.
 */
PyObject * IKConstraintSystem::py_setIKHandle( PyObject *args )
{
	BW_GUARD;

	PyObject * ikHandle = NULL;

	if (!PyArg_ParseTuple( args, "O", &ikHandle )
		||
		!IKHandle::Check( ikHandle ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.setIKHandle expects: "
			"IKHandle ikHandle" );
		return NULL;
	}

	IKHandlePtr ikHandleTyped( static_cast<IKHandle *>(ikHandle) );
	setIKHandle( ikHandleTyped );
	return Py_None;
}


/**
 *	Python-accessible version of removeIKHandle.
 */
PyObject * IKConstraintSystem::py_removeIKHandle( PyObject *args )
{
	BW_GUARD;

	const char * identifier = NULL;

	if (!PyArg_ParseTuple( args, "s", &identifier ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.removeIKHandle expects: "
			"string identifier" );
		return NULL;
	}

	return (removeIKHandle( identifier ) ? Py_True : Py_False);
}


/**
 *	Python-accessible version of getIKHandle.
 */
PyObject * IKConstraintSystem::py_getIKHandle( PyObject *args )
{
	BW_GUARD;

	const char * identifier = NULL;

	if (!PyArg_ParseTuple( args, "s", &identifier ))
	{
		PyErr_Format( PyExc_TypeError,
			"IKConstraintSystem.getIKHandle expects: "
			"string identifier" );
		return NULL;
	}

	MF_ASSERT( identifier != NULL );
	IKHandlePtr result = getIKHandle( identifier );
	if (result.hasObject())
	{
		result->incRef();
		return static_cast<PyObject *>(result.get());
	}
	else
	{
		return Py_None;
	}
}


/**
 *	@return Python list of current IK handle names.
 */
PyObject * IKConstraintSystem::py_getIKHandles( PyObject *args )
{
	BW_GUARD;

	PyObject* result = PyList_New( 0 );
	MF_ASSERT( result != NULL)
	
	for (IKHandleMap::const_iterator itr = ikHandles_.begin();
		itr != ikHandles_.end();
		++itr)
	{
		PyObject * str = PyString_FromString( itr->first.c_str() );
		PyList_Append( result, str );
		Py_XDECREF(str);
	}

	return result;
}


/**
 *	@pre A model is currently assigned (via makeCopy).
 *	@post Applied the constraints and IK handles to the model, if the
 *		nodes in the model are compatible with it, it's not lodded out,
 *		etc. Returned true if modifications were applied.
 */
bool IKConstraintSystem::apply()
{
	BW_GUARD;

	IF_NOT_MF_ASSERT_DEV( pOwner_ != NULL)
	{
		return false;
	}

	if (loddedOut_ || (constraints_.empty() && ikHandles_.empty()))
	{
		return false;
	}

	SuperModel * model = pOwner_->pSuperModel();
	if (model == NULL)
	{
		return false;
	}

	const SuperModelNodeTree * tree = model->getInstanceNodeTree();
	if (tree == NULL)
	{
		WARNING_MSG( "No instance node tree for the model, so "
			"IKConstraintSystem cannot apply.\n" );
		return false;
	}

	compileInternal( *model, *tree );
	
	bool modified = false;
	for (BW::vector<IKCSApplier *>::iterator itr = appliers_.begin();
		itr != appliers_.end();
		++itr)
	{
		modified = (*itr)->apply( *model, *this ) || modified;
	}

	return modified;
}


/**
 *	@pre True.
 *	@post Sets the constraint for the associated identifier to @a constraint.
 */
void IKConstraintSystem::setConstraint( ConstraintPtr constraint )
{
	BW_GUARD;
	
	IF_NOT_MF_ASSERT_DEV( constraint.hasObject() != NULL)
	{
		return;
	}

	constraints_[constraint->getIdentifier()] = constraint;
	calculateDependencies_ = true;
}


/**
 *	@return True return: removed constraint named @a identifier.
 *		False return: no such constraint, done nothing.
 */
bool IKConstraintSystem::removeConstraint( const BW::string & identifier )
{
	BW_GUARD;
	
	if (constraints_.erase( identifier ) != 0)
	{
		calculateDependencies_ = true;
		return true;
	}
	return false;
}


/**
 *	@pre True.
 *	@post The constraint for the identifier @a identifier or NULL if none.
 */
ConstraintPtr IKConstraintSystem::getConstraint(
	const BW::string & identifier )
{
	BW_GUARD;
	
	ConstraintMap::iterator itr = constraints_.find( identifier );

	if (itr == constraints_.end())
	{
		return ConstraintPtr();
	}
	return itr->second;
}


///Const-version
ConstConstraintPtr IKConstraintSystem::getConstraint(
	const BW::string & identifier ) const
{
	BW_GUARD;
	
	//We can const_cast to call the other version because we know it
	//isn't modifying the data.
	return const_cast<IKConstraintSystem &>(*this).getConstraint( identifier );
}


/**
 *	@pre True.
 *	@post Sets the IK handle for the associated identifier to @a ikHandle,
 *		and called resetCachedIds on it.
 */
void IKConstraintSystem::setIKHandle( IKHandlePtr ikHandle )
{
	BW_GUARD;
	
	IF_NOT_MF_ASSERT_DEV( ikHandle.hasObject() != NULL)
	{
		return;
	}

	ikHandles_[ikHandle->getIdentifier()] = ikHandle;
	ikHandle->resetCachedIds();
	calculateDependencies_ = true;
}


/**
 *	@return True return: removed IK handle named @a identifier.
 *		False return: no such constraint, done nothing.
 */
bool IKConstraintSystem::removeIKHandle( const BW::string & identifier )
{
	BW_GUARD;
	
	if (ikHandles_.erase( identifier ) != 0)
	{
		calculateDependencies_ = true;
		return true;
	}
	return false;
}


/**
 *	@pre True.
 *	@post The IK handle for the identifier @a identifier or NULL if none.
 */
IKHandlePtr IKConstraintSystem::getIKHandle(
	const BW::string & identifier )
{
	BW_GUARD;
	
	IKHandleMap::iterator itr = ikHandles_.find( identifier );

	if (itr == ikHandles_.end())
	{
		return IKHandlePtr();
	}
	return itr->second;
}


/**
 *	@pre True.
 *	@post If this is attached to a model with an instance node tree, generated
 *		the apply order for the system. This can be useful to call when a
 *		system has been configured and attached to a model, so the cost
 *		of building the apply order is incurred at that time rather than on
 *		the first draw.
 */
void IKConstraintSystem::compile()
{
	BW_GUARD;

	if ((pOwner_ == NULL) || (constraints_.empty() && ikHandles_.empty()))
	{
		return;
	}

	SuperModel * model = pOwner_->pSuperModel();
	if (model == NULL)
	{
		return;
	}

	const SuperModelNodeTree * tree = model->getInstanceNodeTree();
	if (tree == NULL)
	{
		return;
	}

	compileInternal( *model, *tree );
}


/**
 *	@pre A model is currently assigned (via makeCopy).
 *	@post Returned the current world transform for the model.
 */
const Matrix & IKConstraintSystem::getWorldTransform() const
{
	MF_ASSERT( pOwner_ != NULL );
	return pOwner_->worldTransform();
}


/**
 *	This function is intended to only be used for debugging, and returns a
 *	description of the IKConstraintSystem.
 */
BW::string IKConstraintSystem::desc() const
{
	BW_GUARD;

	BW::ostringstream resultStr;
	resultStr << "maxLod_=" << maxLod_ << "; " <<
		constraints_.size() << " constraints; " <<
		ikHandles_.size() << " IK handles.\n"
		"Constraints:\n";

	//Sort the constraints by id for consistent printing order.
	BW::vector<BW::string> identifiers;
	identifiers.reserve( std::max( constraints_.size(), ikHandles_.size() ) );
	for (ConstraintMap::const_iterator itr = constraints_.begin();
		itr != constraints_.end();
		++itr)
	{
		identifiers.push_back( itr->first );
	}
	std::sort( identifiers.begin(), identifiers.end() );
	for( BW::vector<BW::string>::const_iterator itr = identifiers.begin();
		itr != identifiers.end();
		++itr)
	{
		ConstraintMap::const_iterator srchRes = constraints_.find( *itr );
		MF_ASSERT( srchRes != constraints_.end() );
		resultStr << "\t" << srchRes->second->desc() << "\n";
	}

	resultStr << "IKHandles:\n";
	identifiers.clear();
	for (IKHandleMap::const_iterator itr = ikHandles_.begin();
		itr != ikHandles_.end();
		++itr)
	{
		identifiers.push_back( itr->first );
	}
	std::sort( identifiers.begin(), identifiers.end() );
	for( BW::vector<BW::string>::const_iterator itr = identifiers.begin();
		itr != identifiers.end();
		++itr)
	{
		IKHandleMap::const_iterator srchRes = ikHandles_.find( *itr );
		MF_ASSERT( srchRes != ikHandles_.end() );
		resultStr << "\t" << srchRes->second->desc() << "\n";
	}

	return resultStr.str();
}


///Const-version
ConstIKHandlePtr IKConstraintSystem::getIKHandle(
	const BW::string & identifier ) const
{
	BW_GUARD;
	
	//We can const_cast to call the other version because we know it
	//isn't modifying the data.
	return const_cast<IKConstraintSystem &>(*this).getIKHandle( identifier );
}


/**
 *	This class represents a key in the DependencyGraph used to calculate
 *	processing dependencies in an IKConstraintSystem.
 */
class IKCDependencyGraphKey
{
public:
	/**
	 *	Enumerates the possible node types in the DependencyGraph.
	 */
	enum Type
	{
		///A node in the associated model.
		MODEL_NODE,
		///Propagates animation transforms to the associated model node from
		///its parent.
		ANIM_PROPAGATOR,
		///A group of constraints.
		CONSTRAINT_GROUP,
		///An IK handle node.
		IK_HANDLE
	};


	/**
	 *	@param applier Object applying the effect for this node. NULL if no
	 *		effect is generated (this is the case for nodes that only exist
	 *		for controlling dependencies).
	 *	@param nodeType The type of graph node.
	 *	@param identifier Used to identify this graph node.
	 *	@param constraintGroupTargetType If the type is CONSTRAINT_GROUP, this
	 *		identifies the type of target for the constraint group, otherwise
	 *		it's ignored.
	 */
	IKCDependencyGraphKey(
		IKCSApplier * applier,
		Type nodeType,
		const BW::string & identifier,
		ConstraintTarget::Type constraintGroupTargetType =
			ConstraintTarget::MODEL_NODE )
		:
		applier_( applier )
		, nodeType_( nodeType )
		, identifier_( identifier )
		, constraintGroupTargetType_( constraintGroupTargetType )
	{
		BW_GUARD;
	}

	bool operator == ( const IKCDependencyGraphKey & rhs ) const
	{
		BW_GUARD;
		
		return
			(applier_ == rhs.applier_)
			&&
			(nodeType_ == rhs.nodeType_)
			&&
			(identifier_ == rhs.identifier_)
			&&
			(constraintGroupTargetType_ == rhs.constraintGroupTargetType_);
	}
		
	///Applies the effect of this node to the model.
	IKCSApplier * applier_;
	///Type of node on the graph.
	Type nodeType_;
	///Identifies this node uniquely among nodes of the same type.
	BW::string identifier_;
	///If this is a CONSTRAINT_GROUP type, specifies the type of target
	///for the constraint group (otherwise value is ignored).
	ConstraintTarget::Type constraintGroupTargetType_;
};

BW_END_NAMESPACE


///Specialize hashing for IKCDependencyGraphKey.
BW_HASH_NAMESPACE_BEGIN

template<>
struct hash< BW_NAMESPACE IKCDependencyGraphKey > : public std::unary_function<
	BW_NAMESPACE IKCDependencyGraphKey, std::size_t >
{
public:
	std::size_t operator()( const BW_NAMESPACE IKCDependencyGraphKey & v ) const
	{
		std::size_t seed = 0;
		//applier_ shouldn't be part of the hash; it makes the ordering
		//of maps dependent on pointer values, which could make any bugs
		//hard to reproduce.
		//BW::hash_combine( seed, v.applier_ );
		BW::hash_combine( seed, v.nodeType_ );
		BW::hash_combine( seed, v.identifier_ );
		BW::hash_combine( seed, v.constraintGroupTargetType_ );
		return seed;
	}
};

BW_HASH_NAMESPACE_END

BW_BEGIN_NAMESPACE

#if !CONSUMER_CLIENT_BUILD
namespace
{
	///Used to make debug printing more convenient.
	std::ostream & operator << (
		std::ostream & output, IKCDependencyGraphKey::Type type )
	{
		BW_GUARD;
		
		const char * str = NULL;
		switch (type)
		{
		case IKCDependencyGraphKey::CONSTRAINT_GROUP:
			str = "constraintgroup";
			break;

		case IKCDependencyGraphKey::MODEL_NODE:
			str = "modelnode";
			break;

		case IKCDependencyGraphKey::IK_HANDLE:
			str = "ikhandle";
			break;

		case IKCDependencyGraphKey::ANIM_PROPAGATOR:
			str = "animpropagator";
			break;

		default:
			MF_ASSERT( false );
			str = "[unhandled IKCDependencyGraphKey::Type]";
			break;
		}

		return output << str;
	}


	///Used to make debug printing more convenient.
	void debugPrint(
		const BW::string & prefix, const IKCDependencyGraphKey & type )
	{
		BW_GUARD;
		
		BW::ostringstream output;

		output << "nodeType=" << type.nodeType_ << ", "
			"identifier=" << type.identifier_ << ", "
			"has applier=" << (type.applier_ != NULL ? "yes" : "no") << ", "
			"constraintGroupTargetType=" << type.constraintGroupTargetType_ <<
			".";

		DEBUG_MSG( "%s%s\n", prefix.c_str(), output.str().c_str() );
	}
}
#endif


namespace
{
	///The type of key used for IK/constraint dependency graphs.
	typedef IKCDependencyGraphKey DepKey;


	typedef BW::unordered_map<BW::string, ConstraintPtr> ConstraintMap;
	typedef
		std::pair<ConstraintTarget::Type, BW::string>
		ConstraintTargetKey;
	typedef
		BW::unordered_map<ConstraintTargetKey, ConstraintGroup>
		ConstraintGroupMap;
	/**
	 *	Used internally by generateApplyOrder.
	 *	@pre True.
	 *	@post Went through all constraints in @a constraints and added a
	 *		ConstraintGroup object to @a constraintGroups for each target.
	 */
	void generateConstraintGroups(
		ConstraintGroupMap & constraintGroups,
		ConstraintMap & constraints )
	{
		BW_GUARD;
	
		typedef BW::vector<ConstraintPtr> ConstraintVector;
		typedef
			BW::unordered_map<ConstraintTargetKey, ConstraintVector>
			ClumpMap;

		//Stores vectors of constraints clumped together by target.
		ClumpMap clumps;
		for (ConstraintMap::iterator itr = constraints.begin();
			itr != constraints.end();
			++itr)
		{
			ConstConstraintTargetPtr target = itr->second->getTarget();
			ConstraintTargetKey targetKey(
				target->getType(), target->getIdentifier() );

			ClumpMap::iterator srchRes = clumps.find( targetKey );
			if (srchRes != clumps.end())
			{
				srchRes->second.push_back( itr->second );
			}
			else
			{
				clumps[targetKey] = ConstraintVector( 1, itr->second );
			}
		}

		constraintGroups.clear();
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1700)
		constraintGroups.reserve( clumps.size() );
#endif
		for (ClumpMap::iterator itr = clumps.begin();
			itr != clumps.end();
			++itr)
		{
			constraintGroups[itr->first] = ConstraintGroup( itr->second );
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	@pre True.
	 *	@post @a animPropagators has an element for each node in @a tree.
	 *		The element for a given index propagates parent animation transform
	 *		to the node with the same index in @a tree.
	 */
	void generateAnimAppliers(
		BW::vector<ParentTransformApplier> & animPropagators,
		const SuperModelNodeTree & tree )
	{
		BW_GUARD;
		
		animPropagators.clear();
		animPropagators.reserve( tree.getNumNodes() );
		for (int i = 0; i < tree.getNumNodes(); ++i)
		{
			animPropagators.push_back( ParentTransformApplier( i ) );
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds two nodes for each model node in @a tree to @a dependencyGraph,
	 *	one of type ANIM_PROPAGATOR (with an applier) and one of type
	 *	MODEL_NODE.
	 *	
	 *	@a outModelNodeKeys and @a outAnimPropKeys will be populated with
	 *	keys corresponding to the node index in @a tree.
	 *	@a animPropagators is expected to contain at each index an
	 *	animation propagator for the node with that index in @a tree.
	 */
	void addModelAndAnimPropNodesToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & outModelNodeKeys,
		BW::vector<IKCDependencyGraphKey> & outAnimPropKeys,
		BW::vector<ParentTransformApplier> & animPropagators,
		const SuperModelNodeTree & tree )
	{
		BW_GUARD;
		
		MF_ASSERT( static_cast<std::ptrdiff_t>(animPropagators.size()) ==
			tree.getNumNodes() );
		
		outModelNodeKeys.reserve( tree.getNumNodes() );
		outAnimPropKeys.reserve( tree.getNumNodes() );
		for(int index = 0; index < tree.getNumNodes(); ++index)
		{
			outModelNodeKeys.push_back( DepKey(
				//Node has no effect (NULL applier); it only exists so
				//dependencies can be attached.
				NULL,
				DepKey::MODEL_NODE,
				tree.getNode( index ).getIdentifier() ) );
			dependencyGraph.addNode( outModelNodeKeys.back() );

			outAnimPropKeys.push_back( DepKey(
				& animPropagators[index],
				DepKey::ANIM_PROPAGATOR,
				tree.getNode( index ).getIdentifier() ) );
			dependencyGraph.addNode( outAnimPropKeys.back() );
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds a node for each element of @a constraintGroups to
	 *	@a dependencyGraph (of type CONSTRAINT_GROUP).
	 *	
	 *	@a outModelNodeKeys will be populated with keys corresponding to
	 *	the current iterator ordering of @a constraintGroups.
	 */
	void addConstraintGroupNodesToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & outConstraintGroupKeys,
		ConstraintGroupMap & constraintGroups )
	{
		BW_GUARD;
		
		outConstraintGroupKeys.reserve( constraintGroups.size() );
		for (ConstraintGroupMap::iterator itr = constraintGroups.begin();
			itr != constraintGroups.end();
			++itr)
		{
			outConstraintGroupKeys.push_back( DepKey(
				&(itr->second),
				DepKey::CONSTRAINT_GROUP,
				itr->first.second,
				itr->first.first ) );
			dependencyGraph.addNode( outConstraintGroupKeys.back() );
		}
	}


	typedef BW::unordered_map<BW::string, IKHandlePtr> IKHandleMap;
	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds a node for each element of @a ikHandles to
	 *	@a dependencyGraph (of type IK_HANDLE).
	 *	
	 *	@a outIKHandleKeys will be populated with keys corresponding to
	 *	the current iterator ordering of @a ikHandles.
	 */
	void addIKHandleNodesToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & outIKHandleKeys,
		IKHandleMap & ikHandles )
	{
		BW_GUARD;
		
		outIKHandleKeys.reserve( ikHandles.size() );
		for (IKHandleMap::iterator itr = ikHandles.begin();
			itr != ikHandles.end();
			++itr)
		{
			outIKHandleKeys.push_back( DepKey(
				itr->second.get(),
				DepKey::IK_HANDLE,
				itr->first ) );
			dependencyGraph.addNode( outIKHandleKeys.back() );
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds dependencies for the model nodes to @a dependencyGraph.
	 */
	void addModelAndAnimPropDepsToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & modelNodeKeys,
		BW::vector<IKCDependencyGraphKey> & animPropNodeKeys,
		const SuperModelNodeTree & tree )
	{
		BW_GUARD;
		
		MF_ASSERT( modelNodeKeys.size() == animPropNodeKeys.size() );
		MF_ASSERT( static_cast<std::ptrdiff_t>(modelNodeKeys.size()) ==
			tree.getNumNodes() );

		for (int index = 0; index < tree.getNumNodes(); ++index)
		{
			//Add a dependency for each model node on the corresponding
			//animation propagation (since animation should have been
			//propagated to the model node before anything uses it).
			dependencyGraph.addDependency(
				modelNodeKeys[index], animPropNodeKeys[index] );

			//We also add a dependency from the animation propagation node
			//to the parent of the model node (since the parent must have been
			//updated completely before we propagate to the child).
			const int parentIndex = tree.getNode( index ).getParentIndex();
			if (parentIndex < 0)
			{
				//No parent, so no propagation necessary.
				continue;
			}
			MF_ASSERT(
				static_cast<std::size_t>(parentIndex) < modelNodeKeys.size() );

			dependencyGraph.addDependency(
				animPropNodeKeys[index], modelNodeKeys[parentIndex] );
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds dependencies for the constraint group nodes to @a dependencyGraph.
	 */
	void addConstraintGroupDepsToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & constraintGroupKeys,
		ConstraintGroupMap & constraintGroups,
		ConstraintMap & constraints,
		IKHandleMap & ikHandles,
		BW::vector<IKCDependencyGraphKey> & modelNodeKeys,
		BW::vector<IKCDependencyGraphKey> & animPropNodeKeys,
		const SuperModelNodeTree & tree )
	{
		BW_GUARD;
		
		MF_ASSERT( constraintGroupKeys.size() == constraintGroups.size() );
		MF_ASSERT( modelNodeKeys.size() == animPropNodeKeys.size() );
		MF_ASSERT( static_cast<std::ptrdiff_t>(modelNodeKeys.size()) ==
			tree.getNumNodes() );

		
		int index = 0;
		for (ConstraintGroupMap::iterator groupItr =
				constraintGroups.begin();
			groupItr != constraintGroups.end();
			++groupItr, ++index)
		{
			//Constraint group dependency setup does three things:
			// - the group's target depends on the group.
			// - For model node's, the group depends on the animation
			//	propagation to the node.
			// - the group depends on all of its sources' dependencies.

			const IKCDependencyGraphKey & groupKey =
				constraintGroupKeys[index];
			const BW::vector<ConstraintPtr> & groupConstraints =
				groupItr->second.getConstraints();


			//Setup target dependency on this group.
			switch( groupKey.constraintGroupTargetType_ )
			{
			case ConstraintTarget::MODEL_NODE:
				{
					const int nodeIndex =
						tree.getNodeIndex( groupKey.identifier_ );
					if ( nodeIndex < 0 )
					{
						WARNING_MSG( "Constraint target model node "
							"'%s' cannot be found.\n",
							groupKey.identifier_.c_str() );
						continue;
					}

					MF_ASSERT( static_cast<std::size_t>(nodeIndex) <
						modelNodeKeys.size() );
					dependencyGraph.addDependency(
						modelNodeKeys[nodeIndex], groupKey );

					//Make the group depend on the transform propagation,
					//so that it's applied before the group executes.
					MF_ASSERT( static_cast<std::size_t>(nodeIndex) <
						animPropNodeKeys.size() );
					dependencyGraph.addDependency(
						groupKey, animPropNodeKeys[nodeIndex] );
				}
				break;

			/*case ConstraintTarget::CONSTRAINT:
				{
					ConstraintMap::iterator srchConstraintRes =
						constraints.find( groupKey.identifier_ );
					if ( srchConstraintRes == constraints.end() )
					{
						WARNING_MSG( "Constraint target constraint "
							"'%s' cannot be found.\n",
							groupKey.identifier_.c_str() );
						continue;
					}

					//Get the constraint group that this constraint belongs
					//to (constraint groups are identified by their target).
					ConstraintTargetPtr cTgt =
						srchConstraintRes->second->getTarget();
					ConstraintTargetKey cTgtKey(
						cTgt->getType(), cTgt->getIdentifier() );
					ConstraintGroupMap::iterator srchConstraintGrpRes =
						constraintGroups.find( cTgtKey );
					MF_ASSERT( srchConstraintGrpRes != constraintGroups.end() );

					//Make that constraint group depend on this constraint
					//group.
					dependencyGraph.addDependency(
						DepKey(
							&(srchConstraintGrpRes->second),
							DepKey::CONSTRAINT_GROUP,
							cTgtKey.second,
							cTgtKey.first ),
						groupKey );
				}
				break;*/
				
			case ConstraintTarget::IK_HANDLE_TARGET:
			case ConstraintTarget::IK_HANDLE_POLE_VECTOR:
				{
					IKHandleMap::iterator srchRes =
						ikHandles.find( groupKey.identifier_ );
					if ( srchRes == ikHandles.end() )
					{
						WARNING_MSG( "Constraint target IK handle "
							"'%s' cannot be found.\n",
							groupKey.identifier_.c_str() );
						continue;
					}
					dependencyGraph.addDependency(
						DepKey(
							ikHandles[groupKey.identifier_].get(),
							DepKey::IK_HANDLE,
							groupKey.identifier_ ),
						groupKey );
				}
				break;

			default:
				MF_ASSERT(
					false && "Unhandled constraint group target type. " );
			}

			for (BW::vector<ConstraintPtr>::const_iterator cItr =
					groupConstraints.begin();
				cItr != groupConstraints.end();
				++cItr)
			{
				//Get the source of the constraint, if there's a dependency
				//we need to add it to the graph.
				ConstConstraintSourcePtr src = (*cItr)->getSource();
				const ConstraintSource::Dependency srcDep =
					src->getDependency();

				//Need to check whether this source of the constraint is
				//the same as the group's target. This is OK if the target
				//is a model node (the animation propagation will have
				//already run, so we just skip adding in a dependency
				//on this source). For other source types the dependency
				//cycle is illogical though so we print a warning and
				//return.
				//NOTE: this won't detect indirect cycles. They would be
				//detected later and a less helpful warning displayed
				//(complaining of a cycle in the graph).
				if (srcDep.first == ConstraintSource::MODEL_NODE &&
					groupKey.constraintGroupTargetType_ ==
						ConstraintTarget::MODEL_NODE &&
					srcDep.second == groupKey.identifier_)
				{
					continue;
				}
				else if (srcDep.first == ConstraintSource::CONSTRAINT &&
					( groupKey.constraintGroupTargetType_ ==
						ConstraintTarget::IK_HANDLE_TARGET ||
						groupKey.constraintGroupTargetType_ ==
							ConstraintTarget::IK_HANDLE_POLE_VECTOR ) &&
					srcDep.second == groupKey.identifier_)
				{
					WARNING_MSG( "Constraint source found "
						"that's the same as the target and isn't a model "
						"node (id='%s'). "
						"This dependency cycle has been removed but the "
						"evaluation order could be incorrect.\n",
						srcDep.second.c_str() );
					continue;
				}

				//Add a dependency of this group on this source.
				switch(srcDep.first)
				{
				case ConstraintSource::NONE:
					continue;

				case ConstraintSource::MODEL_NODE:
					{
						const int nodeIndex = tree.getNodeIndex(
							srcDep.second );
						if ( nodeIndex < 0 )
						{
							WARNING_MSG( "Constraint source model node "
								"'%s' cannot be found.\n",
								srcDep.second.c_str() );
							continue;
						}
						MF_ASSERT( static_cast<std::size_t>(nodeIndex) <
							modelNodeKeys.size() );
						dependencyGraph.addDependency(
							groupKey, modelNodeKeys[nodeIndex] );
					}
					break;

				case ConstraintSource::CONSTRAINT:
					{
						ConstraintMap::iterator srchConstraintRes =
							constraints.find( srcDep.second );
						if ( srchConstraintRes == constraints.end() )
						{
							WARNING_MSG( "Constraint source constraint "
								"'%s' cannot be found.\n",
								srcDep.second.c_str() );
							continue;
						}

						//Get the constraint group that this constraint belongs
						//to (constraint groups are identified by their target).
						ConstraintTargetPtr cTgt =
							srchConstraintRes->second->getTarget();
						ConstraintTargetKey cTgtKey(
							cTgt->getType(), cTgt->getIdentifier() );
						ConstraintGroupMap::iterator srchConstraintGrpRes =
							constraintGroups.find( cTgtKey );
						MF_ASSERT(
							srchConstraintGrpRes != constraintGroups.end() );

						//Make this constraint group depend on that constraint
						//group.
						dependencyGraph.addDependency(
							groupKey,
							DepKey(
								&(srchConstraintGrpRes->second),
								DepKey::CONSTRAINT_GROUP,
								cTgtKey.second,
								cTgtKey.first ) );
					}
					break;

				/*case ConstraintSource::IK_HANDLE:
					{
						IKHandleMap::iterator srchRes =
							ikHandles.find( srcDep.second );
						if ( srchRes == ikHandles.end() )
						{
							WARNING_MSG( "Constraint source IK handle "
								"'%s' cannot be found.\n",
								srcDep.second.c_str() );
							continue;
						}
						dependencyGraph.addDependency(
							groupKey,
							DepKey(
								srchRes->second.get(),
								DepKey::IK_HANDLE,
								srcDep.second ) );
					}
					break;*/
				}
			}
		}
	}


	/**
	 *	Used internally by generateApplyOrder.
	 *	Adds dependencies for the IK handle nodes to @a dependencyGraph.
	 */
	void addIKHandleDepsToDepTree(
		DependencyGraph<DepKey> & dependencyGraph,
		BW::vector<IKCDependencyGraphKey> & ikHandleKeys,
		IKHandleMap & ikHandles,
		const SuperModel & model,
		BW::vector<IKCDependencyGraphKey> & modelNodeKeys,
		BW::vector<IKCDependencyGraphKey> & animPropNodeKeys,
		ConstraintGroupMap & constraintGroups )
	{
		BW_GUARD;
		
		MF_ASSERT( ikHandleKeys.size() == ikHandles.size() );
		MF_ASSERT( modelNodeKeys.size() == animPropNodeKeys.size() );
		const SuperModelNodeTree * tree = model.getInstanceNodeTree();
		MF_ASSERT( tree != NULL );
		MF_ASSERT( modelNodeKeys.size() ==
			static_cast<size_t>( tree->getNumNodes() ) );
		
		//For each IK handle:
		// - animation propagation to the start model node depends on the
		//		IK handle.
		// - the IK handle depends on the start model node's parent node.
		int index = 0;
		for (IKHandleMap::iterator itr = ikHandles.begin();
			itr != ikHandles.end();
			++itr, ++index)
		{
			IKCDependencyGraphKey & handleKey = ikHandleKeys[index];
			
			MF_ASSERT( handleKey.applier_ ==
				static_cast<IKCSApplier *>(itr->second.get()) );

			IKHandle * ikHandle = itr->second.get();
			const BW::vector<int> & targetBoneIndices =
				ikHandle->getTargetBoneIndices( model );

			if (targetBoneIndices.empty())
			{
				WARNING_MSG( "IKHandle '%s' is invalid (no "
					"target bone indices).\n",
					ikHandle->getIdentifier().c_str() );
				continue;
			}
			const int startNodeIndex = targetBoneIndices.front();
			
			MF_ASSERT( static_cast<std::size_t>(startNodeIndex) <
				animPropNodeKeys.size() );
			dependencyGraph.addDependency(
				animPropNodeKeys[startNodeIndex], handleKey );
			
			const int startNodeParentIndex =
				tree->getNode( startNodeIndex ).getParentIndex();
			if (startNodeParentIndex >= 0)
			{
				MF_ASSERT( static_cast<std::size_t>(startNodeParentIndex) <
					modelNodeKeys.size() );
				dependencyGraph.addDependency(
					handleKey, modelNodeKeys[startNodeParentIndex] );
			}
		}
	}
}


/**
 *	@pre @a model is the model associated with this system.
 *	@pre @a tree is the instance node tree for @a model.
 *	@post Updated the appliers_ collection if necessary (new model was set,
 *		instance node tree modified, calculateDependencies_ flag set).
 */
void IKConstraintSystem::compileInternal(
	const SuperModel & model, const SuperModelNodeTree & tree )
{
	BW_GUARD;

	MF_ASSERT( pOwner_ != NULL );
	MF_ASSERT( &model == pOwner_->pSuperModel() );
	MF_ASSERT( model.getInstanceNodeTree() == &tree );
	
	if (newModelSet_)
	{
		//Make sure cached ids in IKHandle objects are reset.
		for (IKHandleMap::iterator itr = ikHandles_.begin();
			itr != ikHandles_.end();
			++itr)
		{
			itr->second->resetCachedIds();
		}

		newModelSet_ = false;
		calculateDependencies_ = true;
		lastDependencyCalcTreeRegens_ = std::numeric_limits<uint32>::max();
	}

	if (calculateDependencies_ ||
		(lastDependencyCalcTreeRegens_ !=
			model.getNumInstanceNodeTreeRegens()))
	{
		generateApplyOrder( model, tree );
		calculateDependencies_ = false;
		lastDependencyCalcTreeRegens_ = model.getNumInstanceNodeTreeRegens();
	}
}


/**
 *	@pre @a model is the model associated with this system.
 *	@pre @a tree is the instance node tree for @a model.
 *	@post Updates the constraint groups and populates appliers_ with
 *		objects to apply all modifications performed by this system in the
 *		order that they should be applied.
 */
void IKConstraintSystem::generateApplyOrder(
	const SuperModel & model, const SuperModelNodeTree & tree )
{
	BW_GUARD;

	MF_ASSERT( pOwner_ != NULL );
	MF_ASSERT( &model == pOwner_->pSuperModel() );
	MF_ASSERT( model.getInstanceNodeTree() == &tree );
	
	//Update constraintGroups_ and animPropagators_.
	generateConstraintGroups( constraintGroups_, constraints_ );
	generateAnimAppliers( animPropagators_, tree );


	typedef IKCDependencyGraphKey DepKey;
	DependencyGraph<DepKey> dependencyGraph;
	

	//---Add nodes to the graph---
	//Model and animation propagation nodes.
	BW::vector<IKCDependencyGraphKey> modelNodeKeys;
	BW::vector<IKCDependencyGraphKey> animPropNodeKeys;
	addModelAndAnimPropNodesToDepTree(
		dependencyGraph, modelNodeKeys, animPropNodeKeys,
		animPropagators_, tree );
	//Constraint groups.
	BW::vector<IKCDependencyGraphKey> constraintGroupKeys;
	addConstraintGroupNodesToDepTree(
		dependencyGraph, constraintGroupKeys, constraintGroups_ );
	//IK handles.
	BW::vector<IKCDependencyGraphKey> ikHandleKeys;
	addIKHandleNodesToDepTree(
		dependencyGraph, ikHandleKeys, ikHandles_ );


	//---Add dependencies to the graph---
	//Model and animation propagation nodes.
	addModelAndAnimPropDepsToDepTree(
		dependencyGraph, modelNodeKeys, animPropNodeKeys, tree );
	//Constraint groups.
	addConstraintGroupDepsToDepTree(
		dependencyGraph, constraintGroupKeys,
		constraintGroups_, constraints_, ikHandles_,
		modelNodeKeys, animPropNodeKeys, tree );
	//IK handles.
	addIKHandleDepsToDepTree(
		dependencyGraph, ikHandleKeys, ikHandles_, model,
		modelNodeKeys, animPropNodeKeys, constraintGroups_ );


	//---Get the output ordering---
	BW::vector<DepKey> order;
	appliers_.clear();
	if (!dependencyGraph.getOrder( order ))
	{
		WARNING_MSG( "IKConstraintSystem setup is invalid. There's a "
			"dependency cycle.\n" );
		return;
	}
	MF_ASSERT( order.size() == (constraintGroups_.size() +
		ikHandles_.size() + (animPropagators_.size() * 2)) );
	MF_ASSERT( order.size() == dependencyGraph.size() );
	appliers_.reserve( order.size() );

	bool gotNonAnimPropNode = false;
	for (BW::vector<DepKey>::iterator itr = order.begin();
		itr != order.end();
		++itr)
	{
		if (itr->nodeType_ == DepKey::MODEL_NODE)
		{
			//debugPrint( "Skipping: ", *itr );
			MF_ASSERT( itr->applier_ == NULL );
			continue;
		}
		
		//Trim off the animation propagation appliers at the start of the
		//collection until we reach one that's not just an animation
		//propagator. This is because we assume that the transforms have
		//already been correctly calculated before the IKConstraintSystem is
		//applied, so we only need to propagate after making modifications.
		if (!gotNonAnimPropNode)
		{
			if (itr->nodeType_ == DepKey::ANIM_PROPAGATOR)
			{
				//debugPrint( "Ignoring: ", *itr );
				continue;
			}
			else
			{
				gotNonAnimPropNode = true;
			}
		}

		MF_ASSERT( itr->applier_ != NULL );
		appliers_.push_back( itr->applier_ );
		//debugPrint( "Added: ", *itr );
	}
}

BW_END_NAMESPACE
