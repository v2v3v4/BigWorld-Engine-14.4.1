#include "pch.hpp"
#include "property_scaler.hpp"
#include "tool_manager.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( PropertyScaler )

PY_BEGIN_METHODS( PropertyScaler )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PropertyScaler )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PropertyScaler::PropertyScaler( float scaleSpeedFactor,
	FloatProxyPtr scaleX,
	FloatProxyPtr scaleY,
	FloatProxyPtr scaleZ,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	scaleX_( scaleX ),
	scaleY_( scaleY ),
	scaleZ_( scaleZ ),
	scaleSpeedFactor_(scaleSpeedFactor),
	grabOffsetSet_( false ),
	invGrabOffset_( Matrix::identity )
{
	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/PROPERTY_SCALER" );
}


void PropertyScaler::doApply( float dTime, Tool& tool )
{
	BW_GUARD;

	if (tool.locator())
	{
		// If this is our first time in update, grab our frame of reference etc
		if (!grabOffsetSet_)
		{
			grabOffsetSet_ = true;

			// Get the initial picked position on the tool
			Matrix grabOffset = tool.locator()->transform();

			// Normalise the tool transform
			grabOffset[0].normalise();
			grabOffset[1].normalise();
			grabOffset[2].normalise();

			// Store the inverse of the tool transform
			invGrabOffset_.invert( grabOffset );

			// init the scaler helper
			scalerHelper_.init( grabOffset );
		}

		// Calculate the scale value, the scale value is the position of the 
		// currently picked point relative to the first picked point
		// in gizmo space
		Vector3 scale = invGrabOffset_.applyPoint( 
			tool.locator()->transform().applyToOrigin() ) * scaleSpeedFactor_;

		// Fix up the scale values for all the axis, a negative scale value
		// means shrink the object, positive means That the object grows
		if (scale.x < 0.0)
		{
			scale.x = - 1 / ( scale.x - 1 ) - 1;
		}

		if (scale.y < 0.0)
		{
			scale.y = - 1 / ( scale.y - 1 ) - 1;
		}

		if (scale.z < 0.0)
		{
			scale.z = - 1 / ( scale.z - 1 ) - 1;
		}

		scale += Vector3( 1, 1, 1 );

		// Set the scale values on the proxies
		// This is used for the feedback display of the scale
		if (scaleX_)
		{
			scaleX_->set( scale.x, true );
		}

		if (scaleY_)
		{
			scaleY_->set( scale.y, true );
		}

		if (scaleZ_)
		{
			scaleZ_->set( scale.z, true );
		}

		scalerHelper_.updateScale( scale );
	}
}


void PropertyScaler::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		scalerHelper_.fini( true );

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void PropertyScaler::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;
	if (applying_)
	{
		// Cancel the scale operation
		scalerHelper_.fini( false );

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}

BW_END_NAMESPACE
// property_scaler.cpp
