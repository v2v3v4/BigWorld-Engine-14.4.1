#include "pch.hpp"

#include "general_properties.hpp"
#include "current_general_properties.hpp"
#include "coord_mode_provider.hpp"
#include "snap_provider.hpp"

#include "property_rotater_helper.hpp"

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
PropertyRotaterHelper::PropertyRotaterHelper()
{
}


/**
 *	This method initialises the property rotater helper
 *	@param gizmoTransform the current gizmo transform
 *		used to apply rotation along its axis
 */
void PropertyRotaterHelper::init( const Matrix& gizmoTransform )
{
	BW_GUARD;

	// Clear the properties
	props_.clear();
	positionOnlyProps_.clear();

	// Iterate over all the properties and init the propertyinfo structure
	BW::vector<GenRotationProperty*> props = CurrentRotationProperties::properties();
	for (BW::vector<GenRotationProperty*>::iterator i = props.begin(); i != props.end(); ++i)
	{
		PropInfo pi;

		pi.prop_ = *i;
		pi.prop_->pMatrix()->recordState();
		pi.prop_->pMatrix()->getMatrix( pi.initialMatrix_, true );

		props_.push_back(pi);
	}

	// Iterate over all position properties, if a property in the position property list
	// is not in the rotation property list, add it to our position properties list
	BW::vector<GenPositionProperty*> positionProps = CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = positionProps.begin(); i != positionProps.end(); ++i)
	{
		PositionOnlyPropInfo ppi;

		ppi.prop_ = *i;

		// Check if our matrixproxy is shared with one of the rotation properties, if not, we 
		// hold on to the position property so that we can modify the position in case of
		// world space scale being applied
		// Note: this assumes that the rotation and position properties share the same matrix proxy
		if (std::find_if( props_.begin(), props_.end(), PropFinder( ppi.prop_->pMatrix() ) ) == props_.end() )
		{
			ppi.prop_->pMatrix()->recordState();
			ppi.prop_->pMatrix()->getMatrix( ppi.initialMatrix_, true );

			positionOnlyProps_.push_back(ppi);
		}
	}
	
	// Store the frame of reference of the group
	// This is the center of all the selected objects in the space of 
	// the gizmo
	groupFrame_ = gizmoTransform;
	groupFrame_.translation(CurrentRotationProperties::averageOrigin());

	// Store the inverse of the group frame of reference
	invGroupFrame_.invert( groupFrame_ );
}


/**
 *	This method updates the rotation of the current rotation properties
 *	@param angle the angle to rotate the object at
 *	@param axis the normalised axis to rotate the object around
 */
