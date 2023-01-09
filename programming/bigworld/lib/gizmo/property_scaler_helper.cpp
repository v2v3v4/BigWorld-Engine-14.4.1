#include "pch.hpp"

#include "general_properties.hpp"
#include "current_general_properties.hpp"
#include "coord_mode_provider.hpp"
#include "snap_provider.hpp"

#include "property_scaler_helper.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
PropertyScalerHelper::PropertyScalerHelper()
{
}


/**
 *	This method initialises the property scale helper
 *	@param gizmoTransform the current gizmo transform
 *		used to apply scale along its axis
 */
void PropertyScalerHelper::init( const Matrix& gizmoTransform )
{
	BW_GUARD;

	// Clear the props
	props_.clear();

	// Iterate over all the properties and init the propertyinfo structure
	BW::vector<GenScaleProperty*> props = CurrentScaleProperties::properties();
	for (BW::vector<GenScaleProperty*>::iterator i = props.begin(); i != props.end(); ++i)
	{
		PropInfo pi;

		pi.prop_ = *i;
		pi.prop_->pMatrix()->recordState();
		pi.prop_->pMatrix()->getMatrix( pi.initialMatrix_, true );

		props_.push_back(pi);
	}
	
	// Iterate over all position properties, if a property in the position property list
	// is not in the scale property list, add it to our position properties list
	BW::vector<GenPositionProperty*> positionProps = CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = positionProps.begin(); i != positionProps.end(); ++i)
	{
		PositionOnlyPropInfo ppi;

		ppi.prop_ = *i;

		// Check if our matrixproxy is shared with one of the scale properties, if not, we 
		// hold on to the position property so that we can modify the position in case of
		// world space scale being applied
		// Note: this assumes that the scale and position properties share the same matrix proxy
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
	groupFrame_.translation(CurrentScaleProperties::averageOrigin());

	// Store the inverse of the group frame of reference
	invGroupFrame_.invert( groupFrame_ );
}


/**
 *	This method updates the scale of the current scale properties
 *	@param scale the scale value which will be applied along the
 *		gizmo axis
 */
void PropertyScalerHelper::updateScale( const Vector3& scale )
{
	BW_GUARD;

	// Calculate the scale matrix, the scale matrix is applied in group
	// space
	Matrix scaleMatrix;
	scaleMatrix.setScale( scale.x, scale.y, scale.z );
	scaleMatrix.postMultiply( groupFrame_ );
	scaleMatrix.preMultiply( invGroupFrame_ );

	// Iterate over the group of properties to apply the scale to
	BW::vector<PropInfo>::iterator propIt = props_.begin();
	for (; propIt != props_.end(); ++propIt)
	{
		PropInfo& pi = *propIt;

		// Scale the initial position of the object in world space
		Matrix newMatrix = pi.initialMatrix_;
		newMatrix.postMultiply( scaleMatrix );

		// If we are currently working in local space, we don't update 
		// object positions based on scale so we put the original 
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

			// Scale the initial position of the object in world space
			Matrix newMatrix = ppi.initialMatrix_;
			newMatrix.translation( scaleMatrix.applyPoint( ppi.initialMatrix_.applyToOrigin() ) );

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
 *	This method finishes up the scaling
 *	@param success if this value is true, the last scale value
 *		will be commited to the scale properties, otherwise they
 *		revert to the initial scale values
 */
void PropertyScalerHelper::fini( bool success )
{
	BW_GUARD;
	BW::vector<MatrixProxyPtr> commitMatrixProxies;

	// Commit the scale properties
	for (PropInfo prop: props_)
	{
		commitMatrixProxies.push_back( prop.prop_->pMatrix() );
	}

	// Commit the position properties
	for (PositionOnlyPropInfo prop: positionOnlyProps_)
	{
		commitMatrixProxies.push_back( prop.prop_->pMatrix() );
	}

	// If we are successful commit all transforms,
	// otherwise return the transforms to their original states
	if (success)
	{
		for (auto proxy: commitMatrixProxies)
		{
			proxy->commitState( false, false );
		}
	}
	else
	{
		for (auto proxy: commitMatrixProxies)
		{
			proxy->commitState( true );
		}
	}
}


/**
 *	Constructor for the PropFinder helper class
 *	@param pMatrix the matrix proxy to use when searching for a property
 */
PropertyScalerHelper::PropFinder::PropFinder( MatrixProxyPtr pMatrix ) :
 pMatrix_( pMatrix )
{
}

 
 /**
 *	() operator used to find a matrix proxy
 *	@param prop the PropInfo structure to compare our matrix proxy with
 */
bool PropertyScalerHelper::PropFinder::operator ()( PropertyScalerHelper::PropInfo& prop )
{
	BW_GUARD;

	if (pMatrix_ == prop.prop_->pMatrix())
	{
		return true;
	}
	return false;
}


BW_END_NAMESPACE
// property_scaler_helper.cpp
