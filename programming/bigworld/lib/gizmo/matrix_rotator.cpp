#include "pch.hpp"
#include "chunk/chunk_item.hpp"
#include "current_general_properties.hpp"
#include "item_view.hpp"
#include "matrix_rotator.hpp"
#include "resmgr/string_provider.hpp"
#include "snap_provider.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( MatrixRotator )

PY_BEGIN_METHODS( MatrixRotator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixRotator )
PY_END_ATTRIBUTES()

PY_FACTORY( MatrixRotator, Functor )


MatrixRotator::MatrixRotator( MatrixProxyPtr pMatrix,  
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	grabOffsetSet_( false ),
	invGrabOffset_( Matrix::identity ),
	initialToolLocation_( Vector3::zero() )
{
	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/MATRIX_ROTATOR" );
}


void MatrixRotator::doApply( float dTime, Tool& tool )
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

			Vector3 toolLocation = grabOffset.applyToOrigin();

			// Make the grabOffset the actual gizmo location
			grabOffset.translation( CurrentRotationProperties::averageOrigin() );

			// Store the inverse of the tool transform
			invGrabOffset_.invert( grabOffset );

			// Get the initial tool location in gizmo space
			initialToolLocation_ = invGrabOffset_.applyPoint( toolLocation );
			initialToolLocation_.normalise();

			// init the rotator helper
			rotaterHelper_.init( grabOffset );
		}

		// Get the current tool location
		Vector3 currentToolLocation = invGrabOffset_.applyPoint( 
			tool.locator()->transform().applyToOrigin() );

		currentToolLocation.normalise();

		// If the angle between the current tool location and the initial
		// tool location is less than 0.5 degrees we don't do anything,
		// this is because we use the cross product between the tool
		// locations to work out the rotation axis, and if the two directions
		// are very similar we can't get the axis
		if (fabsf(currentToolLocation.dotProduct( initialToolLocation_ ) )
			< cos( DEG_TO_RAD( 0.5f ) ))
		{
			// Get the axis we are rotating about
			Vector3 axis = initialToolLocation_.crossProduct(
				currentToolLocation );
			axis.normalise();

			// Calculate the angle to rotate
			float angle = acosf( Math::clamp( -1.f, 
				currentToolLocation.dotProduct( initialToolLocation_ ), 1.f ));

			// Do angle snapping
			float snapAmount =
				SnapProvider::instance()->angleSnapAmount() / 180 * MATH_PI;

			if( snapAmount != 0.0f )
			{
				float snapAngle = int( angle / snapAmount ) * snapAmount;
				if( angle - snapAngle >= snapAmount / 2 )
				{
					angle = snapAngle + snapAmount;
				}
				else
				{
					angle = snapAngle;
				}
			}

			// Update rotation
			rotaterHelper_.updateRotation( angle, axis );
		}
	}
}


void MatrixRotator::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;
	if (applying_)
	{
		// Tell our rotation helper that we are finished
		rotaterHelper_.fini( true );

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void MatrixRotator::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;
	if (applying_)
	{
		// Cancel the rotate operation
		rotaterHelper_.fini( false );
	
		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


/**
 *	Factory method
 */
PyObject * MatrixRotator::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pPyRev = NULL;
	if (!PyArg_ParseTuple( args, "O", &pPyRev ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_TypeError, "MatrixRotator() "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );

	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );
	if (items.size() != 1)
	{
		PyErr_Format( PyExc_ValueError, "MatrixRotator() "
			"Revealer must reveal exactly one item, not %d", items.size() );
		return NULL;
	}

	ChunkItemPtr pItem = items[0];
	if (pItem->chunk() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "MatrixRotator() "
			"Item to move is not in the scene" );
		return NULL;
	}

	return new MatrixRotator( MatrixProxy::getChunkItemDefault( pItem ) );
}

BW_END_NAMESPACE
// matrix_rotator.cpp
