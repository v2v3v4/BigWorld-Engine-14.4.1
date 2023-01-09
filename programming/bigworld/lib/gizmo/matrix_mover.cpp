#include "pch.hpp"
#include "matrix_mover.hpp"
#include "current_general_properties.hpp"
#include "general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "tool_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( MatrixMover )

PY_BEGIN_METHODS( MatrixMover )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixMover )
PY_END_ATTRIBUTES()

PY_FACTORY( MatrixMover, Functor )


int MatrixMover::moving_ = 0;


/**
 *	Constructor.
 */
MatrixMover::MatrixMover( MatrixProxyPtr pMatrix,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	lastLocatorPos_( Vector3::zero() ),
	totalLocatorOffset_( Vector3::zero() ),
	gotInitialLocatorPos_( false ),
	snap_( true ),
	rotate_( false ),
	snapMode_( SnapProvider::instance()->snapMode() )
{
	BW_GUARD;

	++moving_;

	BW::vector<GenPositionProperty*> props =
		CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
		i != props.end(); ++i)
	{
		GenPositionProperty* prop = *i;
		MF_ASSERT( prop );
		MF_ASSERT( prop->pMatrix().exists() );
		prop->pMatrix()->recordState();
	}
}

MatrixMover::MatrixMover( MatrixProxyPtr pMatrix,
	bool snap,
	bool rotate,
	bool allowedToDiscardChanges,
	PyTypeObject * pType ) :

	AlwaysApplyingFunctor( allowedToDiscardChanges, pType ),
	lastLocatorPos_( Vector3::zero() ),
	totalLocatorOffset_( Vector3::zero() ),
	gotInitialLocatorPos_( false ),
	snap_( snap ),
	rotate_( rotate ),
	snapMode_( SnapProvider::instance()->snapMode() )
{
	BW_GUARD;

	++moving_;

	BW::vector<GenPositionProperty*> props =
		CurrentPositionProperties::properties();
	for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
		i != props.end(); ++i)
	{
		GenPositionProperty* prop = *i;
		MF_ASSERT( prop );
		MF_ASSERT( prop->pMatrix().exists() );
		prop->pMatrix()->recordState();
	}
}

MatrixMover::~MatrixMover()
{
	--moving_;
}


/**
 *	Check if something is moving.
 *	@return true if a MatrixMover exists.
 */
/*static*/ bool MatrixMover::moving()
{
	BW_GUARD;
	return (moving_ != 0);
}


bool MatrixMover::checkStopApplying() const
{
	BW_GUARD;

	// Left mouse button released or snap mode wrong
	return (!InputDevices::isKeyDown( KeyCode::KEY_LEFTMOUSE ) ||
		snapMode_ != SnapProvider::instance()->snapMode());
}


void MatrixMover::doApply( float dTime, Tool& tool )
{
	BW_GUARD;

	// figure out movement
	if (tool.locator())
	{
		Vector3 locatorPos = tool.locator()->transform().applyToOrigin();

		if (!gotInitialLocatorPos_)
		{
			lastLocatorPos_ = tool.locator()->transform().applyToOrigin();
			gotInitialLocatorPos_ = true;

			BW::vector<GenPositionProperty*> props =
				CurrentPositionProperties::properties();
			if (props.size() == 1)
			{
				Vector3 objPos = props[0]->pMatrix()->get().applyToOrigin();
				Vector4 v4ClipPos( objPos, 1. );
				Moo::rc().viewProjection().applyPoint( v4ClipPos, v4ClipPos );
				Vector3 clipPos( v4ClipPos[0] / v4ClipPos[3], 
								v4ClipPos[1] / v4ClipPos[3], 
								v4ClipPos[2] / v4ClipPos[3] );
				clipPos.x = ( clipPos.x + 1 ) / 2 * Moo::rc().screenWidth();
				clipPos.y = ( 1 - clipPos.y ) / 2 * Moo::rc().screenHeight();

				POINT pt;
				pt.x = LONG( clipPos.x );
				pt.y = LONG( clipPos.y );
				::ClientToScreen( Moo::rc().windowHandle(), &pt );
				::SetCursorPos( pt.x, pt.y );

				if (Moo::rc().device()) 
				{
					Moo::rc().device()->SetCursorPosition( pt.x, pt.y, 0 );
				}

				lastLocatorPos_ = locatorPos = objPos;
			}
		}

		totalLocatorOffset_ += locatorPos - lastLocatorPos_;
		lastLocatorPos_ = locatorPos;


		BW::vector<GenPositionProperty*> props =
			CurrentPositionProperties::properties();
		for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
			i != props.end(); ++i)
		{
			GenPositionProperty* prop = *i;
			MF_ASSERT( prop );

			MatrixProxyPtr pMatrix = prop->pMatrix();
			MF_ASSERT( pMatrix.exists() );

			Matrix oldMatrix;
			pMatrix->getMatrix( oldMatrix );

			// reset the last change we made
			pMatrix->commitState( true );

			Matrix m;
			pMatrix->getMatrix( m );

			Vector3 delta = totalLocatorOffset_;

			if( snap_ )
			{
				SnapProvider::instance()->snapPositionDelta( delta );
			}

			Vector3 newPos = m.applyToOrigin() + delta;

			bool snapPosOK = true;
			if( snap_ )
			{
				snapPosOK = SnapProvider::instance()->snapPosition( newPos );
			}

			if( rotate_ && snapPosOK )
			{
				Vector3 normalOfSnap =
					SnapProvider::instance()->snapNormal( newPos );
				Vector3 yAxis( 0, 1, 0 );
				yAxis = m.applyVector( yAxis );

				Vector3 binormal = yAxis.crossProduct( normalOfSnap );

				normalOfSnap.normalise();
				yAxis.normalise();
				binormal.normalise();

				float angle = acosf(
					Math::clamp(-1.0f,
					yAxis.dotProduct( normalOfSnap ),
					+1.0f) );

				Quaternion q( binormal.x * sinf( angle / 2.f ),
					binormal.y * sinf( angle / 2.f ),
					binormal.z * sinf( angle / 2.f ),
					cosf( angle / 2.f ) );

				q.normalise();

				Matrix rotation;
				rotation.setRotate( q );

				m.postMultiply( rotation );
			}

			if (snapPosOK)
			{
				m.translation( newPos );

				Matrix worldToLocal;
				pMatrix->getMatrixContextInverse( worldToLocal );

				m.postMultiply( worldToLocal );

				pMatrix->setMatrix( m );
			}
			else
			{
				// snapping the position failed, revert back to the previous
				// good matrix:
				Matrix worldToLocal;
				pMatrix->getMatrixContextInverse( worldToLocal );
				oldMatrix.postMultiply( worldToLocal );
				pMatrix->setMatrix( oldMatrix );
			}
		}
	}
}


