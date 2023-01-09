#include "pch.hpp"
#include "chunk/chunk_item.hpp"
#include "matrix_scaler.hpp"
#include "general_properties.hpp"
#include "item_view.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( MatrixScaler )

PY_BEGIN_METHODS( MatrixScaler )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixScaler )
PY_END_ATTRIBUTES()

PY_FACTORY( MatrixScaler, Functor )


/**
 *	Constructor.
 */
MatrixScaler::MatrixScaler( MatrixProxyPtr pMatrix,
	float scaleSpeedFactor,
	FloatProxyPtr scaleX,
	FloatProxyPtr scaleY,
	FloatProxyPtr scaleZ,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	pMatrix_( pMatrix ),
	scaleSpeedFactor_(scaleSpeedFactor),
	grabOffset_( 0, 0, 0 ),
	grabOffsetSet_( false ),
	scaleX_( scaleX ),
	scaleY_( scaleY ),
	scaleZ_( scaleZ )
{
	BW_GUARD;

	MF_ASSERT( pMatrix_ );
	pMatrix_->recordState();
	pMatrix_->getMatrix( initialMatrix_, false );

	initialScale_.set( initialMatrix_[0].length(),
		initialMatrix_[1].length(),
		initialMatrix_[2].length() );

	initialMatrix_[0] /= initialScale_.x;
	initialMatrix_[1] /= initialScale_.y;
	initialMatrix_[2] /= initialScale_.z;

	invInitialMatrix_.invert( initialMatrix_ );
}


void MatrixScaler::doApply( float dTime, Tool& tool )
{
	BW_GUARD;

	// figure out movement
	if (tool.locator())
	{
		Matrix localPosn;
		pMatrix_->getMatrixContextInverse( localPosn );
		localPosn.preMultiply( tool.locator()->transform() );

		if (!grabOffsetSet_)
		{
			grabOffsetSet_ = true;
			grabOffset_ = localPosn.applyToOrigin();
		}

		Vector3 scale = localPosn.applyToOrigin() - grabOffset_;
		scale *= scaleSpeedFactor_;

		scale = invInitialMatrix_.applyVector( scale );

		Vector3 direction = invInitialMatrix_.applyVector(
			tool.locator()->direction() );
		direction.normalise();

		scale = scale + initialScale_;

		const float scaleEpsilon = 0.01f;
		scale.x = max( scale.x, scaleEpsilon );
		scale.y = max( scale.y, scaleEpsilon );
		scale.z = max( scale.z, scaleEpsilon );

		if (scaleX_)
		{
			scaleX_->set( scale.x, false );
		}
		if (scaleY_)
		{
			scaleY_->set( scale.y, false );
		}
		if (scaleZ_)
		{
			scaleZ_->set( scale.z, false );
		}

		Matrix curPose;
		curPose.setScale( scale );
		curPose.postMultiply( initialMatrix_ );

		pMatrix_->setMatrix( curPose );
	}
}

void MatrixScaler::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		// set its transform permanently
		pMatrix_->commitState();

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void MatrixScaler::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;

	if (applying_)
	{
		// set the item back to it's original pose
		pMatrix_->commitState( true );

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


/**
 *	Factory method
 */
PyObject * MatrixScaler::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pPyRev = NULL;
	if (!PyArg_ParseTuple( args, "O", &pPyRev ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_TypeError, "MatrixScaler() "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );

	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );
	if (items.size() != 1)
	{
		PyErr_Format( PyExc_ValueError, "MatrixScaler() "
			"Revealer must reveal exactly one item, not %d", items.size() );
		return NULL;
	}

	ChunkItemPtr pItem = items[0];
	if (pItem->chunk() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "MatrixScaler() "
			"Item to move is not in the scene" );
		return NULL;
	}

	return new MatrixScaler( MatrixProxy::getChunkItemDefault( pItem ) );
}

BW_END_NAMESPACE
// matrix_scaler.cpp
