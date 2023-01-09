#include "pch.hpp"
#include "matrix_positioner.hpp"
#include "general_properties.hpp"

#include "tool_manager.hpp"
#include "appmgr/options.hpp"
#include "undoredo.hpp"
#include "snap_provider.hpp"
#include "coord_mode_provider.hpp"
#include "pyscript/py_data_section.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/debug.hpp"
#include "current_general_properties.hpp"
#include "item_view.hpp"
#include "chunk/chunk_item.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( MatrixPositioner )

PY_BEGIN_METHODS( MatrixPositioner )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixPositioner )
PY_END_ATTRIBUTES()

PY_FACTORY( MatrixPositioner, Functor )


/**
 *	Constructor.
 */
MatrixPositioner::MatrixPositioner( MatrixProxyPtr pMatrix,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	lastLocatorPos_( Vector3::zero() ),
	totalLocatorOffset_( Vector3::zero() ),
	gotInitialLocatorPos_( false )
{
	BW_GUARD;

	BW::vector<GenPositionProperty*> props =
		CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
		i != props.end(); ++i)
	{
		(*i)->pMatrix()->recordState();
	}

	matrix_ = pMatrix;
}


void MatrixPositioner::doApply( float dTime, Tool& tool )
{
	BW_GUARD;

	// figure out movement
	if (tool.locator())
	{
		if (!gotInitialLocatorPos_)
		{
			lastLocatorPos_ = tool.locator()->transform().applyToOrigin();
			gotInitialLocatorPos_ = true;
		}

		totalLocatorOffset_ += tool.locator()->transform().applyToOrigin() -
			lastLocatorPos_;
		lastLocatorPos_ = tool.locator()->transform().applyToOrigin();


		// reset the last change we made
		matrix_->commitState( true );


		Matrix m;
		matrix_->getMatrix( m );

		Vector3 newPos = m.applyToOrigin() + totalLocatorOffset_;

		SnapProvider::instance()->snapPosition( newPos );

		m.translation( newPos );

		Matrix worldToLocal;
		matrix_->getMatrixContextInverse( worldToLocal );

		m.postMultiply( worldToLocal );


		matrix_->setMatrix( m );
	}
}


void MatrixPositioner::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;
	if (applying_)
	{
		if (matrix_->hasChanged())
		{
			// set its transform permanently
			matrix_->commitState( false );
		}
		else
		{
			matrix_->commitState( true );
		}

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, addUndoBarrier );
	}
}


void MatrixPositioner::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;

	if (applying_)
	{
		// Set the items back to their original states
		matrix_->commitState( true );

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


/**
 *	Factory method
 */
PyObject * MatrixPositioner::pyNew( PyObject * args )
{
	BW_GUARD;

	if (CurrentPositionProperties::properties().empty())
	{
		PyErr_Format( PyExc_ValueError,
			"MatrixPositioner()  No current editor" );
		return NULL;
	}

	return new MatrixPositioner( NULL );
}

BW_END_NAMESPACE
// matrix_positioner.cpp