void MatrixMover::stopApplyCommitChanges( Tool& tool, bool addUndoBarrier )
{
	BW_GUARD;

	if (applying_)
	{
		
		BW::vector<MatrixProxyPtr> matrixes;

		BW::vector<GenPositionProperty*> props =
			CurrentPositionProperties::properties();

		bool addUndoBarrier = (props.size() > 1) || (undoName_ != "");

		bool success = true;
		for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
			i != props.end(); ++i)
		{
			GenPositionProperty* prop = *i;
			MF_ASSERT( prop );
			
			MatrixProxyPtr pMatrix = prop->pMatrix();
			MF_ASSERT( pMatrix.exists() );

			matrixes.push_back( pMatrix );
		}

		for (BW::vector<MatrixProxyPtr>::iterator i = matrixes.begin();
			i != matrixes.end(); ++i)
		{

			MatrixProxyPtr pMatrix = *i;
			MF_ASSERT( pMatrix.exists() );

			if (pMatrix->hasChanged())
			{
				// set its transform permanently
				if (!pMatrix->commitState( false, !addUndoBarrier ))
				{
					success = false;
				}
			}
			else
			{
				pMatrix->commitState( true );
			}
		}

		if (addUndoBarrier)
		{
			if (undoName_ != "")
			{
				UndoRedo::instance().barrier( undoName_, false );
			}
			else
			{
				UndoRedo::instance().barrier(
					LocaliseUTF8( L"GIZMO/UNDO/MATRIX_MOVER" ), true );
			}

			if (!success)
			{
				UndoRedo::instance().undo();
			}
		}

		AlwaysApplyingFunctor::stopApplyCommitChanges( tool, false );
	}
}


void MatrixMover::stopApplyDiscardChanges( Tool& tool )
{
	BW_GUARD;

	if (applying_)
	{
		// Set the items back to their original states
		BW::vector<GenPositionProperty*> props =
			CurrentPositionProperties::properties();

		for (BW::vector<GenPositionProperty*>::iterator i = props.begin();
			i != props.end(); ++i)
		{
			GenPositionProperty* prop = *i;
			MF_ASSERT( prop );
			MF_ASSERT( prop->pMatrix().exists() );
			prop->pMatrix()->commitState( true );
		}

		AlwaysApplyingFunctor::stopApplyDiscardChanges( tool );
	}
}


/**
 *	Factory method
 */
PyObject * MatrixMover::pyNew( PyObject * args )
{
	BW_GUARD;

	if (CurrentPositionProperties::properties().empty())
	{
		PyErr_Format( PyExc_ValueError, "MatrixMover() "
			"No current editor" );
		return NULL;
	}

	return new MatrixMover( NULL );
}

BW_END_NAMESPACE
// matrix_mover.cpp
