
#include "pch.hpp"
#include "tee_functor.hpp"

BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( TeeFunctor )

PY_BEGIN_METHODS( TeeFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TeeFunctor )
PY_END_ATTRIBUTES()

PY_FACTORY( TeeFunctor, Functor )

TeeFunctor::TeeFunctor( ToolFunctor* f1,
	ToolFunctor* f2,
	KeyCode::Key altKey,
	PyTypeObject * pType ) :

	ToolFunctor( true, pType ),
	f1_(f1),
	f2_(f2),
	altKey_(altKey)
{
}

void TeeFunctor::update( float dTime, Tool& tool )
{
	BW_GUARD;

	activeFunctor()->update( dTime, tool );
}


void TeeFunctor::stopApplying( Tool & tool, bool saveChanges )
{
	BW_GUARD;
	f1_->stopApplying( tool, saveChanges );
	f2_->stopApplying( tool, saveChanges );
}


void TeeFunctor::onBeginUsing( Tool & tool )
{
	BW_GUARD;
	f1_->onBeginUsing( tool );
	f2_->onBeginUsing( tool );
}


void TeeFunctor::onEndUsing( Tool & tool )
{
	BW_GUARD;
	f2_->onEndUsing( tool );
	f1_->onEndUsing( tool );
}


bool TeeFunctor::isAllowedToDiscardChanges() const
{
	BW_GUARD;
	return activeFunctor()->isAllowedToDiscardChanges();
}


bool TeeFunctor::handleKeyEvent( const KeyEvent & event, Tool& tool )
{
	BW_GUARD;

	return activeFunctor()->handleKeyEvent( event, tool );
}

bool TeeFunctor::handleMouseEvent( const MouseEvent & event, Tool& tool )
{
	BW_GUARD;

	return activeFunctor()->handleMouseEvent( event, tool );
}

bool TeeFunctor::applying() const
{
	BW_GUARD;

	return f1_->applying() || f2_->applying();
}

ToolFunctor* TeeFunctor::activeFunctor() const
{
	BW_GUARD;

	if (InputDevices::isKeyDown( altKey_ ))
	{
		return f2_;
	}
	else
	{
		return f1_;
	}
}


PyObject * TeeFunctor::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject* f1;
	PyObject* f2;
	int i;
	if (!PyArg_ParseTuple( args, "OOi", &f1, &f2, &i ) ||
		!ToolFunctor::Check( f1 ) ||
		!ToolFunctor::Check( f2 ))
	{
		PyErr_SetString( PyExc_TypeError, "TeeFunctor() "
			"expects two ToolFunctors and a key" );
		return NULL;
	}

	return new TeeFunctor( 
		static_cast<ToolFunctor*>( f1 ),
		static_cast<ToolFunctor*>( f2 ),
		static_cast<KeyCode::Key>( i )
		);
}
BW_END_NAMESPACE

