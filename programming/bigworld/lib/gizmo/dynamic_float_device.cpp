#include "pch.hpp"
#include "dynamic_float_device.hpp"
#include "general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( DynamicFloatDevice )

PY_BEGIN_METHODS( DynamicFloatDevice )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( DynamicFloatDevice )
PY_END_ATTRIBUTES()

PY_FACTORY( DynamicFloatDevice, Functor )

/**
 *	Constructor.
 */
DynamicFloatDevice::DynamicFloatDevice( MatrixProxyPtr pCenter,
   FloatProxyPtr pFloat,
   float adjFactor,
   bool allowedToDiscardChanges,
   PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	pCenter_( pCenter ),
	pFloat_( pFloat ),
	grabOffset_( 0, 0, 0 ),
	grabOffsetSet_( false ),
	adjFactor_( adjFactor )
{
	BW_GUARD;

	initialFloat_ = pFloat->get();
	pCenter->getMatrix( initialCenter_, true );

	undoName_ = LocaliseUTF8( L"GIZMO/UNDO/DYNAMIC_FLOAT_DEVICE" );
}


void DynamicFloatDevice::doApply( float dTime, Tool & tool )
{
	BW_GUARD;

	// figure out radius
	if (tool.locator())
	{
		// Raymond, for bug 4734, the scale function has been changed
		// Now it will be smooth near 1:1 and quick when far away from 1:1
		// Also it meets 0 in the middle of the RadiusGizmo
		if (!grabOffsetSet_)
		{
			grabOffsetSet_ = true;
			grabOffset_ = tool.locator()->transform().applyToOrigin() -
				initialCenter_.applyToOrigin();

			Vector4 v4Locator( tool.locator()->transform().applyToOrigin(), 1. );

			Moo::rc().viewProjection().applyPoint( v4Locator, v4Locator );
			grabOffset_[0] = v4Locator[0] / v4Locator[3];
			grabOffset_[1] = v4Locator[1] / v4Locator[3];
			grabOffset_[2] = v4Locator[2] / v4Locator[3];

			Vector4 v4InitialCenter( initialCenter_.applyToOrigin(), 1 );

			Moo::rc().viewProjection().applyPoint( v4InitialCenter, v4InitialCenter );
			grabOffset_[0] -= v4InitialCenter[0] / v4InitialCenter[3];
			grabOffset_[1] -= v4InitialCenter[1] / v4InitialCenter[3];
			grabOffset_[2] -= v4InitialCenter[2] / v4InitialCenter[3];
			grabOffset_[2] = 0;
		}

		Vector3 offset = (tool.locator()->transform().applyToOrigin() -
			initialCenter_.applyToOrigin());

		Vector4 v4Locator( tool.locator()->transform().applyToOrigin(), 1. );

		Moo::rc().viewProjection().applyPoint( v4Locator, v4Locator );
		offset[0] = v4Locator[0] / v4Locator[3];
		offset[1] = v4Locator[1] / v4Locator[3];
		offset[2] = v4Locator[2] / v4Locator[3];

		Vector4 v4InitialCenter( initialCenter_.applyToOrigin(), 1 );

		Moo::rc().viewProjection().applyPoint( v4InitialCenter, v4InitialCenter );
		offset[0] -= v4InitialCenter[0] / v4InitialCenter[3];
		offset[1] -= v4InitialCenter[1] / v4InitialCenter[3];
		offset[2] -= v4InitialCenter[2] / v4InitialCenter[3];
		offset[2] = 0;

		float ratio = offset.length() / grabOffset_.length();
		// Bug 5153 fix: There was a problem if the initial radius was 0.0.
		if (initialFloat_ == 0.0f)
		{
			pFloat_->set( initialFloat_ +
				((offset.length() - grabOffset_.length()) * adjFactor_), true );
		}
		else if( ratio < 1.0f )
		{
			pFloat_->set( initialFloat_ *
				( 1 - ( ratio - 1 ) * ( ratio - 1 ) ), true );
		}
		else
		{
			pFloat_->set( initialFloat_ * ratio * ratio, true );
		}
	}
}


void DynamicFloatDevice::stopApplyCommitChanges( Tool& tool,
	bool addUndoBarrier )
{
	BW_GUARD;
	if (applying_)
	{
		// set its value permanently
		float finalValue = pFloat_->get();

		pFloat_->set( initialFloat_, true );
		pFloat_->set( finalValue, false );

		if (UndoRedo::instance().barrierNeeded() && addUndoBarrier)
		{
			UndoRedo::instance().barrier( undoName_, false );
		}

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, false );
	}
}

void DynamicFloatDevice::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;
	if (applying_)
	{
		// set the item back to it's original pose
		pFloat_->set( initialFloat_, true );

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


/**
 *	Factory method
 */
PyObject * DynamicFloatDevice::pyNew( PyObject * args )
{
	/*PyObject * pPyRev = NULL;
	if (!PyArg_ParseTuple( args, "O", &pPyRev ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_TypeError, "DynamicFloatDevice() "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );

	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );
	if (items.size() != 1)
	{
		PyErr_Format( PyExc_ValueError, "DynamicFloatDevice() "
			"Revealer must reveal exactly one item, not %d", items.size() );
		return NULL;
	}

	ChunkItemPtr pItem = items[0];
	if (pItem->chunk() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "DynamicFloatDevice() "
			"Item to set radius is not in the scene" );
		return NULL;
	}

	return new DynamicFloatDevice(
		MatrixProxy::getChunkItemDefault( pItem ),
		FloatProxy::getChunkItemDefault( pItem ) );*/
	return NULL;
}

BW_END_NAMESPACE
// dynamic_float_device.cpp