void PropertyRotaterHelper::updateRotation( float angle, const Vector3& axis )
{
	BW_GUARD;

	// Calculate the rotation matrix, the rotation matrix is applied in group
	// space we go via a quaternion as we can easily create a rotation quaternion
	// from the angle and axis
	Quaternion rotationQuaternion;
	rotationQuaternion.fromAngleAxis( angle, axis );
	
	Matrix rotationMatrix;
	rotationMatrix.setRotate( rotationQuaternion );

	// bring the rotation matrix into the appopriate space
	rotationMatrix.postMultiply( groupFrame_ );
	rotationMatrix.preMultiply( invGroupFrame_ );

	// Iterate over the group of properties to apply the rotation to
	BW::vector<PropInfo>::iterator propIt = props_.begin();
	for (; propIt != props_.end(); ++propIt)
	{
		PropInfo& pi = *propIt;

		// Rotate the initial position of the object in world space
		Matrix newMatrix = pi.initialMatrix_;
		newMatrix.postMultiply( rotationMatrix );

		// If we are currently working in local space, we don't update 
		// object positions based on the rotation so we put the original 
		// translation back in the matrix
		if (CoordModeProvider::ins()->getCoordMode() == 
				CoordModeProvider::COORDMODE_OBJECT)
		{
			newMatrix.translation( pi.initialMatrix_.applyToOrigin() );
		}
		else if (SnapProvider::instance()->snapMode() != SnapProvider::SNAPMODE_OBSTACLE)
		{
			Vector3 translation = newMatrix.applyToOrigin();
			SnapProvider::instance()->snapPosition( translation );
			newMatrix.translation( translation );
		}

		// Grab the inverse of the chunktransform the object is currently in
		Matrix invChunkMatrix;
		pi.prop_->pMatrix()->getMatrixContextInverse( invChunkMatrix );

		// Transform the object to chunk space
		newMatrix.postMultiply( invChunkMatrix );

		// Set the object matrix
		pi.prop_->pMatrix()->setMatrix( newMatrix );
	}

	if (CoordModeProvider::ins()->getCoordMode() != 
			CoordModeProvider::COORDMODE_OBJECT)
	{
		// Iterate over the group of properties to apply the rotation to
		BW::vector<PositionOnlyPropInfo>::iterator posPropIt = positionOnlyProps_.begin();
		for (; posPropIt != positionOnlyProps_.end(); ++posPropIt)
		{
			PositionOnlyPropInfo& ppi = *posPropIt;

			// Rotate the initial position of the object in world space
			Matrix newMatrix = ppi.initialMatrix_;
			newMatrix.translation( rotationMatrix.applyPoint( ppi.initialMatrix_.applyToOrigin() ) );

			if (SnapProvider::instance()->snapMode() != SnapProvider::SNAPMODE_OBSTACLE)
			{
				Vector3 translation = newMatrix.applyToOrigin();
				SnapProvider::instance()->snapPosition( translation );
				newMatrix.translation( translation );
			}

			// Grab the inverse of the chunktransform the object is currently in
			Matrix invChunkMatrix;
			ppi.prop_->pMatrix()->getMatrixContextInverse( invChunkMatrix );

			// Transform the object to chunk space
			newMatrix.postMultiply( invChunkMatrix );

			// Set the object matrix
			ppi.prop_->pMatrix()->setMatrix( newMatrix );
		}
	}
}


/**
 *	This method finishes up the rotation
 *	@param success if this value is true, the last rotation value
 *		will be commited to the rotation properties, otherwise they
 *		revert to the initial rotation values
 */
void PropertyRotaterHelper::fini( bool success )
{
	BW_GUARD;

	// Commit the rotation properties
	BW::vector<PropInfo>::iterator pit = props_.begin();
	for (; pit != props_.end(); ++pit)
	{
		// If we are successful commit all transforms,
		// otherwise return the transforms to their original states
		if (success)
		{
			(*pit).prop_->pMatrix()->commitState( false, false );
		}
		else
		{
			(*pit).prop_->pMatrix()->commitState( true );
		}
	}

	// Commit the position properties
	BW::vector<PositionOnlyPropInfo>::iterator ppit = positionOnlyProps_.begin();
	for (; ppit != positionOnlyProps_.end(); ++ppit)
	{
		// If we are successful commit all transforms,
		// otherwise return the transforms to their original states
		if (success)
		{
			(*ppit).prop_->pMatrix()->commitState( false, false );
		}
		else
		{
			(*ppit).prop_->pMatrix()->commitState( true );
		}
	}
}


/**
 *	Constructor for the PropFinder helper class
 *	@param pMatrix the matrix proxy to use when searching for a property
 */
PropertyRotaterHelper::PropFinder::PropFinder( MatrixProxyPtr pMatrix ) :
 pMatrix_( pMatrix )
{
}

 
 /**
 *	() operator used to find a matrix proxy
 *	@param prop the PropInfo structure to compare our matrix proxy with
 */
bool PropertyRotaterHelper::PropFinder::operator ()( PropertyRotaterHelper::PropInfo& prop )
{
	BW_GUARD;

	if (pMatrix_ == prop.prop_->pMatrix())
	{
		return true;
	}
	return false;
}


BW_END_NAMESPACE
// property_rotater_helper.cpp
